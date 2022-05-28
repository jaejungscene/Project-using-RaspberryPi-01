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
#define POUT 17

#define VALUE_MAX 30

void error_handling(char *message){
  fputs(message,stderr);
  fputc('\n',stderr);
  exit(1);
}

/* ./client <server IP> <server Port> */
#define MAX_MSG 30
int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    char msg[MAX_MSG];
    int str_len;

    if(argc != 3){
        printf("<IP> : %s, <port> : %s\n", argv[1], argv[2]);
        exit(1);
    }

    sock = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP
    if(sock == -1) error_handling("socket() error");
    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET; //IPv4
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2])); //argv[2] = port number

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("connect() error");
    }
    printf("** complete connecting **\n");

    while(1){
        str_len = read(sock, msg, sizeof(msg));
        if(str_len == -1){
            error_handling("read() error");
        }
        printf("Receive message from server : %s\n", msg);
    }

    close(sock);

    return 0;
}