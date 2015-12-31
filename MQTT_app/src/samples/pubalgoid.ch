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

#include "lib_crc.h"
#include "time.h"

#define ADDRESS     "localhost:1883"
#define CLIENTID    "ExampleClientPub"
#define TOPIC       "MQTT"
//#define PAYLOAD     "Sync Hello World"
#define QOS         0
#define TIMEOUT     10000L


unsigned char PAYLOAD[100]={1,00,02,255,255,02,0,04, 0x15, 02, 0xaa, 0xaa, 0xa2, 0, 3, 61, 62, 63, 0,0};

int main(int argc, char* argv[])
{
    MQTTClient client;
    MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
    MQTTClient_message pubmsg = MQTTClient_message_initializer;
    MQTTClient_deliveryToken token;
    int rc;

    MQTTClient_create(&client, ADDRESS, CLIENTID,
        MQTTCLIENT_PERSISTENCE_NONE, NULL);

    conn_opts.keepAliveInterval = 20;
    conn_opts.cleansession = 1;

    if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
    {
        printf("Failed to connect, return code %d\n", rc);
        exit(-1);
    }
    unsigned short msgID;
    short i;
    short j;

    for (i=0;i<3600;i++){
    	msgID = rand();
    	printf("\nMessage ID: %d", msgID);
		PAYLOAD[3]=(msgID & 0xFF00)>>8;
		PAYLOAD[4]=msgID & 0x00FF;

		PAYLOAD[8]= rand() & 0x00FF;
		PAYLOAD[15]= rand() & 0x00FF;

	    unsigned short crc16=0;
	    for(j=0;j < 18;j++){
	    	crc16=update_crc_16(crc16, PAYLOAD[j]);
	    }

	    printf("\nmy CRC: %d\n", crc16);

	    PAYLOAD[18]=(crc16&0xFF00)>>8;
	    PAYLOAD[19]=(crc16&0x00FF);

		MQTTClient_publish(client, TOPIC, 20, PAYLOAD, QOS, 0, &token);

		printf("Waiting for up to %d seconds for publication of %s\n"
		        "on topic %s for client with ClientID: %s\n",
		         (int)(TIMEOUT/1000), PAYLOAD, TOPIC, CLIENTID);
	    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
		printf("Message with delivery token %d delivered\n", token);
		sleep(2);
    }


    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return rc;
}