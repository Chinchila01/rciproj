#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "helper.h"
#include "communications.h"

/* Appended to file used to store registered clients */
#define SRVFILE "clientlist"

struct command_d commands[] = {
	{"list", "Lists users currently registered with the server"},
	{"exit", "Exits the program cleanly"},
	{"help", "Prints a rudimentary help"}
};

int ncmd = sizeof(commands)/sizeof(commands[0]); /* Number of commands */
char* servername; /* Surname served by this server */
char* serverfile; /* file name of the server, used to store the registered clients' list */

/*
 * Function:  main
 * --------------------
 * Main program - where our proper name server starts
 *
 *  Options: -n -> surname of proper name server
 *           -s -> ip address of the machine where proper name server is hosted in format xxx.xxx.xxx.xxx
 *           -q -> port number where the proper name server is going to listen
 *           -i -> [Optional] ip address of surname server in format xxx.xxx.xxx.xxx
 *           -p -> [Optional] port number of surname server
 *
 *  returns:  0 if terminated cleanly
 *           -1 any other case
 */
int main(int argc, char* argv[]) {
	int stop = 0; /* Used in the while loop so the program exits cleanly */
	/* Variables related with UDP communication */
	struct hostent *h; 	
	int len,addrlen, ret, nread, port;
	struct sockaddr_in addr;
	char buffer[128];
	char *answer;
	char *reply;
	/*Variables related with standard input */
	char localinput[512];
	/* Variables related to command line arguments */
	char c;
	char options[10] = "n:s:q:i:p:"; /* Required and optional arguments for main (used in getopt) */
	char *surname = NULL ,*snpip = NULL ,*snpport = NULL,*saip = NULL,*saport = NULL;
	/* File descriptors, pointer */
	int fd;
	fd_set rfds;
	int maxfd;
	/* Loop variables */
	int i;
	int retry = 1;
	char cinput = 0;
	char reconnectInput[512]; 
	int valid = 0;
	char* garbage = NULL;

	/*Check and retrieve given arguments	*/
	while((c=getopt(argc,argv,options)) != -1) {
		switch(c)
		{
		    case 'n':
			surname = optarg;
			break;
		    case 's':
			snpip = optarg;
			break;
		    case 'q':
			snpport = optarg;
			break;
		    case 'i':
			saip = optarg;
			break;
		    case 'p':
			saport = optarg;
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
			show_usage(1);
			return -1;
		}
	}
	/* Check if required arguments are given   */
	if(surname == NULL || snpip == NULL || snpport == NULL) {
		show_usage(1);
		return -1;
	}

	for(i = 0; i < strlen(surname); i++) {
		if(isalpha(surname[i])) {
			surname[i] = tolower(surname[i]);
		}else{
			printf(ANSI_COLOR_RED "Surname can only contain regular alphabet");
			printf(ANSI_COLOR_WHITE "\n");
			return -1;
		}
	}

	printf(ANSI_COLOR_YELLOW "The surname will be lowercased to help with users finding it");
	printf(ANSI_COLOR_WHITE "\n");
	servername = surname;
	serverfile = malloc((strlen(servername)+strlen(SRVFILE) + 1)*sizeof(char) + 1);
	sprintf(serverfile,"%s.%s",servername,SRVFILE);

	printf("serverfile name: %s\n",serverfile);

	/* Check if saip or saport are set, otherwise use default values */
	if(saip == NULL) {
		if((h=gethostbyname("tejo.tecnico.ulisboa.pt"))==NULL) {
			printf(ANSI_COLOR_RED "Couldn't reach default host" ANSI_COLOR_RESET);
			printf(ANSI_COLOR_WHITE "\n");
			return -1;
		}
		saip = inet_ntoa(*(struct in_addr*)h->h_addr_list[0]);
	}

	if(saport == NULL) {
		saport = "58000";
	}

	/* Get port where our server will listen */
	if(sscanf(snpport,"%d\n",&port)!=1) {
		printf(ANSI_COLOR_RED "error: port is not a valid number" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_WHITE "\n");
		return -1;
	}
	
	/* Open UDP socket for our server */
	if((fd=socket(AF_INET,SOCK_DGRAM,0))==-1) {
		printf(ANSI_COLOR_RED "error: %s" ANSI_COLOR_RESET,strerror(errno));
		printf(ANSI_COLOR_WHITE "\n");
		return -1;
	}	
	
	/* Reset and set addr structure */
	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	addr.sin_port=htons(port);

	/* Assigning name to socket */
	ret=bind(fd,(struct sockaddr*)&addr,sizeof(addr));
	if(ret==-1){
		printf(ANSI_COLOR_RED "Error binding name to socket" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_WHITE "\n");
		close(fd);
		return -1;
	}

	/* Registering with surname server                                */
	/* If it fails, we prompt the user to check if he wants to try again or exit */
	do{
		printf("Attempting surname registering...\n");
		if(reg_sa(saip,saport,surname,snpip,snpport)==-1) {
			printf(ANSI_COLOR_RED "error registering with surname server" ANSI_COLOR_RESET);
			printf(ANSI_COLOR_WHITE "\n");	
			
			do {
				printf(ANSI_COLOR_BLUE "Want to try to reconnect? (Y)es/(N)0");
				printf(ANSI_COLOR_WHITE "\n");
				fgets(reconnectInput,512,stdin);
				sscanf(reconnectInput,"%c%s\n",&cinput,garbage);
				if(cinput == 'N'){
					valid = 0;
					close(fd);
					return -1;
				}
				else if(cinput == 'Y'){ 
					valid = 0;
					retry = 1; 
				}else{
					printf("Sorry, that option is not accepted\n");
					valid = 1;
				}
			}while(valid);
		}else retry = 0;

	}while(retry == 1);

	/* Resetting server file */
	if(reset_file(serverfile) == -1) {
		printf(ANSI_COLOR_RED "error resetting file" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_WHITE "\n");
		close(fd);
		return -1;
	}

	/* Allocating memory for first 3 chars of buffer (facilitates message parsing) */
	answer = malloc(3*sizeof(char));
	
	/* Begin listening  */
	printf(ANSI_COLOR_GREEN "Welcome to our server!" ANSI_COLOR_RESET);
	printf(ANSI_COLOR_WHITE "\n");
	while(!stop) {
		FD_ZERO(&rfds);
		FD_SET(fd,&rfds);
		FD_SET(STDIN_FILENO,&rfds);
		maxfd=fd;

		if(select(maxfd+1,&rfds,NULL,NULL,NULL)<0){
			printf(ANSI_COLOR_RED "Select() error: %s" ANSI_COLOR_RESET,strerror(errno));
			printf(ANSI_COLOR_WHITE "\n");
			stop = 1;
		}

		if(FD_ISSET(fd,&rfds)){
			addrlen=sizeof(addr);
			nread=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,(socklen_t*)&addrlen);
			if(nread==-1) {
				printf(ANSI_COLOR_RED "Error receiving bytes, closing..." ANSI_COLOR_RESET);
				printf(ANSI_COLOR_WHITE "\n");
				close(fd);
				stop = 1;
			}

			/* Parsing... */
			sprintf(answer,"%.*s",3,buffer);

			if(strcmp(answer,"REG") == 0){
				reply = reg_user(buffer,nread,serverfile,servername);
				if(sendto(fd,reply,strlen(reply),0,(struct sockaddr*)&addr,addrlen) == -1){
					printf(ANSI_COLOR_RED "Error replying to user" ANSI_COLOR_RESET);
					printf(ANSI_COLOR_WHITE "\n");
				}
			}else if(strcmp(answer,"UNR")==0){
				reply = unreg_user(buffer,nread,serverfile,servername);
				if(sendto(fd,reply,strlen(reply),0,(struct sockaddr*)&addr,addrlen) == -1) {
					printf(ANSI_COLOR_RED "Error replying to user" ANSI_COLOR_RESET);
					printf(ANSI_COLOR_WHITE "\n");
				}
			}else if(strcmp(answer,"QRY")==0){
				reply = find_user(buffer,nread,saip,saport, serverfile, servername);
				if(sendto(fd,reply,strlen(reply),0,(struct sockaddr*)&addr,addrlen) == -1){
					printf(ANSI_COLOR_RED "Error replying to user" ANSI_COLOR_RESET);
					printf(ANSI_COLOR_WHITE "\n");
				}
			}else {
				printf(ANSI_COLOR_RED "Command not recognized\n" ANSI_COLOR_RESET);
				if(sendto(fd,"NOK",3,0,(struct sockaddr*)&addr,addrlen) == -1){
						printf(ANSI_COLOR_RED "Error replying to user" ANSI_COLOR_RESET);
						printf(ANSI_COLOR_WHITE "\n");
					}
			}
		}
		if(FD_ISSET(STDIN_FILENO,&rfds)){
			fgets(localinput,sizeof(localinput),stdin);
			len = strlen(localinput);
			if(localinput[len-1] == '\n') localinput[len-1] = '\0';
			if(strcmp(localinput,"exit") == 0) stop=1;
			else if(strcmp(localinput,"list") == 0) list_users(serverfile,servername);
			else if(strcmp(localinput, "help") == 0) help(commands,ncmd);
			else printf("Command not recognized\n");
		}
	}
	unreg_sa(saip,saport,surname);
	free(answer);	
	close(fd);
	printf(ANSI_COLOR_GREEN "Bye Bye :(" ANSI_COLOR_RESET);
	printf(ANSI_COLOR_WHITE "\n");
	return 0;	
}
