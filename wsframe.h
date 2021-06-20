#ifndef WSFRAME_H
#define WSFRAME_H

#include <stdbool.h>
#include <stdint.h>

struct wsframe
{
	uint8_t state;
	bool fin;
	bool rsv1;
	bool rsv2;
	bool rsv3;
	uint8_t opcode;
	bool masked;
	uint64_t len;
	char *payload;
	int payload_len;
	uint8_t masking_key[4];
	uint8_t masking_key_len;
};

typedef struct wsframe wsframe_t;

wsframe_t		*wsframe_create();
void			 wsframe_print(wsframe_t *);
int			 wsframe_add_bytes(wsframe_t *, uint8_t *, int);
void			 wsframe_set_payload(wsframe_t *, const char *);
const char		*wsframe_get_payload(wsframe_t *);
void			 wsframe_write(wsframe_t *, int);

#endif
