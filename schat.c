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

bool dirRegister(char * snpip, char * port){
	struct sockaddr_in addr;
	struct in_addr *a;
	int ret, n;
	char * msg = "Hello Baby!\n";
	struct in_addr temp;
	struct hostent* h;

	/* Open UDP socket for our server */
	if((surDir_sock=socket(AF_INET,SOCK_DGRAM,0))==-1) {
		printf("error: %s\n",strerror(errno));
		return -1;
	}	
	

	inet_pton(AF_INET, snpip, &temp);
	h=gethostbyaddr(&temp,sizeof(temp),AF_INET);
	a=(struct in_addr*)h->h_addr_list[0];

	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr=*a;
	addr.sin_port=htons(atoi(port));

	n=sendto(surDir_sock,msg,strlen(msg),0,(struct sockaddr*)&addr,sizeof(addr));
	if(n==-1) {
		printf("reg_sa: error sending to surname server\n");
		free(msg);
		close(surDir_sock);
		return -1;
	}

	/* Registering with surname server                                * 
	 * --NEEDS: Check if aport,surname,localip and snpport are valid  */
	printf("Attempting surname registering...\n");
	/*if(reg_sa(h,aport,surname,snpip,snpport)==-1) {
		printf("error registering with surname server\n");	
		return -1;
	}*/

	return true;
}

bool dirExit(){


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
	struct hostent *h;
	char options[10] = "n:s:q:i:p:";
	char *in_name_surname = NULL ,*in_ip = NULL ,*in_scport = NULL,*in_snpip = NULL,*in_snpport = NULL;
	char c;	
	int fd, addrlen, ret, nread, port, aport;
	struct sockaddr_in addr;
	char buffer[128];
	char *answer;
	int ufile = 0;

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
			dirRegister(in_snpip, in_snpport);

		}else if(strcmp(usrIn,"leave\n") == 0){

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