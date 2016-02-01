/*
 * subalgoid.h
 *
 *  Created on: 22 dÃ©c. 2015
 *      Author: raph
 */

#ifndef SAMPLES_SUBALGOID_H_
#define SAMPLES_SUBALGOID_H_

#define MYSENDERID	0x0B


#define RXTXSTACK_SIZE 16

#define MAXMQTTBYTE	1000			// Nombre maximum d'octet par trame MQTT
#define MAXINSTRUCTION	10		    // Nombre maximum de parametres pouvant etre recu par commande
#define MAXINSTRUCTIONBYTE	120		// Nombre maximum d'octet par instruction

#define MAXPARAM	10			    // Nombre maximum de parametres pouvant etre recu par commande
#define MAX_SHORT_ARRAY	100       	// Taille maximum du nombre d'octet contenu par tableau (!TABLEAU DE SHORT = 50*2 octets!)

#define TOPIC_MGR  "ESMGR"			// Canal de négociation eduspider / Algoid, toujours online

// TYPE OF ALGOID MESSAGE
#define T_MSGID	 0x01
#define T_CMD	 0x02
#define T_MSGANS 0x03
#define T_EVENT	 0x04

#define PS_1   	 0xA1		// SCALAR PARAMETER OF ALGOID MESSAGE (CHAR, SHORT ou INT)
#define PSA_1	 0xA2		// SHORT ARRAY PARAMETER OF ALGOID MESSAGE
#define PCA_1	 0xA3		// CHAR ARRAY PARAMETER OF ALGOID MESSAGE

#define T_NEGOC	 0xD1

#define T_ERROR  0xE1
#define T_MSGACK 0xE2

// DEFINITION DES COMMANDE DE NEGOCIATION
#define NEGOC_ONLINE	0x01
#define NEGOC_OFFLINE	0x02
#define NEGOC_TX_CHANNEL 0x03
#define NEGOC_ADD_RX_CHANNEL 0x04
#define NEGOC_REM_RX_CHANNEL 0x05




// DEFINITION DE LA STRUCTURE DU MESSAGE ALGOID
typedef struct{
	unsigned int msg_id; 				 				 // Algoid message ID
	unsigned short msg_type;			 				 // Algoid Type command/event/ack/negociation/error, etc
	int msg_type_value;									 // Algoid Type value for type
	unsigned char msg_param[MAXPARAM];					 // Algoid parameter type (optionnal)
	int msg_param_value[MAXPARAM];				 		 // Algoid parameter value
	unsigned short msg_param_count[MAXPARAM];		 	 // Number of parameter
	short msg_param_array[MAXPARAM][MAX_SHORT_ARRAY];  	 // Algoid parameter array (if parameter is 0xb2, 0xb3)
	char msg_string_array[MAXPARAM][MAX_SHORT_ARRAY];  	 // Algoid parameter array (if parameter is 0xb2, 0xb3)
	char topic[25];					 // MQTT Topic name
} ALGOID;

//unsigned int AlgoidMessageReady;

unsigned char EndOfApp;

extern unsigned char HOSTSENDERID;

// Variable d'entree/sortie vers algoid
ALGOID algoidMsgRXStack[RXTXSTACK_SIZE],algoidMsgTXStack[RXTXSTACK_SIZE];
ALGOID algoidMsgTX, algoidMsgRX;


// recupere le premier message disponible dans la pile
char algo_getMessage(ALGOID destMsg, ALGOID *srcMsgStack);
char algo_setMessage(ALGOID srcMsg, ALGOID *destMsgStack);

int createAlgoidTask(void);

#endif /* SAMPLES_SUBALGOID_H_ */
