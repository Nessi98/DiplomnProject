#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>
#include <math.h>

#include "MQTTClient.h"
#include "MQTTClientPersistence.h"

#define BROKER     "127.0.0.1"
#define CLIENTID    "raspi"
#define PAYLOAD     "001"
#define QOS         2	
#define TIMEOUT     1000L

// Topic headers
#define SERVER			"/system_name/server"
#define SERVERACTION	"/system_name/server_action"
#define CONFIG			"/config"
#define DATA			"/data"
#define ACKNOWLEDGE 	"/config_ack"
#define SYSTEM			"/system_name/"
#define DISCOVER		"/system_name/discover"

#define IDLEMESSAGE 	"op_Mode: IDLE; sleep_time: 50000"
#define REALTIMEMESSAGE "Real Time"
#define CONFIGMESSAGE	"Config"

#define QUERY 			"SELECT temp, hum FROM Data Where unitID = %d ORDER BY time DESC LIMIT 1"
#define UNITEXISTENCE	"SELECT name FROM SensorUnit WHERE id = %s"
#define INSERTMESSAGE	"INSER INTO Data (unitID, temp, hum, time) VALUES (%d, %s, %s, %s)"

static MQTTClient client;

volatile MQTTClient_deliveryToken deliveredtoken;

int unitExist(char* id);
void createRecordInDB(char* unitID);
void loadDataToDB(int unitID, char* dataMessage);
void loadDataToServer(char * serverMessage);

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
		printf("Subscribed for topic %s successful\n", SERVER);
	}
	
	loadDataToDB(1, "24.5, 48%, 2018/11/24 15:45:78");
    
	while(1){}
	
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
	
	return rc;
}

void connlost(void *context, char *cause){
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

void delivered(void *context, MQTTClient_deliveryToken dt){
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken =  dt;
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
		//check if the arrieved discover already exist in the db
		if(!unitExist(message->payload)){
			createRecordInDB(message->payload);
			subscribeForSensorUnit(message->payload);
		}else{
			printf("Record already exitst in the DB!\n");
		}
	}else if(strstr(topicName, DATA) != NULL){
		loadDataToDB(1, message->payload);
	}else if(strstr(topicName, ACKNOWLEDGE) != NULL){
		
	}else if(strstr(topicName, SERVERACTION) != NULL){
		loadDataToServer(message->payload);
	}
	
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
	
	
    return 1;
}

void createRecordInDB(char* unitID){
	
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
	
	char query[strlen(UNITEXISTENCE) + strlen(id) + 1];
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
	
	sqlite3_finalize(stm);
	
	sqlite3_close(db);
	
	return result;
}

void loadDataToServer(char* serverMessage){
	
	sqlite3 *db;
	sqlite3_stmt *stmt, *stmt2;
	
	int size = 1;
	
	char* comma = ",";
	char* message = (char*) malloc (size);
	
	int len;
	int flag = 0;
	int count;
	int rc = sqlite3_open("automation.db", &db);
	
	if(rc){
		printf("Failed to open\n");
		//return -1;
	}
	
	printf("Performing query...\n");
	if(strstr(serverMessage, REALTIMEMESSAGE) != NULL){
		sqlite3_prepare_v2(db, "SELECT name, id from SensorUnit WHERE opMode != 'IDLE'", -1, &stmt, NULL);
	}else if(strstr(serverMessage, CONFIGMESSAGE) != NULL){
		sqlite3_prepare_v2(db, "SELECT name, opMode from SensorUnit", -1, &stmt, NULL);
		flag = 1;
	}else{
		int id = 2;
		time_t t = time(NULL);
		struct tm tm = *localtime(&t);
	
		char start[19];
		char end[19];
		sprintf(start, "%d/%d/%d 00:00:00", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);
		sprintf(end, "%d/%d/%d 23:59:59", tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday);

		printf("now: %s\n", start);
		
		char query[255];
		//sprintf(query, "SELECT temp, hum, time FROM Data WHERE unitID = %d AND time > %s AND time < %s", id, start, end);
		sprintf(query, "SELECT temp, hum, time FROM Data WHERE unitID = %d", id);
		printf("%s\n", query);
		sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
	}
		
	printf("Got results:\n");
	while(sqlite3_step(stmt) != SQLITE_DONE) {

		int col_num = sqlite3_column_count(stmt);

		for (count = 0; count < col_num; count ++){
				
			switch (sqlite3_column_type(stmt, count))
			{
			case(SQLITE3_TEXT):
				
				len = strlen(sqlite3_column_text(stmt, count)) + 1;
				size += len;
					
				message = (char*) realloc (message, size);
				memcpy(message + size - (len + 1), sqlite3_column_text(stmt, count), len);
				memcpy(message + size - 2, comma, 2);
				printf("Unit Name = %s\n", sqlite3_column_text(stmt, count));
					
				break;
				
			case(SQLITE_INTEGER): ;

				char* statement = malloc (strlen(QUERY) + 10);
				sprintf(statement, QUERY, sqlite3_column_int(stmt, count));
					
				printf("Statement = %s\n", statement);
					
				sqlite3_prepare_v2(db, statement, -1, &stmt2, NULL);
					
				while(sqlite3_step(stmt2) != SQLITE_DONE) {
						
					int tempLen = strlen(sqlite3_column_text(stmt2, 0)); 
					int humLen = strlen(sqlite3_column_text(stmt2, 1));
					
					size += tempLen + humLen + 2 * strlen(comma);
					message = (char*) realloc (message, size);
						
					memcpy(message + size - (tempLen + humLen + 3), sqlite3_column_text(stmt2, 0), tempLen); 
					memcpy(message + size - (humLen + 3), comma, 1);
					memcpy(message + size - (humLen + 2), sqlite3_column_text(stmt2, 1), humLen); 
					memcpy(message + size - 2, comma, 2); 
						
					printf("Temp = %s\n", sqlite3_column_text(stmt2, 0));
					printf("Hum = %s\n", sqlite3_column_text(stmt2, 1));
				}
				
				free(statement);
				
				printf("Unit id = %d\n", sqlite3_column_int(stmt, count));
				break;
			}
		printf("Message = %s\n", message);
		}
	}
	sqlite3_finalize(stmt);
	
	publishMessage(message, SERVER);
	
	free(message);
	sqlite3_close(db);
}

