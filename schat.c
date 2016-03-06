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

typedef int bool;
#define true 1
#define false 0

// Program States
#define init 0 // Initial State
#define registered 1 // Initial State

int stateMachine = init;

int surDir_sock;

/*
 * Function:  show_usage
 * --------------------
 * Show the correct usage of the program command
 *
 *  returns:  -1 always (when this runs, it means something about the user's entered arguments went wrong)
 */
int show_usage(){
	printf("Usage: schat –n name.surname [–i ip] [-p scport] -s snpip -q snpport\n");
	return -1;

}

char * comUDP(char * msg, char * dst_ip, char * dst_port){
	struct sockaddr_in addr;
	struct in_addr *a;
	int n, addrlen;
	struct in_addr temp;
	struct hostent* h;
	char buffer[512];
	char * answer;

	/* Open UDP socket for our server */
	if((surDir_sock=socket(AF_INET,SOCK_DGRAM,0))==-1) {
		printf("Error: %s\n",strerror(errno));
		return NULL;
	}	
	
	inet_pton(AF_INET, dst_ip, &temp);
	h=gethostbyaddr(&temp,sizeof(temp),AF_INET);
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

	return answer;
}


bool usrRegister(char * snpip, char * port, char * full_name, char * my_ip, char * my_port){
	char * msg;
	char buffer[512];
	char * answer;

	sprintf(buffer,"REG %s;%s;%s\n",full_name,my_ip,my_port);

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

	printf("User registration successful!\n");

	return true;
}


bool usrExit(char * snpip, char * port, char * full_name){
	char buffer[512];
	char *answer;
	char * msg;

	sprintf(buffer,"UNR %s\n",full_name);

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
	FILE * fp_ip; // file for getting own ip
	char ip_path[1035], garb_0[1035], ip[1035];
	int lineCount = 2;

	char usrIn[100];
	char options[10] = "n:s:q:i:p:";
	char *in_name_surname = NULL ,*in_ip = NULL ,*in_scport = NULL,*in_snpip = NULL,*in_snpport = NULL;
	char c;

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
	if(in_name_surname == NULL || in_snpip == NULL || in_snpport == NULL) {
		show_usage();
		return -1;
	}

	if(in_ip == NULL) {
		/* Open the command for reading. */
		fp_ip = popen("ip a | grep wlan", "r");
		if (fp_ip == NULL) {
			printf("Cannot get local ip address.\n" );
			return(-1);
		}

		/* Read the output a line at a time - output it. */
		while (fgets(ip_path, sizeof(ip_path)-1, fp_ip) != NULL) {
			lineCount--;
			if (lineCount == 0){
				sscanf(ip_path,"%s %[^'/']%s",garb_0,ip);
			}
		}

		pclose(fp_ip);

		in_ip = ip;
	}

	if(in_scport == NULL) {
		in_scport = "6000";
	}



	printf("Connection Data:\nLocalhost:%s:%s\nSurname Server:%s:%s\n",in_ip,in_scport,in_snpip,in_snpport);

	while(1){
		fgets(usrIn,100,stdin);

		if (strcmp(usrIn,"join\n") == 0 && stateMachine == init){
			usrRegister(in_snpip, in_snpport,in_name_surname,in_ip,in_scport);

		}else if(strcmp(usrIn,"leave\n") == 0){
			usrExit(in_snpip, in_snpport,in_name_surname);

		}else if(strcmp(usrIn,"find\n") == 0){

		}else if(strcmp(usrIn,"connect\n") == 0){

		}else if(strcmp(usrIn,"message\n") == 0){

		}else if(strcmp(usrIn,"disconnect\n") == 0){

		}else if(strcmp(usrIn,"exit\n") == 0){
			// close connections
			break;
		}else{
			printf("You are not allowed to do that my friend.\n");
		}
	}

	printf("\n\nThe End\n\n\n");

	return 0;	
}