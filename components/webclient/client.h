#ifndef WEBCLIENT_CLIENT_H_
#define WEBCLIENT_CLIENT_H_

/****************************************************************
 * Includes
 ****************************************************************/
// cstdlib includes
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

/****************************************************************
 * Typedefs, structs, enums
 ****************************************************************/
typedef struct kvp
{
	char * key;
	char * value;
} kvp_t;
typedef struct kvp header_t;

/****************************************************************
 * Function declarations
 ****************************************************************/
bool WebClient_Init();

bool WebClient_Get(char * request, size_t bufferSize, char * buffer);

#endif /* WEBCLIENT_CLIENT_H_ */
