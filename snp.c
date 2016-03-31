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

#define ANSI_COLOR_BLUE "\x1b[34m"
#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_WHITE   "\x1B[37m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define SRVFILE "clientlist"

/* Structure where we list valid commands and description, to be used by function help() */
struct command_d{
	char* cmd_name;
	char* cmd_help;
}
const commands[] = {
	{"list", "Lists users currently registered with the server"},
	{"exit", "Exits the program cleanly"},
	{"help", "Prints a rudimentary help"}
};

int ncmd = sizeof(commands)/sizeof(commands[0]); /* Number of commands */
extern int errno;
char* servername; /* Surname served by this server */
char* serverfile; /* file name of the server, used to store the registered clients' list */

/* Function: help
*  -------------------
*  Displays a list of all commands available and a description
* 
*  returns: 0
*/
int help() {
	int i;

	printf(ANSI_COLOR_BLUE "HELP - Here are the commands available:" ANSI_COLOR_RESET);
	printf(ANSI_COLOR_WHITE "\n");

	for(i = 0; i < ncmd; i++) {
		printf("%s - %s\n",commands[i].cmd_name,commands[i].cmd_help);
	}
	return 0;
}

/* Function: comUDP
*  -------------------
*  Sends a UDP message and waits for an answer
* 
*  Parameters: char* msg -> message to be sent
*  			   char* dst_ip -> ip of the destination of the message
*              char* dst_port -> port of the destination of the message
*
*  returns: Reply in char* format
*           NULL if error ocurred
*/
char * comUDP(char * msg, char * dst_ip, char * dst_port){
	int surDir_sock;
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
		free(msg);
		close(surDir_sock);
		return NULL;
	}

	a=(struct in_addr*)h->h_addr_list[0];

	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr=*a;
	addr.sin_port=htons(atoi(dst_port));

	n=sendto(surDir_sock,msg,strlen(msg),0,(struct sockaddr*)&addr,sizeof(addr));
	if(n==-1) {
		printf("UDP error: sending message to the server\n");
		free(msg);
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
		free(msg);
		close(surDir_sock);
		return NULL;
	}

	answer=malloc(n);
	sprintf(answer,"%.*s",n,buffer);

	close(surDir_sock);
	free(msg);
	return answer;
}

/* Function: list_users
*  -------------------
*  Lists all users currently registered on the server
* 
*  returns: 0 if listed successfully
*           -1 if error ocurred
*/
int list_users() {
	FILE *ufile;
	char buffer[512],uname[512],uip[512],uport[512];

	ufile = fopen(serverfile,"r");
	if(ufile == NULL) {
		printf(ANSI_COLOR_RED "list_users: error: %s", strerror(errno));
		printf(ANSI_COLOR_WHITE "\n");
		return -1;
	}	

	while(fgets(buffer,512,ufile) != NULL){
		if(strstr(buffer,"#") == NULL) {	/* If buffer contains # then line is not valid */
			if(sscanf(buffer,"%[^';'];%[^';'];%s",uname,uip,uport) != 3) {
				printf(ANSI_COLOR_RED "list_users: error reading user file" ANSI_COLOR_RESET);
				printf(ANSI_COLOR_WHITE "\n");
				fclose(ufile);
				return -1;
			}
			printf("%s.%s - %s - %s\n",uname,servername,uip,uport);
		}
	}
	printf(ANSI_COLOR_GREEN "list_users: Done!" ANSI_COLOR_RESET);
	printf(ANSI_COLOR_WHITE "\n");
	fclose(ufile);
	return 0;
} 

/* Function: reset_file
*  ---------------------
*   Resets server file when server starts
*
*   returns: 0 if reset was successful
*            -1 if error ocurred
*/
int reset_file(){
	FILE *ufile;
	ufile = fopen(serverfile,"w");
	if(ufile == NULL) {
		printf(ANSI_COLOR_RED "reset_file: error: %s" ANSI_COLOR_RESET,strerror(errno));
		printf(ANSI_COLOR_WHITE "\n");
		return -1;
	}
	
	fclose(ufile);
	return 0;
}

/*
 * Function:  show_usage
 * --------------------
 * Show the correct usage of the program commands
 *
 *  returns: -1 always (when this runs, it means something about the user's entered arguments went wrong)
 */
