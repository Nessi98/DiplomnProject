#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <math.h>

#include "MQTTClient.h"
#include "MQTTAsync.h"
#include "MQTTClientPersistence.h"

#define BROKER     "127.0.0.1"
#define CLIENTID    "raspi"
#define PAYLOAD     "001"
#define QOS         2	
#define TIMEOUT     500L

// Topic headers
#define SERVER			"/system_name/server"
#define SERVERACTION	"/system_name/server_action"
#define CONFIG			"/config"
#define ACKNOWLEDGE 	"/config_ack"
#define SYSTEM			"/system_name/"
#define DISCOVER		"/system_name/discover"

// Time
#define DAY				"Daily"
#define MONTH			"Month"
#define YEAR			"Year"
#define TIME			"%d:00"

// Messages
#define IDLE			"IDLE"
#define RELAY			"Relay"
#define SENSOR			"Sensor"
#define CONFIGMESSAGE	"Config"
#define REALTIME	 	"Real Time"
#define STATISTICS		"Statistics"
#define SENSORANDRELAY	"SensorAndRelay"
#define IDLEMESSAGE 	"op_Mode: IDLE; sleep_time: 50000"

// Query For the Database
#define REALTIMEDATA 	"SELECT temp, hum FROM Data Where unitID = %d ORDER BY time DESC LIMIT 1"
#define UNITEXISTENCE	"SELECT name FROM SensorUnit WHERE id = %s"
#define INSERTMESSAGE	"INSERT INTO Data (unitID, temp, hum, time) VALUES (%d, '%s', '%s', '%s')"
#define INSERTRECORD	"INSERT INTO SensorUnit (id, name, opMode) VALUES (%d, '%s', '%s')"
#define UPDATERECORD	"UPDATE SensorUnit SET opMode = '%s' WHERE id = %d"
#define UPDATENAME		"UPDATE SensorUnit SET %s WHERE id = %d"

#define DAILY	"SELECT AVG(temp), AVG(hum), strftime('%H', time) FROM Data WHERE unitID = %d AND date(time) = '%s' GROUP BY strftime('%H', time)"

static MQTTClient client;
int finished = 0;

volatile MQTTAsync_token deliveredtoken;
volatile MQTTClient_deliveryToken deliveredtoken;

void serverAction(char* message);

int executeQuery(char* query);

int unitExist(char* id);
void getData(char* query); 
void createRecordInDB(char* unitID);
void loadDataToServer(char * serverMessage);
void loadDataToDB(int unitID, char* temp, char* hum, char* time);

void connlost(void *context, char *cause);
void subscribeForSensorUnit(char* discover);
void publishMessage(char* message, char* topic);
void delivered(void *context, MQTTClient_deliveryToken dt);
int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message); 

