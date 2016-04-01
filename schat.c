#include <sys/time.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
#include <time.h>

/* Terminal colors definition */
#define ANSI_COLOR_RED     "\x1b[31m"
#define ANSI_COLOR_GREEN   "\x1b[32m"
#define ANSI_COLOR_YELLOW  "\x1b[33m"
#define ANSI_COLOR_BLUE    "\x1b[34m"
#define ANSI_COLOR_MAGENTA "\x1b[35m"
#define ANSI_COLOR_CYAN    "\x1b[36m"
#define ANSI_COLOR_WHITE   "\x1B[37m"
#define ANSI_COLOR_RESET   "\x1b[0m"

/* Name of file where hashtable is located */
#define HASHTABLE "hashtable.txt"

/* Structure where we list valid commands and description, to be used by function help() */
struct command_d{
	char* cmd_name;
	char* cmd_help;
}
const commands[] = {
	{"join", "Joins the proper name server "},
	{"leave", "Unregister and leave the proper name server"},
	{"connect", "Connect to a peer"},
	{"disconnect", "Disconnect from a peer"},
	{"message", "Used to send a message to a connected peer"},
	{"history","Shows old connections, enables quick reconnection and prints conversation history"},
	{"exit","Exits the program cleanly"},
	{"help","Prints a rudimentary help"}
};

int ncmd = sizeof(commands)/sizeof(commands[0]); /* Number of commands */

#define max(A,B) ((A)>=(B)?(A):(B))

typedef int bool;
#define true 1
#define false 0

/* Program States */
#define init 0 /* Initial State */
#define registered 1
#define onChat_received 2
#define onChat_sent 3
#define onChat_authenticating_step_1 4 
#define onChat_authenticating_step_2 5
#define onChat_authenticating_step_3 6

int stateMachine = init;

int surDir_sock;

struct stat st = {0}; /* Sincerely, why do I need this? */

/* Function: help
*  -------------------
*  Displays a list of all commands available and a description
* 
*  returns: 0
*/
int help() {
	int i;

	printf(ANSI_COLOR_BLUE "HELP - Here are the commands available:\n");
	printf(ANSI_COLOR_BLUE "//------------------------------------------------\\\\");
	printf(ANSI_COLOR_WHITE "\n");

	for(i = 0; i < ncmd; i++) {
		printf("\t %s - %s\n",commands[i].cmd_name,commands[i].cmd_help);
	}
	printf(ANSI_COLOR_BLUE "\\\\------------------------------------------------//");
	printf(ANSI_COLOR_WHITE "\n");
	return 0;
}

char* get_time(){
	time_t timer;
	char buffer[26];
	char* timestring;
	struct tm* tm_info;

	time(&timer);
	tm_info = localtime(&timer);

	strftime(buffer, 26, "%Y:%m:%d %H:%M:%S", tm_info);

	timestring = malloc(26*sizeof(char) + 1);
	sprintf(timestring,"%s",buffer);

	return timestring;
}

int print2history(char* msg, char* friend_name, char* full_name, int sent){
	FILE* historyfile = NULL;
	char* hfilename;
	char buffer[512];

	hfilename = malloc((22+strlen(friend_name)+strlen(full_name))*sizeof(char)+1);
	sprintf(hfilename,"./conversations/%s.%s.conv",friend_name,full_name);
	historyfile = fopen(hfilename,"a");
	
	if(historyfile != NULL){
		if(sent == 0) {
			fprintf(historyfile,"<%s>%s: %s\n",get_time(),friend_name,msg);
			sprintf(buffer,"<%s>%s: %s\n",get_time(),friend_name,msg);
		}else{
			fprintf(historyfile,"<%s>%s: %s\n",get_time(),full_name,msg);
			sprintf(buffer,"<%s>%s: %s\n",get_time(),full_name,msg);
		}
		fclose(historyfile);
		printf("print2history: printed %s to file %s\n",buffer,hfilename);
		return 0;
	}else { 
		printf(ANSI_COLOR_RED "The following message was not stored");
		printf(ANSI_COLOR_WHITE "\n");
		return -1;
	}
}

