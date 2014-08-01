#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#include "hostnameip.h"

int hostname_to_ip(char *hostname, char *ip, int ip_len)
{
    struct addrinfo hints, *servinfo, *p;
    struct sockaddr_in *h;
    int retval;

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;	// use AF_INET6 to force IPv6
    hints.ai_socktype = SOCK_STREAM;

    if ((retval = getaddrinfo(hostname, "http", &hints, &servinfo)) != 0) {
	fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(retval));
	return 1;
    }
    //FIXME bad address selection
    // loop through all the results and connect to the first we can
    for (p = servinfo; p != NULL; p = p->ai_next) {
	h = (struct sockaddr_in *) p->ai_addr;
	strncpy(ip, inet_ntoa(h->sin_addr), ip_len);
    }

    freeaddrinfo(servinfo);	// all done with this structure
    return 0;
}
