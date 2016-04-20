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
#include <signal.h>
#include "helper.h"
#include "communications.h"

/* Program States */

#define init 0 /* initial state, not registered on network. No network actions allowed */
#define registered 1 /* after join, can leave, find users and connect */
#define onChat_received 2 /* state for incoming connection */
#define onChat_sent 3 /* state for outgoing connection */
/* authentication steps for sequential message exchange */
#define onChat_authenticating_step_1 4
#define onChat_authenticating_step_2 5
#define onChat_authenticating_step_3 6

#define max(A,B) ((A)>=(B)?(A):(B)) // returns max value between A and B

typedef void (*sighandler_t)(int);
sighandler_t signal(int signum, sighandler_t handler);

struct command_d commands[] = {
	{"join", "Joins the proper name server "},
	{"leave", "Unregister and leave the proper name server"},
	{"connect", "Connect to a peer"},
	{"disconnect", "Disconnect from a peer"},
	{"message", "Used to send a message to a connected peer"},
	{"find", "Used to find the location a user"},
	{"exit","Exits the program cleanly"},
	{"help","Prints a rudimentary help"}
};

int ncmd = sizeof(commands)/sizeof(commands[0]); /* Number of commands */

/* Name of file where hashtable is located, can be updated during execution */
char * HASHTABLE = "hashtable.txt"; /* ------------ WHY, temos um hashtable e um hashfile, nao vale a pena inicializar -------------------------- */