/*
 * Function:  show_usage
 * --------------------
 * Show the correct usage of the program command
 *
 *  returns:  -1 always (when this runs, it means something about the user's entered arguments went wrong)
 */
int show_usage(){
	printf("Usage: schat –n name.surname –i ip -p scport -s snpip -q snpport\n");
	return -1;
}

/* Function: encrypt
 * --------------------
 * Encrypt byte according to table on hashTable
 *
 * Parameters: int c -> byte to encrypt
 *
 * returns: encrypted byte if successful
 *          empty char if error ocurred
 */
bool encrypt(unsigned int* encrypted, unsigned int c) {
	FILE *table;
	char buffer[512];
	int i;

	table = fopen(HASHTABLE,"r");

	if(table == NULL) {
		printf(ANSI_COLOR_RED "encrypt: error: %s",strerror(errno));
		printf(ANSI_COLOR_WHITE "\n");
		printf("lel");
		encrypted = NULL;
		return false;
	}

	for(i = 0;i < c+1 ; i++){
		fgets(buffer,512,table);
	}

	if(sscanf(buffer,"%u",encrypted) != 1) {
		printf(ANSI_COLOR_RED "encrypt: error parsing file");
		printf(ANSI_COLOR_WHITE "\n");
		encrypted = NULL;
		return false;
	}
		
	fclose(table);

	return true;
}

/* Function: comUDP
*  -------------------
*  Sends a UDP message and waits for an answer
* 
*  Parameters: char* msg      -> message to be sent
*  			   char* dst_ip   -> ip of the destination of the message
*              char* dst_port -> port of the destination of the message
*
*  returns: Reply in char* format
*           NULL if error ocurred
*/
char * comUDP(char * msg, char * dst_ip, char * dst_port){
	struct sockaddr_in addr;
	struct in_addr *a;
	int n, addrlen;
	struct in_addr temp;
	struct hostent* h;
	char buffer[512];
	char * answer;
	struct timeval tv;

	/* Open UDP socket for our server */
	if((surDir_sock=socket(AF_INET,SOCK_DGRAM,0))==-1) {
		printf("UDP error: %s\n",strerror(errno));
		return NULL;
	}	
	
	inet_pton(AF_INET, dst_ip, &temp);
	if((h=gethostbyaddr(&temp,sizeof(temp),AF_INET)) == NULL) {
		printf("UDP error: sending getting host information\n");
		close(surDir_sock);
		return NULL;
	}

	a=(struct in_addr*)h->h_addr_list[0];

	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr=*a;
	addr.sin_port=htons(atoi(dst_port));

	printf("UDP Sending: %s\n", msg);

	n=sendto(surDir_sock,msg,strlen(msg),0,(struct sockaddr*)&addr,sizeof(addr));
	if(n==-1) {
		printf("UDP error: sending message to the server\n");
		close(surDir_sock);
		return NULL;
	}

	/* Setting UDP message timeout */
	tv.tv_sec = 5;
	tv.tv_usec = 0;

	if (setsockopt(surDir_sock, SOL_SOCKET, SO_RCVTIMEO,&tv,sizeof(tv)) < 0) {
    	perror("Error");
	}

	addrlen = sizeof(addr);
	n=recvfrom(surDir_sock,buffer,512,0,(struct sockaddr*)&addr,(socklen_t*)&addrlen);

	if(n==-1) {
		printf("UDP error: receiving message to the server\n");
		close(surDir_sock);
		return NULL;
	}

	answer=malloc(n);
	sprintf(answer,"%.*s",n,buffer);

	printf("Raw answer: %s\n",answer);

	close(surDir_sock);

	return answer;
}

