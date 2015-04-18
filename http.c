
typedef struct http_header {
	//fill this out!
} header_t;

//getting/processing getaddrinfo stuff; returns ip addr
char *host_to_ipaddr(char *host);


char *host_to_ipaddr(char *host) {
//	int sockfd; 
	int rv;
	char *ip_addr;
	struct addrinfo hints, *servinfo, *p;
	struct sockaddr_in *info;

	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_INET;
	hints.ai_socktype = SOCK_STREAM;

	rv = getaddrinfo(host, NULL, &hints, &servinfo);
	if (rv != 0) {
		printf("Error with getaddrinfo: %s\n", gai_strerror(rv));
		return NULL;
	}
	
	//loop through all results, connect with first one possible
	for (p = servinfo; p != NULL; p = p->ai_next) {
/*		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("Socket");
			continue;
		}
		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("Connect");
			continue;
		}
		//what does all of this do anyway!?
*/		
		info = (struct sockaddr_in *)p->ai_addr;
		ip_addr = (char *)inet_ntoa(info->sin_addr);
//		break; //must have connected successfuly if reach this point
	}
/*	if (p == NULL) {
		//got to end of the list, made no connections
		printf("Failed to connect in getaddrinfo\n");
		free(host);
		return NULL;
	}
*/	
	freeaddrinfo(servinfo); //finished with this struct
//	free(host);
	return ip_addr;
}


