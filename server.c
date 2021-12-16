#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define BUFSIZE		4096
#define LISTENQ		64
#define MAX_PLAYER	10
#define MAX_NAME	20

struct player{
	char name[MAX_NAME];
	int game_state;
	int connfd;
};
struct player players[MAX_PLAYER];
char user[10][BUFSIZE] = {
	"cat/cute",
	"cake/delicious",
	"deadline/horrible",
	"tainan/sweet",
	"winter/cold",
	"dog/crazy",
	"math/hard",
	"wanna/sleep",
	"aaa/123",
	"bbb/456"
};
int isWin(char board[]);
void print_board(char b[], char board[]);
void commen(int index); 
int findEmptyConn(void);
void login(int index);
void server_control(int listenfd);
void server_handle(int index);
void usage(void);
void show_list(int index);
void game_play(int index ,int id);
int b_isEmpty(char *b);
int main(){
	int listenfd, length, i;
	struct sockaddr_in servaddr, cliaddr;
	char buffer[BUFSIZE];
	pthread_t server_t;

	/* listening */
	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(8080);
	bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr));
	listen(listenfd, LISTENQ);
	
	/* create thread to control server */
	pthread_create(&server_t, NULL, (void *)(&server_control), (void *)listenfd);
	
	/* init player */
	for(i=0; i<MAX_PLAYER; i++){
		strcpy(players[i].name, "\0");
		players[i].game_state = -1;
		players[i].connfd = -1;
	}

	/* waiting for clients */
	fprintf(stderr, "Waiting connect...\n");
	while(1){
		length = sizeof(cliaddr);
		i = findEmptyConn();
		players[i].connfd = accept(listenfd, (struct sockaddr*)&cliaddr, &length);
		pthread_create(malloc(sizeof(pthread_t)), NULL, (void *)(&server_handle), (void *)i);
	}
}

void server_control(int listenfd){
	char input[10];	
	while(1){
		scanf("%s", input);
		if(strcmp(input, "-q") == 0 || strcmp(input, "-quit") == 0){
			printf("close server\n");
			close(listenfd);
			exit(0);
		}
		else if(strcmp(input, "-h") == 0 || strcmp(input, "-help") == 0){
			usage();			
		}
		else{
			fprintf(stderr, "Unkown commen\n");
			fprintf(stderr, "-h or -help for usage.\n");
		}
	}
}

void usage(){
	fprintf(stderr,"-q, -quit :quit the server\n");
	fprintf(stderr,"-h, -help :show usage\n\n");
}

void server_handle(int index){
	printf("Player %d is connecting...\n", index);
	login(index);
	commen(index);
	pthread_exit(NULL);
}

void login(int index){
	char msg[BUFSIZE]; 
	char acc_msg[] = "account:\n";
	char pwd_msg[] = "password:\n";
	char login_succ[] = "\nLogin success\npress enter\n";
	char login_fail[] = "\nLogin failed\ntry again\npress enter\n";
	char acc_input[BUFSIZE], pwd_input[BUFSIZE], tmp[BUFSIZE];

	int ret, exist = 0;
	char *ptr = NULL;
	char acc_pwd[BUFSIZE];
	/* input acc/pwd and check */
	while(1){
		sprintf(msg, "Your player id is %d, please log in.\n", index);
		send(players[index].connfd, msg, strlen(msg), 0);
		/* account */
		send(players[index].connfd, acc_msg, sizeof(acc_msg), 0);
		ret = recv(players[index].connfd ,acc_input ,BUFSIZE , 0);
		if(ptr = strchr(acc_input, '\n')){
			*ptr = '\0';
		}
		/* password */
		send(players[index].connfd, pwd_msg, sizeof(pwd_msg), 0);
		ret = recv(players[index].connfd, pwd_input, BUFSIZE , 0);
		if(ptr = strchr(pwd_input, '\n')){
			*ptr = '\0';
		}
		/* check */
		memset(acc_pwd, '\0', BUFSIZE);
		strcat(acc_pwd, acc_input);
		strcat(acc_pwd, "/");
		strcat(acc_pwd, pwd_input);
		for(int i=0; i<MAX_PLAYER; i++){
			if(strcmp(acc_pwd, user[i]) == 0){
				exist = 1;
				break;
			}
		}
		if(exist){
			send(players[index].connfd, login_succ, sizeof(login_succ), 0);
			ret = recv(players[index].connfd, tmp, BUFSIZE, 0);
			strcpy(players[index].name, acc_input);
			break;
		}
		else{
			send(players[index].connfd, login_fail, sizeof(login_fail), 0);
			ret = recv(players[index].connfd, tmp, BUFSIZE, 0);
		}
	}
	printf("\nplayer id: %d name: %s Login success\n\n", index, players[index].name);
}

