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

#define ANSI_COLOR_RED "\x1b[31m"
#define ANSI_COLOR_GREEN "\x1b[32m"
#define ANSI_COLOR_RESET "\x1b[0m"
#define SRVFILE "clientlist"

extern int errno;
char* servername;

/* Function: list_users
*  -------------------
*  Lists all users currently registered on the server
* 
*  returns: 0 if listed successfully
*           -1 if listed unsuccessfully
*/
int list_users() {
	FILE *ufile;
	char buffer[512],uname[512],uip[512],uport[512];

	ufile = fopen(SRVFILE,"r");
	if(ufile == NULL) {
		printf(ANSI_COLOR_RED "list_users: error: %s\n" ANSI_COLOR_RESET, strerror(errno));
		return -1;
	}	

	while(fgets(buffer,512,ufile) != NULL){
		if(strstr(buffer,"#") == NULL) {
			if(sscanf(buffer,"%[^';'];%[^';'];%s",uname,uip,uport) != 3) {
				printf(ANSI_COLOR_RED "list_users: error reading user file\n" ANSI_COLOR_RESET);
			}
			printf("%s.%s - %s - %s\n",uname,servername,uip,uport);
		}
	}
	printf(ANSI_COLOR_GREEN "list_users: Done!\n" ANSI_COLOR_RESET);
	fclose(ufile);
	return 0;
} 

/* Function: reset_file
*  ---------------------
*   Resets server file when server starts
*
*   returns: 0 if successful
*            -1 if unsuccessful
*/
int reset_file(){
	FILE *ufile;
	ufile = fopen(SRVFILE,"w");
	if(ufile == NULL) {
		printf(ANSI_COLOR_RED "reset_file: error: %s\n" ANSI_COLOR_RESET,strerror(errno));
		return -1;
	}
	
	fclose(ufile);
	return 0;
}

/*
 * Function:  show_usage
 * --------------------
 * Show the correct usage of the program command
 *
 *  returns:  -1 always (when this runs, it means something about the user's entered arguments went wrong)
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
 *  h: structure with host information about the surname server
 *  aport: integer with surname server port
 *  surname: string with the surname to be unregistered 
 *  localip: ip of the machine in which the local server is located
 *  snpport: port that the local server is listening to
 *
 *  returns:  0 if register was successfull
 *			 -1 if unregister was unsuccessfull
 *	TODO:
 *		  Verify localip type is the required	
 */

int reg_sa(struct hostent* h, int aport, char* surname, char* localip, char* snpport){
	int fd,n,addrlen;
	struct sockaddr_in addr;
	char buffer[128];
	struct in_addr *a;
	int msglen;
	char *msg;
	char *answer;

	/* Preparing msg to send */
	msglen = 7 + strlen(surname) + strlen(localip) + strlen(snpport);
	msg = malloc(msglen*sizeof(char)+1);

	if(sprintf(msg,"SREG %s;%s;%s",surname,localip,snpport) != (msglen)){
		printf(ANSI_COLOR_RED "reg_sa: error building string\n" ANSI_COLOR_RESET);
		free(msg);
		return -1;
	}
	
	a=(struct in_addr*)h->h_addr_list[0];
	fd=socket(AF_INET,SOCK_DGRAM,0);
	if(fd==-1) {
		printf(ANSI_COLOR_RED "reg_sa: error opening socket\n" ANSI_COLOR_RESET);
		free(msg);
		return -1;
	}

	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr=*a;
	addr.sin_port=htons(aport);

	n=sendto(fd,msg,msglen,0,(struct sockaddr*)&addr,sizeof(addr));
	if(n==-1) {
		printf(ANSI_COLOR_RED "reg_sa: error sending to surname server\n" ANSI_COLOR_RESET);
		free(msg);
		close(fd);
		return -1;
	}

	addrlen = sizeof(addr);
	n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,(socklen_t*)&addrlen);
	if(n==-1) {
		printf(ANSI_COLOR_RED "reg_sa: error receiving from surname server\n" ANSI_COLOR_RESET);
		free(msg);
		close(fd);
		return -1;
	}
	
	answer=malloc(n);

	sprintf(answer,"%.*s",n,buffer);
	if(strcmp(answer,"OK") != 0) {
		printf("%s\n",answer);
		free(msg);
		free(answer);
		close(fd);
		return -1;
	}

	free(msg);
	free(answer);
	close(fd);
	printf(ANSI_COLOR_GREEN "reg_sa: Successful!\n" ANSI_COLOR_RESET);
	return 0;
}