/* Function: usrRegister
*  -------------------
*  Registers a user
* 
*  Parameters: char* snpip    -> pointer to string with proper name server's ip address in format xxx.xxx.xxx.xxx
*  			   char* port     -> pointer to a string with proper name server's port number
*              char* fullname -> pointer to a string with full name of user in format name.surname
*			   char* my_ip    -> pointer to a string with user ip address in format xxx.xxx.xxx.xxx
*              char* my_port  -> pointer to a string with user port number
*
*  returns: true if registration was successful
*           false if error ocurred
*/
bool usrRegister(char * snpip, char * port, char * full_name, char * my_ip, char * my_port){
	char * msg;
	char buffer[512];
	char * answer;

	sprintf(buffer,"REG %s;%s;%s",full_name,my_ip,my_port);

	msg = malloc(strlen(buffer));
	msg = buffer;

	answer = comUDP(msg, snpip, port);

	if (answer == NULL){
		printf("UDP error: empty message received\n");
		close(surDir_sock);
		return false;
	}

	if(strcmp(answer,"OK") != 0) {
		printf("Error: %s\n",answer);
		free(answer);
		return false;
	}

	printf("User registration successful!\n");
	free(answer);
	return true;
}

/* Function: usrExit
*  -------------------
*  Unregisters the user from the proper name server
* 
*  Parameters: char* snpip    -> pointer to string with proper name server's ip address in format xxx.xxx.xxx.xxx
*  			   char* port     -> pointer to a string with proper name server's port number
*              char* fullname -> pointer to a string with full name of user in format name.surname
*
*  returns: true if unregistration was successful
*           false if error ocurred
*/
bool usrExit(char * snpip, char * port, char * full_name){
	char buffer[512];
	char *answer;
	char * msg;

	sprintf(buffer,"UNR %s",full_name);

	msg = malloc(strlen(buffer));
	msg = buffer;

	answer = comUDP(msg, snpip, port);

	if (answer == NULL){
		printf("UDP error: empty message received\n");
		close(surDir_sock);
		return false;
	}

	if(strcmp(answer,"OK") != 0) {
		printf("Error: %s\n",answer);
		return false;
	}

	printf("User unregistered successful!\n");

	return true;
}

/* Function: queryUser
*  -------------------
*  Queries proper name server about the location of a certain user
* 
*  Parameters: char* snpip -> pointer to string with proper name server's ip address in format xxx.xxx.xxx.xxx
*  			   char* port  -> pointer to a string with proper name server's port number
*              char* user  -> pointer to a string with user's name to query about in format name.fullname
*
*  returns: string with user's location if query was successful
*           NULL if user was not found or an error ocurred
*/
char * queryUser(char * snpip, char * port, char * user){
	char buffer[512], garb_0[512];
	char *answer;
	char * msg;
	char * location;

	sprintf(buffer,"QRY %s",user);

	msg = malloc(strlen(buffer));
	msg = buffer;

	answer = comUDP(msg, snpip, port);

	if (answer == NULL){
		printf("UDP error: empty message received\n");
		close(surDir_sock);
		return NULL;
	}

	if(strstr(answer,"NOK") != NULL) {
		printf("Error: %s\n",answer);
		return NULL;
	}

	memset(buffer, 0, sizeof(buffer));

	sscanf(answer,"RPL %[^';'];%s", garb_0, buffer);

	location = malloc(strlen(buffer));

	location = buffer;

	return location;
}

/*
 * Function:  main
 * --------------------
 * Main program yo, dis' the server
 *
 * Options: -n -> name of user of our application in format name.surname
 *          -i -> ip address of machine running our application in format xxx.xxx.xxx.xxx
 *			-p -> port number where our schat will communicate
 *          -s -> ip address of proper name server in format xxx.xxx.xxx.xxx
 *          -q -> port number of proper name server
 *            
 *  returns:  0 if terminated cleanly
 *           -1 any other case
 */

