#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>
#include <ctype.h>

#define BUFSIZE 	4096
#define MAX_NAME	20

void msgHandle(int connfd);

int main(){
	struct sockaddr_in servaddr;
	int serverfd, response;
	char user[MAX_NAME];
	pthread_t t;
	char msg[BUFSIZE];

	/* connection setting */
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = inet_addr("0.0.0.0");
	servaddr.sin_port = htons(8080);

	serverfd = socket(AF_INET, SOCK_STREAM, 0);
	response = connect(serverfd , (struct sockaddr*) &servaddr , sizeof(servaddr));	

	pthread_create(&t , NULL , (void*) &msgHandle , (void*) serverfd);
	memset(msg ,'\0' ,BUFSIZE);
	while(1){
		fflush(stdout);
		memset(msg ,'\0' ,BUFSIZE); 
		fgets(msg ,BUFSIZE ,stdin);
		send(serverfd ,msg ,sizeof(msg) ,0);
		if(strncmp(msg ,"-q", 2) == 0 || strncmp(msg, "-quit", 5) == 0){
			break;
		}
		memset(msg ,'\0' ,BUFSIZE);
	}

	pthread_join(t , NULL);
	close(serverfd);
	return 0;
}

void msgHandle(int connfd){
	int ret;
	char recv_msg[BUFSIZE];
	char send_msg[BUFSIZE];
	while(1){
		memset(recv_msg ,'\0' ,BUFSIZE);
		fflush(stdout);
		ret = recv(connfd ,recv_msg ,BUFSIZE , 0);
		if(ret > 0){
			printf("%s" ,recv_msg);
			if(strncmp(recv_msg ,"quit", 4) == 0){
				break;
			}
		}
	}
}
