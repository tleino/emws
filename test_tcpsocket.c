#include "tcpsocket.h"

#include <stdio.h>

int main(int argc, char **argv)
{
	tcpsocket_t *server = tcpsocket_create(TCPSOCKET_TYPE_SERVER, "127.0.0.1", 8090);
	printf("Server fd: %d\n", tcpsocket_get_fd(server));
	tcpsocket_t *client = tcpsocket_create(TCPSOCKET_TYPE_CLIENT, "127.0.0.1", 8090);
	
	printf("Client fd is: %d\n", tcpsocket_get_fd(client));

	tcpsocket_free(server);
	tcpsocket_free(client);
	return 0;
}
