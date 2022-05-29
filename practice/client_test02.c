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

void clientConnecting(int *sock, struct sockaddr_in *dest_addr, char *args[]){
   *sock = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP // SOCK_DGRAM = UDP
   if(*sock == -1) error_handling("socket() error");
   memset(dest_addr, 0, sizeof(*dest_addr));
   dest_addr->sin_family = AF_INET; //IPv4
   dest_addr->sin_addr.s_addr = inet_addr(args[0]); // The inet_addr(const char *cp) function shall convert the string pointed to by cp, in the standard IPv4 dotted decimal notation, to an integer value suitable for use as an Internet address.
   dest_addr->sin_port = htons(atoi(args[1])); //argv[2] = port number // htonl, htons : host byte order을 따르는 데이터를 network byte order로 변경한다.

   if(connect(*sock, (struct sockaddr*)dest_addr, sizeof(*dest_addr)) == -1)
      error_handling("connect() error");
   printf("** complete connecting with %s **\n", args[0]);
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
    }

    clientConnecting(&sock, &serv_addr, &argv[1]);

    while(1){
        printf("wait read...\n");
        str_len = read(sock, msg, sizeof(msg));
        if(str_len == -1)
            error_handling("read() error");
        else 
            printf("from server : %s", msg);
        printf(">> ");
        fgets(msg, sizeof(msg), stdin);
        msg[strlen(msg)-1] = '\0';
        write(sock, msg, sizeof(msg));
        if(0 == strcmp(msg, "exit")){
            printf("** disconnect **\n");
            break;
        }
    }
    printf("== finish ==\n");
    close(sock);

    return 0;
}