/*
 * Function:  unreg_sa
 * --------------------
 * Unregisters a surname from the surname server using UDP messages.
 *
 *  h: structure with host information about the surname server
 *  aport: integer with surname server port
 *  surname: string with the surname to be unregistered 
 *
 *  returns:  0 if unregister was successfull
 *	     -1 if unregister was unsuccessfull
 *	TODO: 
 */

int unreg_sa(struct hostent* h, int aport, char* surname){
	int fd,n,addrlen;
	struct sockaddr_in addr;
	char buffer[128];
	struct in_addr *a;
	int msglen;
	char *msg;
	char *answer;
	
	a=(struct in_addr*)h->h_addr_list[0];
	fd=socket(AF_INET,SOCK_DGRAM,0);
	if(fd==-1) {
		printf(ANSI_COLOR_RED "unreg_sa: error opening socket\n" ANSI_COLOR_RESET);
		return -1;
	}

	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr=*a;
	addr.sin_port=htons(aport);

	/* Preparing Unregister Message */
	msglen = 5 + strlen(surname);
	msg = malloc(msglen*sizeof(char)+1);

	if(sprintf(msg,"SUNR %s",surname) != (msglen)){
		printf(ANSI_COLOR_RED "unreg_sa: error printing second message\n" ANSI_COLOR_RESET);
		free(msg);
		close(fd);
		return -1;
	}	

	/* Sending it to the Surname Server */
	n=sendto(fd,msg,msglen,0,(struct sockaddr*)&addr,sizeof(addr));
	if(n==-1) {
		printf(ANSI_COLOR_RED "unreg_sa: error sending to surname server\n" ANSI_COLOR_RESET);
		free(msg);
		close(fd);
		return -1;
	}

	/* Receiving to check if OK */
	addrlen = sizeof(addr);
	n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,(socklen_t*)&addrlen);
	if(n==-1) {
		printf(ANSI_COLOR_RED "unreg_sa: error receiving from surname server\n" ANSI_COLOR_RESET);
		free(msg);
		close(fd);
		return -1;
	}

	answer = malloc(n);

	sprintf(answer,"%.*s",n,buffer);
	if(strcmp(answer,"OK") != 0){
		printf(ANSI_COLOR_GREEN "%s\n" ANSI_COLOR_RESET,answer);
		free(msg);
		free(answer);
		close(fd);
		return -1;
	}

	free(msg);	
	free(answer);
	close(fd);
	printf(ANSI_COLOR_GREEN "unreg_sa: Successful!\n" ANSI_COLOR_RESET);
	return 0;
}

