#include "message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <assert.h>
#include <time.h>

#define READBUFSZ 1024
#define WRITEBUFSZ 1024 * 100

struct header
{
	char *field;
	char *value;
	struct header *next;
};

static const char _methods[NUM_METHOD][4 + 1] = {
	"NONE",
	"GET"
};

static const char _versions[NUM_VERSION][8 + 1] = {
	"HTTP/0.9",
	"HTTP/1.0",
	"HTTP/1.1"
};

static const char _status[NUM_STATUS][20 + 1] = {
	"OK",
	"Bad Request",
	"Not Implemented",
	"Not Found",
	"Switching Protocols"
};

static const char _statusNo[NUM_STATUS][3 + 1] = {
	"200",
	"400",
	"501",
	"404",
	"101"
};

const char _mime[NUM_MIME][20 + 1] = {
	"text/plain",
	"text/html"
};

void
message_set_header_int(struct message *m, const char *header, int value)
{
	char buf[128];
	snprintf(buf, sizeof(buf), "%d", value);
	message_set_header(m, header, buf);
}

struct message*
message_create(int type)
{
	struct message *m = calloc(1, sizeof(struct message));
	m->type = type;
	m->version = VERSION_09;
	return m;
}

struct message*
message_create_response(uint8_t status, struct message *req)
{
	assert(status >= 0 && status <= NUM_STATUS);
	assert(req);
	assert(req->version >= 0 && req->version <= NUM_VERSION);

	struct message *m = message_create(MSG_RESPONSE);
	m->status = status;
	m->version = req->version;
	m->request = req;
	return m;
}

void
message_set_body(struct message *m, uint8_t mime, const char *body)
{
	assert(mime >= 0 && mime < NUM_MIME);
	m->mime = mime;
	m->body = strdup(body);
}

void
message_load_body(struct message *m, uint8_t mime, const char *file)
{
	FILE *fp;
	char *buf;
	int n;

	fp = fopen(file, "r");
	if (!fp) {
		m->status = STATUS_NOT_FOUND;
		message_set_body(m, MIME_PLAIN, "Couldn't open file");
		return;
	}
	buf = calloc(1, WRITEBUFSZ);
	n = fread(buf, WRITEBUFSZ, 1, fp);
	if (!feof(fp)) { // Couldn't read the file to the end
		m->status = STATUS_NOT_FOUND;
		if (n >= WRITEBUFSZ)
			message_set_body(m, MIME_PLAIN, "File too long");
		else
			message_set_body(m, MIME_PLAIN,
			    "Error while reading file");
		free(buf);
		fclose(fp);
		return;
	}
	message_set_body(m, mime, buf);
	free(buf);
	fclose(fp);
}

void
message_print_response(struct message *m, int fd)
{
	// Update or add header values
	time_t t = time(NULL);

	char *tstr = ctime(&t);
	tstr[strlen(tstr)-1] = 0;
	message_set_header(m, "Date", tstr);
	message_set_header(m, "Server", "libemweb/0.0");

	if (m->protocol == PROTOCOL_HTTP)
		message_set_header(m, "Connection", "close");
	else
		message_set_header(m, "Connection", "Upgrade");

	if (m->body) {
		message_set_header(m, "Content-type", _mime[m->mime]);
		message_set_header_int(m, "Content-length", strlen(m->body));
	}

	// Status Line
	if (m->version > VERSION_09) {
		char buf[256];
		snprintf(buf, sizeof(buf), "%s %s %s\r\n", _versions[m->version], _statusNo[m->status], _status[m->status]);
		write(fd, buf, strlen(buf));
	}

	// Print headers
	struct header *n = m->firstHeader;
	while (n) {
		char buf[256];
		snprintf(buf, sizeof(buf), "%s: %s\r\n", n->field, n->value);
		write(fd, buf, strlen(buf));
		n = n->next;
	}

	// Body
	if (m->body) {
		char buf[256];
		snprintf(buf, sizeof(buf), "\r\n%s\r\n", m->body);
		write(fd, buf, strlen(buf));
	} else {
		char buf[256];
		snprintf(buf, sizeof(buf), "\r\n");
		write(fd, buf, strlen(buf));
	}

	/* Log it to file */
	FILE *fp = fopen("log", "a");
	const char *agent = message_get_header(m->request, "User-Agent");
	const char *host = message_get_header(m->request, "Host");
	char *timestr = ctime(&t);
	timestr[strlen(timestr)-1] = 0;

	fprintf(fp, "--------\n");
	n = m->request->firstHeader;
	while (n) {
		fprintf(fp, "%s: %s\n", n->field, n->value);
		n = n->next;
	}
	fprintf(fp, "%s %s %s %s %s\n",
	    timestr, _statusNo[m->status],
	    agent ? agent : "null", host ? host : "null", m->request->uri);
	fclose(fp);
}

void
message_free(struct message *m)
{
	if (m->body)
		free(m->body);
	free(m->uri);
	free(m);
}

void
message_add_line(struct message *m, const char *line)
{
	assert(m->type == MSG_TYPE_REQUEST);

	// Parse HTTP Request-Line e.g. GET / HTTP/1.1
	if (m->method == 0) {
		char *field, *fields = strdup(line);

		// Parse method, delimeted by ' '
		field = strtok(fields, " ");
		if (!field) { free(fields); return; }
		if (!strcmp(field, _methods[METHOD_GET]))
			m->method = METHOD_GET;

		// Parse uri, delimeted by ' '
		field = strtok(NULL, " ");
		if (!field) { free(fields); return; }
		m->uri = strdup(field);

		// Parse version, delimeted by CRLF
		field = strtok(NULL, "\n");
		if (!field) { free(fields); return; }
		if (!strncmp(field, _versions[VERSION_09], 8))
			m->version = VERSION_09;
		else if (!strncmp(field, _versions[VERSION_10], 8))
			m->version = VERSION_10;
		else if (!strncmp(field, _versions[VERSION_11], 8))
			m->version = VERSION_11;
		else
			m->version = VERSION_11;

		free(fields);
	} else { // Parse headers
		char *field = strdup(line);
		char *value = strchr(field, ':');
		field[value - field] = '\0';
		if (!value) { free(field); return; }
		value++;
		if (*value == ' ') value++;
		message_set_header(m, field, value);

	}
}

void
message_set_header(struct message *m, const char *field, const char *value)
{
	// Try find existing header
	struct header *n = m->firstHeader;
	while (n) {
		if (!strcmp(n->field, field)) {
			strcpy(n->value, value);
			return;
		}
		n = n->next;
	}

	// Add a new header
	struct header *h = calloc(1, sizeof(struct header));
	h->field = strdup(field);
	h->value = strdup(value);
	if (m->firstHeader == NULL)
		m->firstHeader = h;
	else {
		n = m->firstHeader;
		while (n->next) n = n->next;
		n->next = h;
	}
}

const char*
message_get_header(struct message *m, const char *header)
{
	assert(m);
	struct header *n = m->firstHeader;
	while (n) {
		if (!strcasecmp(n->field, header)) {
			return n->value;
		}
		n = n->next;
	}
	return NULL;
}
