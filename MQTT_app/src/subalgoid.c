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
											// Assignation via manager de "CLIENTID" et topic dedie RX et TX
#define QOS         0
#define TIMEOUT     10000L

pthread_t th_algoid;

MQTTClient_deliveryToken deliveredtoken, token;
MQTTClient client;

char TOPIC_TX[25]= TOPIC_MGR;			// Topic initial, sera modife apres negociation
char TOPIC_RX[25]= TOPIC_MGR;			// Topic initial, sera modife apres negociatio
//char TOPIC_RX[25]= "MQ2ES12";			// Topic initial, sera modife apres negociatio
char CLIENTID[20]= "ES12";		    	// MQTT Client ID is ES+"the last byte of IP address"

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

// Efface un champs donnee dans la pile de message
void algo_clearMessage(ALGOID algMsg);

// Envoie le messageau brocker MQTT
int algo_putMessage(char *topic, char *data, unsigned short lenght);

// Affichage de l'état de la pile séléctionnée
void diplay_algoStack(ALGOID *msgStack);

// Envoie le prochain message algoid de la pile si disponible
char SendQueueingTXmsg(ALGOID *msgStack);

// Défini les cannaux sur lesquels doit se connecter Eduspider pour l'emission
// des message et la reception
char algoSetTXChannel(char * topicName);
char algoAddRXChannel(char * topicName);
char algoRemoveRXChannel(char * topicName);

// Traitement de la commande de négociation avec le manager
char processNegociation(ALGOID message);

void error(char *msg) {
    perror(msg);
    exit(0);
}


// ------------------------------------------------------------------------------------
// MAIN: Point d'entreee programme, initialisation connexion MQTT, souscription et publication
// ------------------------------------------------------------------------------------

void *algoidTask (void * arg){
	printf ("# Demarrage tache ALGOID: OK\n");
	char prtFindNegocMsg=0;
	char j;
		int err;

		printf("\nMQQT-ALGO POC 19/01/2016\n");
		printf("\nConnexion au brocker MQTT -> %s", ADDRESS);

		err=mqtt_init(ADDRESS, CLIENTID, msgarrvd);

		if(!err){

			algoSetTXChannel(TOPIC_MGR);
			algoAddRXChannel(TOPIC_MGR);
		}else
			printf(": Erreur de connexion au brocker, code erreur: %d\n", err);

		sleep(10);

	 													// duty cycle is 50% for ePWM0A , 25% for ePWM0B;

// BOUCLE PRINCIPALE
	    while(!EndOfApp)
	    {
	    	ALGOID algoidNegociationRX;

	    	// Recherche d'éventuels message de negociation ou en provenance du manager
	    	for(prtFindNegocMsg=0;prtFindNegocMsg<RXTXSTACK_SIZE;prtFindNegocMsg++){
				if((algoidMsgRXStack[prtFindNegocMsg].msg_type==T_NEGOC) || (!strcmp(algoidMsgRXStack[prtFindNegocMsg].topic, TOPIC_MGR))){

					// Récupère le message de la pile
					algoidNegociationRX=algoidMsgRXStack[prtFindNegocMsg];

					// Liberation de l'espace dans la pile
					for(j=prtFindNegocMsg;j<RXTXSTACK_SIZE-1;j++)
										algoidMsgRXStack[j]=algoidMsgRXStack[j+1];

					// Verification que le message est bien destiné à ce spider ([0] = destinataire du message)
					if(!strcmp(algoidNegociationRX.msg_string_array[0], CLIENTID)){
						printf("\n MESSAGE MESSAGE NEGOCIATION EN TRAITEMENT \n");
						processNegociation(algoidNegociationRX); // Traitement du message me concernant
					}
					else
					{
						printf("\n MESSAGE NEGOCIATION NON TRAITE, MAUVAIS DESTINATAIRE \n");
					}
					//algo_clearMessage(algoidNegociationRX); // Efface le message recu

				}
	    	}

	    	// Envoie le prochaine message en attente dans la pile
	    	SendQueueingTXmsg(algoidMsgTXStack);

	    	sleep(3);
	    }
 // FIN BOUUCLE PRINCIPAL

	    MQTTClient_disconnect(client, 10000);
	    MQTTClient_destroy(&client);

	    return 0;
  usleep(5000);

  printf( "# ARRET tache ALGOID\n");

  usleep(10000);
  pthread_exit (0);
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
// Fonction Call-Back reception d'un message MQTT
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
    printf("              cause: %s\n", cause);
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

        printf("\n\n -------------------------------------");
        printf("\n NOUVEAU MESSAGE ALGOID RECU");
        printf("\n -------------------------------------");

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
       for(algoMsgStackPtr=0;(destMsgStack[algoMsgStackPtr].msg_id!=0) && algoMsgStackPtr<RXTXSTACK_SIZE;algoMsgStackPtr++);

       if(algoMsgStackPtr>=RXTXSTACK_SIZE){
    	   printf("\n!!! ALGO MESSAGES STACK OVERFLOW !!!");
       }
       else{
    	   strcpy(destMsgStack[algoMsgStackPtr].topic,topicName);

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
					case T_NEGOC :  destMsgStack[algoMsgStackPtr].msg_type=T_NEGOC;
									destMsgStack[algoMsgStackPtr].msg_type_value=algo_GetValue(algo_command[ptrInstruction], 1);
									break;

					case PS_1 : destMsgStack[algoMsgStackPtr].msg_param[ptrParam]=PS_1;
								destMsgStack[algoMsgStackPtr].msg_param_value[ptrParam]=algo_GetValue(algo_command[ptrInstruction], 4);
								ptrParam++;
								break;

					case PSA_1:	destMsgStack[algoMsgStackPtr].msg_param[ptrParam]=PSA_1;
								destMsgStack[algoMsgStackPtr].msg_param_count[ptrParam]=((algo_command[ptrInstruction][1]<<8)+algo_command[ptrInstruction][2])/2;
								for(i=0;i<destMsgStack[algoMsgStackPtr].msg_param_count[ptrParam];i++)
									destMsgStack[algoMsgStackPtr].msg_param_array[ptrParam][i]=((algo_command[ptrInstruction][(i*2)+3])<<8)|(algo_command[ptrInstruction][(i*2)+4]) ;
								ptrParam++;
								break;

					case PCA_1:	destMsgStack[algoMsgStackPtr].msg_param[ptrParam]=PCA_1;
								destMsgStack[algoMsgStackPtr].msg_param_count[ptrParam]=((algo_command[ptrInstruction][1]<<8)+algo_command[ptrInstruction][2]);
								for(i=0;i<destMsgStack[algoMsgStackPtr].msg_param_count[ptrParam];i++)
									destMsgStack[algoMsgStackPtr].msg_string_array[ptrParam][i]=algo_command[ptrInstruction][i+3];
								ptrParam++;
								break;
					default : break;
			   }
		   }
       }
    }else{

    	printf("\n\n ----------------------------------------------------------------");
    	printf("\n ! MESSAGE ALGOID INVALID, CRC MESSAGE: %d, CRC CALC: %d ! \n",crc_msg ,crc_calc);
    	printf("\n ***** NUMBER OF DATA W/O CRC: %d",ptrMQTTbyte);
    	printf("\n ***** DATA WITH CRC:  ");
    	for(i=0;i<ptrMQTTbyte+2;i++) printf("%x ",mqttMsg[i]);
    	printf("\n ----------------------------------------------------------------");
    }
}



