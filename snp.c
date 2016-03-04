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
extern int errno;

char* servername;
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
		printf("reg_sa: error building string\n");
		free(msg);
		return -1;
	}
	
	a=(struct in_addr*)h->h_addr_list[0];
	fd=socket(AF_INET,SOCK_DGRAM,0);
	if(fd==-1) {
		printf("reg_sa: error opening socket\n");
		free(msg);
		return -1;
	}

	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr=*a;
	addr.sin_port=htons(aport);

	n=sendto(fd,msg,msglen,0,(struct sockaddr*)&addr,sizeof(addr));
	if(n==-1) {
		printf("reg_sa: error sending to surname server\n");
		free(msg);
		close(fd);
		return -1;
	}

	addrlen = sizeof(addr);
	n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,(socklen_t*)&addrlen);
	if(n==-1) {
		printf("reg_sa: error receiving from surname server\n");
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
	printf("Successful!\n");
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
		printf("reg_sa: error opening socket\n");
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
		printf("reg_sa: error printing second message\n");
		free(msg);
		close(fd);
		return -1;
	}	

	/* Sending it to the Surname Server */
	n=sendto(fd,msg,msglen,0,(struct sockaddr*)&addr,sizeof(addr));
	if(n==-1) {
		printf("reg_sa: error sending to surname server\n");
		free(msg);
		close(fd);
		return -1;
	}

	/* Receiving to check if OK */
	addrlen = sizeof(addr);
	n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,(socklen_t*)&addrlen);
	if(n==-1) {
		printf("reg_sa: error receiving from surname server\n");
		free(msg);
		close(fd);
		return -1;
	}

	answer = malloc(n);

	sprintf(answer,"%.*s",n,buffer);
	if(strcmp(answer,"OK") != 0){
		printf("%s\n",answer);
		free(msg);
		free(answer);
		close(fd);
		return -1;
	}

	free(msg);	
	free(answer);
	close(fd);
	printf("Successful!\n");
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
int reg_user(char* buffer, int n, FILE *fd) {
	char name[128], surname[128], ip[128], port[128];
	buffer[n] = '\0';
	printf("\nreg_user: buffer received: %s\n",buffer);
	if(sscanf(buffer,"REG %[^'.'].%[^';'];%[^';'];%s",name,surname,ip,port)!= 4){
		printf("reg_user: Message format not recognized\n");
		return -1;
	}
	printf("name: %s\n",name);
	printf("surname: %s\n",surname);
	printf("ip: %s\n",ip);
	printf("port: %s\n",port);

	if(strcmp(servername,surname) != 0){
		printf("reg_user: Surname and servername don't match\n");
		return -1;
	}
	fprintf(fd,"%s;%s;%s\n",name,ip,port);
	return -1;	
}

/* Function: unreg_user
*  --------------------
*  Unregisters a user
*/
int unreg_user();

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
	struct hostent *h;
	char options[10] = "n:s:q:i:p:";
	char *surname = NULL ,*snpip = NULL ,*snpport = NULL,*saip = NULL,*saport = NULL;
	char c;	
	int fd, addrlen, ret, nread, port, aport;
	struct sockaddr_in addr;
	char buffer[128];
	char *answer;
	FILE *ufile;

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
			printf("Couldn't reach default host\n");
			return -1;
		}
	}

	if(saport == NULL) {
		aport = 58000;
	}

	/* Get port where our server will listen */
	if(sscanf(snpport,"%d\n",&port)!=1) {
		printf("error: port is not a valid number\n");
		return -1;
	}
	
	/* Open UDP socket for our server */
	if((fd=socket(AF_INET,SOCK_DGRAM,0))==-1) {
		printf("error: %s\n",strerror(errno));
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
		printf("Error binding name to socket\n");
		close(fd);
		return -1;
	}

	/* Registering with surname server                                * 
	 * --NEEDS: Check if aport,surname,localip and snpport are valid  */
	printf("Attempting surname registering...\n");
	if(reg_sa(h,aport,surname,snpip,snpport)==-1) {
		printf("error registering with surname server\n");	
		return -1;
	}

	/* Opening server file */
	ufile = fopen("clientlist","w+");
	if(ufile == NULL) {
		printf("Error opening client file: %s\n", strerror(errno));
		close(fd);
		return -1;
	}		

	/* Begin listening  */
	printf("Welcome to our server!\n");
	while(1) {
		addrlen=sizeof(addr);
		nread=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,(socklen_t*)&addrlen);
		if(nread==-1) {
			printf("Error receiving bytes, closing...\n");
			close(fd);
			return -1;
		}
		write(1,"echo: ",6);
		write(1,buffer,nread);

		/* Parsing... */
		answer = malloc(3);
		sprintf(answer,"%.*s",3,buffer);
		printf("answer is: %s\n",answer);	
		if(strcmp(answer,"REG") == 0){
			if(reg_user(buffer,nread,ufile) == -1) {
				printf("error registering user\n");
				return -1;
			}
		}else if(strcmp(answer,"UNR")==0){
			/*unreg_user();*/
			printf("lel1\n");
		}else {
			printf("Command not recognized\n");
		}

		printf("Sending bytes back\n");
		ret = sendto(fd,buffer,nread,0,(struct sockaddr*)&addr,addrlen);
		if(ret==-1) {
			printf("Error sending bytes, closing...\n");
			close(fd);
			return -1;
		}
	}
	
	close(fd);
	return 0;	
}
