#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <ctype.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <errno.h>
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



/* Function: qry_namesrv
*  -------------------
*  Function to query the proper name server
*
*  Parameters: char* snpip -> pointer to a string with proper name server ip address in format xxx.xxx.xxx.xxx
*  			   char* snpport-> pointer to a string with proper name server port number
*			   char* firstname -> pointer to a string with firstname to query address
*  			   char* surname -> pointer to a string with surname to query address
*
*  returns: string w/ reply from proper name server in the format surname;snpip;snpport
*           empty string if not successfull
*/
char* qry_namesrv(char* snpip, char* snpport, char* firstname, char* surname) {
	int msglen;
	char *msg, *answer;

	/* Preparing msg */
	msglen = strlen("QRY ") + strlen(firstname) + strlen(".") + strlen(surname);
	msg = malloc(msglen*sizeof(char)+1);

	if(sprintf(msg,"QRY %s.%s",firstname,surname) != (msglen)){
		printf(ANSI_COLOR_RED "qry_namesrv: error printing message" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_WHITE "\n");
		free(msg);
		return "";
	}

	printf("MSG: %s TO IP: %s TO PORT: %s\n",msg,snpip,snpport);
	/* Sending message */
	answer = comUDP(msg,snpip,snpport);

	if(answer == NULL) {
		printf(ANSI_COLOR_RED "qry_namesrv: error contacting propername server. Are you sure the address is right?");
		printf(ANSI_COLOR_WHITE "\n");
		return "";
	}

	printf(ANSI_COLOR_GREEN "qry_namesrv: Successful!" ANSI_COLOR_RESET);
	printf(ANSI_COLOR_WHITE "\n");
	return answer;
}

/* Function: find_user
*  -----------------------
*  Uses qry_aserv or qry_namesrv if needed to find the location of a user
*
*  Parameters: char* buffer -> pointer to a string with query in format QRY name.surname
*                int n      -> integer with number of bytes received
*              char* saip   -> pointer to a string with surname server's ip address in format xxx.xxx.xxx.xxx
*			   char* saport -> pointer to a string with surname server's port number
*
*  returns: string with info of user if successful
*           string with NOK if unsuccessful
*/
char* find_user(char* buffer,int n,char* saip, char* saport) {
	FILE *ufile;
	int found = 0;
	char name[128],surname[128];
	char tempname[128],tempip[128],tempport[128];
	char snpip[128],snpport[128];
	char buff2[512],temp[512];
	char* reply;

	sprintf(buff2,"%s\n",buffer);
	buff2[n] = '\0';

	/* Checking buffer format */
	if(sscanf(buff2,"QRY %[^'.'].%s",name,surname) != 2){
		return "NOK - Query badly formatted";
	}

	printf("find_user: searching for %s.%s\n",name,surname);

	if(strcmp(surname,servername) == 0) {
		/* Search on our file */
		ufile = fopen(serverfile,"r");
		if(ufile == NULL) return "NOK - client list unreachable";
		while(fgets(temp,512,ufile) != NULL){
			if(sscanf(temp,"%[^';'];%[^';'];%s",tempname,tempip,tempport) != 3) {
				printf(ANSI_COLOR_RED "find_user: Error parsing file contents" ANSI_COLOR_RESET);
				printf(ANSI_COLOR_WHITE "\n");
				fclose(ufile);
				return "NOK - Error with local file";
			}
			if((strstr(temp,"#") == NULL) && (strcmp(tempname,name) == 0)) {
				printf(ANSI_COLOR_GREEN "find_user: found" ANSI_COLOR_RESET);
				printf(ANSI_COLOR_WHITE "\n");
				found = 1;
				break;
			}
		}
		if(found != 1) {
			printf(ANSI_COLOR_RED "find_user: User not found" ANSI_COLOR_RESET);
			printf(ANSI_COLOR_WHITE "\n");
			return "NOK - User not found";
		}
		reply = malloc((7+strlen(name)+strlen(surname)+strlen(tempip)+strlen(tempport))*sizeof(char)+1);
		sprintf(reply,"RPL %s.%s;%s;%s",name,surname,tempip,tempport);
		fclose(ufile);
	}
	else {
		/* Query surname server */
		reply = qry_asrv(saip, saport, surname);
		if(strcmp(reply,"") == 0) printf(ANSI_COLOR_RED "find_user: not found" ANSI_COLOR_RESET);
		if(sscanf(reply,"SRPL %[^';'];%[^';'];%s",surname,snpip,snpport) != 3) return "NOK - Reply not parseable";

		/* Query np server */	
		reply = qry_namesrv(snpip, snpport, name, surname);
		if(strcmp(reply,"") == 0) {
			printf(ANSI_COLOR_RED "find_user: not found" ANSI_COLOR_RESET);
			printf(ANSI_COLOR_WHITE "\n");
		}
		/* return the reply*/
		printf("find_user: reply: %s\n",reply);
	}
	return reply;
}
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
				reply = find_user(buffer,nread,saip,saport);
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
		}
	}
	unreg_sa(saip,saport,surname);
	free(answer);	
	close(fd);
	printf(ANSI_COLOR_GREEN "Bye Bye :(" ANSI_COLOR_RESET);
	printf(ANSI_COLOR_WHITE "\n");
	return 0;	
}