/* Function: reg_user
*  -----------------------
*  Registers a user
*
*  buffer: message received from client in format REG name.surname;ip;port
*  n: number of bytes read by the server
*  fd: file descriptor of file where server stores user information
*
*  Returns: 0 if successful
*	    -1 if unsuccessful
*/
char* reg_user(char* buffer, int n) {
	FILE *ufile;
	char name[128], surname[128], ip[128], port[128];
	char temp[512];
	buffer[n] = '\0';

	printf("%s\n",buffer);

	if(sscanf(buffer,"REG %[^'.'].%[^';'];%[^';'];%s",name,surname,ip,port)!= 4){
		printf(ANSI_COLOR_RED "reg_user: Message format not recognized\n" ANSI_COLOR_RESET);
		return "NOK - Message format not recognized";
	}

	if(strcmp(servername,surname) != 0){
		printf(ANSI_COLOR_RED "reg_user: Surname and servername don't match\n" ANSI_COLOR_RESET);
		return "NOK - Surname and Servername don't match";
	}

	if(strcmp(name,"#") == 0){
		printf(ANSI_COLOR_RED "reg_user: User tried registering a name with # character\n" ANSI_COLOR_RESET);
		return "NOK - Special character # is not allowed";
	}

	ufile = fopen(SRVFILE,"a+");
	if(ufile == NULL) {
		printf(ANSI_COLOR_RED "reg_user: error: %s\n" ANSI_COLOR_RESET,strerror(errno));
		return "NOK - Client list not accessible";
	}

	printf("reg_user: Checking username uniqueness\n");
	/* Checking username is unique */
	fseek(ufile,0L,SEEK_SET);
	while(fgets(temp,512,ufile) != NULL) {
		printf("lel\n");
		if(strstr(temp,"#") != NULL) continue;
		if((strstr(temp, name)) != NULL) {
			printf(ANSI_COLOR_RED "User name not unique :(\n" ANSI_COLOR_RESET);
			fclose(ufile);
			return "NOK - Username not unique";
		}
	}	
	fseek(ufile,0L,SEEK_END);

	if(fprintf(ufile,"%s;%s;%s\n",name,ip,port) < 0) {
		printf(ANSI_COLOR_RED "reg_user: Error writing to file" ANSI_COLOR_RESET);
		fclose(ufile);
		return "NOK - Client list not writable";
	}
	printf(ANSI_COLOR_GREEN "New user registration! :)\n" ANSI_COLOR_RESET);
	fclose(ufile);
	return "OK";
}

/* Function: unreg_user
*  --------------------
*  Unregisters a user
*
*  buffer: message received from client
*  n: number of bytes read
*
*  returns: -1 if unregister is unsuccessful
*           0 if unregister is successful
*/
char* unreg_user(char *buffer, int n){
	FILE *ufile;
	char name[128],surname[128],temp[512];
	int line;
	buffer[n] = '\0';
	if(sscanf(buffer,"UNR %[^'.'].%s",name,surname) != 2){
		printf(ANSI_COLOR_RED "unreg_user: Message format not recognized\n" ANSI_COLOR_RESET);
		return "NOK - Message format not recognized";
	}

	if(strcmp(servername,surname) != 0) {
		printf(ANSI_COLOR_RED "unreg_user: Surname and Server name don't match\n" ANSI_COLOR_RESET);
		return "NOK - Surname and Server name don't match";
	}

	ufile = fopen(SRVFILE,"r+");
	if(ufile == NULL) {
		printf(ANSI_COLOR_RED "unreg_user: error: %s\n" ANSI_COLOR_RESET, strerror(errno));
		return "NOK - Client List not accessible";
	}
	while(fgets(temp,512,ufile) != NULL) {
		if(strstr(temp,"#") != NULL) continue;
		if((strstr(temp, name)) != NULL) {
			line = ftell(ufile);
			fseek(ufile,line-(int)strlen(temp),SEEK_SET);
			if(fprintf(ufile,"#") < 0) {
				printf(ANSI_COLOR_RED "unreg_user: error unregistering user\n" ANSI_COLOR_RESET);
			}
			fseek(ufile,0L,SEEK_END);
			printf(ANSI_COLOR_GREEN "User unregistered :(\n" ANSI_COLOR_RESET);
			return "OK";
		}
	}	
	fseek(ufile,0L,SEEK_END);
	return "NOK - Username not found";	
}

