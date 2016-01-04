/*******************************************************************************
 *	Librairie pour communication protocol ALGOID
 *	Nécéssite les source PAHO MQTT.
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
#define CLIENTID    "Algoid"
#define TOPIC       "MQTT"
#define QOS         0
#define TIMEOUT     10000L

MQTTClient_deliveryToken deliveredtoken, token;
MQTTClient client;

// Variable d'entrée/sortie vers algoid
ALGOID algoidMsgRXStack[10], algoidMsgRX;


int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message); 	// Call-back message MQTT recu
void delivered(void *context, MQTTClient_deliveryToken dt);		// Call-back message MQTT emis
void connlost(void *context, char *cause);						// Call-back perte de connexion MQTT

// Initialisation connexion MQTT (IPADDR:PORT, CLIENT ID, FUNC CALL-BACK MSG RECU)
int mqtt_init(const char *IPaddress, const char *clientID, MQTTClient_messageArrived* msgarr);

// Construction message algoid, controle CRC algo message, empilage des messages dans variable type ALGOID
void processMqttMsg(char *mqttMsg, unsigned int msgLen, char *topicName, ALGOID *destMsgStack);

// Retourne la donnée codée sur n bytes
long algo_GetValue(unsigned char *MsgVal, unsigned char byteLen);

// Récupère le premier message disponible dans la pile
unsigned char algo_getMessage(ALGOID destMsg, ALGOID *srcMsgStack);

// Efface un champs donné dans la pile de message
void algo_clearStack(unsigned char ptr, ALGOID *destMsgStack);

// !!!!!!!!!!!   FONCTION DEBUG  A RETRAVAILLER...
int algo_putMessage(char *topic, unsigned short msgID, unsigned char *data, unsigned short lenght);


// ------------------------------------------------------------------------------------
// MAIN: Point d'entrée programme, initialisation connexion MQTT, souscription et publication
// ------------------------------------------------------------------------------------
int main(int argc, char* argv[])
{
	// Trame de test de type ALGOID à publier
	unsigned char PAYLOAD[100]={1,00,02,255,255,02,0,04, 0x15, 02, 0xaa, 0xaa, 0xa2, 0, 3, 61, 62, 63, 0,0};
	int ch;
	int err;
	unsigned char i;

	printf("\nTentative de connexion au brocker MQTT...\n");
	err=mqtt_init(ADDRESS, CLIENTID, msgarrvd);

	if(!err){
		printf("- réussite, tentative de souscription topic MQTT: ");
		// Configuration souscription
		if(!MQTTClient_subscribe(client, TOPIC, QOS))printf("OK\n");
		else printf("ERREUR\n");
	}else printf("Erreur de connexion au brocker !\n");

    do 
    {
    	// - Test: traitement du message algoid recu
    	if(AlgoidMessageReady){
    		if(AlgoidMessageReady==1){
    			printf("\n Main: MESSAGE MQTT ALGOID VALID RECU: %d \n", AlgoidMessageReady);

    			// Affichage des messages de la piles
    		    for (i=0;i<10;i++)
    		    {
    		    	printf("\n#%d Algo Topic: %s, Sender: %x, message ID: %d, command: %d, value: %d, param: %d, value: %d",i,algoidMsgRXStack[i].topicName, (algoidMsgRXStack[i].msg_id&0xF000), algoidMsgRXStack[i].msg_id, algoidMsgRXStack[i].msg_type,
    		    			algoidMsgRXStack[i].msg_type_value, algoidMsgRXStack[i].msg_param, algoidMsgRXStack[i].msg_param_value);
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
        if((ch=='a')){
        	short al_msgID;
        	int result;

        	//token=rand();
        	al_msgID=(short)rand();						// Génération d'un ID aléatoire
    		printf("\nMessage ID: %d", al_msgID);

    		// Génération des données aléatoire de test
    		PAYLOAD[8]= rand() & 0x00FF;
    		PAYLOAD[15]= rand() & 0x00FF;

    		al_msgID = (al_msgID & 0x0FFF) | 0xA000;    // Ajout d'un code expéditeur

    		// Envoie du message
    		result=algo_putMessage("MQTT", al_msgID, PAYLOAD, 20);
        	if(!result) printf("Message with delivery token %d delivered\n", token);
        }

    } while(ch != 'Q' && ch != 'q');

    MQTTClient_disconnect(client, 10000);
    MQTTClient_destroy(&client);
    return 0;
}


// -------------------------------------------------------------------
// Fonction Call-back de retour de token MQTT pour contrôle
// -------------------------------------------------------------------

void delivered(void *context, MQTTClient_deliveryToken dt)
{
    printf("Message with token value %d delivery confirmed\n", dt);
    deliveredtoken = dt;
}


// -------------------------------------------------------------------
// Fonction Call-Back récéption d'un message MQTT
// -------------------------------------------------------------------
int msgarrvd(void *context, char *topicName, int topicLen, MQTTClient_message *message)
{
    unsigned int i;
    char* payloadptr;
    char mqtt_rcv_message[309]; // 3x byte TL + 3x100 bytes V

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
// retourne la valeur codée sur n bytes, d'un champ de la trame algoid
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
// Efface les champs d'un emplacemment donné dans la pile
// -------------------------------------------------------------------
void algo_clearStack(unsigned char ptr, ALGOID *destMsgStack){
	unsigned char i;

	destMsgStack[ptr].msg_id=0;
	destMsgStack[ptr].msg_type=0;
	destMsgStack[ptr].msg_type_value=0;
	destMsgStack[ptr].msg_param=0;
	destMsgStack[ptr].msg_param_count=0;
	destMsgStack[ptr].msg_param_value=0;
	for(i=0;i<100;i++) destMsgStack[ptr].msg_param_array[i]=0;
	for(i=0;i<sizeof(destMsgStack[ptr].topicName);i++)destMsgStack[ptr].topicName[i]=0;
}


// -------------------------------------------------------------------
// Construction message algoid, controle CRC algo message, empilage des messages dans variable type ALGOID
// -------------------------------------------------------------------
void processMqttMsg(char *mqttMsg, unsigned int msgLen, char *topicName, ALGOID *destMsgStack){

	static unsigned char algoMsgStackPtr;
	unsigned char algo_command[3][103];

    //--------- DECODAGE DU MESSAGE RECU
	unsigned int i;
    unsigned short countDataFrame;

    unsigned short indexCommand;

    unsigned short dataLenght;
    unsigned short dataCommandCount;

    printf("\n-------------------------------------");
    printf("\nNOUVEAU MESSAGE MQTT RECU");
    printf("\n-------------------------------------");
    printf("\n Topic: %s", topicName);

    unsigned short crc16=0;
    unsigned short msg_crc;

    for(countDataFrame=0;countDataFrame < msgLen-2;countDataFrame++){
    	crc16=update_crc_16(crc16, mqttMsg[countDataFrame]);
    }

    msg_crc=(mqttMsg[msgLen-2]<<8)+(mqttMsg[msgLen-1]);



    if(msg_crc==crc16){
       indexCommand=0;

       printf("\n CRC MESSAGE: %d, CRC CALC: %d  \n",msg_crc ,crc16);

       for(countDataFrame=0;countDataFrame < msgLen-2;){

    	// Calcule de la longeur des donnéee de la commande
    	    dataLenght=(mqttMsg[countDataFrame+1]<<8)+mqttMsg[countDataFrame+2];

        	printf("\n ALGOID MESSAGE: -> ", dataLenght);

        	// Recuperation des messages contenu dans la trame.
        	for(dataCommandCount=0;dataCommandCount < dataLenght+3;dataCommandCount++){
        		algo_command[indexCommand][dataCommandCount] = mqttMsg[countDataFrame];
        		printf(" %d ",algo_command[indexCommand][dataCommandCount]);
        		countDataFrame++;
        	}
        	indexCommand++;
    }

       printf("\n");
       // Recherche un emplacement libre dans la pile de messages
       for(algoMsgStackPtr=0;(destMsgStack[algoMsgStackPtr].msg_id!=0) && algoMsgStackPtr<10;algoMsgStackPtr++);

       if(algoMsgStackPtr>=10)
    	   printf("\n!!! ALGO MESSAGES STACK OVERFLOW !!!");
       else
    	   strcpy(destMsgStack[algoMsgStackPtr].topicName,topicName);

		   for(indexCommand=0;indexCommand<3;indexCommand++){
			   switch( algo_command[indexCommand][0]){
					case T_MSGID : destMsgStack[algoMsgStackPtr].msg_id=algo_GetValue(algo_command[indexCommand], 1);
								  break;
					case T_CMD	: destMsgStack[algoMsgStackPtr].msg_type=T_CMD;
								destMsgStack[algoMsgStackPtr].msg_type_value=algo_GetValue(algo_command[indexCommand], 1);
								  break;
					case T_MSGANS : destMsgStack[algoMsgStackPtr].msg_type=T_MSGANS;
					destMsgStack[algoMsgStackPtr].msg_type_value=algo_GetValue(algo_command[indexCommand], 1);
									break;
					case T_MSGACK : destMsgStack[algoMsgStackPtr].msg_type=T_MSGACK;
					destMsgStack[algoMsgStackPtr].msg_type_value=algo_GetValue(algo_command[indexCommand], 1);
									break;
					case T_EVENT : destMsgStack[algoMsgStackPtr].msg_type=T_EVENT;
					destMsgStack[algoMsgStackPtr].msg_type_value=algo_GetValue(algo_command[indexCommand], 1);
								   break;
					case T_ERROR : destMsgStack[algoMsgStackPtr].msg_type=T_ERROR;
					destMsgStack[algoMsgStackPtr].msg_type_value=algo_GetValue(algo_command[indexCommand], 1);
								   break;

					case T_IDNEG : break;

					case PS_BOOL : break;
					case PS_INT : destMsgStack[algoMsgStackPtr].msg_param=PS_INT;
					destMsgStack[algoMsgStackPtr].msg_param_value=algo_GetValue(algo_command[indexCommand], 4); break;
					case PS_CHAR : destMsgStack[algoMsgStackPtr].msg_param=PS_CHAR;
					destMsgStack[algoMsgStackPtr].msg_param_value=algo_GetValue(algo_command[indexCommand], 1); break;
					case PS_SHORT : destMsgStack[algoMsgStackPtr].msg_param=PS_SHORT;
					destMsgStack[algoMsgStackPtr].msg_param_value=algo_GetValue(algo_command[indexCommand], 2); break;
					case PS_HOLE : break;
					case PS_COLL : break;
					case PS_COLR : break;

					case PA_INT : break;
					case PA_STR : break;
					default : break;
			   }
		   }
		   AlgoidMessageReady=1;
    }else{
    	AlgoidMessageReady=-1;
    	printf("\n ***** MESSAGE ALGOID INVALID, CRC MESSAGE: %d, CRC CALC: %d ***** \n",msg_crc ,crc16);
    }
/*
    for (i=0;i<10;i++)
    {
    	printf("\n#%d Algo Topic: %s, Sender: %x, message ID: %d, command: %d, value: %d, param: %d, value: %d",i,destMsgStack[i].topicName, (destMsgStack[i].msg_id&0xF000), destMsgStack[i].msg_id, destMsgStack[i].msg_type,
    			destMsgStack[i].msg_type_value, destMsgStack[i].msg_param,
				destMsgStack[i].msg_param_value);
    }
    printf("\n");
    */
    //---------

}


// -------------------------------------------------------------------
// Récupère le premier message disponible dans la pile
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

		// Configuration des paramètres de connexion
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
	int algo_putMessage(char *topic, unsigned short msgID, unsigned char *data, unsigned short lenght){
    	short j;
    	int rc;
    	unsigned short crc16=0;

    	// Ajout du message ID dans la trame
    	data[3]=(msgID & 0xFF00)>>8;
		data[4]=msgID & 0x00FF;

		// Génération du CRC du message
		for(j=0;j < 18;j++){
			crc16=update_crc_16(crc16, data[j]);
		}

		// Ajout du CRC dans la trame
		data[18]=(crc16&0xFF00)>>8;
		data[19]=(crc16&0x00FF);

		// Publication du message
		MQTTClient_publish(client, topic, lenght, data, QOS, 0, &token);
		rc = MQTTClient_waitForCompletion(client, token, TIMEOUT);

		return (rc);
}
