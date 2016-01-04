/*
 * subalgoid.h
 *
 *  Created on: 22 déc. 2015
 *      Author: raph
 */

#ifndef SAMPLES_SUBALGOID_H_
#define SAMPLES_SUBALGOID_H_

// TYPE OF ALGOID MESSAGE
#define T_MSGID	 0x01
#define T_CMD	 0x02
#define T_MSGANS 0x03
#define T_MSGACK 0x04
#define T_EVENT	 0x05
#define T_ERROR  0x06
#define T_IDNEG	 0x07

// SCALAR PARAMETER OF ALGOID MESSAGE
#define PS_BOOL   0xa1
#define PS_INT    0xa2
#define PS_CHAR   0xa3
#define PS_HOLE   0xa4
#define PS_COLL	  0xa5
#define PS_COLR	  0xa6
#define PS_SHORT  0xa7

// ARRAY PARAMETER OF ALGOID MESSAGE
#define PA_INT	  0xb2
#define PA_STR	  0xb3

// DEFINITION DE LA STRUCTURE DU MESSAGE ALGOID
typedef struct{
	unsigned short msg_id; 			// Algoid message ID
	unsigned short msg_type;		// Algoid Type command/event/ack/negociation/error, etc
	int msg_type_value;				// Algoid Type value for type
	short msg_param;				// Algoid parameter (optionnal)
	int msg_param_value;			// Algoid parameter value
	unsigned char msg_param_array[100];  // Algoid parameter array (if parameter is 0xb2, 0xb3)
	unsigned short msg_param_count;		 // Number of parameter
	char topicName[25];					 // MQTT Topic name
} ALGOID;

char AlgoidMessageReady;

#endif /* SAMPLES_SUBALGOID_H_ */