/* Function: qry_asrv
*  -------------------
*  surname: surname to query the surname server
*
*  h: structure with address information
*  aport: surname server port
*  surname: surname to query address
*
*  returns: string w/ reply from surname server in the format surname;snpip;snpport
*           empty string if not successfull
*/
char* qry_asrv(struct hostent* h, int aport, char* surname) {
	int fd,n,addrlen,la;
	struct sockaddr_in addr;
	struct in_addr *a;
	int msglen = strlen("SQRY ") + strlen(surname);
	char* query = malloc(msglen*sizeof(char) + 1);
	char* answer;
	char buffer[512];

	a=(struct in_addr*)h->h_addr_list[0];
	fd=socket(AF_INET,SOCK_DGRAM,0);
	if(fd==-1) {
		printf(ANSI_COLOR_RED "qry_asrv: error opening socket\n" ANSI_COLOR_RESET);
		return "";
	}

	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr=*a;
	addr.sin_port=htons(aport);

	if((la = sprintf(query,"SQRY %s",surname)) != (msglen)){
		printf(ANSI_COLOR_RED "qry_asrv: error printing message\n" ANSI_COLOR_RESET);
		free(query);
		close(fd);
		return "";
	}	

	/* Sending it to the Surname Server */
	n=sendto(fd,query,msglen,0,(struct sockaddr*)&addr,sizeof(addr));
	if(n==-1) {
		printf(ANSI_COLOR_RED "qry_asrv: error sending to surname server\n" ANSI_COLOR_RESET);
		free(query);
		close(fd);
		return "";
	}

	/* Receiving to check if OK */
	addrlen = sizeof(addr);
	n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,(socklen_t*)&addrlen);
	if(n==-1) {
		printf(ANSI_COLOR_RED "qry_asrv: error receiving from surname server\n" ANSI_COLOR_RESET);
		free(query);
		close(fd);
		return "";
	}

	answer = malloc(n);

	sprintf(answer,"%.*s",n,buffer);

	free(query);	
	close(fd);
	printf(ANSI_COLOR_GREEN "qry_asrv: Successful!\n" ANSI_COLOR_RESET);
	return answer;
}

/* Function: qry_namesrv
*  -------------------
*  surname: surname to query the proper name server
*
*  snpip: proper name server ip 
*  snpport: proper name server port
*  surname: surname to query address
*
*  returns: string w/ reply from proper name server in the format surname;snpip;snpport
*           empty string if not successfull
*/
char* qry_namesrv(char* snpip, char* snpport, char* firstname, char* surname) {
	int fd,n,addrlen;
	struct hostent* h;
	struct in_addr* a;
	struct sockaddr_in addr;
	struct in_addr temp;
	int port;
	int msglen = strlen("QRY ") + strlen(firstname) + strlen(".") + strlen(surname);
	char* query = malloc(msglen*sizeof(char)+1);
	char buffer[512];
	char* answer;

	if(sprintf(query,"QRY %s.%s",firstname,surname) != (msglen)){
		printf(ANSI_COLOR_RED "qry_namesrv: error printing message\n" ANSI_COLOR_RESET);
		free(query);
		return "";
	}

	/* Converting ip and creating h struct */
	inet_pton(AF_INET, snpip, &temp);
	if((h=gethostbyaddr(&temp,sizeof(temp),AF_INET))==NULL){
		printf(ANSI_COLOR_RED "qry_namesrv: Couldn't reach np server\n" ANSI_COLOR_RESET);
		return "";
	}
	/* Converting port number */
	port = atoi(snpport);

	/* Opening socket */
	fd=socket(AF_INET,SOCK_DGRAM,0);
	if(fd == -1) {
		printf(ANSI_COLOR_RED "qry_namesrv: error: %s\n" ANSI_COLOR_RESET,strerror(errno));
	}

	a=(struct in_addr*)h->h_addr_list[0];

	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr=*a;
	addr.sin_port=htons(port);

	/* Sending it to the name Server */
	n=sendto(fd,query,msglen,0,(struct sockaddr*)&addr,sizeof(addr));
	if(n==-1) {
		printf(ANSI_COLOR_RED "qry_namesrv: error sending to surname server\n" ANSI_COLOR_RESET);
		free(query);
		close(fd);
		return "";
	}

	/* Receiving to check if OK */
	addrlen = sizeof(addr);
	n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,(socklen_t*)&addrlen);
	if(n==-1) {
		printf(ANSI_COLOR_RED "qry_namesrv: error receiving from surname server\n" ANSI_COLOR_RESET);
		free(query);
		close(fd);
		return "";
	}

	answer = malloc(n);

	sprintf(answer,"%.*s",n,buffer);

	free(query);	
	close(fd);
	printf(ANSI_COLOR_GREEN "qry_namesrv: Successful!\n" ANSI_COLOR_RESET);
	return answer;
}