int main(int argc, char* argv[]){

	int rc;
	char* username = "mosquitto"; 
	char* password = "password";

	
	MQTTClient_deliveryToken token;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	conn_opts.username = username;
	conn_opts.password = password;

	MQTTClient_create(&client, BROKER, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;
	
	MQTTClient_setCallbacks(client, NULL, connlost, messageArrived, delivered);

    if((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS){
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }
	
	printf("Successful conntection with the Brocker.\n");
	
	rc = MQTTClient_subscribe(client, DISCOVER, QOS);
	if(rc == 0){
		printf("Subscribed for topic %s successful\n", DISCOVER);
	}
	
	rc = MQTTClient_subscribe(client, SERVERACTION, QOS);
	if(rc == 0){
		printf("Subscribed for topic %s successful\n", SERVERACTION);
	}	
    
	while(1){}
	
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
	
	return rc;
}

void connlost(void *context, char *cause){
    /*MQTTAsync client = (MQTTAsync)context;
	MQTTAsync_connectOptions conn_opts = MQTTAsync_connectOptions_initializer;
	
	int rc;

	printf("\nConnection lost\n");
	if (cause)
		printf("     cause: %s\n", cause);

	printf("Reconnecting\n");
	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	if ((rc = MQTTAsync_connect(client, &conn_opts)) != MQTTASYNC_SUCCESS)
	{
		printf("Failed to start connect, return code %d\n", rc);
		finished = 1;
	}*/
}

void delivered(void *context, MQTTClient_deliveryToken dt){
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken =  dt;
}

int executeQuery(char* query){
	sqlite3 *db;
	
	char* errMsg = 0;
	
	int rc = sqlite3_open("automation.db", &db);
	
	if(rc){
		printf("Filed to open the DB.\n");
		return 0;
	}
	
	rc = sqlite3_exec(db, query, 0, 0, &errMsg);
	if (rc != SQLITE_OK ) {
        
		fprintf(stderr, "SQL error: %s\n", errMsg);
		sqlite3_free(errMsg);    

		return 0;
	}
	
	sqlite3_close(db);
	
	return 1;
}

int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message){
    int count;
    char* payloadptr;

    printf("Message arrived\n");
    printf("    topic: %s\n", topicName);
    printf("	message: ");
    payloadptr = message->payload;
	
    for(count = 0; count < message->payloadlen; count ++){
        putchar(*payloadptr ++);
    }
	
	putchar('\n');
	
	if(strstr(topicName, DISCOVER) != NULL){
		//Check if the arrieved id of sensor unit already exist in the Database
		if(!unitExist(message->payload)){
			printf("Record doesn't exist.\n");	
			createRecordInDB(message->payload);
		}else{
			printf("Record already exitst in the DB!\n");
		}
		
		//subscribeForSensorUnit(message->payload);
	}else if(strstr(topicName, ACKNOWLEDGE) != NULL){
		char* token1;
		char temp[5];
		char hum[5];
		char time[17];
		
		int turn = 0;
		
		token1 = strtok(message->payload, ",");
		while(token1 != NULL){
			switch(turn){
				case 0: 
					sprintf(temp, "%s", token1);
					break;
				case 1:
					sprintf(hum, "%s", token1);
					break;
				default:
					sprintf(time, "%s", token1);
					break;
			}
	
			token1 = strtok(NULL, ",");
			turn ++;
		}
		
		printf("Temperature = %s, Humidity = %s, Time = %s\n", temp, hum, time);
		loadDataToDB(1, temp, hum, time);
		
	}else if(strstr(topicName, ACKNOWLEDGE) != NULL){
		
	}else if(strstr(topicName, SERVERACTION) != NULL){
		serverAction(message->payload);
	}
	
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
	
	
    return 1;
}

void serverAction(char * message){
	
	if(strstr(message, REALTIME) != NULL || strstr(message, CONFIGMESSAGE) != NULL){
		loadDataToServer(message);
	}else if(strstr(message, STATISTICS) != NULL){
		loadDataToServer(message);
	}else if(strstr(message, "Change settings;") != NULL){
		
		char* ptr = strstr(message, "Change settings;");
		ptr += strlen("Change settings;");
		printf("Ptr = %s\n", ptr);
		
		char*  settings = malloc(strlen(ptr) - strlen(strstr(ptr, ";unitID")) + 1);
		memcpy(settings, ptr, strlen(ptr) - strlen(strstr(ptr, ";unitID")));
		printf("Settings = %s\n", settings);
		

		ptr = strstr(message, ";unitID=") + strlen(";unitID=");
		
		char* query = malloc(strlen(UPDATENAME) + strlen(settings) + strlen(ptr));
		sprintf(query, UPDATENAME, settings, atoi(ptr));
		
		free(settings);
		
		if(executeQuery(query) == 1) {
			printf("Unit updated successful!\n");
		}else{
			printf("Error in updating unit name!\n");
		}
		
		free(query);
		
	}else{
		char mode[strlen(SENSORANDRELAY) + 1];
		
		if(strstr(message, "enabled") != NULL){
			stpcpy(mode, SENSOR);
		}else if(strstr(message, SENSORANDRELAY) != NULL){
			stpcpy(mode, SENSORANDRELAY);
		}else if(strstr(message, RELAY)){
			stpcpy(mode, RELAY);
		}else if(strstr(message, SENSOR) != NULL){
			strcpy(mode, SENSOR);
		}else if(strstr(message, "disabled")){
			stpcpy(mode, IDLE);
		}
		
		printf("Mode: %s\n", mode);
	
		char* ptr = NULL;
		ptr = strstr(message, "unitID=") + strlen("unitID=");

		int size = strlen(UPDATERECORD) + strlen(mode) + strlen(ptr) + 1;
		char* query = malloc(size);
	
		sprintf(query, UPDATERECORD, mode, atoi(ptr)); 
		
		printf("Query = %s\n", query);

		if(executeQuery(query) == 1){
			printf("Record updated successful!\n");
		}else{
			printf("Error in updating record!\n");
		}
		
		free(query);  
		
		char* nodeMessage = (char*) malloc (strlen("opMode = ") + strlen(mode) + 1);
		sprintf(nodeMessage, "opMode=%s", mode);
		
		char* topic = (char*) malloc (strlen(SYSTEM) + 1 + strlen(CONFIG) + 1);
		sprintf(topic, "%s%d%s", SYSTEM, atoi(ptr), CONFIG);

		publishMessage(nodeMessage, topic);
		
		free(nodeMessage);
		free(topic);
	}
}

void createRecordInDB(char* unitID){
	char opMode[] = IDLE;

	printf("Preparing query...\n");
	
	char* query = malloc(strlen(INSERTRECORD) + strlen(unitID) + strlen(opMode) + 1);
	sprintf(query, INSERTRECORD, atoi(unitID), unitID, opMode);
	
	if(executeQuery(query) == 1){
		printf("Record successful!\n");
	}else{
		printf("Error in making record!\n");
	}
	
	free(query);
}

int unitExist(char* id){
	
	sqlite3 *db;
	sqlite3_stmt *stm;
	
	int result = 0;
	int rc = sqlite3_open("automation.db", &db);
	
	if(rc){
		printf("Filed to open the DB.\n");
		result = 1;
	}
	
	char* query = malloc(strlen(UNITEXISTENCE) + strlen(id) + 1);
	sprintf(query, UNITEXISTENCE, id);

	sqlite3_prepare_v2(db, query, -1, &stm, NULL);
	
	int count;
	
	while(sqlite3_step(stm) != SQLITE_DONE) {

		int col_num = sqlite3_column_count(stm);
		for (count = 0; count < col_num; count ++){
			result = 1;
			printf("There is sensor unit with id = %s\n", id);
			
			break;
		}
	}
	free(query);
	sqlite3_finalize(stm);
	
	sqlite3_close(db);
	
	return result;
}

void getData(char* query){
	
	sqlite3 *db;
	sqlite3_stmt *stmt, *stmt2;
	
	int rc = sqlite3_open("automation.db", &db);
	
	if(rc){
		printf("Failed to open\n");
	}
	
	int len = 0;
	int size = strlen("Stats,") + 1;
	int count = 0;
	
	char* delimiter = ",";
	char* result = malloc(size);
	memcpy(result, "Stats,", size);
	
	printf("Result: %s\n", result);
	
	sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
	
	while(sqlite3_step(stmt) != SQLITE_DONE) {

		int col_num = sqlite3_column_count(stmt);
		
		for (count = 0; count < col_num; count ++){
				
			switch (sqlite3_column_type(stmt, count))
			{
			case(SQLITE3_TEXT): ;
			
				len = strlen(sqlite3_column_text(stmt, count));
				size += len + strlen(delimiter);
				
				result = (char*) realloc (result, size);
				
				memcpy(result + size - (len + strlen(delimiter) + 1), sqlite3_column_text(stmt, count), len);
				memcpy(result + size - (strlen(delimiter) + 1), delimiter, strlen(delimiter) + 1);
				
				break;
				
			case(SQLITE_INTEGER):
				break;
			default: ;
				char data[6];

				sprintf(data, "%0.2f", sqlite3_column_double(stmt, count));

				len = strlen(data);
				size = size + len + strlen(delimiter);
				
				result = (char*) realloc (result, size);

				memcpy(result + size - (len + strlen(delimiter) + 1), data, len);
				memcpy(result + size - (strlen(delimiter) + 1), delimiter, strlen(delimiter) + 1);
				
				break;
			}
		}
	}
	
	sqlite3_finalize(stmt);
	sqlite3_close(db);
	
	printf("Result = %s\n", result);
	
	publishMessage(result, SERVER);
	
	free(result);	
}

void loadDataToServer(char* serverMessage){
	
	sqlite3 *db;
	sqlite3_stmt *stmt, *stmt2;
	
	int size = 1;
	
	char comma[] = ",";
	char* message = (char*) malloc (size);
	
	int len = 0;
	int flag = 0;
	int count;
	int rc = sqlite3_open("automation.db", &db);
	
	if(rc){
		printf("Failed to open\n");
	}
	
	printf("Performing query...\n");
	
	if(strstr(serverMessage, REALTIME) != NULL){
		sqlite3_prepare_v2(db, "SELECT name, id from SensorUnit WHERE opMode == 'Sensor' OR opMode == 'SensorAndRelay'", -1, &stmt, NULL);
	}else if(strstr(serverMessage, CONFIGMESSAGE) != NULL){
		sqlite3_prepare_v2(db, "SELECT * FROM SensorUnit", -1, &stmt, NULL);
		flag = 1;
	}else if(strstr(serverMessage, STATISTICS) !=NULL){
		if(strstr(serverMessage, DAY) != NULL){

			time_t t = time(NULL);
			struct tm tm = *localtime(&t);

			int hour = tm.tm_hour;
			int count;
			char start[] = "00:00";
			char end[12];
			
			char date[11];
			if(tm.tm_mon < 9 && tm.tm_mday < 10){
				sprintf(date, "%d-0%d-0%d", tm.tm_year + 1900, tm.tm_mday, tm.tm_mon + 1); 
			}else if(tm.tm_mday < 10){
				sprintf(date, "%d-0%d-%d", tm.tm_year + 1900, tm.tm_mday, tm.tm_mon + 1); 
			}else if(tm.tm_mon < 9){
				sprintf(date, "%d-%d-0%d", tm.tm_year + 1900, tm.tm_mday, tm.tm_mon + 1); 
			}else{
				sprintf(date, "%d-%d-%d", tm.tm_year + 1900, tm.tm_mday, tm.tm_mon + 1); 
			}
			
			sprintf(end, TIME, hour);
			char* id = strstr(serverMessage, "unitID=") + strlen("unitID=");

			char* query = (char*) malloc (strlen(DAILY) + strlen(id) + strlen(date) + 1);
			sprintf(query, DAILY, atoi(id), date);
			
			printf("Query = %s\n", query);

			getData(query);
			
			free(message);
			free(query);
			
			return;
		}else if(strstr(serverMessage, MONTH)){

		}else{
		
			sqlite3_prepare_v2(db, "SELECT id, name from SensorUnit WHERE opMode = 'Sensor' OR opMode = 'SensorAndRelay'", -1, &stmt, NULL);
			flag = 1;
			printf("Flag = %d\n", flag);
		}
	}
		
	printf("Got results:\n");

	while(sqlite3_step(stmt) != SQLITE_DONE) {

		int col_num = sqlite3_column_count(stmt);

		for (count = 0; count < col_num; count ++){
				
			switch (sqlite3_column_type(stmt, count))
			{
			case(SQLITE3_TEXT):
				
				len = strlen(sqlite3_column_text(stmt, count));
				size = size + len + strlen(comma);
					
				message = (char*) realloc (message, size);
				memcpy(message + size - (len + strlen(comma) + 1), sqlite3_column_text(stmt, count), len);
				memcpy(message + size - 2, comma, 2);
					
				break;
				
			case(SQLITE_INTEGER): ;
				if(flag){
					char id[5];
					sprintf(id, "%d", sqlite3_column_int(stmt, count));
					
					len = strlen(id);
					size += len + strlen(comma);
					
					message = (char*) realloc (message, size);
					memcpy(message + size - (len + strlen(comma) + 1), id, len);
					memcpy(message + size - strlen(comma) - 1, comma, 2);
					
				}else{

					char* statement = malloc (strlen(REALTIMEDATA) + 10);
					sprintf(statement, REALTIMEDATA, sqlite3_column_int(stmt, count));
						
					printf("Statement = %s\n", statement);

					//if(sqlite3_prepare_v2(db, statement, -1, &stmt2, NULL) == SQLITE_OK && sqlite3_step(stmt2) != SQLITE_DONE){		
						sqlite3_prepare_v2(db, statement, -1, &stmt2, NULL);
						while(sqlite3_step(stmt2) != SQLITE_DONE) {
							
							char temp[5];
							char hum[5]; 
							
							sprintf(temp, "%0.2f", sqlite3_column_double(stmt2, 0));
							sprintf(hum, "%0.2f", sqlite3_column_double(stmt2, 1));
							
							int tempLen = strlen(temp);
							int humLen = strlen(hum);
							
							size += tempLen + humLen + 2 * strlen(comma);
							message = (char*) realloc (message, size);
								
							memcpy(message + size - (tempLen + humLen + 3), temp, tempLen); 
							memcpy(message + size - (humLen + 3), comma, 1);
							memcpy(message + size - (humLen + 2), hum, humLen); 
							memcpy(message + size - 2, comma, 2); 
								
							printf("Temp = %s\n", temp);
							printf("Hum = %s\n", hum);
						}
					/*}else{
						printf("Dummy\n");
						char dummy[] = "0,0,";
						
						size += strlen(dummy);
						message = (char*) realloc (message, size);
						
						memcpy(message + size - (strlen(dummy) + 1), dummy, (strlen(dummy) + 1));						
					}*/
					free(statement);
				}
				break;
			default:
				printf("Result = %f\n", sqlite3_column_double(stmt, count));
				break;
			}
		}
		printf("Message = %s\n", message);
	}
	sqlite3_finalize(stmt);
	
	publishMessage(message, SERVER);
	
	free(message);
	sqlite3_close(db);
}

void loadDataToDB(int unitID, char* temp, char* hum, char* time){

	char* query = malloc(strlen(INSERTMESSAGE) + strlen(temp) + strlen(hum) + strlen(time));
	sprintf(query, INSERTMESSAGE, unitID, temp, hum, time);
		
	printf("Query = %s\n", query);
		
	if(executeQuery(query) == 1){
		printf("Insert data successful!\n");
	}else{
		printf("Error in inserting data!\n");
	}
	
	free(query);
}

void publishMessage(char* message, char* topic){
	
	int rc = 0;
	
	printf("Message %s\n", message);
	
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	
	pubmsg.payload = message;
    pubmsg.payloadlen = strlen(pubmsg.payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
	
	MQTTClient_publishMessage(client, topic, &pubmsg, &token);
    printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(TIMEOUT/20), pubmsg.payload, topic, CLIENTID);
    //rc = MQTTClient_waitForCompletion(client, token, TIMEOUT/20);
	
	if(token == -1){
		publishMessage(message, topic);
	}
	
	printf("Message with delivery token %d delivered\n", token);
}

void subscribeForSensorUnit(char* discover){
	
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	
	printf("%s\n", discover);
	
	const size_t systemLen = strlen(SYSTEM);
	const size_t discoverLen = strlen(discover);
	
	const size_t ackLen = strlen(ACKNOWLEDGE);
	const size_t configLen = strlen(CONFIG);
	
	char* configAckTopic = malloc(systemLen + discoverLen + ackLen + 1);
	
	memcpy(configAckTopic, SYSTEM, systemLen);
	memcpy(configAckTopic + systemLen, discover, discoverLen);
	memcpy(configAckTopic + systemLen + discoverLen, ACKNOWLEDGE, ackLen + 1);

	printf("Preparing\n");
	int count;
	for(count = 0; count < strlen(configAckTopic); count++){
		printf("%c ", configAckTopic[count]);
	}
	int rc = MQTTClient_subscribe(client, configAckTopic, QOS);
	printf("Topic: %s\n", configAckTopic);
	printf("Result from subscribe: %d\n", rc);
	
	if(rc == 0){
		printf("Subscribe for topic %s successful\n", configAckTopic);
	}

	free(configAckTopic);
	
	char* configTopic = malloc(systemLen + discoverLen + configLen);
	memcpy(configTopic, SYSTEM, systemLen);
	memcpy(configTopic + systemLen, discover, discoverLen);
	memcpy(configTopic + systemLen + discoverLen, CONFIG, configLen + 1);
	
	publishMessage(IDLEMESSAGE, configTopic);
	
	free(configTopic);
}