/*******************************************************************************
 *	Librairie pour communication protocol ALGOID
 *	Nï¿½cï¿½ssite les source PAHO MQTT.
 *	MAJ: 04.01.2016 / Raphael Thurnherr
 *
 *
 *******************************************************************************/

#include "stdio.h"
#include "stdlib.h"
#include "string.h"
#include "lib_mqtt/MQTTClient.h"
#include "subalgoid.h"
#include "lib_crc.h"

#define ADDRESS     "localhost:1883"
//#define ADDRESS     "raphdev.ddns.net:1883"
#define CLIENTID    "Algoid"
#define TOPIC       "MQTT"
#define QOS         0
#define TIMEOUT     10000L
#define SENDERID	0x0A

MQTTClient_deliveryToken deliveredtoken, token;
MQTTClient client;

// Variable d'entrï¿½e/sortie vers algoid
ALGOID algoidMsgRXStack[10], algoidMsgRX, algoidMsgTX;


int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message); 	// Call-back message MQTT recu
void delivered(void *context, MQTTClient_deliveryToken dt);		// Call-back message MQTT emis
void connlost(void *context, char *cause);						// Call-back perte de connexion MQTT

// Initialisation connexion MQTT (IPADDR:PORT, CLIENT ID, FUNC CALL-BACK MSG RECU)
int mqtt_init(const char *IPaddress, const char *clientID, MQTTClient_messageArrived* msgarr);

// Construction message algoid, controle CRC algo message, empilage des messages dans variable type ALGOID
void processMqttMsg(char *mqttMsg, unsigned int msgLen, char *topicName, ALGOID *destMsgStack);

unsigned short buildMqttMsg(char *mqttMsgTX, ALGOID srcMsg);

// Retourne la donnï¿½e codï¿½e sur n bytes
long algo_GetValue(unsigned char *MsgVal, unsigned char byteLen);

// Rï¿½cupï¿½re le premier message disponible dans la pile
unsigned char algo_getMessage(ALGOID destMsg, ALGOID *srcMsgStack);

// Efface un champs donnï¿½ dans la pile de message
void algo_clearStack(unsigned char ptr, ALGOID *destMsgStack);

// !!!!!!!!!!!   FONCTION DEBUG  A RETRAVAILLER...
int algo_putMessage(char *topic, unsigned char *data, unsigned short lenght);



void error(char *msg) {
    perror(msg);
    exit(0);
}


