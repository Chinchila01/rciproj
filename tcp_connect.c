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

int main(int argc, char* argv[]){

	int fd, n;
	struct sockaddr_in addr;
	char * dst_ip = "127.0.0.1";
	struct in_addr temp;
	struct hostent* h;
	struct in_addr *a;


	if (argc <= 1){
		printf("Missing arguments.\n");
		return -1;
	}

	fd=socket(AF_INET,SOCK_STREAM,0);//TCP socket

	if(fd==-1){
		printf("FAIL\n");
		return -1;//error
	}

	inet_pton(AF_INET, dst_ip, &temp);
	h=gethostbyaddr(&temp,sizeof(temp),AF_INET);
	a=(struct in_addr*)h->h_addr_list[0];

	memset((void*)&addr,(int)'\0',sizeof(addr));

	addr.sin_family=AF_INET;
	addr.sin_addr=*a;
	addr.sin_port=htons(7000);

	n=connect(fd,(struct sockaddr*)&addr,sizeof(addr));

	while (1){

		write(fd,argv[1],strlen(argv[1]));//stdout

		sleep(1);

		if(n==-1){
			printf("FAIL\n");
			return -1;
		}

	}

	printf("LIGOU\n");

	return 0;

}