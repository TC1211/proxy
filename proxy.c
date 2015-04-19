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

cache_t *cache;
int cache_size/*in B*/, max_cache_size/*in B*/;

extern int accept4(int sock, struct sockaddr *client_addr, socklen_t *client_addr_len, int flags);
int send_data(int, char *);
int proxy(uint16_t);
int client(char *, char *, int, char *);
void *receive_func(void *);
int parse_request(char *, int);
int cache_check(char *, int);

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

	if (send(port, message, sizeof(message), 0) < 0) {
		perror("Send error");
		return 1;
	} 
	return 0;
}


/******************************************************

int client: called if entry is not in cache

******************************************************/
int client(char *addr, char *message, int sock_proxy, char *URL) {
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
	printf("////////////////////////\n%s\n**********************\n", message);
	if (send(sock, message, MAX_MSG_LENGTH, 0) < 0) {
		perror("Send error");
		return 1;
	}
	
	int readlen;
	char *packet = (char *)malloc(MAX_MSG_LENGTH*3);
	memset(packet, 0, MAX_MSG_LENGTH*3);
	while ((readlen = recv(sock, response, sizeof(response),0))!= 0){
		//printf("%s", response);
		strcat(packet, response);
        	send(sock_proxy, response, readlen,0);
	}
	
	printf("%s\n", packet);
	if(strstr(packet, "200 OK") != NULL){ //must be 200 response
		while((cache_size+sizeof(packet)) > max_cache_size){
			cache_t *temp = cache;
			cache = cache->next;
			cache_size -= temp->length;
			free(temp);
		}
		printf("\n\n\nmax: %i current: %i\n\n\n", max_cache_size, (int)strlen(packet));
		add_cache_entry(&cache, packet, URL);
		cache_size += (int)strlen(packet);
	}
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
		memset(msg, 0, MAX_MSG_LENGTH);
		if (recv(accepted_client, msg, MAX_MSG_LENGTH, 0) < 0 ) {
			perror("Recv error");
		}
		if(strstr(msg, "Connection: close")!=NULL)	break;
		if(msg[0] == 'G') {
			printf("Received a GET request\n");
			parse_request(msg, accepted_client);
		}
		
	}
	pthread_exit(NULL);
	return 0;
}


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
			memcpy(URL, copy, i);
			return URL; 
		}
	}
	return NULL;
}

/******************************************************

char *get_host_from_request: self-explanatory

******************************************************/
char *get_host_from_request(char *request) {
	char *host;
	host = strtok(request,"//");
	host = strtok(NULL,"/");
	return host;
}

/******************************************************

int parse_request: deals with GET requests

******************************************************/
int parse_request(char *msg, int sock) {
	printf(" *** INCOMING REQUEST ***\n");
	char *msg_whole;
	msg_whole = malloc(MAX_MSG_LENGTH);
	strcpy(msg_whole, msg);
	char *URL = get_URL_from_request(msg); //get target URL; will need for caching
	char *host = get_host_from_request(msg); //get host
	printf("FINAL URL: %s\nFINAL HOST: %s\n", URL, host);
	char *ip_addr = host_to_ipaddr(host);
	if(ip_addr == NULL)	return 1;
	if(cache_check(URL, sock))	return 0;
	client(ip_addr, msg_whole, sock, URL);


	free(URL);
	return 0;
}

int cache_check(char *URL, int accepted_client){
	cache_t *cache_entry;
	if((cache_entry = search_cache(&cache, URL)) == NULL){
		printf("Entry not i cache\n\n");
		return 0;
	} else {
		printf("entry in cache\n\n");
		enforce_LRU_middle(&cache, URL); 
		send_data(accepted_client, cache_entry->content);
		return 1;
	}
	return 0; 
}