/* Function: find_user
*  -----------------------
*
*   Uses qry_aserv or qry_namesrv if needed to find the location of a user
*
*   returns: string with info of user if successful
*            string with NOK if unsuccessful
*/
char* find_user(char* buffer,int n,struct hostent *h, int aport) {
	int found = 0;
	char name[128];
	char surname[128];
	char temp[512];
	char snpip[128],snpport[128];
	char uip[128],uport[128];
	char buff2[512];
	char* reply;
	FILE *ufile;

	printf("find_user: starting...\n");
	sprintf(buff2,"%s\n",buffer);
	buff2[n] = '\0';

	if(sscanf(buff2,"QRY %[^'.'].%s",name,surname) != 2){
		return "NOK - Query badly formatted";
	}

	printf("find_user: searching for %s.%s\n",name,surname);

	if(strcmp(surname,servername) == 0) {
		/* Search on our file */
		ufile = fopen(SRVFILE,"r");
		if(ufile == NULL) return "NOK - client list unreachable";
		while(fgets(temp,512,ufile) != NULL){
			if((strstr(temp,"#") == NULL) && (strstr(temp,name) != NULL)) {
				printf(ANSI_COLOR_GREEN "find_user: found\n" ANSI_COLOR_RESET);
				sscanf(temp,"%[^';'];%[^';'];%s", name,uip,uport);
				found = 1;
				break;
			}
		}
		if(found != 1) return "NOK - User not found";
		reply = malloc((7+strlen(name)+strlen(surname)+strlen(uip)+strlen(uport))*sizeof(char)+1);
		sprintf(reply,"RPL %s.%s;%s;%s",name,surname,uip,uport);
		fclose(ufile);
	}
	else {
		/* Query surname server */
		reply = qry_asrv(h, aport, surname);
		if(strcmp(reply,"") == 0) printf(ANSI_COLOR_RED "find_user: not found" ANSI_COLOR_RESET);
		if(sscanf(reply,"SRPL %[^';'];%[^';'];%s",surname,snpip,snpport) != 3) return "NOK - Reply not parseable";

		/* Query np server */	
		reply = qry_namesrv(snpip, snpport, name, surname);
		if(strcmp(reply,"") == 0) printf(ANSI_COLOR_RED "find_user: not found\n" ANSI_COLOR_RESET);
		/* return the reply*/
		printf("find_user: reply: %s\n",reply);
	}
	return reply;
}
/*
 * Function:  main
 * --------------------
 * Main program yo, dis' the server
 *
 *  h: structure with host information about the surname server
 *  aport: integer with surname server port
 *  surname: string with the surname to be unregistered 
 *
 *  returns:  0 if terminated cleanly
 *            -1 any other case
 *	TODO: Validate surname, snpip, snpport, saip, saport
 *		  Implement the protocol
 *        Implement exit command
 *        Implement list command
 *		  Comment
 *		  Create .h and .c auxiliary files	
 */
