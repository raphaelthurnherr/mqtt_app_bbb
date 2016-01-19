/*
 * subalgoid.h
 *
 *  Created on: 22 déc. 2015
 *      Author: raph
 */

#ifndef SAMPLES_SUBALGOID_H_
#define SAMPLES_SUBALGOID_H_

#define MAXMQTTBYTE	1000			// Nombre maximum d'octet par trame MQTT
#define MAXINSTRUCTION	10		    // Nombre maximum de parametres pouvant etre recu par commande
#define MAXINSTRUCTIONBYTE	120		// Nombre maximum d'octet par instruction

#define MAXPARAM	10			    // Nombre maximum de parametres pouvant etre recu par commande
#define MAX_SHORT_ARRAY	100       	// Taille maximum du nombre d'octet contenu par tableau (!TABLEAU DE SHORT = 50*2 octets!)

// TYPE OF ALGOID MESSAGE
#define T_MSGID	 0x01
#define T_CMD	 0x02
#define T_MSGANS 0x03
#define T_MSGACK 0x04
#define T_EVENT	 0x05
#define T_ERROR  0x06
#define T_IDNEG	 0x07

// SCALAR PARAMETER OF ALGOID MESSAGE
#define PS_1   	  0xa1

// ARRAY PARAMETER OF ALGOID MESSAGE
#define PAS_1	  0xb1

// DEFINITION DE LA STRUCTURE DU MESSAGE ALGOID
typedef struct{
	unsigned int msg_id; 				 // Algoid message ID
	unsigned short msg_type;			 // Algoid Type command/event/ack/negociation/error, etc
	int msg_type_value;					 // Algoid Type value for type
	unsigned char msg_param[MAXPARAM];			 // Algoid parameter type (optionnal)
	int msg_param_value[MAXPARAM];				 // Algoid parameter value
	short msg_param_array[MAXPARAM][MAX_SHORT_ARRAY];  	 // Algoid parameter array (if parameter is 0xb2, 0xb3)
	unsigned short msg_param_count[MAXPARAM];		 // Number of parameter
	char topicName[25];					 // MQTT Topic name
} ALGOID;

char AlgoidMessageReady;

#endif /* SAMPLES_SUBALGOID_H_ */