int show_usage(){
	printf("Usage: snp -n surname -s snpip -q snpport [-i saip] [-p saport]\n");
	return -1;

}

/*
 * Function:  reg_sa
 * --------------------
 * Registers a surname on the surname server using UDP messages.
 *
 *  Parameters: char* saip    -> pointer to a string that contains the surname server's ip address in xxx.xxx.xxx.xxx format
 *  	 		char* saport  -> pointer to a string that contains the surname server's port
 *  		    char* surname -> pointer to a string with the surname to be registered 
 *  			char* localip -> ip of the machine in which the local server is located
 *  			char* snpport -> port that the local server is listening to
 *
 *  returns:  0 if register was successful
 *			 -1 if unregister was unsuccessful	
 */

int reg_sa(char* saip, char* saport, char* surname, char* localip, char* snpport){
	int msglen;
	char *msg;
	char *answer;

	/* Preparing msg to send */
	msglen = 7 + strlen(surname) + strlen(localip) + strlen(snpport);
	msg = malloc(msglen*sizeof(char)+1);

	if(sprintf(msg,"SREG %s;%s;%s",surname,localip,snpport) != (msglen)){
		printf(ANSI_COLOR_RED "reg_sa: error building string" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_WHITE "\n");
		free(msg);
		return -1;
	}

	answer = comUDP(msg, saip,saport);
	if(answer == NULL) {
		printf(ANSI_COLOR_RED "reg_sa: error sending contacting surname server. Are you sure the address is right?");
		printf(ANSI_COLOR_WHITE "\n");
		return -1;
	}

	if(strcmp(answer,"OK") != 0) {
		printf(ANSI_COLOR_RED "%s" ANSI_COLOR_RESET,answer);
		printf(ANSI_COLOR_WHITE "\n");
		free(answer);
		return -1;
	}

	printf(ANSI_COLOR_GREEN "reg_sa: Successful!" ANSI_COLOR_RESET);
	printf(ANSI_COLOR_WHITE "\n");
	return 0;
}

/*
 * Function:  unreg_sa
 * --------------------
 * Unregisters a surname from the surname server using UDP messages.
 *
 *  Parameters: char* saip    -> pointer to a string with surname server's ip address in format xxx.xxx.xxx.xxx
 *  			char* saport  -> pointer to a string with surname server's port number
 *  			char* surname -> pointer to a string with the surname to be unregistered 
 *
 *  returns:  0 if unregister was successful
 *	         -1 if unregister was unsuccessful	
 */
int unreg_sa(char* saip, char* saport, char* surname){
	int msglen;
	char *msg, *answer;
	
	/* Preparing Unregister Message */
	msglen = strlen("SUNR ") + strlen(surname);
	msg = malloc(msglen*sizeof(char)+1);

	if(sprintf(msg,"SUNR %s",surname) != (msglen)){
		printf(ANSI_COLOR_RED "unreg_sa: error printing second message" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_WHITE "\n");
		free(msg);
		return -1;
	}	

	answer = comUDP(msg,saip,saport);

	if(answer == NULL) {
		printf(ANSI_COLOR_RED "unreg_sa: error sending contacting surname server. Are you sure the address is right?");
		printf(ANSI_COLOR_WHITE "\n");
		return -1;
	}

	if(strcmp(answer,"OK") != 0){
		printf(ANSI_COLOR_RED "%s" ANSI_COLOR_RESET,answer);
		printf(ANSI_COLOR_WHITE "\n");
		free(answer);
		return -1;
	}

	free(answer);
	printf(ANSI_COLOR_GREEN "unreg_sa: Successful!" ANSI_COLOR_RESET);
	printf(ANSI_COLOR_WHITE "\n");
	return 0;
}

