#include "wsframe.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define _STATE_INIT1		0
#define _STATE_INIT2		1
#define _STATE_EXTRA_LENGTH	2
#define _STATE_MASKING_KEY	3
#define _STATE_PAYLOAD		4
#define _STATE_COMPLETE		5
#define _NUM_STATE		6	

static const char *_state_str[] = {
	"init1",
	"init2",
	"extra_length",
	"masking_key",
	"payload",
	"complete"
};

#define _OPCODE_CONTINUATION	0
#define _OPCODE_TEXT		1
#define _OPCODE_BINARY		2
#define _NUM_OPCODE		3

static const char *_opcode_str[] = {
	"continuation",
	"text",
	"binary"
};

wsframe_t*
wsframe_create()
{
	wsframe_t *ws = calloc(1, sizeof(wsframe_t));
	return ws;
}

void
wsframe_print(wsframe_t *ws)
{
	printf("state=%s ", _state_str[ws->state]);
	if (ws->fin) printf("FIN ");
	if (ws->rsv1) printf("RSV1 ");
	if (ws->rsv2) printf("RSV2 ");
	if (ws->rsv3) printf("RSV3 ");
	printf("opcode=%s ", _opcode_str[ws->opcode]);
	if (ws->masked) printf("MASKED ");
	printf("len=%llu ", ws->len);
	printf("payload_len=%d ", ws->payload_len);
	if (ws->payload_len && ws->payload_len == ws->len)
		printf("payload= %s", ws->payload);
	printf("\n");
}

int
wsframe_add_bytes(wsframe_t *ws, uint8_t *bytes, int len)
{
	if (ws->state == _STATE_COMPLETE) ws->state = _STATE_INIT1;

	int i;
	for (i = 0; i < len; i++) {
		char byte = bytes[i];

		if (ws->state == _STATE_INIT1) {
			if (ws->payload) free(ws->payload);
			memset(ws, 0, sizeof(wsframe_t));

			ws->fin = (byte & 128) >> 7; // %1000 0000
			ws->rsv1 = (byte & 64) >> 6; // %0100 0000
			ws->rsv2 = (byte & 32) >> 5; // %0010 0000
			ws->rsv3 = (byte & 16) >> 4; // %0001 0000
			ws->opcode = (byte & 0x0F); // %0000 1111
			ws->state = _STATE_INIT2;
			continue;
		}
		if (ws->state == _STATE_INIT2) {
			ws->masked = (byte & 128) >> 7; // %1000 0000
			ws->len = (byte & 127); // %0111 1111
			ws->payload = malloc(ws->len + 1);
			if (ws->masked) ws->state = _STATE_MASKING_KEY;
			else ws->state = _STATE_PAYLOAD;
			continue;
		}
		if (ws->state == _STATE_MASKING_KEY) {
			ws->masking_key[ws->masking_key_len++] = byte;
			if (ws->masking_key_len == 4)
				ws->state = _STATE_PAYLOAD;
			continue;
		}
		if (ws->state == _STATE_PAYLOAD) {
			if (ws->masked) {
				int j = ws->payload_len % 4;
				byte = byte ^ ws->masking_key[j];
			}
			ws->payload[ws->payload_len++] = byte;
			if (ws->payload_len == ws->len) {
				ws->payload[ws->payload_len] = 0;
				ws->state = _STATE_COMPLETE;
				return i;
			}
		}
	}
	return i;
}

void
wsframe_set_payload(wsframe_t *ws, const char *payload)
{
	memset(ws, 0, sizeof(wsframe_t));
	ws->fin = true;
	ws->len = strlen(payload);
	ws->payload = strdup(payload);
	ws->payload_len = ws->len;
	ws->opcode = _OPCODE_TEXT;
	ws->state = _STATE_COMPLETE;
}

const char*
wsframe_get_payload(wsframe_t *ws)
{
	return ws->payload;
}

void
wsframe_write(wsframe_t *ws, int fd)
{
	char buf[256];
	buf[0] = (unsigned char) 0x81; // %1000 0001
	buf[1] = ws->len;
	strcpy(&buf[2], ws->payload);
	write(fd, buf, strlen(buf));
	wsframe_add_bytes(ws, (uint8_t *) buf, strlen(buf));
	wsframe_print(ws);
}

#ifdef TEST
int main(int argc, char **argv)
{
	wsframe_t *ws;
	static const uint8_t b[7] = {
		0x81, 0x05, 0x48, 0x65, 0x6c, 0x6c, 0x6f
	};
	static const uint8_t B[11] = {
		0x81, 0x85, 0x37, 0xfa, 0x21, 0x3d, 0x7f, 0x9f,
		0x4d, 0x51, 0x58
	};

	ws = wsframe_create();

	wsframe_print(ws);
	int n = wsframe_add_bytes(ws, b, 3);
	wsframe_print(ws);
	wsframe_add_bytes(ws, &(b[n]), 4);
	wsframe_print(ws);

	printf("----\n");

	wsframe_print(ws);
	n = wsframe_add_bytes(ws, B, 11);
	wsframe_print(ws);

	printf("Payload: %s\n", wsframe_get_payload(ws));

	wsframe_set_payload(ws, "Foobar");
	wsframe_print(ws);

	wsframe_write(ws, 1);
}
#endif
