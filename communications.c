#include "communications.h"

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
	int surDir_sock;

	/* Open UDP socket for our server */
	if((surDir_sock=socket(AF_INET,SOCK_DGRAM,0))==-1) {
		printf("UDP error: %s\n",strerror(errno));
		return NULL;
	}	
	
	// get host info
	inet_pton(AF_INET, dst_ip, &temp);
	if((h=gethostbyaddr(&temp,sizeof(temp),AF_INET)) == NULL) {
		printf("UDP error: sending getting host information\n");
		close(surDir_sock);
		return NULL;
	}

	a=(struct in_addr*)h->h_addr_list[0];

	// set remote host address and port for connection
	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr=*a;
	addr.sin_port=htons(atoi(dst_port));

	printf("UDP Sending: %s\n", msg);

	// all set up, send message
	n=sendto(surDir_sock,msg,strlen(msg),0,(struct sockaddr*)&addr,sizeof(addr));

	// check for errors on message send
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

	// get incoming message response
	addrlen = sizeof(addr);
	n=recvfrom(surDir_sock,buffer,512,0,(struct sockaddr*)&addr,(socklen_t*)&addrlen);

	// check for errors on message receive
	if(n==-1) {
		printf("UDP error: receiving message to the server\n");
		close(surDir_sock);
		return NULL;
	}


	answer=malloc(n+1); // alocate answer according to received bytes
	sprintf(answer,"%.*s",n,buffer);

	printf("Raw answer: %s\n",answer);

	close(surDir_sock); // close udp socket
	free(msg);
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

	// format user register message
	sprintf(buffer,"REG %s;%s;%s",full_name,my_ip,my_port);

	msg = malloc(strlen(buffer));
	msg = buffer;

	// send message through comUDP function
	answer = comUDP(msg, snpip, port);

	// check for error
	if (answer == NULL){
		printf("UDP error: empty message received\n");
		return false;
	}

	// check for ack or error message
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

	// format user unregister message
	sprintf(buffer,"UNR %s",full_name);

	msg = malloc(strlen(buffer));
	msg = buffer;

	// send message through comUDP function
	answer = comUDP(msg, snpip, port);

	// check for error
	if (answer == NULL){
		printf("UDP error: empty message received\n");
		return false;
	}

	// check for ack or error message
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

	// format user query message
	sprintf(buffer,"QRY %s",user);

	msg = malloc(strlen(buffer));
	msg = buffer;

	// send message through comUDP function
	answer = comUDP(msg, snpip, port);

	// check for error
	if (answer == NULL || strcmp(answer,"") == 0){
		printf("UDP error: empty message received\n");
		return "";
	}

	// check for query problems (ex: user not found)
	if(strstr(answer,"NOK") != NULL) {
		printf("Error: %s\n",answer);
		free(answer);
		return "";
	}

	memset(buffer, 0, sizeof(buffer));

	// process information and store address data on buffer
	sscanf(answer,"RPL %[^';'];%s", garb_0, buffer);

	location = malloc(strlen(buffer));

	strcpy(location, buffer);

	free(answer);
	return location;
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
	free(answer);
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
*              char* serverfile -> string with name of file where the server stores names
*			   char* servername -> string with surname associated with the server
*
*  Returns: "OK" if successful
*	       "NOK - " + error specification if error occurs
*/
char* reg_user(char* buffer, int n, char* serverfile, char* servername) {
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
*			   char* serverfile -> string with name of file where the server stores names
*			   char* servername -> string with surname associated with the server
*
*  returns: "OK" if unregister is successful
*          "NOK - " + error specification if error occurs
*/
char* unreg_user(char *buffer, int n, char* serverfile, char* servername){
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
		printf(ANSI_COLOR_RED "qry_asrv: error sending contacting surname server. Are you sure the address is right?");
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
char* find_user(char* buffer,int n,char* saip, char* saport, char* serverfile, char* servername) {
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