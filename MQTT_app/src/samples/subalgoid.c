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

#define ADDRESS     "localhost:1883"
#define CLIENTID    "Algoid"
#define TOPIC       "MQTT"

#define QOS         0
#define TIMEOUT     10000L

MQTTClient_deliveryToken deliveredtoken, token;
MQTTClient client;

unsigned char PAYLOAD[100]={1,00,02,255,255,02,0,04, 0x15, 02, 0xaa, 0xaa, 0xa2, 0, 3, 61, 62, 63, 0,0};

char mqtt_rcv_message[309]; // 3x byte TL + 3x100 bytes V
unsigned char algo_command[3][103];
unsigned char algoMsgStackPtr;

struct mqtt_msg{
	unsigned short msg_id;

	unsigned short msg_type;
	int msg_type_value;
	short msg_param;
	int msg_param_value;
	unsigned char msg_param_array[100];
	unsigned short msg_param_count;
	char topicName[25];
} algoidMsgStack[10], algoidMsg;

void processMqttMsg(unsigned int msgLen, char *topicName);

int mqtt_init(void);

long algo_GetTypeValue(unsigned char *MsgVal);
long algo_GetParamValue(unsigned char *MsgVal, unsigned char byteLen);
void algo_clearStack(unsigned char ptr);
unsigned char algo_getMessage(void);

int main(int argc, char* argv[])
{
	int ch;
	int err;

	printf("\nTentative de connexion au brocker MQTT...\n");
	err=mqtt_init();

	if(!err){
		printf("- r�ussite, tentative de souscription topic MQTT: ");
		// Configuration souscription
		if(!MQTTClient_subscribe(client, TOPIC, QOS))printf("OK\n");
		else printf("ERREUR\n");
		// Connexion au topic 2
		printf("- r�ussite, tentative de souscription topic MQTT2: ");
		if(!MQTTClient_subscribe(client, "MQTT2", QOS))printf("OK\n");
		else printf("ERREUR\n");
	}else printf("Erreur de connexion au brocker !\n");

    do 
    {
        ch = getchar();
        if(ch=='d'){
        	if(algo_getMessage())printf("\nOK\n");
        	else printf("\nPAS DE MESSAGE\n");
        }
        if((ch=='a') & (!err)){

        	unsigned short msgID;
        	    short j;


        	        	msgID = rand();
        	        	msgID = (msgID & 0x0FFF) | 0xA000;
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

        	    		MQTTClient_publish(client, "MQTT2", 20, PAYLOAD, QOS, 0, &token);
//        	    	    rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);
        	    	    MQTTClient_waitForCompletion(client, token, TIMEOUT);
        	    		printf("Message with delivery token %d delivered\n", token);
        	    		sleep(1);
        }
    } while(ch != 'Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return 0;
}


void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
    //token=dt;
}

int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    unsigned int i;
    char* payloadptr;

    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        mqtt_rcv_message[i]=payloadptr[i];
    }

    processMqttMsg(message->payloadlen, topicName);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}

void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}


//--------  algo_GetMessageIDalgo_GetMessageID
//Decode l'identifiant du message
long algo_GetTypeValue(unsigned char *MsgVal){
	long myVal;
	unsigned short idLenght;

	unsigned char i;

	idLenght=(MsgVal[1]<<8)+MsgVal[2];

	myVal=0;

	for(i=0;i<idLenght;i++){
		myVal = (myVal<<8)+MsgVal[i+3];
	}
	return(myVal);
}

long algo_GetParamValue(unsigned char *MsgVal, unsigned char byteLen){
	long myVal;
	unsigned short idLenght;

	unsigned char i;
	idLenght=(MsgVal[1]<<8*byteLen)+MsgVal[2];

	myVal=0;

	for(i=0;i<idLenght;i++){
		myVal = (myVal<<8)+MsgVal[i+3];
	}
	return(myVal);
}

// Efface les champs de la stucture
void algo_clearStack(unsigned char ptr){
	unsigned char i;

	algoidMsgStack[ptr].msg_id=0;
	algoidMsgStack[ptr].msg_type=0;
	algoidMsgStack[ptr].msg_type_value=0;
	algoidMsgStack[ptr].msg_param=0;
	algoidMsgStack[ptr].msg_param_count=0;
	algoidMsgStack[ptr].msg_param_value=0;
	for(i=0;i<100;i++) algoidMsgStack[ptr].msg_param_array[i]=0;
	for(i=0;i<sizeof(algoidMsgStack[ptr].topicName);i++)algoidMsgStack[ptr].topicName[i]=0;
}

