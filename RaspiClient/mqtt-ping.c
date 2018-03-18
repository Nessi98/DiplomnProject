#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "MQTTClient.h"
#include "MQTTClientPersistence.h"

#define BROKER     "127.0.0.1"
#define CLIENTID    "raspi"
#define PAYLOAD     "Hello"
#define QOS         2	
#define TIMEOUT     10000L

volatile MQTTClient_deliveryToken deliveredtoken;

void delivered(void *context, MQTTClient_deliveryToken dt){
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int messageArrived(void *context, char *topicName, int topicLen, MQTTClient_message *message){
    int i;
    char* payloadptr;
    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);
    printf("   message: ");
    payloadptr = message->payload;
    for(i=0; i < message->payloadlen; i++)
    {
        putchar(*payloadptr++);
    }
    putchar('\n');
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
	
    pubmsg.payload = discover;
    pubmsg.payloadlen = 11;
    pubmsg.qos = QOS;
    pubmsg.retained = 0;
	
	rc = MQTTClient_subscribe(client, discover, QOS);
	if(rc == 0){
		printf("Subscribe for topic %s successful\n", discover);
	}
	//assert("Good rc from subscribe", rc == MQTTCLIENT_SUCCESS, "rc was %d", rc);
	
	int discoverLen = strlen(discover);
	MQTTClient_message* message = NULL;
	char* topicName = NULL;
	int topicLen;
	
	rc = MQTTClient_receive(client, &topicName, &topicLen, &message, 5000);
	printf("Message received on topic %s, lenght is %d \n", topicName, message->payloadlen);
	if (pubmsg.payloadlen != message->payloadlen || memcmp(message->payload, pubmsg.payload, message->payloadlen) != 0){}
	MQTTClient_free(topicName);
	MQTTClient_freeMessage(&message);

    
	MQTTClient_publishMessage(client, discover, &pubmsg, &token);
    printf("Waiting for up to %d seconds for publication of %s\n"
            "on topic %s for client with ClientID: %s\n",
            (int)(TIMEOUT/1000), PAYLOAD, discover, CLIENTID);
    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
    
	printf("Message with delivery token %d delivered\n", token);
    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    
	return rc;
}