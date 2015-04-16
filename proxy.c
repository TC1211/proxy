#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <netdb.h>

#include "helper.c"

extern int accept4(int sock, struct sockaddr *client_addr, socklen_t *client_addr_len, int flags);
int proxy(uint16_t port);
int client(const char * addr, int sock_proxy);
void *receive_func(void *);
int parse_http_header(char *, struct addrinfo **);

int main(int argc, char ** argv)
{
	if (argc != 3) {
		printf("Error: input format should be: ./proxy <port number> <cache size in MB>\n");
		return 1;
	}
	int port = atoi(argv[1]);
	if (port < 1024 || port > 65535) {
		printf("Invalid port number--must be between 1024 and 65535\n");
		return 1;
	}
	int cache_size = atoi(argv[2]);
	if (cache_size < 0) {
		printf("Invalid cache size entered\n");
		return 1;
	}
	// Initialization of the cache <---- check if this is correct!!!
	cache = (cache_t *)malloc(sizeof(cache_t));
	cache->length = 0;
	cache->next = NULL;
	cache->URL = "";
	cache->content = "";

	cache_size = 0;
	printf("port: %d\tcache size: %d MB\n", port, cache_size);
	return proxy(port);
}

int client(const char * addr, int sock_proxy) {
	int sock;
	struct sockaddr_in server_addr;
	char msg[MAX_MSG_LENGTH], reply[MAX_MSG_LENGTH*3];

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Create socket error:");
		return 1;
	}

	printf("Socket created\n");
	server_addr.sin_addr.s_addr = inet_addr(addr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80); //convention for Internet
	
	if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("Connect error: ");
		return 1;
	}

	printf("Connected to server %s\n", addr);

	int recv_len = 0;
	if (send(sock, msg, MAX_MSG_LENGTH, 0) < 0) {
		perror("Send error: ");
		return 1;
	}
	recv_len = read(sock, reply, MAX_MSG_LENGTH);
	if (recv_len < 0) {
		perror("Recv error: ");
		return 1;
	} else if (recv_len == 0) {
 		close(sock);
		return 0;
	} 
	return 0;
}

int proxy(uint16_t port)
{
	/* Create server socket*/
	int sock, accepted_client;
	unsigned int client_addr_len;
	struct sockaddr_in socket_addr, client_addr;
	//char msg[MAX_MSG_LENGTH], reply[MAX_MSG_LENGTH*3];
	//pid_t pid;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Create socket error:");
		return 1;
	}

	printf("Socket created\n");

	socket_addr.sin_addr.s_addr = htonl(INADDR_ANY); //<----- should this be changed to localhost?
	socket_addr.sin_family = AF_INET;
	socket_addr.sin_port = htons(port);
	
	/*Bind socket to an address*/
	if (bind(sock, (struct sockaddr *) &socket_addr, sizeof(socket_addr)) < 0) {
		perror("Bind error");
		return 1;
	}
	
	
	printf("Listening for connections...\n");
	/*Listen for connections */
	listen(sock, 0);
	
	while (1){
		/* Accept the connection */	
	   	accepted_client = accept4(sock, (struct sockaddr *) &client_addr, &client_addr_len,0);
		if (accepted_client < 0) {
	      		perror("Accept error");
	      		return 1;
	      	}
		printf("Accepted client: %i \n", accepted_client);
		
		pthread_t tid;
		int result;
		if((result = pthread_create(&tid, NULL, (void *)receive_func, (void *)&accepted_client))){ 
			perror("Create thread Failed"); 
			return 1; 
		}
	}
	
	return 0;
}

void *receive_func(void *arg) { 
    	int accepted_client = *(int*)arg;
	char *msg = (char *)malloc(MAX_MSG_LENGTH);
	struct addrinfo *addrbuf = (struct addrinfo *)malloc(sizeof(struct addrinfo));

	while(1){ // <---- this makes the connection go on forever, must change to include keep alives
		
		if(recv(accepted_client, msg, MAX_MSG_LENGTH, 0) < 0) {
			perror("Recv error:");
		}
		if(parse_http_header(msg, &addrbuf)){
			printf("Unable to resolve HTTP header\n");
			continue;
		};
		char reply[MAX_MSG_LENGTH*3];
		memset(reply, 0, sizeof(reply));
		strncpy(reply, msg, strlen(msg));
		strncat(reply, msg, strlen(msg));
		strncat(reply, msg, strlen(msg));
		reply[strlen(reply)] = '\0';
		if (send(accepted_client, reply, MAX_MSG_LENGTH*3, 0) < 0) {
			perror("Send error:");
		}
	}
	free(msg);
	return 0;
}

int parse_http_header(char *msg, struct addrinfo ** addrbuf){
	char *parse = malloc(1024);
	char *firstline = malloc(1024);
	parse = strtok(msg, "\r\n");
	firstline = strtok(parse, " ");

	char get[4];
	strncpy(get, "GET", 4);
	if(strcmp(firstline, get)!=0){
		printf("Not a GET request\n");
		return 1;
	}
		
	firstline = strtok(NULL, " ");
	char urlcmp[2] = "\\"; // <---- should be "\" but i had to escape the escape character. I think this is how you do it?
	if(strcmp(firstline,urlcmp)!=0){
		char *url = malloc(strlen(firstline)+1);
		strcpy(url, firstline);
		// getaddrinfo!!!!!!!!!
	} else {
		while(parse != NULL){
			parse = strtok(NULL, "\r\n");
			char *host = "HOST";
			if(strstr(parse, host) != NULL){
				char hostline[1024];
				strcpy(hostline, parse);
				char *addr = malloc(64);
				addr = strtok(hostline, " :");
				addr = strtok(NULL, " :");
			}
		}
		// getaddrinfo!!!!!!!!!!!
	}
	return 0;
}