// ------------------------------------------------------------------------------------
// MAIN: Point d'entrï¿½e programme, initialisation connexion MQTT, souscription et publication
// ------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{

	// Trame de test de type ALGOID a publier
	unsigned char PAYLOADRX[MAXMQTTBYTE]={1,00,02,255,255,02,0,04, 0x15, 02, 0xaa, 0xaa, 0xa2, 0, 3, 61, 62, 63, 0,0};
	unsigned char PAYLOADTX[MAXMQTTBYTE]={1,00,02,255,255,02,0,04, 0x15, 02, 0xaa, 0xaa, 0xa2, 0, 3, 61, 62, 63, 0,0};

	int ch;
	int err;
	unsigned char i,j, ptrParam;

	printf("\nMQQT-ALGO POC 19/01/2016\n");
	printf("\nTentative de connexion au brocker MQTT...\n");

	err=mqtt_init(ADDRESS, CLIENTID, msgarrvd);

	if(!err){
		printf("- reussite, tentative de souscription topic MQTT: ");
		// Configuration souscription
		if(!MQTTClient_subscribe(client, TOPIC, QOS))printf("OK\n");
		else printf("ERREUR\n");
	}else printf("Erreur de connexion au brocker !\n");

    do 
    {
    	// - Test: traitement du message algoid recu
    	if(AlgoidMessageReady){
    		if(AlgoidMessageReady==1){
    			//printf("\n Main: MESSAGE MQTT ALGOID VALID RECU: %d \n", AlgoidMessageReady);

    			// Affichage des messages de la piles
    		    for (i=0;i<10;i++)
    		    {
    		    	printf("\n#%d Algo Topic: %s, Sender: %x, message ID: %d, command: %d, cmd value: %d",i,algoidMsgRXStack[i].topicName, ((algoidMsgRXStack[i].msg_id&0xFF000000)>>24), algoidMsgRXStack[i].msg_id, algoidMsgRXStack[i].msg_type,
    		    	    		    			algoidMsgRXStack[i].msg_type_value);
    		    	for(ptrParam=0;algoidMsgRXStack[i].msg_param[ptrParam]!=0;ptrParam++){

    		    	 if(algoidMsgRXStack[i].msg_param[ptrParam]==0xa1)

    		    		 printf("\n        value: %d",algoidMsgRXStack[i].msg_param_value[ptrParam]);
    		    	 if(algoidMsgRXStack[i].msg_param[ptrParam]==0xb1){
    		    		 printf("\n        array: ");
    		    		 for(j=0;j<algoidMsgRXStack[i].msg_param_count[ptrParam];j++) printf("%d ", algoidMsgRXStack[i].msg_param_array[ptrParam][j]);
    		    	 }
    		    	}
    		    	printf("\n");
    		    }
    		    printf("\n");
    		}
    		else printf("\n Main: MESSAGE MQTT NON-ALGOID RECU \n");

    		AlgoidMessageReady=0;
    	}
    	// -----------------------

        ch = getchar();
        if(ch=='d'){
        	if(algo_getMessage(algoidMsgRX, algoidMsgRXStack))printf("\nOK\n");
        	else printf("\nPAS DE MESSAGE\n");
        }

        if(ch=='p'){
        	int result;
        	unsigned char nbChar;

        	algoidMsgTX.msg_id = rand()&0x00FFFFFF;
        	algoidMsgTX.msg_id |= SENDERID<<24;

        	algoidMsgTX.msg_type=T_CMD;
        	algoidMsgTX.msg_type_value=0x03;

        	algoidMsgTX.msg_param[1]=PAS_1;
        	algoidMsgTX.msg_param_count[1]=rand()&0x000000F;
        	for(i=0;i<algoidMsgTX.msg_param_count[1];i++)
        		algoidMsgTX.msg_param_array[1][i]=rand()&0x0000FFFF;

        	algoidMsgTX.msg_param[0]=PS_1;
        	algoidMsgTX.msg_param_value[0]=((rand()<<16)+rand())&0x00FFFFFF;


          	algoidMsgTX.msg_param[2]=PS_1;
            	algoidMsgTX.msg_param_value[2]=((rand()<<16)+rand())&0x00FFFFFF;

        	algoidMsgTX.msg_param[3]=PAS_1;
        	algoidMsgTX.msg_param_count[3]=rand()&0x000000F;
        	for(i=0;i<algoidMsgTX.msg_param_count[3];i++)
        		algoidMsgTX.msg_param_array[3][i]=rand()&0x0000FFFF;


        	algoidMsgTX.msg_param[4]=PAS_1;
        	algoidMsgTX.msg_param_count[4]=rand()&0x000000F;
        	for(i=0;i<algoidMsgTX.msg_param_count[4];i++)
        		algoidMsgTX.msg_param_array[4][i]=rand()&0x0000FFFF;

        	nbChar = buildMqttMsg(PAYLOADTX, algoidMsgTX);

        	result=algo_putMessage("MQTT",PAYLOADTX, nbChar);

        	if(!result) printf("\n Message with delivery token %d delivered\n", token);
        }
    } while(ch != 'Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);

    return 0;
}


// -------------------------------------------------------------------
// Fonction Call-back de retour de token MQTT pour contrï¿½le
// -------------------------------------------------------------------

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}


// -------------------------------------------------------------------
// Fonction Call-Back rï¿½cï¿½ption d'un message MQTT
// -------------------------------------------------------------------
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    unsigned int i;
    char* payloadptr;
    char mqtt_rcv_message[500]; // 3x byte TL + 3x100 bytes V

    payloadptr = message->payload;
    for(i=0; i<message->payloadlen; i++)
    {
        mqtt_rcv_message[i]=payloadptr[i];
    }

    // Reconstruction du message ALGOID
    processMqttMsg(mqtt_rcv_message, message->payloadlen, topicName, algoidMsgRXStack);

    MQTTClient_freeMessage(&message);
    MQTTClient_free(topicName);
    return 1;
}