int main(int argc, char* argv[]) {
	char usrIn[100];
	char options[10] = "n:s:q:i:p:";
	char *in_name_surname = NULL ,*in_ip = NULL ,*in_scport = NULL,*in_snpip = NULL,*in_snpport = NULL;
	char c;
	char buffer[512];
	char * name2connect;
	char * location;

	int fd = NULL, addrlen, newfd = NULL;
	struct sockaddr_in addr;
	int n, nw;

	int fd_out = NULL;
	struct sockaddr_in addr_out;
	struct in_addr temp;
	struct hostent* h;
	struct in_addr *a;

	fd_set rfds;
	int maxfd, counter;

	char remote_port [24];
	char remote_ip [24];
	char friend_name [128];

	int randNum;
	unsigned int randChar,recvChar;
	unsigned int *encrypted = NULL;

	int yes=1; /* YESSSS */

	int save_history = 0;

	srand(time(NULL)); /* initializing pseudo-random number generator */

	/*Check and retrieve given arguments	*/
	while((c=getopt(argc,argv,options)) != -1) {
		switch(c)
		{
		    case 'n':
			in_name_surname = optarg;
			break;
		    case 's':
			in_snpip = optarg;
			break;
		    case 'q':
			in_snpport = optarg;
			break;
		    case 'i':
			in_ip = optarg;
			break;
		    case 'p':
			in_scport = optarg;
			break;
		    case '?':
			if (optopt == 'n' || optopt == 's' || optopt == 'q' || optopt == 'i' || optopt == 'p'){
				printf("Option -%c requires an argument.\n",optopt);
			}
			else if (isprint(optopt)) {
				printf("Unknown option -%c.\n",optopt);
			}
			else{
				printf("Unknown option character `\\x%x`.\n",optopt);
			}
			return -1;
		     default:
			show_usage();
			return -1;
		}
	}
	/* Check if required arguments are given   */
	if(in_name_surname == NULL || in_snpip == NULL || in_snpport == NULL || in_scport == NULL || in_ip == NULL) {
		show_usage();
		return -1;
	}

	printf("Connection Data:\nLocalhost:%s:%s\nSurname Server:%s:%s\n",in_ip,in_scport,in_snpip,in_snpport);

	if((fd=socket(AF_INET,SOCK_STREAM,0)) == -1){
		exit(1);
	}

	/* DEIXAMOS ISTO? É A MELHOR FORMA DE RESOLVER O PROBLEMA?? E O SIGPIPE???
	 avoid the "Address already in use" error message immediately after closing socket  */
	if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) { 
		perror("setsockopt"); 
		exit(1); 
	}
	
	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	addr.sin_port=htons(atoi(in_scport));

	if(bind(fd,(struct sockaddr*)&addr,sizeof(addr))==-1){
		printf("ERROR: Cannot bind.\n");
		exit(1);
	}

	if(listen(fd,5)==-1){
		printf("ERRO\n");
		exit(1);
	}

	/* Creating directory for storing user conversation history */
	if (stat("./conversations", &st) == -1) {
    	if(mkdir("./conversations", 0700) == -1) {
    		printf(ANSI_COLOR_RED "Error creating conversation folder - it will not be possible to store conversation history");
    		printf(ANSI_COLOR_WHITE "\n");
    		save_history = 1;
    	}
	}

	save_history = 1;
	printf("Let's start..\n");

	while(1){
		FD_ZERO(&rfds);
		FD_SET(fileno(stdin),&rfds);
		FD_SET(fd,&rfds);
		FD_SET(newfd,&rfds);
		FD_SET(fd_out,&rfds);
		
		maxfd = fileno(stdin);
		maxfd = max(maxfd,fd);
		maxfd = max(maxfd,newfd);
		maxfd = max(maxfd,fd_out);

 		memset(usrIn, 0, sizeof(usrIn));

		counter=select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);

 		if(counter == -1){
 			printf("ERROR-: %s\n",strerror(errno));
 			exit(1);
 		}


 		if(FD_ISSET(fileno(stdin),&rfds)){
			fgets(usrIn,100,stdin);

			if (strcmp(usrIn,"join\n") == 0 && stateMachine == init){
				if(usrRegister(in_snpip, in_snpport,in_name_surname,in_ip,in_scport)){
					stateMachine = registered;
				}else{
					printf(ANSI_COLOR_RED "Registration failed :(");
					printf(ANSI_COLOR_WHITE "\n");
				}
				/* close(fd); exit(0); */

			}else if(strcmp(usrIn,"leave\n") == 0 && stateMachine != init){
				usrExit(in_snpip, in_snpport,in_name_surname);
				
				shutdown(fd_out, SHUT_RDWR);
				shutdown(newfd, SHUT_RDWR);

				stateMachine = init;

			}else if(strstr(usrIn,"find") != NULL && stateMachine != init){

				memset(buffer, 0, sizeof(buffer));

				sscanf(usrIn,"find %s", buffer);

				name2connect = malloc(strlen(buffer));

				strcpy(name2connect,buffer);

				if (strlen(name2connect) <= 1){
					printf("Argument missing. Who do you want to talk to?\n");
				}else{
					location = queryUser(in_snpip, in_snpport, name2connect);

					if (location == NULL){
						printf("Ups.. User not found.\n");
					}else{
						printf("User located at: %s\n", location);
					}
				}

			}else if(strstr(usrIn,"connect") != NULL && strcmp(usrIn,"disconnect\n") != 0 && stateMachine != init){

				memset(buffer, 0, sizeof(buffer));

				sscanf(usrIn,"connect %s", buffer);

				name2connect = malloc(strlen(buffer));
				strcpy(name2connect,buffer);

				if (strcmp(name2connect,in_name_surname) == 0){
					printf("Ahah are you ok? Do you really want to talk to yourself?\n");
				}else{
					if (strlen(name2connect) <= 1){
						printf("Argument missing. Who do you want to talk to?\n");
					}else{
						location = queryUser(in_snpip, in_snpport, name2connect);

						if (location == NULL){
							printf("Ups.. User not found.\n");
						}else{
							printf("User located at: %s\n", location);
							
							printf("Trying to connect..\n");

							strcpy(buffer,location);

							sscanf(buffer,"%[^';'];%s",remote_ip, remote_port);

							fd_out=socket(AF_INET,SOCK_STREAM,0); /* TCP socket */

							if(fd_out==-1){
								printf("FAIL\n");
								return -1;
							}

							inet_pton(AF_INET, remote_ip, &temp);
							h=gethostbyaddr(&temp,sizeof(temp),AF_INET);
							a=(struct in_addr*)h->h_addr_list[0];

							memset((void*)&addr_out,(int)'\0',sizeof(addr_out));

							addr_out.sin_family=AF_INET;
							addr_out.sin_addr=*a;
							addr_out.sin_port=htons(atoi(remote_port));

							n=connect(fd_out,(struct sockaddr*)&addr_out,sizeof(addr_out));

							if (n == -1){
								printf("ERROR connecting\n");
								return -1;
							}

							nw = write(fd_out,in_name_surname,strlen(in_name_surname));

							if (nw <= 1){
								printf("Could not send message.\n");
							}

							printf(ANSI_COLOR_GREEN "\nConnected with %s !!\n" ANSI_COLOR_RESET, name2connect);
							printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);

							stateMachine = onChat_authenticating_step_1; /* onChat_sent */ 
						}	
					}				
				}

			}else if(strstr(usrIn,"message") != NULL && (stateMachine == onChat_sent || stateMachine == onChat_received) && stateMachine != init){

				memset(buffer, 0, sizeof(buffer));

				sscanf(usrIn,"message %[^\n]", buffer);

				if (stateMachine == onChat_sent){

					nw = write(fd_out,buffer,strlen(buffer));

				}else if(stateMachine == onChat_received){
					nw = write(newfd,buffer,strlen(buffer));
				}

				if (nw <= 1){
					printf("Could not send message.\n");
				}

				memset(buffer, 0, sizeof(buffer));

			}else if(strcmp(usrIn,"disconnect\n") == 0 && (stateMachine == onChat_sent || stateMachine == onChat_received) && stateMachine != init){

				if (stateMachine == onChat_sent){

					shutdown(fd_out, SHUT_RDWR);

				}else if(stateMachine == onChat_received){
					
					shutdown(newfd, SHUT_RDWR);

				}

				stateMachine = registered;

			}else if(strcmp(usrIn,"exit\n") == 0){

				if(stateMachine != init){
					if(!usrExit(in_snpip, in_snpport,in_name_surname)) {
						printf(ANSI_COLOR_RED "Unregistration was not possible, exiting anyway...");
						printf(ANSI_COLOR_WHITE "\n");
					}
				}

				shutdown(fd_out, SHUT_RDWR);
				shutdown(newfd, SHUT_RDWR);
				shutdown(fd, SHUT_RDWR);

				close(newfd);
				close(fd_out);
				close(fd);

				newfd = NULL;
				fd = NULL;
				fd_out = NULL;

				printf("Exiting..\n");
				break;
			}else{
				if (strcmp(usrIn,"join\n") == 0){
					printf("You already joined the network.\n");

				}else if(strcmp(usrIn,"leave\n") == 0){
					printf("You have to be part of the network to leave it.\nPlease join, you are welcome.\n");

				}else if(strstr(usrIn,"find") != NULL){
					printf("You have to be part of the network to find someone.\nPlease join, you are welcome.\n");
				
				}else if(strstr(usrIn,"connect") != NULL && strcmp(usrIn,"disconnect\n") != 0){
					printf("You have to be part of the network to connect with someone.\nPlease join, you are welcome.\n");

				}else if(strstr(usrIn,"message") != NULL){
					if (stateMachine != onChat_sent && stateMachine != onChat_received  && stateMachine != init){
						printf("Ups.. you have to connect with someone before messaging him.\n");

					}else if(stateMachine == init){
						printf("You have to be part of the network to message someone.\nPlease join, you are welcome.\n");
					}
				}else if(strcmp(usrIn,"disconnect\n") == 0){
					if (stateMachine != onChat_sent && stateMachine != onChat_received && stateMachine != init){
						printf("You have to be messaging with someone to end the conversation.\n");

					}else if(stateMachine == init){
						printf("You have to be part of the network and messaging with someone to end the conversation.\nPlease join, you are welcome.\n");
					}
				}else if(strcmp(usrIn,"help\n") == 0){
					help();
				}else{
					printf("You are not allowed to do that my friend.\n");
				}
			}
		}

		if(FD_ISSET(fd,&rfds)){

			addrlen=sizeof(addr);

			if((newfd=accept(fd,(struct sockaddr*)&addr,(socklen_t*)&addrlen))==-1){
				printf("ERROR: Cannot start listening TCP.\n");
				exit(1); 
			}

			printf(ANSI_COLOR_GREEN "\nReceived incoming connection !!\n" ANSI_COLOR_RESET);
			printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);

			stateMachine = onChat_authenticating_step_1;

		}

		/* Incoming connection */

		if (newfd != NULL){

			if(FD_ISSET(newfd,&rfds)){
					
				memset(buffer, 0, sizeof(buffer));

				n=read(newfd,buffer,512);

				if(n == -1){
					printf("ERROR: Cannot read message\n");
				}else if(n == 0){
					printf(ANSI_COLOR_RED "Connection closed by your friend." ANSI_COLOR_RESET);
					printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);
				 	close(newfd);
				 	newfd = NULL;
				}else{

					if (stateMachine == onChat_received){

							if(save_history) print2history(buffer,friend_name,in_name_surname,0);

							printf(ANSI_COLOR_CYAN "%s: %s" ANSI_COLOR_RESET,friend_name, buffer);
							printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);
					
					}else if(stateMachine == onChat_authenticating_step_1){
						
						/* check if the name is well formatted */
						strcpy(friend_name,buffer);

						randNum = (rand() % 256) + 0;

						randChar = randNum;

						sprintf(buffer,"AUTH %d\n",randChar);

						nw = write(newfd,buffer,strlen(buffer));

						stateMachine = onChat_authenticating_step_2;

					}else if (stateMachine == onChat_authenticating_step_2){
						
						encrypted = malloc(sizeof(unsigned char));
						encrypt(encrypted, randChar);
						if(encrypted == NULL) {
							printf(ANSI_COLOR_RED "%s -> Not authorized",friend_name);
							printf(ANSI_COLOR_WHITE "\n");

							stateMachine = registered; 
						}

						sscanf(buffer,"AUTH %u",&recvChar);

						if(recvChar == *encrypted) {
							printf(ANSI_COLOR_GREEN "%s -> Authenticated !!" ANSI_COLOR_RESET, friend_name);
							printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);

							stateMachine = onChat_authenticating_step_3;
						}else{
							printf(ANSI_COLOR_RED "%s -> Not authorized." ANSI_COLOR_RESET, friend_name);
							printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);

							stateMachine = registered;
						}

					}else if (stateMachine == onChat_authenticating_step_3){

						sscanf(buffer,"AUTH %u",&randChar);

						encrypted = malloc(sizeof(unsigned char));
						encrypt(encrypted, randChar);

						printf("Received %d | encrypted %u\n", randChar, *encrypted);

						sprintf(buffer,"AUTH %u\n",*encrypted);
							
						nw = write(newfd,buffer,strlen(buffer));

						stateMachine = onChat_received;
						
					}
				}
			}
		}

		/* Outgoing connection */

		if (fd_out != NULL){
			if(FD_ISSET(fd_out,&rfds)){

				memset(buffer, 0, sizeof(buffer));

				n=read(fd_out,buffer,512);

				if(n == -1){
						printf("ERROR: Cannot read message\n");
					}else if(n == 0){
						printf(ANSI_COLOR_RED "Connection closed by your friend." ANSI_COLOR_RESET);
						printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);
					 	close(fd_out);
					 	fd_out = NULL;
					}else{
						if (stateMachine == onChat_sent){
							
							if(save_history) print2history(buffer,friend_name,in_name_surname,0);

							printf(ANSI_COLOR_CYAN "%s: %s" ANSI_COLOR_RESET, name2connect, buffer);
							printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);
										
						}else if (stateMachine == onChat_authenticating_step_1){

							sscanf(buffer,"AUTH %u",&randChar);

							encrypted = malloc(sizeof(unsigned char));
							encrypt(encrypted, randChar);

							printf("Received %d | encrypted %u\n", randChar, *encrypted);

							sprintf(buffer,"AUTH %u\n",*encrypted);
							
							nw = write(fd_out,buffer,strlen(buffer));

							randNum = (rand() % 256) + 0;

							randChar = randNum;

							sprintf(buffer,"AUTH %d\n",randChar);

							nw = write(fd_out,buffer,strlen(buffer));

							stateMachine = onChat_authenticating_step_2;

						}else if (stateMachine == onChat_authenticating_step_2){

							encrypted = malloc(sizeof(unsigned char));
							encrypt(encrypted, randChar);

							sscanf(buffer,"AUTH %u",&recvChar);

							if(recvChar == *encrypted) {
								printf(ANSI_COLOR_GREEN "%s -> Authenticated !!" ANSI_COLOR_RESET, name2connect);
								printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);

								stateMachine = onChat_sent;
							}else{
								printf(ANSI_COLOR_RED "%s -> Not authorized." ANSI_COLOR_RESET, name2connect);
								printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);

								stateMachine = registered;
							}
						}
					}
			}
		}
		
	}

	printf("\n\nThe End\n\n\n");

	return 0;	
}
