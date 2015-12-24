/*******************************************************************************
 * Copyright (c) 2012, 2013 IBM Corp.
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the Eclipse Public License v1.0
 * and Eclipse Distribution License v1.0 which accompany this distribution. 
 *
 * The Eclipse Public License is available at 
 *   http://www.eclipse.org/legal/epl-v10.html
 * and the Eclipse Distribution License is available at 
 *   http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * Contributors:
 *    Ian Craggs - initial contribution
 *******************************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "../MQTTClient.h"
#include "subalgoid.h"
#include "lib_crc.h"

#define ADDRESS     "192.168.1.1:1883"
#define CLIENTID    "ExampleClientSub"
#define TOPIC       "MQTT"

#define PAYLOAD     "Hello World! Sync"
#define QOS         0
#define TIMEOUT     10000L

volatile MQTTClient_deliveryToken deliveredtoken;

char mqtt_rcv_message[150];
unsigned char algo_command[3][50];


struct mqtt_msg{
	short msg_id;
	short msg_cmd;
	short msg_data;
	unsigned char msg_datarray[100];
} algoid;

int algo_GetMessageID(short dataLenght);

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    unsigned int i;
    char* payloadptr;


    printf("Message arrived\n");
    printf("     topic: %s\n", topicName);

    printf("     message: ");

    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        mqtt_rcv_message[i]=payloadptr[i];
        printf(" %d ",mqtt_rcv_message[i]);
    }
    putchar('\n');


    //--------- DECODAGE DU MESSAGE RECU

    unsigned short countDataFrame;


    unsigned short indexCommand;

    unsigned short dataLenght;
    unsigned short dataCommandCount;

    printf("Payload: %d\n",message->payloadlen);

    unsigned short crc16=0;
    unsigned short msg_crc;

    for(countDataFrame=0;countDataFrame < message->payloadlen-2;countDataFrame++){
    	crc16=update_crc_16(crc16, mqtt_rcv_message[countDataFrame]);
    }

    msg_crc=(mqtt_rcv_message[message->payloadlen-2]<<8)+(mqtt_rcv_message[message->payloadlen-1]);

    if(msg_crc==crc16){
       indexCommand=0;
       for(countDataFrame=0;countDataFrame < message->payloadlen-2;){

    	// Calcule de la longeur des donnée de la commande
    	    dataLenght=(mqtt_rcv_message[countDataFrame+1]<<8)+mqtt_rcv_message[countDataFrame+2];

        	printf("\n datalenght: %d -> ", dataLenght);
        	
        	// Récuperation des messages contenu dans la trame.
        	for(dataCommandCount=0;dataCommandCount < dataLenght+3;dataCommandCount++){
        		algo_command[indexCommand][dataCommandCount] = mqtt_rcv_message[countDataFrame];
        		printf(" %d ",algo_command[indexCommand][dataCommandCount]);
        		countDataFrame++;
        	}
        	indexCommand++;
    }

    printf("\n");
    }else printf("\n ***** MESSAGE INVALID, CRC MESSAGE: %d, CRC CALC: %d ***** \n",msg_crc ,crc16);

    //---------

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}

int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    int rc;
    int ch;

    MQTTClient_create(&client, ADDRESS, CLIENTID,
    MQTTCLIENT_PERSISTENCE_NONE, NULL);
    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);
    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);	
    }

    printf("Subscribing to topic %s\nfor client %s using QoS%d\n\n"
           "Press Q<Enter> to quit\n\n", TOPIC, CLIENTID, QOS);
    MQTTClient_subscribe(client, TOPIC, QOS);

    do 
    {
        ch = getchar();
    } while(ch!='Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}


//--------  algo_GetMessageIDalgo_GetMessageID
//Décode l'identifiant du message

int algo_GetMessageID(short dataLenght){

	return(1);
}