// -------------------------------------------------------------------
// Fonction call-back perte de connexion avec le brocker
// -------------------------------------------------------------------
void connlost(void *context, char *cause)
{
    printf("\nConnection lost\n");
    printf("     cause: %s\n", cause);
}


// -------------------------------------------------------------------
// retourne la valeur codï¿½e sur n bytes, d'un champ de la trame algoid
// -------------------------------------------------------------------
long algo_GetValue(unsigned char *MsgVal, unsigned char byteLen){
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


// -------------------------------------------------------------------
// Efface les champs d'un emplacemment donnï¿½ dans la pile
// -------------------------------------------------------------------
void algo_clearStack(unsigned char ptr, ALGOID *destMsgStack){
	unsigned char i, ptrParam;

	destMsgStack[ptr].msg_id=0;
	destMsgStack[ptr].msg_type=0;
	destMsgStack[ptr].msg_type_value=0;
	for(ptrParam=0;ptrParam<MAXPARAM;ptrParam++){
		destMsgStack[ptr].msg_param[ptrParam]=0;
		destMsgStack[ptr].msg_param_count[ptrParam]=0;
		destMsgStack[ptr].msg_param_value[ptrParam]=0;
		for(i=0;i<MAX_SHORT_ARRAY;i++) destMsgStack[ptr].msg_param_array[ptrParam][i]=0;
	}
	for(i=0;i<sizeof(destMsgStack[ptr].topicName);i++)destMsgStack[ptr].topicName[i]=0;
}


// -------------------------------------------------------------------
// Construction message algoid, controle CRC algo message, empilage des messages dans variable type ALGOID
// -------------------------------------------------------------------
void processMqttMsg(char *mqttMsg, unsigned int msgLen, char *topicName, ALGOID *destMsgStack){

    unsigned short crc_calc=0;						// crc calculé
    unsigned short crc_msg, i;					// crc message

	unsigned char ptrParam, ptrInstruction =0;	// pointeur de param dans linstruction, pointeur d'instruction
	static unsigned char algoMsgStackPtr;		// Pointeur du message algoid dans la pile
	unsigned char algo_command[MAXINSTRUCTION][100];

    //--------- DECODAGE DU MESSAGE RECU
    unsigned short ptrMQTTbyte;					// Pointeur d'octet de la trame MQTT
    unsigned short nbInstruction;				// pointeur sur l'instruction en cours
    unsigned short instructionLenght;			// Taille de la trame MQTT
    unsigned short ptrInstrByte;			    // Pointeur d'octet contenue dans l'instrruction


    printf("\n\n-------------------------------------");
    printf("\nNOUVEAU MESSAGE MQTT RECU");
    printf("\n-------------------------------------");
    //printf("\n Topic: %s", topicName);

 // RECUPERE LE CRC DE LA TRAME (= 2 derniers octet)
    crc_msg=(mqttMsg[msgLen-2]<<8)+(mqttMsg[msgLen-1]);

 // CALCULE DU CRC DE LA TRAME MQTT (Exclus 2 dernier byte (=crc))
    for(ptrMQTTbyte=0;ptrMQTTbyte < msgLen-2;ptrMQTTbyte++){
    	crc_calc=update_crc_16(crc_calc, mqttMsg[ptrMQTTbyte]);
    	//printf("%x ",mqttMsg[ptrMQTTbyte]);
    }


// CONTROLE LA CORRESPONDANCE DU CRC MESSAGE ET CRC CALCULE
    if(crc_calc==crc_msg){
    	nbInstruction=0;

      // printf("\n CRC MESSAGE: %d, CRC CALC: %d  \n",msg_crc ,crc16);

       for(ptrMQTTbyte=0;ptrMQTTbyte < msgLen-2;){

    	// Calcule de la longueur des donnees de l'instruction
    	   instructionLenght=(mqttMsg[ptrMQTTbyte+1]<<8)+mqttMsg[ptrMQTTbyte+2];

        	printf("\n ALGOID MESSAGE: -> ");

        	// Recuperation de l'ensemble des instructions contenu contenue dans la trame MQTT
        	for(ptrInstrByte=0;ptrInstrByte < instructionLenght+3;ptrInstrByte++){
        		algo_command[nbInstruction][ptrInstrByte] = mqttMsg[ptrMQTTbyte];
        		printf(" %x ",algo_command[nbInstruction][ptrInstrByte]);
        		ptrMQTTbyte++;
        	}
        	nbInstruction++;
    }

       printf("\n");
       // Recherche un emplacement libre dans la pile de messages
       for(algoMsgStackPtr=0;(destMsgStack[algoMsgStackPtr].msg_id!=0) && algoMsgStackPtr<10;algoMsgStackPtr++);

       if(algoMsgStackPtr>=10)
    	   printf("\n!!! ALGO MESSAGES STACK OVERFLOW !!!");
       else
    	   strcpy(destMsgStack[algoMsgStackPtr].topicName,topicName);

		   for(ptrInstruction=0;ptrInstruction<nbInstruction;ptrInstruction++){
			   switch(algo_command[ptrInstruction][0]){
					case T_MSGID : destMsgStack[algoMsgStackPtr].msg_id=algo_GetValue(algo_command[ptrInstruction], 1);
								  break;
					case T_CMD	: destMsgStack[algoMsgStackPtr].msg_type=T_CMD;
								destMsgStack[algoMsgStackPtr].msg_type_value=algo_GetValue(algo_command[ptrInstruction], 1);
								  break;
					case T_MSGANS : destMsgStack[algoMsgStackPtr].msg_type=T_MSGANS;
					destMsgStack[algoMsgStackPtr].msg_type_value=algo_GetValue(algo_command[ptrInstruction], 1);
									break;
					case T_MSGACK : destMsgStack[algoMsgStackPtr].msg_type=T_MSGACK;
					destMsgStack[algoMsgStackPtr].msg_type_value=algo_GetValue(algo_command[ptrInstruction], 1);
									break;
					case T_EVENT : destMsgStack[algoMsgStackPtr].msg_type=T_EVENT;
					destMsgStack[algoMsgStackPtr].msg_type_value=algo_GetValue(algo_command[ptrInstruction], 1);
								   break;
					case T_ERROR : destMsgStack[algoMsgStackPtr].msg_type=T_ERROR;
					destMsgStack[algoMsgStackPtr].msg_type_value=algo_GetValue(algo_command[ptrInstruction], 1);
								   break;
					case T_IDNEG : break;

					case PS_1 : destMsgStack[algoMsgStackPtr].msg_param[ptrParam]=PS_1;
								destMsgStack[algoMsgStackPtr].msg_param_value[ptrParam]=algo_GetValue(algo_command[ptrInstruction], 4);
								ptrParam++;
								break;

					case PAS_1:	destMsgStack[algoMsgStackPtr].msg_param[ptrParam]=PAS_1;
								destMsgStack[algoMsgStackPtr].msg_param_count[ptrParam]=((algo_command[ptrInstruction][1]<<8)+algo_command[ptrInstruction][2])/2;
								for(i=0;i<destMsgStack[algoMsgStackPtr].msg_param_count[ptrParam];i++)
									destMsgStack[algoMsgStackPtr].msg_param_array[ptrParam][i]=((algo_command[ptrInstruction][(i*2)+3])<<8)|(algo_command[ptrInstruction][(i*2)+4]) ;
								ptrParam++;
								break;
					default : break;
			   }
		   }
		   AlgoidMessageReady=1;
    }else{
    	int i;
    	AlgoidMessageReady=-1;
    	printf("\n ***** MESSAGE ALGOID INVALID, CRC MESSAGE: %d, CRC CALC: %d ***** \n",crc_msg ,crc_calc);
    	printf("\n ***** COUNT DATA W/O CRC: %d",ptrMQTTbyte);
    	printf("\n ***** DATA WITH CRC:  ");
    	for(i=0;i<ptrMQTTbyte+2;i++) printf("%x ",mqttMsg[i]);
    }
}



// -------------------------------------------------------------------
// Recupere le premier message disponible dans la pile
// -------------------------------------------------------------------
unsigned char algo_getMessage(ALGOID destMsg, ALGOID *srcMsgStack){
	unsigned char i;
	if(srcMsgStack[0].msg_id != 0){
		destMsg=srcMsgStack[0];

		for(i=0;i<9;i++){
			srcMsgStack[i]=srcMsgStack[i+1];
		}
		algo_clearStack(9, srcMsgStack);
		return 1;
	}
	else{
		return 0;
	}
}


// -------------------------------------------------------------------
// INITIALISATION DE LA CONNEXION AU BROCKER MQTT
// -------------------------------------------------------------------
int mqtt_init(const char *IPaddress, const char *clientID, MQTTClient_messageArrived* msgarr){
		int rc;

		MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
		//MQTTClient_message pubmsg = MQTTClient_message_initializer;

		// Configuration des paramï¿½tres de connexion
		MQTTClient_create(&client, IPaddress, clientID, MQTTCLIENT_PERSISTENCE_NONE, NULL);

		conn_opts.keepAliveInterval = 20;
		conn_opts.cleansession = 1;
		// Fin de config connexion

		// Configuration de la fonction callback de souscription
		MQTTClient_setCallbacks(client, NULL, connlost, msgarr, delivered);

		// Tentative de connexion au broker mqtt
		if ((rc = MQTTClient_connect(client, &conn_opts)) != MQTTCLIENT_SUCCESS)
		{
			printf("Failed to connect to MQTT brocker, return code %d\n", rc);
			return(rc);
		}else return 0;
	}

	// -------------------------------------------------------------------
	// !!!!!!!!!!!   FONCTION DEBUG  A RETRAVAILLER...
	// -------------------------------------------------------------------
	int algo_putMessage(char *topic, unsigned char *data, unsigned short lenght){
    	short j;
    	int rc;
    	unsigned short crc16=0;

		// Generation du CRC du message
		for(j=0;j < lenght;j++){
			crc16=update_crc_16(crc16, data[j]);
		}

		// Ajout du CRC dans la trame
		data[lenght++]=(crc16&0xFF00)>>8;
		data[lenght++]=(crc16&0x00FF);

		unsigned char i;
		// Publication du message
		printf("\n PUBLISHING: ");
		for(i=0;i<lenght; i++) printf(" %x", data[i]);
		MQTTClient_publish(client, topic, lenght, data, QOS, 0, &token);
		rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);

		return (rc);
}

	// -------------------------------------------------------------------
	// BUILD MQTT MESSAGE, mqtt destination frame, algoid message source
	// -------------------------------------------------------------------
	unsigned short buildMqttMsg(char *mqttMsgTX, ALGOID srcMsg){
		unsigned char i, ptrParam;
		unsigned short ptrChar;

		int Mask=0xFF000000;
		unsigned short byteCount;

// ID -----------------------------------------------
		printf("\nMEssage ID: %x", srcMsg.msg_id);

		// Calcule le nombre de bytes nécéssaire à la variable
		byteCount=4;
		Mask=0xFF000000;
		for(i=0;i<4;i++){
			if((srcMsg.msg_id&Mask)==0)	byteCount--;
			Mask=Mask>>8;
		}

		ptrChar=0;

		// MESSGE ID
		mqttMsgTX[ptrChar++]=T_MSGID;				// Assignation TYPE code message ID
		mqttMsgTX[ptrChar++]=(byteCount&0xFF00)>>8;	//
		mqttMsgTX[ptrChar++]=(byteCount&0x00FF);	//
		for(i=0;i<byteCount;i++){
			mqttMsgTX[ptrChar]=(srcMsg.msg_id&(0x000000FF<<(8*(byteCount-i-1))))>>(8*(byteCount-i-1));
//			printf("\nID-> %x", (srcMsg.msg_id&(0x000000FF<<(8*(byteCount-i-1))))>>(8*(byteCount-i-1)));
			ptrChar++;
		}

// TYPE -----------------------------------------------

		// Calcule le nombre de bytes nécéssaire à la variable
		byteCount=4;
		Mask=0xFF000000;
		for(i=0;i<4;i++){
			if((srcMsg.msg_type_value&Mask)==0)	byteCount--;
			Mask=Mask>>8;
		}

		// MESSGE TYPE
		mqttMsgTX[ptrChar++]=srcMsg.msg_type;	// Assignation TYPE code message ID
		mqttMsgTX[ptrChar++]=(byteCount&0xFF00)>>8;	//
		mqttMsgTX[ptrChar++]=(byteCount&0x00FF);	//
		for(i=0;i<byteCount;i++){
			mqttMsgTX[ptrChar]=(srcMsg.msg_type_value&(0x000000FF<<(8*(byteCount-i-1))))>>(8*(byteCount-i-1));
			//printf("\nTYPE -> %x", (srcMsg.msg_type_value&(0x000000FF<<(8*(byteCount-i-1))))>>(8*(byteCount-i-1)));
			ptrChar++;
		}

	// PARAM -----------------------------------------------
		//--
		for(ptrParam=0;srcMsg.msg_param[ptrParam]!=0;ptrParam++){
				mqttMsgTX[ptrChar++]=srcMsg.msg_param[ptrParam];	// Assignation TYPE code message ID

				if((srcMsg.msg_param[ptrParam]&0xF0) == 0xA0){
					srcMsg.msg_param_value[ptrParam] = ((rand()<<16)+rand())&0xFFFFFFFF;
					//printf("\nMEssage param: %x", srcMsg.msg_param_value[ptrParam]);

					//printf("\n BUILD TYPE VARIABLE");

					// Calcule le nombre de bytes nécéssaire à la variable
					byteCount=4;
					Mask=0xFF000000;
					for(i=0;i<4;i++){
						if((srcMsg.msg_param_value[ptrParam]&Mask)==0)	byteCount--;
						Mask=Mask>>8;
					}
					printf(" bytecound: %d", byteCount);

					// MESSGE TYPE

									mqttMsgTX[ptrChar++]=(byteCount&0xFF00)>>8;	//
									mqttMsgTX[ptrChar++]=(byteCount&0x00FF);	//
									for(i=0;i<byteCount;i++){
										mqttMsgTX[ptrChar]=(srcMsg.msg_param_value[ptrParam]&(0x000000FF<<(8*(byteCount-i-1))))>>(8*(byteCount-i-1));
										//printf("\nPARAM -> %x", (srcMsg.msg_param_value[ptrParam]&(0x000000FF<<(8*(byteCount-i-1))))>>(8*(byteCount-i-1)));
										ptrChar++;
									}
				}

				if(((srcMsg.msg_param[ptrParam]&0xF0) == 0xB0) ||((srcMsg.msg_param[ptrParam]&0xF0) == 0xC0)) {
					//printf("\n BUILD TYPE ARRAY");

					// MESSGE TYPE ARRY SHORT
					// Assignement taille du tableau (constitué de INT=2octet)
					mqttMsgTX[ptrChar++]=((srcMsg.msg_param_count[ptrParam]*2)&0xFF00)>>8;	//
					mqttMsgTX[ptrChar++]=((srcMsg.msg_param_count[ptrParam]*2)&0x00FF);	//

					for(i=0;i<srcMsg.msg_param_count[ptrParam];i++){
						mqttMsgTX[ptrChar++]=(srcMsg.msg_param_array[ptrParam][i] & 0xFF00) >> 8;
						mqttMsgTX[ptrChar++]=(srcMsg.msg_param_array[ptrParam][i] & 0x00FF);
					}
				}
	}
		return(ptrChar);
	}