int findEmptyConn(){
	for(int i=0;i < MAX_PLAYER;i++){
		if(players[i].connfd == -1)
			return i;
	}
	return 0;
}

void commen(int index){
	char comm[] = "-l\tlist online user\n-r id\tinvite player\n-q\tlogout and quit server\n> ";
	char response[BUFSIZE] , tmp[BUFSIZE];
	int ret;
	char *ptr;
	send(players[index].connfd, comm, sizeof(comm), 0);

	while(1){
		if(players[index].game_state != -1){
			continue;
		}
		memset(response, '\0', BUFSIZE);
		fflush(stdout);
		ret = recv(players[index].connfd, response, BUFSIZE, 0);
		if(ptr = strchr(response, '\n')){
			*ptr = '\0';
		}
		/* show list */
		if(strcmp(response, "-l") == 0){
			show_list(index);
		}
		/* log out */
		else if(strncmp(response, "-q", 2) == 0){
			char quit[] = "quit\n";
			printf("\nClient id: %d name: %s Logout success\n\n" , index , players[index].name);
			send(players[index].connfd, quit, sizeof(quit), 0);
			memset(players[index].name, '\0', MAX_NAME);
			players[index].connfd = -1;
			break;
		}
		/* invite other gamer */
		else if(strncmp(response, "-r", 2) == 0){
			int id = -1 ,ptr;
			char recv_id[BUFSIZE];
			char req_fail[] = "invite failed\n> ";
			char req_cancel[] = "invite canceled\n> ";

			for(ptr=2;ptr<BUFSIZE;ptr++){
				if(response[ptr] == ' '){
					continue;
				}
				if(!isdigit(response[ptr])){
					ptr = -1;
					send(players[index].connfd, req_fail ,sizeof(req_fail), 0);
					break;
				}
				else break;
			}
			/* illegal */
			if(ptr == -1 || ptr >= BUFSIZE || !isdigit(response[ptr])){
				continue;
			}

			int shift = ptr;

			for(int i=ptr;i<BUFSIZE;i++){
				if(isdigit(response[i])){
					recv_id[i - shift] = response[i];
				}
				else if(response[i] == '\0'){
					recv_id[i - shift] = response[i];
					id = atoi(recv_id);
					break;
				}
				else{
					id = -1;
					send(players[index].connfd, req_fail, sizeof(req_fail), 0);
					break;
				}
			}

			if(id != -1){
				if((id >= MAX_PLAYER) || (players[id].connfd == -1)){
						char no_user[BUFSIZE];
						sprintf(no_user , "id %d is not exist!\n> ", id);
						send(players[index].connfd, no_user, sizeof(no_user), 0);
						continue;
				}
				if(id == index){
					char self_req[] = "You can't play with yourself!\n> ";
					send(players[index].connfd, self_req, sizeof(self_req), 0);
					continue;
				}
				/* player not free */
				if(players[index].game_state != -1){
					char busy_msg[] = "User is busy\n> ";
					send(players[index].connfd, busy_msg, sizeof(busy_msg), 0);
					continue;
				}
				/* sent invite */
				char req_confirm[BUFSIZE];
				sprintf(req_confirm, "Send invite to %s(y/n)> ", players[id].name);
				send(players[index].connfd, req_confirm, sizeof(req_confirm), 0);
				memset(response, '\0', BUFSIZE);
				ret = recv(players[index].connfd, response, BUFSIZE, 0);
				/* agree */
				if((strncmp(response, "y", 1) == 0) || (strncmp(response, "Y", 1) == 0)){
					char req[BUFSIZE];
					players[id].game_state = index;
					players[index].game_state = id;
					sprintf(req ,"\nuser: %s sent a invite to you\n", players[index].name);
					send(players[id].connfd, req, sizeof(req), 0);
					sprintf(req, "Play wiht %s?(y/n)> ", players[index].name);
					send(players[id].connfd, req, sizeof(req), 0);
					ret = recv(players[id].connfd, response, BUFSIZE, 0);
						
					if((strncmp(response, "y", 1) == 0) || (strncmp(response, "Y", 1) == 0)){
						char msg[BUFSIZE];
						sprintf(msg , "Playing with %s\npress enter\n", players[index].name);
						send(players[id].connfd, msg, sizeof(msg), 0);
						sprintf(msg ,"Playing with %s\npress enter\n", players[id].name);
						send(players[index].connfd, msg, sizeof(msg), 0);

						game_play(index ,id);

						players[index].game_state = -1;
						players[id].game_state = -1;
					}
					else{						
						char msg[] = "invite fail\n> ";
						players[index].game_state = -1;
						players[id].game_state = -1;
						send(players[id].connfd, msg, sizeof(msg), 0);
						send(players[index].connfd, msg, sizeof(msg), 0);
					}				
				}
				else if((strncmp(response, "n", 1) == 0) || (strncmp(response, "N", 1) == 0)){
					send(players[index].connfd, req_cancel, sizeof(req_cancel), 0);
				}
				else{
					send(players[index].connfd, req_fail, sizeof(req_fail), 0);
				}
			}
			else{
				send(players[index].connfd, req_fail, sizeof(req_fail), 0);
			}	
		}	
		else{
			char msg[] = "> ";
			send(players[index].connfd, msg, sizeof(msg), 0);
		}
	}
}
void show_list(int index){
	char head[BUFSIZE] = "┌────────────────────────────────────┐\n id\tuser name\tgame state\n--------------------------------------\n";
	char end[] = "└────────────────────────────────────┘\n> ";
	for(int i=0;i < MAX_PLAYER;i++){
		if(players[i].connfd != -1){
			char tmp[BUFSIZE];
			sprintf(tmp, " %d\t%s\t\t",i, players[i].name);
			if(players[i].game_state == -1){
				strcat(tmp, "free\n");
			}
			else{
				strcat(tmp, "busy\n");
			}
			strcat(head, tmp);
		}
	}
	strcat(head, end);
	send(players[index].connfd, head, sizeof(head), 0);
}
void game_play(int a, int b){
	int ret, tmp;
	char turn = 'a';
	char recv_msg[BUFSIZE], send_msg[BUFSIZE],  print_b[BUFSIZE];
	char board[10];
	for(int i = 0;i < 10;i++){
		board[i] = ' ';
	}
	board[9] = '\0';
	print_board(print_b, board);
	sprintf(send_msg, "%syou are 'O'\n", print_b );
	send(players[b].connfd, send_msg, BUFSIZE, 0);
	
	ret = recv(players[a].connfd, recv_msg, BUFSIZE, 0);
	ret = recv(players[b].connfd, recv_msg, BUFSIZE, 0);

	memset(send_msg, '\0', BUFSIZE);
	sprintf(send_msg, "%syou are 'O'\n", print_b);
	send(players[b].connfd, send_msg, BUFSIZE, 0);
	memset(send_msg, '\0', BUFSIZE);
	sprintf(send_msg, "%syou are 'X'\n" , print_b);
	send(players[a].connfd, send_msg, BUFSIZE, 0);
	
	while(1){
		char choose[] = "\nChoose from 1 to 9> ";
		if(turn == 'b'){
			send(players[b].connfd ,choose, sizeof(choose), 0);
			ret = recv(players[b].connfd, recv_msg, BUFSIZE, 0);
			int place;
			sscanf(recv_msg ,"%d" ,&place);

			if(board[place - 1] != ' '){
				char msg[] = "Choose again\n> ";
				send(players[b].connfd ,msg ,sizeof(msg) ,0);
				continue;
			}
			board[place - 1] = 'O';
			print_board(print_b , board);
			
			if(isWin(board)){
				memset(send_msg, '\0', BUFSIZE);
				sprintf(send_msg ,"%s\ngame over\nYou win!\n" ,print_b);				
				send(players[b].connfd, send_msg, sizeof(send_msg), 0);
				memset(send_msg, '\0', BUFSIZE);
				sprintf(send_msg ,"%s\ngame over\nYou lose\n" ,print_b);
				send(players[a].connfd ,send_msg, sizeof(send_msg), 0);

				break;
			}
			if(!b_isEmpty(board)){
				memset(send_msg, '\0', BUFSIZE);
				sprintf(send_msg, "%s\ngame over\nTie\n", print_b);
				send(players[b].connfd, send_msg, sizeof(send_msg), 0);
				memset(send_msg, '\0', BUFSIZE);
				sprintf(send_msg, "%s\ngame over\nTie\n", print_b);
				send(players[a].connfd, send_msg, sizeof(send_msg), 0);
				break;
			}

			send(players[b].connfd, print_b, sizeof(print_b), 0);
			send(players[a].connfd, print_b, sizeof(print_b), 0);
			turn = 'a';
		}

		else{
			send(players[b].connfd ,choose, sizeof(choose), 0);
			ret = recv(players[b].connfd, recv_msg, BUFSIZE, 0);
			int place;
			sscanf(recv_msg ,"%d" ,&place);

			if(board[place - 1] != ' '){
				char msg[] = "You can't place here\nChoose again\n> ";
				send(players[b].connfd ,msg ,sizeof(msg) ,0);
				continue;
			}
			board[place - 1] = 'O';
			print_board(print_b , board);
			
			if(isWin(board)){
				memset(send_msg, '\0', BUFSIZE);
				sprintf(send_msg ,"%s\ngame over\nYou win!\n" ,print_b);				
				send(players[b].connfd, send_msg, sizeof(send_msg), 0);
				memset(send_msg, '\0', BUFSIZE);
				sprintf(send_msg ,"%s\ngame over\nYou lose\n" ,print_b);
				send(players[a].connfd ,send_msg, sizeof(send_msg), 0);

				break;
			}
			if(!b_isEmpty(board)){
				memset(send_msg, '\0', BUFSIZE);
				sprintf(send_msg, "%s\ngame over\nTie\n", print_b);
				send(players[b].connfd, send_msg, sizeof(send_msg), 0);
				memset(send_msg, '\0', BUFSIZE);
				sprintf(send_msg, "%s\ngame over\nTie\n", print_b);
				send(players[a].connfd, send_msg, sizeof(send_msg), 0);
				break;
			}

			send(players[b].connfd, print_b, sizeof(print_b), 0);
			send(players[a].connfd, print_b, sizeof(print_b), 0);
			turn = 'a';
		}
	}
	send(players[a].connfd, "> ", 2, 0);
	send(players[b].connfd, "> ", 2, 0);
}
int isWin(char board[]){
	if((board[0] != ' ') && (board[0] == board[1]) && (board[1] == board[2]))
		return 1;
	if((board[3] != ' ') && (board[3] == board[4]) && (board[4] == board[5]))                                                                        
	   	return 1;
	if((board[6] != ' ') && (board[6] == board[7]) && (board[7] == board[8]))
        return 1;
	if((board[0] != ' ') && (board[0] == board[3]) && (board[3] == board[6]))
        return 1;
	if((board[1] != ' ') && (board[1] == board[4]) && (board[4] == board[7]))
        return 1;
	if((board[2] != ' ') && (board[2] == board[5]) && (board[5] == board[8]))
        return 1;
	if((board[0] != ' ') && (board[0] == board[4]) && (board[4] == board[8]))
        return 1;
	if((board[2] != ' ') && (board[2] == board[4]) && (board[4] == board[6]))
        return 1;

	return 0;
}
int b_isEmpty(char *b){
	for(int i = 0;i < 10;i++){
		if(b[i] == ' ')
			return 1;
	}
	return 0;
}

void print_board(char b[], char board[]){
	memset(b, '\0', BUFSIZE);
	char tmp[BUFSIZE];
	strcat(b, "\n");	
	strcat(b, "┌───┬───┬───┐\t┌───┬───┬───┐\n");   
	sprintf(tmp, "│ 1 │ 2 │ 3 │\t│ %c │ %c │ %c │\n", board[0], board[1], board[2]);   
	strcat(b, tmp);
	strcat(b, "├───┼───┼───┤\t├───┼───┼───┤\n");   
	sprintf(tmp, "│ 4 │ 5 │ 6 │\t│ %c │ %c │ %c │\n", board[3], board[4], board[5]);  
    strcat(b, tmp);
	strcat(b, "├───┼───┼───┤\t├───┼───┼───┤\n"); 
    sprintf(tmp, "│ 7 │ 8 │ 9 │\t│ %c │ %c │ %c │\n", board[6], board[7], board[8]);  
	strcat(b, tmp);
	strcat(b, "└───┴───┴───┘\t└───┴───┴───┘\n\n");
}
