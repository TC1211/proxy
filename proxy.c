#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>
#include <pthread.h>
#include <netdb.h>
#include <signal.h>

#include "cache.c"
#include "http.c"

/*
TODO:
	--caching ;___;
	--creating HTTP headers, send_data_http
*/

extern int accept4(int sock, struct sockaddr *client_addr, socklen_t *client_addr_len, int flags);
int send_data(int port, char *message);
int proxy(uint16_t port);
int client(char *addr, char *message, int sock_proxy);
void *receive_func(void *);
int parse_request(char *msg, int sock);

/******************************************************

int main: self-explanatory

******************************************************/
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
	max_cache_size = atoi(argv[2]) * 1000000;
	if (max_cache_size < 0) {
		printf("Invalid cache size entered\n");
		return 1;
	}
	// Initialization of the cache
	cache = (cache_t *)malloc(sizeof(cache_t));
	cache->length = 0;
	cache->next = NULL;
	cache->URL = "";
	cache->content = "";

	cache_size = 0;
	printf("port: %d\tcache size: %d B\n", port, max_cache_size);
	signal(SIGPIPE, SIG_IGN);
	return proxy(port);
}


/******************************************************

int send_data: sends message using a given port

******************************************************/
int send_data(int port, char *message) {
	//must build http header before sending...
	if (send(port, message, MAX_MSG_LENGTH, 0) < 0) {
		perror("Send error");
		return 1;
	} 
	return 0;
}


/******************************************************

int client: called if entry is not in cache

******************************************************/
int client(char *addr, char *message, int sock_proxy) {
	struct sockaddr_in server_addr;
	char response[MAX_MSG_LENGTH];
	memset(response, 0, MAX_MSG_LENGTH);

	int sock;
	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Create socket error:");
		return 1;
	}

	printf("Socket created (client)\n");
	server_addr.sin_addr.s_addr = inet_addr(addr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(80); //convention for Internet
	
	if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
		perror("Connect error");
		return 1;
	}

	printf("Connected to server %s\n", addr);

	if (send(sock, message, MAX_MSG_LENGTH, 0) < 0) {
		perror("Send error");
		return 1;
	}
	
	//get response
	int recv_len = read(sock, response, MAX_MSG_LENGTH);
	if (recv_len < 0) { //some kind of error
		perror("Recv error");
		return 1;
	} else if (recv_len == 0) { //no data read
 		close(sock);
		return 0;
	}

	//otherwise, send response
	printf("Response: %s\n", response);
	send_data(sock_proxy, response); 
	return 0;

}

/******************************************************

int proxy: listens for connection requests

******************************************************/
int proxy(uint16_t port)
{
	/* Create server socket*/
	int sock, accepted_client;
	unsigned int client_addr_len;
	struct sockaddr_in socket_addr, client_addr;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Create socket error:");
		return 1;
	}

	printf("Socket created (proxy)\n");

	socket_addr.sin_addr.s_addr = htonl(INADDR_ANY); //should this be changed to localhost?
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
			perror("Creation of thread failed"); 
			return 1; 
		}
	}
	return 0;
}

/******************************************************

void *receive_func: threads start in this function; 
	parse message

******************************************************/
void *receive_func(void *arg) {
	int accepted_client = *(int *)arg;
	char msg[MAX_MSG_LENGTH];
	while(1) { //change this later
		if (recv(accepted_client, msg, MAX_MSG_LENGTH, 0) < 0 ) {
			perror("Recv error");
		}
		if(msg[0] == 'G') {
			printf("Received a GET request\n");
			parse_request(msg, accepted_client);
		}
	}
	return 0;
}

/*
void *receive_func(void *arg) { 
    	int accepted_client = *(int*)arg;
	char *msg = (char *)malloc(MAX_MSG_LENGTH);
	char msg[MAX_MSG_LENGTH];
	struct addrinfo *addrbuf = (struct addrinfo *)malloc(sizeof(struct addrinfo));

	while(1){ // <---- this makes the connection go on forever, must change to include keep alives
		
		if(recv(accepted_client, msg, MAX_MSG_LENGTH, 0) < 0) {
			perror("Recv error");
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
	return 0;
}*/


/******************************************************

char *get_URL_from_request: self-explanatory

******************************************************/
char *get_URL_from_request(char *request) {
	char *copy = strdup(request);

	void *temp = (void *)copy;
	temp += 4 * sizeof(char); //get past "GET "
	copy = (char *)temp;

	int i;
	for (i = 0; i < strlen(request); i++) {
		if (copy[i] == ' ') {		
			char *URL = (char *)malloc(i * sizeof(char));
			memcpy(URL, copy, i - 1);
			return URL; 
		}
	}
	return NULL;
}

/******************************************************

char *get_host_from_request: self-explanatory

******************************************************/
char *get_host_from_request(char *request) {
	char *copy = strstr(request, "Host: ");//strstr finds a "needle" in a "haystack"
	void *temp = (void *)copy;
	temp += 6 * sizeof(char); //get past "Host: "
	copy = (char *)temp;

	int i;
	for (i = 0; i < strlen(request); i++) {
		//printf("%d\n", copy[i]);
		if (copy[i] == '\n') {
			char *host = (char *)malloc(i * sizeof(char)); 
			memcpy(host, copy, i - 1);
			return host;
		} 
	}
	return NULL;
}

/******************************************************

int parse_request: deals with GET requests

******************************************************/
int parse_request(char *msg, int sock) {
	printf(" *** INCOMING REQUEST ***\n%s", msg);
	char *URL = get_URL_from_request(msg); //get target URL; will need for caching
	char *host = get_host_from_request(msg); //get host
	printf("FINAL URL: %s\nFINAL HOST: %s\n", URL, host);
	char *ip_addr = host_to_ipaddr(host);
	
	client(ip_addr, msg, sock);

/*	firstline = strtok(NULL, " ");
	char urlcmp[2] = "\\"; // <---- should be "\" but i had to escape the escape character. I think this is how you do it?
	if(strcmp(firstline,urlcmp) != 0){
		char *url = (char *)malloc(strlen(firstline) + 1);
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
*/
	free(URL);
	return 0;
}

