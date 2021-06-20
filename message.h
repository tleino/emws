#ifndef MESSAGE_H
#define MESSAGE_H

#include <stdint.h>

#define MSG_TYPE_NONE		0
#define MSG_TYPE_REQUEST	1
#define MSG_RESPONSE	2

#define METHOD_NONE		0
#define METHOD_GET		1
#define NUM_METHOD		2

#define VERSION_09		0
#define VERSION_10		1
#define VERSION_11		2
#define NUM_VERSION		3

#define STATUS_OK		0
#define STATUS_BAD_REQUEST	1
#define STATUS_NOT_IMPLEMENTED	2
#define STATUS_NOT_FOUND	3
#define STATUS_SWITCHING_PROTOCOLS	4
#define NUM_STATUS			5

#define PROTOCOL_HTTP		0
#define PROTOCOL_WEBSOCKET	1
#define NUM_PROTOCOL		2

#define MIME_PLAIN		0
#define MIME_HTML		1
#define NUM_MIME		2

struct message
{
	uint8_t type;
	uint8_t method;
	uint8_t status;
	char *uri;
	uint8_t version;
	struct header *firstHeader;
	char *body;
	uint8_t mime;
	uint8_t protocol;
	struct message *request;
};

struct message			*message_create(int);
struct message			*message_create_response(uint8_t,
				    struct message *);
void				 message_set_body(struct message *,
				    uint8_t, const char *);
void				 message_load_body(struct message *,
				    uint8_t, const char *);
void				 message_print_response(struct message *,
				    int);
void				 message_free(struct message *);
void				 message_add_line(struct message *,
				    const char *);
void				 message_set_header(struct message *,
				    const char *, const char *);
const char			*message_get_header(struct message *,
				    const char *);
const char			*message_get_header(struct message *,
				    const char *);
void				 message_set_header(struct message *,
				    const char *, const char *);

#endif