/* Function: reg_user
*  -----------------------
*  Registers a user
*
*  Parameters: char* buffer -> pointer to a string with user's information in format REG name;surname;ip;port
*              	 int n      -> integer with number of bytes received
*
*  Returns: "OK" if successful
*	       "NOK - " + error specification if error occurs
*/
char* reg_user(char* buffer, int n) {
	FILE *ufile;
	char name[128], surname[128], ip[128], port[128];
	char tempname[128],tempip[128],tempport[128];
	char temp[512];
	buffer[n] = '\0';

	printf("NEW MESSAGE: %s\n",buffer);

	if(sscanf(buffer,"REG %[^'.'].%[^';'];%[^';'];%s",name,surname,ip,port)!= 4){
		printf(ANSI_COLOR_RED "reg_user: Message format not recognized" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_WHITE "\n");
		return "NOK - Message format not recognized";
	}

	/* For registration, client name must be equal to the server's name */
	if(strcmp(servername,surname) != 0){
		printf(ANSI_COLOR_RED "reg_user: Surname and servername don't match" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_WHITE "\n");
		return "NOK - Surname and Servername don't match";
	}

	/* # is a special character reserved for deleted registrations, so no user can use it in it's name */
	if(strstr(name,"#") != NULL){
		printf(ANSI_COLOR_RED "reg_user: User tried registering a name with # character" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_WHITE "\n");
		return "NOK - Special character # is not allowed";
	}

	ufile = fopen(serverfile,"a+");
	if(ufile == NULL) {
		printf(ANSI_COLOR_RED "reg_user: error: %s" ANSI_COLOR_RESET,strerror(errno));
		printf(ANSI_COLOR_WHITE "\n");
		return "NOK - Client list not accessible";
	}

	/* Checking username is unique */
	fseek(ufile,0L,SEEK_SET);
	while(fgets(temp,512,ufile) != NULL) {
		if(sscanf(temp,"%[^';'];%[^';'];%s",tempname,tempip,tempport) != 3) {
			printf(ANSI_COLOR_RED "Error parsing clientlist" ANSI_COLOR_RESET);
			printf(ANSI_COLOR_WHITE "\n");
			return "NOK - Error parsing clientlist";
		}
		if(strstr(temp,"#") != NULL) continue;
		if((strcmp(tempname, name)) == 0) {
			printf(ANSI_COLOR_RED "User name not unique :(" ANSI_COLOR_RESET);
			printf(ANSI_COLOR_WHITE "\n");
			fclose(ufile);
			return "NOK - Username not unique";
		}
	}	
	fseek(ufile,0L,SEEK_END);

	if(fprintf(ufile,"%s;%s;%s\n",name,ip,port) < 0) {
		printf(ANSI_COLOR_RED "reg_user: Error writing to file" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_WHITE "\n");
		fclose(ufile);
		return "NOK - Client list not writable";
	}
	printf(ANSI_COLOR_GREEN "New user registration! :)" ANSI_COLOR_RESET);
	printf(ANSI_COLOR_WHITE "\n");
	fclose(ufile);
	return "OK";
}

/* Function: unreg_user
*  --------------------
*  Unregisters a user
*
*  Parameters: char* buffer -> pointer to a string with user's information in format UNR name;surname
*  			     int n      -> integer with number of bytes received
*
*  returns: "OK" if unregister is successful
*          "NOK - " + error specification if error occurs
*/
char* unreg_user(char *buffer, int n){
	FILE *ufile;
	char name[128],surname[128],temp[512];
	char tempname[128],tempip[128],tempport[128];
	int lineNb;
	buffer[n] = '\0';

	printf("NEW MESSAGE: %s\n",buffer);

	if(sscanf(buffer,"UNR %[^'.'].%s",name,surname) != 2){
		printf(ANSI_COLOR_RED "unreg_user: Message format not recognized" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_WHITE "\n");
		return "NOK - Message format not recognized";
	}

	/* For a user to be registered in the server, the server's name and the user's surname must match*/
	if(strcmp(servername,surname) != 0) {
		printf(ANSI_COLOR_RED "unreg_user: Surname and Server name don't match" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_WHITE "\n");
		return "NOK - Surname and Server name don't match";
	}
	/* User's name must not contain the # character. Should not have registered with a # character,
	   checking it here just to be sure */
	if(strstr(name,"#") != 0){
		printf(ANSI_COLOR_RED "unreg_user: User tried unregistering a name with # character" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_WHITE "\n");
		return "NOK - Special character # is not allowed";
	}

	ufile = fopen(serverfile,"r+");
	if(ufile == NULL) {
		printf(ANSI_COLOR_RED "unreg_user: error: %s" ANSI_COLOR_RESET, strerror(errno));
		printf(ANSI_COLOR_WHITE "\n");
		return "NOK - Client List not accessible";
	}

	/* Looking for user's name on our user file */
	while(fgets(temp,512,ufile) != NULL) {
		if(sscanf(temp,"%[^';'];%[^';'];%s",tempname,tempip,tempport) != 3) {
			printf(ANSI_COLOR_RED "unreg_user: Error parsing file contents" ANSI_COLOR_RESET);
			printf(ANSI_COLOR_WHITE "\n");
			fclose(ufile);
			return "NOK - Error with local file";
		}
		if(strstr(temp,"#") != NULL) continue; /* if # is found, it is a deleted registry */
		if((strcmp(tempname, name)) == 0) {
			lineNb = ftell(ufile);
			fseek(ufile,lineNb-(int)strlen(temp),SEEK_SET);	/* Setting file pointer to the beginning of the line */
			if(fprintf(ufile,"#") < 0) { /* Substituting first character of the line with a # */
				printf(ANSI_COLOR_RED "unreg_user: error unregistering user" ANSI_COLOR_RESET);
				printf(ANSI_COLOR_WHITE "\n");
				fclose(ufile);
				return "NOK - Error with local file";
			}
			printf(ANSI_COLOR_GREEN "User unregistered :(" ANSI_COLOR_RESET);
			printf(ANSI_COLOR_WHITE "\n");
			fclose(ufile);
			return "OK";
		}
	}

	fclose(ufile);
	return "NOK - Username not found";	
}

