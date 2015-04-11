#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <stdlib.h>

int server(uint16_t port);
int client(const char * addr, uint16_t port);

#define MAX_MSG_LENGTH (512)
#define MAX_BACK_LOG (5)

int main(int argc, char ** argv)
{
	if (argc < 3) {
		printf("Command should be: myprog s <port> or myprog c <port> <address>\n");
		return 1;
	}
	int port = atoi(argv[2]);
	if (port < 1024 || port > 65535) {
		printf("Port number should be equal to or larger than 1024 and smaller than 65535\n");
		return 1;
	}
	if (argv[1][0] == 'c') {
		if(argv[3]==NULL){
			printf("NO IP address is given\n");
			return 1;
		}
		return client(argv[3], port);
	} else if (argv[1][0] == 's') {
		return server(port);
	} else {
		printf("unknown command type %s\nCommand should be: myprog s <port> or myprog c <port> <address>", argv[1]);
		return 1;
	}
	return 0;
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

int server(uint16_t port)
{
	/* Create server socket*/
	int sock, accepted_client, client_addr_len;
	struct sockaddr_in socket_addr, client_addr;
	char msg[MAX_MSG_LENGTH], reply[MAX_MSG_LENGTH*3];
	pid_t pid;

	if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
		perror("Create socket error:");
		return 1;
	}

	printf("Socket created\n");

	socket_addr.sin_addr.s_addr = htonl(INADDR_ANY);
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

		/* Create new process */
		pid = fork();
		if (pid < 0) {
	         perror("ERROR on fork");
	         return 1;
	        }

		if (pid == 0){   //child -> close parent socket and process new client

			/* Read and triple echo message */
			while(1){ 
			
				if(recv(accepted_client, msg, MAX_MSG_LENGTH, 0) < 0) {
					perror("Recv error:");
					return 1;
				}

				memset(reply,0,sizeof(reply));
				strncpy(reply, msg, strlen(msg));
				strncat(reply, msg, strlen(msg));
				strncat(reply, msg, strlen(msg));
				reply[strlen(reply)] = '\0';
				if (send(accepted_client, reply, MAX_MSG_LENGTH*3, 0) < 0) {
					perror("Send error:");
					return 1;
				}
			}

			return 0;
		}

		if (pid > 0){   // parent -> close client connection to wait for a new one.
			close(accepted_client);
		}
	}
	
	return 0;
}