int stateMachine = init; /* define initial state */

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
	char cipherFile[512];
	char * name2connect;
	char * location;

	int fd = NULL, addrlen, newfd = NULL;
	int refuse_fd = NULL;
	struct sockaddr_in addr;
	int n, nw, ny;

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
	unsigned char randChar,recvChar;
	unsigned char *encrypted = NULL;

	int yes=1; /* YESSSS */

	void (*old_handler)(int); //interrupt handler

	srand(time(NULL)); /* initializing pseudo-random number generator */

	/*Check and retrieve given arguments */
	while((c = getopt(argc,argv,options)) != -1) {
		switch(c){
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
			show_usage(0);
			return -1;
		}
	}

	/* Check if required arguments are given   */
	if(in_name_surname == NULL || in_snpip == NULL || in_snpport == NULL || in_scport == NULL || in_ip == NULL) {
		show_usage(0);
		return -1;
	}

	// show connection data for feedback
	printf("Connection Data:\nLocalhost:%s:%s\nSurname Server:%s:%s\n",in_ip,in_scport,in_snpip,in_snpport);

	// open main socket
	if((fd=socket(AF_INET,SOCK_STREAM,0)) == -1){
		printf("Cannot open main socket\n");
		exit(1);
	}

	// avoid the "Address already in use" error message immediately after closing socket
	if(setsockopt(fd,SOL_SOCKET,SO_REUSEADDR,&yes,sizeof(int)) == -1) { 
		perror("setsockopt"); 
		exit(1); 
	}
	
	// format address for binding port 
	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	addr.sin_port=htons(atoi(in_scport));

	// bind port to receive connections
	if(bind(fd,(struct sockaddr*)&addr,sizeof(addr))==-1){
		printf("ERROR: Cannot bind.\n");
		exit(1);
	}

	// start listening
	if(listen(fd,5)==-1){
		printf("ERRO\n");
		exit(1);
	}

	// From now on, the SIGPIPE signal will be ignored
	if((old_handler=signal(SIGPIPE,SIG_IGN))==SIG_ERR)exit(1); //error

	printf("All set! Let's start..\n");

	while(1){
		// initializing select function
		FD_ZERO(&rfds);
		FD_SET(fileno(stdin),&rfds);
		FD_SET(fd,&rfds);
		FD_SET(newfd,&rfds);
		FD_SET(fd_out,&rfds);
		
		// set maximum file descriptor
		maxfd = fileno(stdin);
		maxfd = max(maxfd,fd);
		maxfd = max(maxfd,newfd);
		maxfd = max(maxfd,fd_out);

		// clean user input variable
 		memset(usrIn, 0, sizeof(usrIn));

 		// select - blocks the code until some file descriptor has new data
		counter=select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);

		// check for select errors
 		if(counter == -1){
 			printf("ERROR-: %s\n",strerror(errno));
 			exit(1);
 		}

 		// execute if stdin has new data - user types something and enter
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


				// here, one can leave the network and, of course, the current conversation (if any)
				if(!usrExit(in_snpip, in_snpport,in_name_surname)) {
						printf(ANSI_COLOR_RED "Unregistration was not possible, leaving anyway...");
						printf(ANSI_COLOR_WHITE "\n");
					}
				
				// close any ongoing TCP connections 
				shutdown(fd_out, SHUT_RDWR);
				shutdown(newfd, SHUT_RDWR);

				// reset status, allowing the user to rejoin the network
				stateMachine = init;

			}else if(strstr(usrIn,"find") != NULL && stateMachine != init){

				// here, one can find other user on the network and get his connection data

				memset(buffer, 0, sizeof(buffer)); // clean buffer

				sscanf(usrIn,"find %s", buffer); // get user to search from stdin

				// buffer processing to get the user name and memory allocation
				name2connect = malloc(strlen(buffer) + 1);
				strcpy(name2connect,buffer); 

				if (strlen(name2connect) <= 1){
					// error check, no user written
					printf("Argument missing. Who do you want to talk to?\n");
				}else{
					// got username, let's search for him - call queryUser function
					location = queryUser(in_snpip, in_snpport, name2connect);

					free(name2connect);

					if (strcmp(location,"") == 0 || location == NULL){
						// error check
						printf("Ups.. User not found.\n");
					}else{
						// got user lcoation, give feedback
						printf("User located at: %s\n", location);
					}

					memset(location, 0, strlen(location)*sizeof(char)); // clean buffer
					free(location);
				}

			}else if(strstr(usrIn,"connect") != NULL && strcmp(usrIn,"disconnect\n") != 0 && stateMachine == registered){

				// here, one can establish connection with another user for chatting

				memset(buffer, 0, sizeof(buffer)); // clean buffer

				ny = sscanf(usrIn,"connect %s %s", buffer, cipherFile); // get command arguments

				if (ny > 2){
					printf ("Too many arguments to the connect function. Type help for instructions.\n");
				}else{

					if (ny == 2){
						// if we have 2 arguments, the user is specifying other key file than the default hashtable.txt

						// input error check
						if (strcmp(cipherFile,"") == 0 || cipherFile == NULL){
							printf("Using default cipher file: hashtable.txt\n");
						}else{
							// alocate and set hashtable global variable to user specific file
							HASHTABLE = malloc(strlen(cipherFile) + 1);
							strcpy(HASHTABLE,cipherFile);
							printf("Using your cipher file: %s\n", HASHTABLE);
						}

					}else if (ny == 1){
						// just one argument, just the name to connect, apply default hashtable file
						printf("Using default cipher file: hashtable.txt\n");
					}

					// alocate and set friend name variable
					name2connect = malloc(strlen(buffer) + 1);
					strcpy(name2connect,buffer);

					if (strcmp(name2connect,in_name_surname) == 0){
						// do not allow establishing connection with myself
						printf("Ahah are you ok? Do you really want to talk to yourself?\n");
					}else{
						if (strlen(name2connect) <= 1){
							printf("Argument missing. Who do you want to talk to?\n");
						}else{
							location = queryUser(in_snpip, in_snpport, name2connect);

							if (strcmp(location,"") == 0 || location == NULL){
								printf("Ups.. User not found.\n");

							}else{
								printf("User located at: %s\n", location);
								
								printf("Trying to connect..\n");

								strcpy(buffer,location);

								memset(location, 0, strlen(location)*sizeof(char));

								sscanf(buffer,"%[^';'];%s",remote_ip, remote_port);

								inet_pton(AF_INET, remote_ip, &temp);
								h=gethostbyaddr(&temp,sizeof(temp),AF_INET);

								if (h == NULL){
									printf("Bad address. Host not found.\n");
								}else{
									a=(struct in_addr*)h->h_addr_list[0];

									if (a == NULL){
										printf("Bad address. Host not found.\n");
									}else{

										fd_out=socket(AF_INET,SOCK_STREAM,0); /* TCP socket */

										if(fd_out==-1){
											printf("FAIL\n");
											return -1;
										}

										memset((void*)&addr_out,(int)'\0',sizeof(addr_out));

										addr_out.sin_family=AF_INET;
										addr_out.sin_addr=*a;
										addr_out.sin_port=htons(atoi(remote_port));

										n=connect(fd_out,(struct sockaddr*)&addr_out,sizeof(addr_out));

										if (n == -1){
											printf("ERROR connecting\n");
											return -1;
										}

										sprintf(buffer,"NAME %s",in_name_surname);

										nw = write(fd_out,buffer,strlen(buffer));

										if (nw <= 1){
											printf("Could not send message.\n");
										}

										printf(ANSI_COLOR_GREEN "\nConnected with %s !!\n" ANSI_COLOR_RESET, name2connect);
										printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);

										stateMachine = onChat_authenticating_step_1; /* onChat_sent */ 
									}
								}
							}	
						}				
					}
				}

			}else if(strstr(usrIn,"message") != NULL && (stateMachine == onChat_sent || stateMachine == onChat_received) && stateMachine != init){

				// here, some message is sent to someone

				memset(buffer, 0, sizeof(buffer)); // clean buffer

				sscanf(usrIn,"message %[^\n]", buffer); // get message to send

				// send message to outgoing connection
				if (stateMachine == onChat_sent){

					nw = write(fd_out,buffer,strlen(buffer));

				}else if(stateMachine == onChat_received){
					// send message to incoming connection
					nw = write(newfd,buffer,strlen(buffer));
				}

				// error check
				if (nw <= 1){
					printf("ERROR: Could not send message.\n");
				}

				memset(buffer, 0, sizeof(buffer)); // clean buffer again

			}else if(strcmp(usrIn,"disconnect\n") == 0 && (stateMachine == onChat_sent || stateMachine == onChat_received) && stateMachine != init){
				// end conversation with someone

				// shutdown outgoing connection
				if (stateMachine == onChat_sent){
					shutdown(fd_out, SHUT_RDWR);

				}else if(stateMachine == onChat_received){
					// shutdown incomming connection
					shutdown(newfd, SHUT_RDWR);

				}

				// update status to allow new connections
				stateMachine = registered;

			}else if(strcmp(usrIn,"exit\n") == 0){

				if(stateMachine == onChat_sent){
					free(name2connect);
					free(location);
				}

				// leave network (unregister from snp) if already registered
				if(stateMachine != init){
					// unregister and check for error
					if(!usrExit(in_snpip, in_snpport,in_name_surname)) {
						printf(ANSI_COLOR_RED "Unregistration was not possible, exiting anyway...");
						printf(ANSI_COLOR_WHITE "\n");
					}
				}

				// shutdown TCP connections
				shutdown(fd_out, SHUT_RDWR);
				shutdown(newfd, SHUT_RDWR);
				shutdown(fd, SHUT_RDWR);

				// close respective sockets
				close(newfd);
				close(fd_out);
				close(fd);

				// set file descriptors to NULL just to be sure
				newfd = NULL;
				fd = NULL;
				fd_out = NULL;

				printf("Exiting..\n");
				break;
			}else{

				// print error messages for user feedback

				if (strcmp(usrIn,"join\n") == 0){
					printf("You already joined the network.\n");

				}else if(strcmp(usrIn,"leave\n") == 0){
					printf("You have to be part of the network to leave it.\nPlease join, you are welcome.\n");

				}else if(strstr(usrIn,"find") != NULL){
					printf("You have to be part of the network to find someone.\nPlease join, you are welcome.\n");
				
				}else if(strstr(usrIn,"connect") != NULL && strcmp(usrIn,"disconnect\n") != 0 && stateMachine == init){
					printf("You have to be part of the network to connect with someone.\nPlease join, you are welcome.\n");

				}else if(strstr(usrIn,"connect") != NULL && strcmp(usrIn,"disconnect\n") != 0 && stateMachine != init){
					printf("You cannot begin a new connection while chatting with someone. Please end the conversation.\n");

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
					help(commands,ncmd);
				}else{
					printf("You are not allowed to do that my friend.\n");
				}
			}
		}

		// execute if main socket buffer has data
		// here, new TCP connections to the main socket are processed
		if(FD_ISSET(fd,&rfds)){

			addrlen=sizeof(addr);

			// if not connected with anyone -> accept connection
			if (stateMachine == registered){

				if((newfd=accept(fd,(struct sockaddr*)&addr,(socklen_t*)&addrlen))==-1){
					printf("ERROR: Cannot start listening TCP.\n");
					exit(1); 
				}

				printf(ANSI_COLOR_GREEN "\nReceived incoming connection !!\n" ANSI_COLOR_RESET);
				printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);

				stateMachine = onChat_authenticating_step_1;
			}else{
				// if connected with someone and other user tries to connect
				// accept connection throught a temporary fd

				if((refuse_fd=accept(fd,(struct sockaddr*)&addr,(socklen_t*)&addrlen))==-1){
					printf("ERROR: Cannot refuse TCP.\n");
					exit(1); 
				}

				// send busy line error message
				sprintf(buffer,"Busy line. Connection Refused");
				nw = write(refuse_fd,buffer,strlen(buffer));

				// disconnect this new connection
				shutdown(refuse_fd, SHUT_RDWR);

				printf("Refused incoming connection because you are chatting with someone.\n");
			}

		}

		// execute if Incoming connection socket buffer has data
		if (newfd != NULL){

			if(FD_ISSET(newfd,&rfds)){
					
				memset(buffer, 0, sizeof(buffer));

				n=read(newfd,buffer,512);

				if(n == -1){
					printf("ERROR: Cannot read message\n");
				}else if(n == 0){
					printf(ANSI_COLOR_RED "Connection closed." ANSI_COLOR_RESET);
					printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);
				 	close(newfd);
				 	newfd = NULL;

				 	if (stateMachine != init){
				 		stateMachine = registered;
				 	}
				}else{

					if (stateMachine == onChat_received){

						printf(ANSI_COLOR_CYAN "%s: %s" ANSI_COLOR_RESET,friend_name, buffer);
						printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);
					
					}else if(stateMachine == onChat_authenticating_step_1){
						
						/* check if the name is well formatted */
						
						sscanf(buffer,"NAME %s\n",friend_name);

						randNum = (rand() % 256) + 0;

						randChar = randNum;

						sprintf(buffer,"AUTH %c",(unsigned char)randChar);

						nw = write(newfd,buffer,strlen(buffer));

						stateMachine = onChat_authenticating_step_2;

					}else if (stateMachine == onChat_authenticating_step_2){
						
						encrypted = malloc(sizeof(unsigned char));
						encrypt(encrypted, randChar, HASHTABLE);

						if(encrypted == NULL) {
							printf(ANSI_COLOR_RED "%s -> Not authorized",friend_name);
							printf(ANSI_COLOR_WHITE "\n");

							stateMachine = registered; 
						}

						sscanf(buffer,"AUTH %c",&recvChar);
						
						if(recvChar == *encrypted) {
							printf(ANSI_COLOR_GREEN "%s -> Authenticated !!" ANSI_COLOR_RESET, friend_name);
							printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);

							stateMachine = onChat_authenticating_step_3;
						}else{
							printf(ANSI_COLOR_RED "%s -> Not authorized." ANSI_COLOR_RESET, friend_name);
							printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);

							shutdown(newfd, SHUT_RDWR);

							stateMachine = registered;
						}

						free(encrypted);


					}else if (stateMachine == onChat_authenticating_step_3){

						sscanf(buffer,"AUTH %c",&randChar);

						encrypted = malloc(sizeof(unsigned char));
						encrypt(encrypted, randChar, HASHTABLE);

						sprintf(buffer,"AUTH %c",*encrypted);
							
						nw = write(newfd,buffer,strlen(buffer));

						stateMachine = onChat_received;

						free(encrypted);
						
					}
				}
			}
		}

		// execute if outgoing connection socket buffer has data
		if (fd_out != NULL){
			if(FD_ISSET(fd_out,&rfds)){

				memset(buffer, 0, sizeof(buffer));

				n=read(fd_out,buffer,512);

				if(n == -1){
						printf("ERROR: Cannot read message\n");
					}else if(n == 0){
						printf(ANSI_COLOR_RED "Connection closed." ANSI_COLOR_RESET);
						printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);
					 	close(fd_out);
					 	fd_out = NULL;

					 	free(name2connect);
						free(location);

					 	if (stateMachine != init){
				 			stateMachine = registered;
				 		}
					}else{
						if (stateMachine == onChat_sent){

							printf(ANSI_COLOR_CYAN "%s: %s" ANSI_COLOR_RESET, name2connect, buffer);
							printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);
										
						}else if (stateMachine == onChat_authenticating_step_1){

							ny = sscanf(buffer,"AUTH %c",&randChar);

							if (ny != 1){
								printf("Out of Protocol: %s\n", buffer);

								stateMachine = registered;

							}else{
								encrypted = malloc(sizeof(unsigned char));

								encrypt(encrypted, randChar,HASHTABLE);

								sprintf(buffer,"AUTH %c",*encrypted);
								
								nw = write(fd_out,buffer,strlen(buffer));

								randNum = (rand() % 256) + 0;

								randChar = randNum;

								sprintf(buffer,"AUTH %c",randChar);

								nw = write(fd_out,buffer,strlen(buffer));

								free(encrypted);

								stateMachine = onChat_authenticating_step_2;
							}

						}else if (stateMachine == onChat_authenticating_step_2){

							encrypted = malloc(sizeof(unsigned char));
							encrypt(encrypted, randChar, HASHTABLE);

							sscanf(buffer,"AUTH %c",&recvChar);

							if(recvChar == *encrypted) {
								printf(ANSI_COLOR_GREEN "%s -> Authenticated !!" ANSI_COLOR_RESET, name2connect);
								printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);

								stateMachine = onChat_sent;
							}else{
								printf(ANSI_COLOR_RED "%s -> Not authorized." ANSI_COLOR_RESET, name2connect);
								printf(ANSI_COLOR_WHITE "\n" ANSI_COLOR_RESET);

								shutdown(fd_out, SHUT_RDWR);

								stateMachine = registered;
							}

							free(encrypted);
						}
					}
			}
		}
		
	}

	printf("\n\nThe End\n\n\n");

	return 0;	
}