void loadDataToDB(int unitID, char* dataMessage){
	
	sqlite3 *db;
	sqlite3_stmt *stmt, *stmt2;

	int rc = sqlite3_open("automation.db", &db);
	
	if(rc){
		printf("Failed to open\n");
	}else{
		//char* query = malloc(strlen(INSERTMESSAGE) + strlen(dataMessage) + 2);
		char* token;
		
		token = strtok(dataMessage, ", ");
		while(token != NULL){
			printf("%s\n", token);	
			token = strtok(NULL, ", ");
		}
		//sprintf(query, INSERTMESSAGE, unitID, temp, hum, time);
		
		//printf("Query = %s\n", query);
		
		//free(query);
	}
	
	sqlite3_close(db);
}

void publishMessage(char* message, char* topic){
	
	int rc;
	
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	
	pubmsg.payload = message;
    pubmsg.payloadlen = strlen(pubmsg.payload);
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
	
	MQTTClient_publishMessage(client, topic, &pubmsg, &token);
    printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(TIMEOUT/700), pubmsg.payload, topic, CLIENTID);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT/700);
	
	printf("Message with delivery token %d delivered\n", token);
}

void subscribeForSensorUnit(char* discover){
	
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	MQTTClient_deliveryToken token;
	
	printf("%s\n", discover);
	
	const size_t systemLen = strlen(SYSTEM);
	const size_t discoverLen = strlen(discover);
	
	const size_t dataLen = strlen(DATA);
	const size_t ackLen = strlen(ACKNOWLEDGE);
	const size_t configLen = strlen(CONFIG);
	
	char* sensorDataTopic = malloc(systemLen + discoverLen + dataLen  + 1);
	
	memcpy(sensorDataTopic, SYSTEM, systemLen);
	memcpy(sensorDataTopic + systemLen, discover, discoverLen);
	memcpy(sensorDataTopic + systemLen + discoverLen, DATA, dataLen + 1);
	
	char* configAckTopic = malloc(systemLen + discoverLen + ackLen + 1);
	
	memcpy(configAckTopic, SYSTEM, systemLen);
	memcpy(configAckTopic + systemLen, discover, discoverLen);
	memcpy(configAckTopic + systemLen + discoverLen, ACKNOWLEDGE, ackLen + 1);
	
	int rc = MQTTClient_subscribe(&client, configAckTopic, QOS);
	printf("Topic: %s\n", configAckTopic);
	if(rc == 0){
		printf("Subscribe for topic %s successful\n", configAckTopic);
	}

	free(sensorDataTopic);
	free(configAckTopic);
	
	char* configTopic = malloc(systemLen + discoverLen + configLen);
	memcpy(configTopic, SYSTEM, systemLen);
	memcpy(configTopic + systemLen, discover, discoverLen);
	memcpy(configTopic + systemLen + discoverLen, CONFIG, configLen + 1);
	
	publishMessage(IDLEMESSAGE, configTopic);
	
	free(configTopic);
}