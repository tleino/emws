#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <time.h>

#include "sha1.h"
#include "base64.h"
#include "tcpsocket.h"
#include "message.h"
#include "wsframe.h"

#define READBUFSZ 1024

int
main(int argc, char **argv)
{
	tcpsocket_t *ts;
	int fd;
	struct message *m;
	char buf[READBUFSZ];
	int emptyLine;
	int n;
	char *saveptr;
	char *lines;
	int lastChar;
	struct message *rm;
	const char *host;
	const char *upgrade;
	const char *websocketkey;
	char *line;
	FILE *fp;
	char accept[256];
	char *out;
	int j;
	wsframe_t *ws;

	if (argc != 2) {
	usage:
		fprintf(stderr, "Usage: %s - | <port>\n", argv[0]);
		return 1;
	}
	if (argv[1][0] == '-') {
		printf("Listening on stdin\n");
		fd = 0;
	} else if (atoi(argv[1]) > 0) {
		ts = tcpsocket_create(TCPSOCKET_TYPE_SERVER, "0.0.0.0",
		    atoi(argv[1]));
		fd = tcpsocket_accept(ts);
	} else {
		goto usage;
	}

	m = message_create(MSG_TYPE_REQUEST);
	emptyLine = 0;
	while (1) {
		n = read(fd, buf, sizeof(buf));
		if (n <= 0) { // EOF or error
			break;
		}
		buf[n] = 0;
		lines = strdup(buf);
		line = strtok_r(lines, "\n", &saveptr);
		if (!line)
			emptyLine++;
		while (line) {
			lastChar = strlen(line)-1;
			if (line[lastChar] == '\r') {
				line[lastChar] = '\0';
			}

			if (line[0] == '\0') {
				emptyLine = 1; break;
			}
			emptyLine = 0;
			message_add_line(m, line);
			if (m->version <= VERSION_09)
				emptyLine = 1;
			line = strtok_r(NULL, "\n", &saveptr);
		}
		free(lines);
		if (emptyLine >= 1) break;
		continue;
	}

	if (m->method != METHOD_GET) {
		rm = message_create_response(STATUS_NOT_IMPLEMENTED, m);
		message_set_body(rm, MIME_PLAIN,
		    "Method not implemented");
	}
	else if (m->uri == NULL || strlen(m->uri) == 0) {
		rm = message_create_response(STATUS_BAD_REQUEST, m);
		message_set_body(rm, MIME_PLAIN, "Invalid URI");
	}
	else {
		host = message_get_header(m, "Host");
		if (host == NULL && m->version >= VERSION_11) {
			rm = message_create_response(
			    STATUS_BAD_REQUEST, m);
			message_set_body(rm, MIME_PLAIN,
			    "Host header missing");
		} else {
			rm = message_create_response(STATUS_OK, m);
			message_set_body(rm, MIME_PLAIN,
			    "Nothing here yet");

			upgrade = message_get_header(m, "Upgrade");
			if (upgrade) {
				if (!strcmp(upgrade, "websocket")) {
					websocketkey = message_get_header(m,
					    "Sec-WebSocket-Key");

				fp = fopen("log", "a");
				if (websocketkey)
					fprintf(fp, "WebSocketKey: %s\n",
					    websocketkey);
				fclose(fp);

				rm = message_create_response(
				    STATUS_SWITCHING_PROTOCOLS, m);
				rm->protocol = PROTOCOL_WEBSOCKET;
				message_set_header(rm, "Upgrade", "websocket");
				message_set_header(rm, "Connection", "upgrade");
				out = NULL;
				snprintf(accept, sizeof(accept), "%s%s",
				    websocketkey,
				    "258EAFA5-E914-47DA-95CA-C5AB0DC85B11");
				SHA1Context sha;
				uint8_t Message_Digest[20];
				SHA1Reset(&sha);
				SHA1Input(&sha, (const unsigned char *) accept,
				    strlen(accept));
				SHA1Result(&sha, Message_Digest);

				fp = fopen("log", "a");
				accept[0] = 0;
				for (j = 0; j < 20; j++)
					sprintf(accept, "%s%02X", accept,
					    Message_Digest[j]); 
				fprintf(fp, "accept: %s\n", accept);
				fclose(fp);

				base64_encode(Message_Digest, 20, &out);
					
				message_set_header(rm, "Sec-WebSocket-Accept",
				    out);
				message_print_response(rm, fd);
				goto secondReadingLoop;

				}
			}
		}
	}
	message_print_response(rm, fd);
	secondReadingLoop:
	if (rm->protocol == PROTOCOL_WEBSOCKET) {
		while (1) {
			n = read(fd, buf, sizeof(buf));
			if (n <= 0) { // EOF or error
				break;
			}
			buf[n] = '\0';

			ws = wsframe_create();

			wsframe_add_bytes(ws, (uint8_t *) buf, n);
			wsframe_print(ws);

			printf("Payload: %s\n", wsframe_get_payload(ws));

			wsframe_set_payload(ws, "Foobar");
			wsframe_print(ws);

			wsframe_write(ws, 1);
			sleep(5);
		}
	}

	message_free(m);
	message_free(rm);

	return 0;
}
