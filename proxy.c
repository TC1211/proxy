#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <netdb.h>

int proxy(uint16_t port);
int client(const char * addr, uint16_t port);
void *receive_func(void *);
int parse_http_header(char *, struct addrinfo **);

#define MAX_MSG_LENGTH (1024*16)
#define MAX_BACK_LOG (5)

typedef struct cache{
	unsigned int length;
	struct cache *next;
	char *URL;
	char *content;
}cache_t;

int main(int argc, char ** argv)
{
	if (argc != 2) {
		printf("Command should be: port number <port> and cache size <size>\n");
		return 1;
	}
	// Initialization of the cache <---- check if this is correct!!!
	cache_t *head = (cache_t *)malloc(sizeof(cache_t));
	head->length = 0;
	head->next = NULL;
	head->URL = "";
	head->content = "";

	int port = atoi(argv[1]);
	return proxy(port);
}

int client(const char * addr, uint16_t port)
{
	int sock;
	struct sockaddr_in server_addr;
	char msg[MAX_MSG_LENGTH], reply[MAX_MSG_LENGTH*3];

	if ((sock = socket(AF_INET, SOCK_STREAM/* use tcp */, 0)) < 0) {
		perror("Create socket error:");
		return 1;
	}

	printf("Socket created\n");
	server_addr.sin_addr.s_addr = inet_addr(addr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(port);
	
	if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("Connect error:");
		return 1;
	}

	printf("Connected to server %s:%d\n", addr, port);

	int recv_len = 0;
	while (1) {
		fflush(stdin);
		printf("Enter message: \n");
		gets(msg);
		if (send(sock, msg, MAX_MSG_LENGTH, 0) < 0) {
			perror("Send error:");
			return 1;
		}
		recv_len = read(sock, reply, MAX_MSG_LENGTH*3);

		if (recv_len < 0) {
			perror("Recv error:");
			return 1;
		}
		reply[recv_len] = 0;
		printf("Server reply:\n%s\n", reply);
		memset(reply, 0, sizeof(reply));
	}
	close(sock);
	return 0;
}

int proxy(uint16_t port)
{
	/* Create server socket*/
	int sock, accepted_client, client_addr_len;
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
		printf("Accepted client: %i \n", accepted_client);
		if (accepted_client < 0) {
	      		perror("Accept error");
	      		return 1;
	      	}

		
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
		memset(reply,0,sizeof(reply));
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
		// getaddrinfo!!!!!!!!!1
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
}
