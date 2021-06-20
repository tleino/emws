#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <assert.h>
#define __USE_BSD
#include <string.h>

#include "tcpsocket.h"

struct tcpsocket
{
	tcpsocket_type_t type;
	int fd;	
	bool established;
	bool have_fd;
	bool have_address;
	char *host;
	int port;
	struct sockaddr_in address;
};

static void _ensure_fd(tcpsocket_t *s)
{
	if (s->have_fd) return;

	if ((s->fd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
		perror("socket");
	s->have_fd = true;
}

static void _ensure_address(tcpsocket_t *s)
{
	if (s->have_address) return;

	s->address.sin_family = AF_INET;
	if (s->type == TCPSOCKET_TYPE_SERVER)
		s->address.sin_addr.s_addr = htonl(INADDR_ANY);
	else
		s->address.sin_addr.s_addr = inet_addr(s->host);
	s->address.sin_port = htons(s->port);
	s->have_address = true;
}

static void _connect(tcpsocket_t *s)
{
	_ensure_fd(s);
	_ensure_address(s);

	if (connect(s->fd, (struct sockaddr *) &s->address, sizeof(struct sockaddr_in)) == -1)
		perror("connect");
	s->established = true;

	printf("Established connection to %s:%d\n", s->host, s->port);
}

static void _set_server_socket_options(tcpsocket_t *s)
{
	int val;
	val = 1;
	if (setsockopt(s->fd, SOL_SOCKET, SO_REUSEADDR, &val, sizeof(int)) == -1)
		perror("setsockopt");
	val = 0;
	if (setsockopt(s->fd, SOL_SOCKET, SO_LINGER, &val, sizeof(int)) == -1)
		perror("setsockopt");
}

static void _listen(tcpsocket_t *s)
{
	_ensure_fd(s);
	_ensure_address(s);
	_set_server_socket_options(s);

	if (bind(s->fd, (struct sockaddr *) &s->address, sizeof(struct sockaddr_in)) == -1)
		perror("bind");
	if (listen(s->fd, 1024) == -1)
		perror("listen");
	s->established = true;

	printf("Listening on port %d\n", s->port);
}

tcpsocket_t* tcpsocket_create(tcpsocket_type_t type, const char *host, int port)
{
	assert(type >= 0 && type < NUM_TCPSOCKET_TYPE);
	assert(host);
	assert(port >= 0);

	tcpsocket_t *s = calloc(1, sizeof(tcpsocket_t));
	assert(s);
	s->type = type;
	s->host = strdup(host);
	s->port = port;

	printf("Created tcpsocket of type %d\n", type);

	_listen(s);

	return s;
}

int tcpsocket_accept(tcpsocket_t *ts)
{
	struct sockaddr_in remote;
	socklen_t t = sizeof(remote);
	int fd = accept(ts->fd, (struct sockaddr *) &remote, &t);
	printf("Accepted fd: %d\n", fd);
	return fd;
}

void tcpsocket_free(tcpsocket_t *s)
{
	assert(s);

	if (s->host) free(s->host);

	free(s);
}

int tcpsocket_get_fd(tcpsocket_t *s)
{
	assert(s);

	if (s->established == false) {
		switch (s->type) {
		case TCPSOCKET_TYPE_CLIENT: _connect(s); break;
		case TCPSOCKET_TYPE_SERVER: _listen(s); break;
		default: break;
		}
	}

	assert(s->fd > 0);
	return s->fd;
}
