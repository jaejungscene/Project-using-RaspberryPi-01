#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <string.h> 
#include <arpa/inet.h>
#include <sys/socket.h>

#define IN 0 
#define OUT 1 
#define LOW 0
#define HIGH 1
#define PIN 20
#define POUT 21
#define VALUE_MAX 40

void error_handling(char *message){
  fputs(message,stderr);
  fputc('\n',stderr);
  exit(1);
}

/* ./server <server Port> */
#define MAX_MSG 30
int main(int argc, char *argv[])
{ 
  int serv_sock, clnt_sock=-1; // socket filedescriptor
  struct sockaddr_in serv_addr,clnt_addr; 
  socklen_t clnt_addr_size;
  char msg[MAX_MSG];

  if(argc!=1){
    printf("<port> : %s\n",argv[1]);
    exit(1);
  }

  printf("01\n");

  serv_sock = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP
  if(serv_sock == -1) error_handling("socket() error");
  memset(&serv_addr, 0 , sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));

  printf("02\n");

  /* bind */
  if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1)
    error_handling("bind() error");

  printf("03\n");

  if(listen(serv_sock,5) == -1) error_handling("listen() error");

  printf("04\n");

  if(clnt_sock<0){
    clnt_addr_size = sizeof(clnt_addr);
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if(clnt_sock == -1)
      error_handling("accept() error");
  }

  printf("05\n");

  while(1){
    printf(">> ");
    fgets(msg, sizeof(msg), stdin);
    write(clnt_sock, msg, sizeof(msg));
  }

  close(clnt_sock);
  close(serv_sock);

  return(0);
}