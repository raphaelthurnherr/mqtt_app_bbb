/*
 * subalgoid.h
 *
 *  Created on: 22 déc. 2015
 *      Author: raph
 */

#ifndef SAMPLES_SUBALGOID_H_
#define SAMPLES_SUBALGOID_H_

#define MAXPARAM	10

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
#define PS_2   	  0xa2

// ARRAY PARAMETER OF ALGOID MESSAGE
#define PAS_1	  0xb1
#define PAS_2	  0xb2

// DEFINITION DE LA STRUCTURE DU MESSAGE ALGOID
typedef struct{
	unsigned int msg_id; 				 // Algoid message ID
	unsigned short msg_type;			 // Algoid Type command/event/ack/negociation/error, etc
	int msg_type_value;					 // Algoid Type value for type
	unsigned char msg_param;			 // Algoid parameter type (optionnal) -> 0xb2b1a2a1
	int msg_param_value;				 // Algoid parameter value
	short msg_param_array[100];  	 // Algoid parameter array (if parameter is 0xb2, 0xb3)
	unsigned short msg_param_count;		 // Number of parameter
	char topicName[25];					 // MQTT Topic name
} ALGOID;

char AlgoidMessageReady;

#endif /* SAMPLES_SUBALGOID_H_ */
