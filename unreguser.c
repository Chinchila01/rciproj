#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <netdb.h>

int main(){
	int fd,n,addrlen;
	struct sockaddr_in addr;
	struct hostent *h;
	struct in_addr *a;
	char buffer[128];

	if((h=gethostbyname("localhost"))==NULL)exit(1);
	a=(struct in_addr*)h->h_addr_list[0];

	printf("network address: %s\n",inet_ntoa(*a));

	fd=socket(AF_INET,SOCK_DGRAM,0);
	if(fd==-1)exit(1);

	memset((void*)&addr,(int)'\0',sizeof(addr));
	addr.sin_family=AF_INET;
	addr.sin_addr=*a;
	addr.sin_port=htons(9000);

	n=sendto(fd,"UNR afhah.lel",13,0,(struct sockaddr*)&addr,sizeof(addr));
	if(n==-1){
		printf("Unsuccessful\n");
		exit(1);
	}
	addrlen=sizeof(addr);
	n=recvfrom(fd,buffer,128,0,(struct sockaddr*)&addr,&addrlen);
	if(n==-1){
		printf("Unsuccessful 2\n");
		exit(1);
	}
	printf("%d\n",n);
	write(1,"echo: ",6);
	write(1,buffer,n);
	write(1,"\n",1);
	
	h=gethostbyaddr(&addr.sin_addr,sizeof(addr.sin_addr),AF_INET);
	if(h==NULL) {
		printf("sent by [%s:%hu]\n",inet_ntoa(addr.sin_addr),ntohs(addr.sin_port));
	}else{
		printf("sent by [%s:%hu]\n",h->h_name,ntohs(addr.sin_port));
	}

	close(fd);
	exit(0);
}
