#include <sys/time.h>
#include <unistd.h>
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

#define max(A,B) ((A)>=(B)?(A):(B))

typedef int bool;
#define true 1
#define false 0

// Program States
#define init 0 // Initial State
#define registered 1
#define onChat 2 

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
	printf("Usage: schat –n name.surname –i ip -p scport -s snpip -q snpport\n");
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
		printf("UDP error: %s\n",strerror(errno));
		return NULL;
	}	
	
	inet_pton(AF_INET, dst_ip, &temp);
	h=gethostbyaddr(&temp,sizeof(temp),AF_INET);
	a=(struct in_addr*)h->h_addr_list[0];

	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr=*a;
	addr.sin_port=htons(atoi(dst_port));

	printf("UDP Sending: %s\n", msg);

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

	printf("Raw answer: %s\n",answer);

	close(surDir_sock);

	return answer;
}


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
		return false;
	}

	printf("User registration successful!\n");

	return true;
}


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
	char buffer[512];
	char * name2connect;
	char * location;

	int fd, addrlen, newfd;
	struct sockaddr_in addr;
	int n, nw;
	char *ptr;

	int fd_out;
	struct sockaddr_in addr_out;
	char * dst_ip = "127.0.0.1";
	struct in_addr temp;
	struct hostent* h;
	struct in_addr *a;

	fd_set rfds;
	int maxfd, counter;

	char remote_port [24];
	char remote_ip [24];

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


	if((fd=socket(AF_INET,SOCK_STREAM,0))==-1)exit(1);//error
			
	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr.s_addr=htonl(INADDR_ANY);
	addr.sin_port=htons(atoi(in_scport));

	if(bind(fd,(struct sockaddr*)&addr,sizeof(addr))==-1){
		printf("ERROR: Cannot bind.\n");
		exit(1);//error
	}

	if(listen(fd,5)==-1){
		printf("ERRO\n");
		exit(1);//error
	}

	printf("Let's start..\n");

	while(1){

		FD_ZERO(&rfds);
		FD_SET(fileno(stdin),&rfds);
		FD_SET(fd,&rfds);
		
		maxfd = fileno(stdin);
		maxfd = max(maxfd,fd);

		counter=select(maxfd+1,&rfds,(fd_set*)NULL,(fd_set*)NULL,(struct timeval *)NULL);
 		
 		if(counter == -1){
 			printf("ERROR-: %s\n",strerror(errno));
 			exit(1);
 		}

 		if(FD_ISSET(fileno(stdin),&rfds)){
			printf("YOO\n");
			fgets(usrIn,100,stdin);
		}

		if(FD_ISSET(fd,&rfds)){
			printf("FDDDD ????\n");

			addrlen=sizeof(addr);

			if((newfd=accept(fd,(struct sockaddr*)&addr,&addrlen))==-1){
				printf("ERROR: Cannot start listening TCP.\n");
				exit(1); //error
			}

			FD_SET(newfd,&rfds);
			maxfd = max(maxfd,newfd);
			

		}

		if (newfd != NULL){

			if(FD_ISSET(newfd,&rfds)){
				printf("YOO ACCEPT YOO\n");
					
				memset(buffer, 0, sizeof(buffer));

				while((n=read(newfd,buffer,512))!=0){

				 	if(n==-1)
				 		break;//error
					printf("BAM: %s\n", buffer);

				 	/*ptr=&buffer[0];

				 	printf("estou bebado %d",n);
						
					while(n>0){
					 	if((nw=write(newfd,ptr,n))<=0){
					 		break;//error
					 	}

					 	n-=nw; ptr+=nw;
					 	printf("x");
					}*/
				}
			}
		}

		if (fd_out != NULL){
			if(FD_ISSET(fd_out,&rfds)){

			}
		}

		printf("BAZOU\n");

		if (strcmp(usrIn,"join\n") == 0 && stateMachine == init){
			usrRegister(in_snpip, in_snpport,in_name_surname,in_ip,in_scport);
			/* close(fd); exit(0); */

		}else if(strcmp(usrIn,"leave\n") == 0){
			usrExit(in_snpip, in_snpport,in_name_surname);
			close(newfd);

		}else if(strstr(usrIn,"find") != NULL){

			memset(buffer, 0, sizeof(buffer));

			sscanf(usrIn,"find %s", buffer);

			name2connect = malloc(strlen(buffer));

			name2connect = buffer;

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

		}else if(strstr(usrIn,"connect") != NULL){

			printf("Trying to locate user..\n");

			memset(buffer, 0, sizeof(buffer));

			sscanf(usrIn,"connect %s", buffer);

			name2connect = malloc(strlen(buffer));
			name2connect = buffer;

			if (strlen(name2connect) <= 1){
				printf("Argument missing. Who do you want to talk to?\n");
			}else{
				location = queryUser(in_snpip, in_snpport, name2connect);

				if (location == NULL){
					printf("Ups.. User not found.\n");
				}else{
					printf("User located at: %s\n", location);
					
					printf("Trying to connect..\n");
				}	
			}

			strcpy(buffer,location);

			sscanf(buffer,"%[^';'];%s",remote_ip, remote_port);

			fd_out=socket(AF_INET,SOCK_STREAM,0);//TCP socket

			if(fd_out==-1){
				printf("FAIL\n");
				return -1;//error
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

			if(stateMachine == onChat){
/*
			if((fd=socket(AF_INET,SOCK_STREAM,0))==-1)exit(1);//error
			
				memset((void*)&addr,(int)'\0',sizeof(addr));
				addr.sin_family=AF_INET;
				addr.sin_addr.s_addr=htonl(INADDR_ANY);
				addr.sin_port=htons(atoi(in_snpport));
				
				if(bind(fd,(struct sockaddr*)&addr,sizeof(addr))==-1)
					exit(1);//error
				
				if(listen(fd,5)==-1)exit(1);//error
				
				while(1){addrlen=sizeof(addr);
				
				if((newfd=accept(fd,(struct sockaddr*)&addr,&addrlen))==-1)
					exit(1);//error
				
				while((n=read(newfd,buffer,128))!=0){if(n==-1)exit(1);//error
					ptr=&buffer[0];
				
				while(n>0){if((nw=write(newfd,ptr,n))<=0)exit(1);//error
					n-=nw; ptr+=nw;}
				}
					close(newfd);
				}*/
			}
		}else if(strstr(usrIn,"message") != NULL){

			sscanf(usrIn,"message %s\n", buffer);

			write(fd_out,buffer,strlen(buffer)); //stdout

			memset(buffer, 0, sizeof(buffer));

		}else if(strcmp(usrIn,"disconnect\n") == 0){

		}else if(strcmp(usrIn,"exit\n") == 0){

			// checkar se nao fizemos leave antes !!!!!!!!!!!!! - maquina de estados
			usrExit(in_snpip, in_snpport,in_name_surname);
			close(newfd);
			close(fd);
			close(fd_out);

			printf("Exiting..\n");
			break;
		}else{
			printf("You are not allowed to do that my friend.\n");
		}
	}

	printf("\n\nThe End\n\n\n");

	return 0;	
}