// Traite la trame MQTT recu
void processMqttMsg(unsigned int msgLen, char *topicName){

    //--------- DECODAGE DU MESSAGE RECU
	unsigned int i;
    unsigned short countDataFrame;

    unsigned short indexCommand;

    unsigned short dataLenght;
    unsigned short dataCommandCount;

    printf("Topic: %s\n", topicName);
    printf("Payload: %d\n",msgLen);

    unsigned short crc16=0;
    unsigned short msg_crc;

    for(countDataFrame=0;countDataFrame < msgLen-2;countDataFrame++){
    	crc16=update_crc_16(crc16, mqtt_rcv_message[countDataFrame]);
    }

    msg_crc=(mqtt_rcv_message[msgLen-2]<<8)+(mqtt_rcv_message[msgLen-1]);

    printf("\n CRC MESSAGE: %d, CRC CALC: %d  \n",msg_crc ,crc16);

    if(msg_crc==crc16){
       indexCommand=0;


       for(countDataFrame=0;countDataFrame < msgLen-2;){

    	// Calcule de la longeur des donn�ee de la commande
    	    dataLenght=(mqtt_rcv_message[countDataFrame+1]<<8)+mqtt_rcv_message[countDataFrame+2];

        	printf("\n datalenght: %d -> ", dataLenght);

        	// Recuperation des messages contenu dans la trame.
        	for(dataCommandCount=0;dataCommandCount < dataLenght+3;dataCommandCount++){
        		algo_command[indexCommand][dataCommandCount] = mqtt_rcv_message[countDataFrame];
        		printf(" %d ",algo_command[indexCommand][dataCommandCount]);
        		countDataFrame++;
        	}
        	indexCommand++;
    }

       printf("\n");
       // Recherche un emplacement libre dans la pile de messages
       for(algoMsgStackPtr=0;(algoidMsgStack[algoMsgStackPtr].msg_id!=0) && algoMsgStackPtr<10;algoMsgStackPtr++);

       if(algoMsgStackPtr>=10)
    	   printf("\n!!! ALGO MESSAGES STACK OVERFLOW !!!\n");
       else
    	   strcpy(algoidMsgStack[algoMsgStackPtr].topicName,topicName);

		   for(indexCommand=0;indexCommand<3;indexCommand++){
			   switch( algo_command[indexCommand][0]){
					case T_MSGID : algoidMsgStack[algoMsgStackPtr].msg_id=algo_GetTypeValue(algo_command[indexCommand]);
								  break;
					case T_CMD	: algoidMsgStack[algoMsgStackPtr].msg_type=T_CMD;
								  algoidMsgStack[algoMsgStackPtr].msg_type_value=algo_GetTypeValue(algo_command[indexCommand]);
								  break;
					case T_MSGANS : algoidMsgStack[algoMsgStackPtr].msg_type=T_MSGANS;
									algoidMsgStack[algoMsgStackPtr].msg_type_value=algo_GetTypeValue(algo_command[indexCommand]);
									break;
					case T_MSGACK : algoidMsgStack[algoMsgStackPtr].msg_type=T_MSGACK;
									algoidMsgStack[algoMsgStackPtr].msg_type_value=algo_GetTypeValue(algo_command[indexCommand]);
									break;
					case T_EVENT : algoidMsgStack[algoMsgStackPtr].msg_type=T_EVENT;
								   algoidMsgStack[algoMsgStackPtr].msg_type_value=algo_GetTypeValue(algo_command[indexCommand]);
								   break;
					case T_ERROR : algoidMsgStack[algoMsgStackPtr].msg_type=T_ERROR;
								   algoidMsgStack[algoMsgStackPtr].msg_type_value=algo_GetTypeValue(algo_command[indexCommand]);
								   break;

					case T_IDNEG : break;

					case PS_BOOL : break;
					case PS_INT : algoidMsgStack[algoMsgStackPtr].msg_param=PS_INT;
									algoidMsgStack[algoMsgStackPtr].msg_param_value=algo_GetParamValue(algo_command[indexCommand], 4); break;
					case PS_CHAR : algoidMsgStack[algoMsgStackPtr].msg_param=PS_CHAR;
									algoidMsgStack[algoMsgStackPtr].msg_param_value=algo_GetParamValue(algo_command[indexCommand], 1); break;
					case PS_SHORT : algoidMsgStack[algoMsgStackPtr].msg_param=PS_SHORT;
									algoidMsgStack[algoMsgStackPtr].msg_param_value=algo_GetParamValue(algo_command[indexCommand], 2); break;
					case PS_HOLE : break;
					case PS_COLL : break;
					case PS_COLR : break;

					case PA_INT : break;
					case PA_STR : break;
					default : break;
			   }
		   }

    }else printf("\n ***** MESSAGE INVALID, CRC MESSAGE: %d, CRC CALC: %d ***** \n",msg_crc ,crc16);

    for (i=0;i<10;i++)
    {
    	printf("\n#%d Algo Topic: %s, Sender: %x, message ID: %d, command: %d, value: %d, param: %d, value: %d",i,algoidMsgStack[i].topicName, (algoidMsgStack[i].msg_id&0xF000), algoidMsgStack[i].msg_id, algoidMsgStack[i].msg_type,
    	    		   	   	   	   	   	   	   	   	   	   	   	   	   	 algoidMsgStack[i].msg_type_value, algoidMsgStack[i].msg_param,
    																	 algoidMsgStack[i].msg_param_value);
    }
    printf("\n");
    //---------

}


unsigned char algo_getMessage(void){
	unsigned char i;
	if(algoidMsgStack[0].msg_id != 0){
		algoidMsg=algoidMsgStack[0];

		for(i=0;i<9;i++){
			algoidMsgStack[i]=algoidMsgStack[i+1];
		}
		algo_clearStack(9);
		return 1;
	}
	else{
		return 0;
	}
}


// INITIALISATION DE LA CONNEXION AU BROCKER MQTT
int mqtt_init(void){
	int rc;

	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	//MQTTClient_message pubmsg = MQTTClient_message_initializer;

	// Configuration des param�tres de connexion
	MQTTClient_create(&client, ADDRESS, CLIENTID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

	conn_opts.keepAliveInterval = 20;
	conn_opts.cleansession = 1;
	// Fin de config connexion

	// Configuration de la fonction callback de souscription
	MQTTClient_setCallbacks(client, NULL, connlost, msgarrvd, delivered);

	// Tentative de connexion au broker mqtt
	if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
	{
		printf("Failed to connect to MQTT brocker, return code %d\n", rc);
		return(rc);
	}else return 0;
}