// -------------------------------------------------------------------
// Recupere le premier message disponible dans la pile
// -------------------------------------------------------------------
char algo_getMessage(ALGOID destMsg, ALGOID *srcMsgStack){
	unsigned char i;

	// Récuperation du message de la pile si disponible
	if(srcMsgStack[0].msg_id != 0){
		destMsg=srcMsgStack[0];
		// Descend la pile de message
		for(i=0;i<RXTXSTACK_SIZE-1;i++){
			srcMsgStack[i]=srcMsgStack[i+1];
		}
		algo_clearMessage(srcMsgStack[RXTXSTACK_SIZE-1]);				// Liberation d'un espace dans la pile
		return 1;
	}
	else{
		return 0;
	}
}

// -------------------------------------------------------------------
// Charge le  message dans la pile d'envoie
// -------------------------------------------------------------------
char algo_setMessage(ALGOID srcMsg, ALGOID *destMsgStack){
	unsigned char ptrAlgoMsgTXstack;

	// Contrôle la présence d'un topic de destination
	if(strlen(srcMsg.topic)){
		if(!srcMsg.msg_id) {
			srcMsg.msg_id |= HOSTSENDERID<<24;
			printf(" - ATTENTION, MESSAGE ID MANQUANT. ENVOYE AVEC ID GENERIQUE");
		}
		// Recherche d'un emplacement libre dans la pile d'envoie
			for(ptrAlgoMsgTXstack=0;destMsgStack[ptrAlgoMsgTXstack].msg_id != 0 && ptrAlgoMsgTXstack<RXTXSTACK_SIZE;ptrAlgoMsgTXstack++);
			// Tentative de mise du message dans la pile
		    if(ptrAlgoMsgTXstack>=RXTXSTACK_SIZE){
		    	// Pile pleine
		    	printf("\n!!! ALGO STACK TX OVERFLOW !!!");
		    	return (-2);
		    }
		    else{
		    	// Message mis en file d'attente
		    	destMsgStack[ptrAlgoMsgTXstack] = srcMsg;
		    	return (1);
		    }
	}
else
	{
		printf("\nERREUR, MESSAGE EN FILE MQTT NON ENVOYE : TOPIC MANQUANT");			// Erreur de transmission
		return -1;
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
			return(rc);
		}else return 0;
	}

	// -------------------------------------------------------------------
	// !!!!!!!!!!!   FONCTION DEBUG  A RETRAVAILLER...
	// -------------------------------------------------------------------
	int algo_putMessage(char *topic, char *data, unsigned short lenght){
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

		// Publication du message
		//printf("\n PUBLICATION: ");
		//for(i=0;i<lenght; i++) printf(" %x", data[i]);

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
		//printf("\nMEssage ID: %x", srcMsg.msg_id);

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
		for(ptrParam=0;srcMsg.msg_param[ptrParam]!=0;ptrParam++){
				mqttMsgTX[ptrChar++]=srcMsg.msg_param[ptrParam];	// Assignation TYPE code message ID

				// MESSAGE DE TYPE SCALAIRE
				//
				if(srcMsg.msg_param[ptrParam]==PS_1){
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

					// MESSGE TYPE

									mqttMsgTX[ptrChar++]=(byteCount&0xFF00)>>8;	//
									mqttMsgTX[ptrChar++]=(byteCount&0x00FF);	//
									for(i=0;i<byteCount;i++){
										mqttMsgTX[ptrChar]=(srcMsg.msg_param_value[ptrParam]&(0x000000FF<<(8*(byteCount-i-1))))>>(8*(byteCount-i-1));
										//printf("\nPARAM -> %x", (srcMsg.msg_param_value[ptrParam]&(0x000000FF<<(8*(byteCount-i-1))))>>(8*(byteCount-i-1)));
										ptrChar++;
									}
				}

				// MESSAGE DE TYPE SHORT ARRAY
				//
				if(srcMsg.msg_param[ptrParam]== PSA_1) {
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

				// MESSAGE DE TYPE STRING ARRAY
				//
				if(srcMsg.msg_param[ptrParam]==PCA_1) {
					//printf("\n BUILD TYPE STRING ARRAY");

					// MESSGE TYPE STRING ARRAY
					// Assignement taille du tableau (constitué de INT=2octet)
					mqttMsgTX[ptrChar++]=((srcMsg.msg_param_count[ptrParam])&0xFF00)>>8;	//
					mqttMsgTX[ptrChar++]=((srcMsg.msg_param_count[ptrParam])&0x00FF);	//

					for(i=0;i<srcMsg.msg_param_count[ptrParam];i++){
						mqttMsgTX[ptrChar++]=srcMsg.msg_string_array[ptrParam][i];
					}
				}
	}
		return(ptrChar);
	}


// -------------------------------------------------------------------
// DISPLAY_ALGOSTACK, affichage de la pile de message ALGOID
// -------------------------------------------------------------------
	void diplay_algoStack(ALGOID *msgStack){
		unsigned char i, j;
		unsigned char ptrParam;

	    for (i=0;i<RXTXSTACK_SIZE;i++)
	    {
	    	printf("\n#%d Algo Topic: %s, Sender: %x, message ID: %d, command: %d, cmd value: %d",i,msgStack[i].topic, ((msgStack[i].msg_id&0xFF000000)>>24), msgStack[i].msg_id, msgStack[i].msg_type,
	    	    		    			msgStack[i].msg_type_value);
	    	for(ptrParam=0;msgStack[i].msg_param[ptrParam]!=0;ptrParam++){

	    	 if(msgStack[i].msg_param[ptrParam]==PS_1)
	    		 printf("\n        value: %d",msgStack[i].msg_param_value[ptrParam]);

	    	 if(msgStack[i].msg_param[ptrParam]==PSA_1){
	    		 printf("\n        array: ");
	    		 for(j=0;j<msgStack[i].msg_param_count[ptrParam];j++) printf("%d ", msgStack[i].msg_param_array[ptrParam][j]);
	    	 }
	    	 if(msgStack[i].msg_param[ptrParam]==PCA_1){
	    		 printf("\n        string: ");
	    		 printf("%s ", msgStack[i].msg_string_array[ptrParam]);
	    	 }
	    	}
	    	printf("\n");
	    }
	    printf("\n");
	}




	// -------------------------------------------------------------------
	// EFFACE LE BUFFER ALGOID TX
	// -------------------------------------------------------------------
	void algo_clearMessage(ALGOID algMsg){

		unsigned char i, ptrParam;

		strcpy(algMsg.topic, "");
		algMsg.msg_id=0;
		algMsg.msg_type=0;
		algMsg.msg_type_value=0;
		for(ptrParam=0;ptrParam<MAXPARAM;ptrParam++){
			algMsg.msg_param[ptrParam]=0;
			algMsg.msg_param_count[ptrParam]=0;
			algMsg.msg_param_value[ptrParam]=0;
			for(i=0;i<MAX_SHORT_ARRAY;i++){
				algMsg.msg_param_array[ptrParam][i]=0;
				algMsg.msg_string_array[ptrParam][i]=0;
			}
		}
		//for(i=0;i<strlen(message.topic);i++)message.topic[i]=0;
	}

	// -------------------------------------------------------------------
	// Envoie le message aalgoid en file d'attente
	// -------------------------------------------------------------------
	char SendQueueingTXmsg(ALGOID *msgStack){
    	int result;
    	unsigned char nbChar, i;
    	char PAYLOADTX[MAXMQTTBYTE]={1,00,02,255,255,02,0,04, 0x15, 02, 0xaa, 0xaa, 0xa2, 0, 3, 61, 62, 63, 0,0};

    	// Controle la présence d'un message dans la pile (=MSGID)
    	if(msgStack[0].msg_id != 0){

    			// Construction de la trame MQTT avec le message ALGOID
				// Récupère le nombre d'octet MQTT a envoyer
				nbChar = buildMqttMsg(PAYLOADTX, msgStack[0]);
				//algo_clearTX();

				// Envoie du message sur TOPIC_TX DU BROCKER
				result=algo_putMessage(msgStack[0].topic,PAYLOADTX, nbChar);
				if(result){
					return -1;
					printf("\nERREUR, MESSAGE EN FILE MQTT NON ENVOYE");			// Erreur de transmission
				}
				else {

					for(i=0;i<RXTXSTACK_SIZE-1;i++){												// Descente des message dans la pile
						msgStack[i]=msgStack[i+1];
					}
					algo_clearMessage(msgStack[RXTXSTACK_SIZE-1]);									// Liberation d'un espace dans la pile
					return 1;
				}

    	}
    	else return 0;
	}


	// -------------------------------------------------------------------
	// Défini le topic sur lequel doit se connecter Eduspider pour la Transmission
	// -------------------------------------------------------------------
	char algoSetTXChannel(char * topicName){
		strcpy(TOPIC_TX,topicName);
		return 0;
	}

	// -------------------------------------------------------------------
	// Défini le topic sur lequel doit se connecter Eduspider pour la Reception
	// -------------------------------------------------------------------
	char algoAddRXChannel(char * topicName){
		// SOUSCRIPTION AU TOPIC MANAGER
		printf("\n - Inscription topic %s",topicName);

		// Configuration souscription
		if(!MQTTClient_subscribe(client, topicName, QOS))printf(": OK");
		else printf(": Erreur d'inscription au topic");
		return 0;
	}

	// -------------------------------------------------------------------
	// Desabonnement a un canal algoid
	// -------------------------------------------------------------------
	char algoRemoveRXChannel(char * topicName){
		// SOUSCRIPTION AU TOPIC MANAGER
		printf("\n - desinscription topic %s",topicName);

		// Configuration souscription
		if(!MQTTClient_unsubscribe(client, topicName))printf(": OK");
		else printf(": Erreur de desinscription au topic");
		return 0;
	}


// ---------------------------------------------------------------------------
// TRAITEMENT DU MESSAGE DE NEGOCIATION AVEC LE MANAGER
// ---------------------------------------------------------------------------
char processNegociation(ALGOID message){

	switch(message.msg_type_value){
		case NEGOC_ONLINE : break;
		case NEGOC_OFFLINE : break;
		case NEGOC_TX_CHANNEL : algoSetTXChannel(message.msg_string_array[1]);
									break;
		case NEGOC_ADD_RX_CHANNEL : algoAddRXChannel(message.msg_string_array[1]);
									printf("\n - Ajout canal d écoute sur topic: %s", message.msg_string_array[1]);
									break;
		case NEGOC_REM_RX_CHANNEL : if(strcmp(message.msg_string_array[1], TOPIC_MGR)){			 // Ne desincrit pas le topic manager
										algoRemoveRXChannel(message.msg_string_array[1]);
										printf("\n - Suppression canal d'écoute: %s", message.msg_string_array[1]);
									}else printf(" - Suppression canal d'écoute %s impossible", message.msg_string_array[1]);
									break;
		default : break;
	}

	//algo_clearMessage(message);
	return 0;
}

	// ---------------------------------------------------------------------------
	// CREATION THREAD UART
	// ---------------------------------------------------------------------------
	int createAlgoidTask(void)
	{
		return (pthread_create (&th_algoid, NULL, algoidTask, NULL));
	}

	// ---------------------------------------------------------------------------
	// DESTRUCTION THREAD UART
	// ---------------------------------------------------------------------------
	int killAlgoidTask(void){
		return (pthread_cancel(&th_algoid));
	}