int main(int argc, char* argv[]) {
	int stop = 0;
	struct hostent *h;
	char options[10] = "n:s:q:i:p:";
	char localinput[512];
	char *surname = NULL ,*snpip = NULL ,*snpport = NULL,*saip = NULL,*saport = NULL;
	char *reply;
	char c;	
	int len,addrlen, ret, nread, port, aport;
	struct in_addr temp;
	struct sockaddr_in addr;
	char buffer[128];
	char *answer;
	/* File descriptors, pointer */
	int fd;
	fd_set rfds;
	int maxfd;

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

	/* Check if saip or saport are set, otherwise use default values */
	if(saip == NULL) {
		if((h=gethostbyname("tejo.tecnico.ulisboa.pt"))==NULL) {
			printf(ANSI_COLOR_RED "Couldn't reach default host\n" ANSI_COLOR_RESET);
			return -1;
		}
	}else{
		inet_pton(AF_INET, saip, &temp);
		if((h=gethostbyaddr(&temp,sizeof(temp),AF_INET))==NULL){
			printf(ANSI_COLOR_RED "Couldn't reach surname server\n" ANSI_COLOR_RESET);
			return -1;
		}
	}

	if(saport == NULL) {
		aport = 58000;
	}else{
		aport = atoi(saport);
	}

	/* Get port where our server will listen */
	if(sscanf(snpport,"%d\n",&port)!=1) {
		printf(ANSI_COLOR_RED "error: port is not a valid number\n" ANSI_COLOR_RESET);
		return -1;
	}
	
	/* Open UDP socket for our server */
	if((fd=socket(AF_INET,SOCK_DGRAM,0))==-1) {
		printf(ANSI_COLOR_RED "error: %s\n" ANSI_COLOR_RESET,strerror(errno));
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
		printf(ANSI_COLOR_RED "Error binding name to socket\n" ANSI_COLOR_RESET);
		close(fd);
		return -1;
	}

	/* Registering with surname server                                * 
	 * --NEEDS: Check if aport,surname,localip and snpport are valid  */
	printf("Attempting surname registering...\n");
	if(reg_sa(h,aport,surname,snpip,snpport)==-1) {
		printf(ANSI_COLOR_RED "error registering with surname server\n" ANSI_COLOR_RESET);	
		return -1;
	}

	if(reset_file() == -1) {
		printf(ANSI_COLOR_RED "error resetting file\n" ANSI_COLOR_RESET);
		close(fd);
		return -1;
	}

	/* Begin listening  */
	printf(ANSI_COLOR_GREEN "Welcome to our server!\n" ANSI_COLOR_RESET);
	while(!stop) {
		FD_ZERO(&rfds);
		FD_SET(fd,&rfds);
		FD_SET(STDIN_FILENO,&rfds);
		maxfd=fd;

		if(select(maxfd+1,&rfds,NULL,NULL,NULL)<0){
			printf(ANSI_COLOR_RED "Select() error: %s\n" ANSI_COLOR_RESET,strerror(errno));
			return -1;
		}

		if(FD_ISSET(fd,&rfds)){
			addrlen=sizeof(addr);
			nread=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,(socklen_t*)&addrlen);
			if(nread==-1) {
				printf(ANSI_COLOR_RED "Error receiving bytes, closing...\n" ANSI_COLOR_RESET);
				close(fd);
				return -1;
			}

			/* Parsing... */
			answer = malloc(3);
			sprintf(answer,"%.*s",3,buffer);

			if(strcmp(answer,"REG") == 0){
				reply = reg_user(buffer,nread);
				if(sendto(fd,reply,strlen(reply),0,(struct sockaddr*)&addr,addrlen) == -1){
					printf(ANSI_COLOR_RED "Error replying to user\n" ANSI_COLOR_RESET);
				}
			}else if(strcmp(answer,"UNR")==0){
				reply = unreg_user(buffer,nread);
				if(sendto(fd,reply,strlen(reply),0,(struct sockaddr*)&addr,addrlen) == -1) {
					printf(ANSI_COLOR_RED "Error replying to user\n" ANSI_COLOR_RESET);
				}
			}else if(strcmp(answer,"QRY")==0){
				reply = find_user(buffer,nread,h,aport);
				if(sendto(fd,reply,strlen(reply),0,(struct sockaddr*)&addr,addrlen) == -1){
					printf(ANSI_COLOR_RED "Error replying to user\n" ANSI_COLOR_RESET);
				}
			}else {
				printf(ANSI_COLOR_RED "Command not recognized\n" ANSI_COLOR_RESET);
				if(sendto(fd,"NOK",3,0,(struct sockaddr*)&addr,addrlen) == -1){
						printf(ANSI_COLOR_RED "Error replying to user\n" ANSI_COLOR_RESET);
					}
			}

			/*printf("Sending bytes back\n");*/
		/*	ret = sendto(fd,buffer,nread,0,(struct sockaddr*)&addr,addrlen);
			if(ret==-1) {
				printf(ANSI_COLOR_RED "Error sending bytes, closing...\n" ANSI_COLOR_RESET);
				close(fd);
				return -1;
			}*/
		}
		if(FD_ISSET(STDIN_FILENO,&rfds)){
			fgets(localinput,sizeof(localinput),stdin);
			len = strlen(localinput);
			if(localinput[len-1] == '\n') localinput[len-1] = '\0';
			if(strcmp(localinput,"exit") == 0) stop=1;
			else if(strcmp(localinput,"list") == 0) list_users();
		}
	}
	unreg_sa(h,aport,surname);	
	close(fd);
	printf(ANSI_COLOR_GREEN "Bye Bye :(\n" ANSI_COLOR_RESET);
	return 0;	
}
