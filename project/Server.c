#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define PIN 20
#define POUT 21

void error_handling(char *message)
{
   fputs(message, stderr);
   fputc('\n', stderr);
   exit(1);
}

void serverPrepare(int *sock, struct sockaddr_in *my_addr, char *args[])
{                                           // 1) create socket  ,  2) bind ,  3) listen
   *sock = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP
   if (*sock == -1)
      error_handling("socket() error");
   memset(my_addr, 0, sizeof(*my_addr));
   my_addr->sin_family = AF_INET;
   my_addr->sin_addr.s_addr = htonl(INADDR_ANY);
   my_addr->sin_port = htons(atoi(args[0]));

   if (bind(*sock, (struct sockaddr *)my_addr, sizeof(*my_addr)) == -1)
      error_handling("bind() error");

   if (listen(*sock, 5) == -1)
      error_handling("listen() error");
}

/* ./server <server Port> */
#define MAX_MSG 30
int main(int argc, char *argv[])
{
   int serv_sock, clnt_sock = -1; // socket filedescriptor
   struct sockaddr_in serv_addr, clnt_addr;
   socklen_t clnt_addr_size;
   char msg[MAX_MSG];

   if (argc != 1)
   {
      printf("<port> : %s\n", argv[1]);
   }

   serverPrepare(&serv_sock, &serv_addr, &argv[1]);

   int str_len;
   while (1)
   {
      if (clnt_sock < 0)
      {
         clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
         if (clnt_sock == -1)
            error_handling("accept() error");
      }

      printf("=== complete connection with %s ===\n", inet_ntoa(clnt_addr.sin_addr));

      printf("wait read...\n");
      while (1)
      {
         str_len = read(clnt_sock, msg, sizeof(msg));
         if (str_len == -1)
         {
            error_handling("read() error");
            continue;
         }
         else if (str_len > 0)
         {
            if(!strcmp(msg, "accident")){ // 안전사고 자동신고 신호
               /** 사이렌 **/
            }
            if(!strcmp(msg, "two")){ // 2인 이상 신고 감지 신호
               /** 로그 입력 **/
            }
            printf("from client : %s\n", msg);
            break;
         }
      }
      close(clnt_sock);
      clnt_sock = -1;
   }

   printf("== finish ==\n");
   close(serv_sock);

   return (0);
}