/* Function: qry_asrv
*  -------------------
*  Function to query surname server about the location of an np server with a certain surname
*
*  Parameters: char* saip   -> pointer to a string with the surname server's ip address in format xxx.xxx.xxx.xxx
*  			   char* saport -> pointer to a string with the surname server's port number
*  			   char* surname-> surname to query surname server
*
*  returns: string w/ reply from surname server in the format surname;snpip;snpport
*           empty string if not successful
*/
char* qry_asrv(char* saip, char* saport, char* surname) {
	int msglen;
	char *msg, *answer;

	msglen = strlen("SQRY ") + strlen(surname);
	msg = malloc(msglen * sizeof(char) + 1);
	if(sprintf(msg,"SQRY %s",surname) != (msglen)){
		printf(ANSI_COLOR_RED "qry_asrv: error printing message" ANSI_COLOR_RESET);
		printf(ANSI_COLOR_WHITE "\n");
		free(msg);
		return "";
	}	

	answer = comUDP(msg,saip,saport);

	if(answer == NULL) {
		printf(ANSI_COLOR_RED "unreg_sa: error sending contacting surname server. Are you sure the address is right?");
		printf(ANSI_COLOR_WHITE "\n");
		return "";
	}

	printf(ANSI_COLOR_GREEN "qry_asrv: Successful!" ANSI_COLOR_RESET);
	printf(ANSI_COLOR_WHITE "\n");
	return answer;
}

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

	/* Sending message */
	answer = comUDP(msg,snpip,snpport);

	if(answer == NULL) {
		printf(ANSI_COLOR_RED "unreg_sa: error sending contacting surname server. Are you sure the address is right?");
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
			show_usage();
			return -1;
		}
	}
	/* Check if required arguments are given   */
	if(surname == NULL || snpip == NULL || snpport == NULL) {
		show_usage();
		return -1;
	}

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
	if(reset_file() == -1) {
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
				reply = reg_user(buffer,nread);
				if(sendto(fd,reply,strlen(reply),0,(struct sockaddr*)&addr,addrlen) == -1){
					printf(ANSI_COLOR_RED "Error replying to user" ANSI_COLOR_RESET);
					printf(ANSI_COLOR_WHITE "\n");
				}
			}else if(strcmp(answer,"UNR")==0){
				reply = unreg_user(buffer,nread);
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
			else if(strcmp(localinput,"list") == 0) list_users();
			else if(strcmp(localinput, "help") == 0) help();
		}
	}
	unreg_sa(saip,saport,surname);
	free(answer);	
	close(fd);
	printf(ANSI_COLOR_GREEN "Bye Bye :(" ANSI_COLOR_RESET);
	printf(ANSI_COLOR_WHITE "\n");
	return 0;	
}
