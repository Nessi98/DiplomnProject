#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MQTTClient.h"
#include "MQTTClientPersistence.h"

#define BROKER     "127.0.0.1"
#define CLIENTID    "raspi"
#define PAYLOAD     "001"
#define QOS         2	
#define TIMEOUT     10000L
#define DATA		"/sensor_data"
#define ACKNOWLEDGE "/config_ack"
#define SYSTEM		"/system_name/"
#define DISCOVER	"system_name/discover"

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt){
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken =  dt;
}

void subscribeForSensorUnit(MQTTClient* client, char* discover){

	const size_t len1 = strlen(SYSTEM);
	const size_t len2 = strlen(discover);
	int len3 = strlen(DATA);
	
	char* sensorDataTopic = malloc(len1 + len2 + len3  + 1);
	memcpy(sensorDataTopic, SYSTEM, len1);
	memcpy(sensorDataTopic + len1, discover, len2);
	memcpy(sensorDataTopic + len1 + len2, DATA, len3 + 1);
	
	len3 = strlen(ACKNOWLEDGE);
	
	char* configAckTopic = malloc(len1 + len2 + len3 + 1);
	memcpy(configAckTopic, SYSTEM, len1);
	memcpy(configAckTopic + len1, discover, len2);
	memcpy(configAckTopic + len1 + len2, ACKNOWLEDGE, len3 + 1);
	
	int rc = MQTTClient_subscribe(client, configAckTopic, 1);
	printf("Status: %d\n", rc);
	if(rc == 0){
		printf("Subscribe for topic %s successful\n", configAckTopic);
	}
	printf("Data Topic - %s\n", sensorDataTopic);
	printf("Config Ack Topic - %s\n", configAckTopic);
	free(sensorDataTopic);
}

int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message){
    int i;
    char* payloadptr;
	printf("Topic Len %d\n", topicLen);
    printf("Message arrived\n");
    printf("    topic: %s\n", topicName);
    printf("	message: ");
    payloadptr = message->payload;
	
    for(i = 0; i < message->payloadlen; i++){
        putchar(*payloadptr++);
    }
	
	if(strstr(topicName, DISCOVER) != NULL){
		//subscribeForSensorUnit(payloadptr);
	}else if(strstr(topicName, DATA) != NULL){
		
	}else if(strstr(topicName, ACKNOWLEDGE) != NULL){
		
	}
	
	putchar('\n');
	printf("Message len %d\n",  message->payloadlen);
    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause){
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}


int main(int argc, char* argv[]){
	
	int rc;
	char* username = "mosquitto"; 
	char* password = "password";
	char* discover = "/system_name/discover";	
	char* config = "/system_name/config";
	
	MQTTClient client;
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
	
	printf("Connection success\n");
	
    pubmsg.payload = config;
    pubmsg.payloadlen = 11;
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
	
	rc = MQTTClient_subscribe(client, discover, QOS);
	if(rc == 0){
		printf("Subscribe for topic %s successful\n", discover);
	}
    
	MQTTClient_publishMessage(client, config, &pubmsg, &token);
    printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(TIMEOUT/1000), PAYLOAD, discover, CLIENTID);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    
	printf("Message with delivery token %d delivered\n", token);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
     
	subscribeForSensorUnit(client, "001");
	 
	return rc;
}