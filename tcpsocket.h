#ifndef __TCPSOCKET_H__
#define __TCPSOCKET_H__

/*
tcpsocket module - provides a TCP socket for clients or for servers
Tommi Leino <tleino@me.com>

tcpsocket_create(type, host, port)	Create a TCP socket
tcpsocket_free()			Free a TCP socket

tcpsocket_write(msg, len)		Write to the socket
tcpsocket_read()			Read from the socket

Things to note
- Automatically reconnects if connection fails
- Socket type cannot be changed midflight (free and create a new)
*/

#include <stdbool.h>

typedef struct tcpsocket tcpsocket_t;

typedef enum tcpsocket_type
{
	TCPSOCKET_TYPE_SERVER = 0,
	TCPSOCKET_TYPE_CLIENT,
	NUM_TCPSOCKET_TYPE
} tcpsocket_type_t;

tcpsocket_t*	tcpsocket_create(tcpsocket_type_t type, const char *host, int port);
int		tcpsocket_accept(tcpsocket_t *ts);
void		tcpsocket_free(tcpsocket_t *tcp);

int		tcpsocket_get_fd(tcpsocket_t *s);

#endif
