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
#define VALUE_MAX 40

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

   while (1)
   {
      /**
       * accept(2)
       * int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
       * accept() 함수는 연결지향 소켓 타입 (SOCK_STREAM, SOCK_SEQPACKET, SOCK_RDM)에 사용된다. 이것은 아직 처리되지 않은 연결들이 대기하고 있는 큐에서 제일 처음 연결된 연결을 가져와서 새로운 연결된 소켓을 만든다. 그리고 소켓을 가르키는 파일 지정자를 할당하고 이것을 리턴한다.
       * 인자 s 는 socket() 로 만들어진 end-point(듣기 소켓)을 위한 파일지정자이다.
       * 인자 addr 는 sockaddr 구조체에 대한 포인터이다. 연결이 성공되면 이 구조체를 채워서 되돌려 주게 되고, 우리는 이구조체의 정보를 이용해서 연결된 클라이언트의 인터넷 정보를 알아낼수 있다. addrlen 인자는 addr의 크기 이다.
       */
      if (clnt_sock < 0)
      {
         clnt_addr_size = sizeof(clnt_addr);
         printf("waiting...\n");
         if (clnt_sock == -2)
         {
            printf(">> ");
            fgets(msg, sizeof(msg), stdin);
            msg[strlen(msg) - 1] = '\0';
            if (!strcmp(msg, "exit"))
            {
               break;
            }
         }
         clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
         if (clnt_sock == -1)
            error_handling("accept() error");
      }

      printf("=== complete connection with %s ===\n", inet_ntoa(clnt_addr.sin_addr));

      // pthread_t receiver, sender;

      int str_len = -1;
      while (1)
      {
         printf(">> ");
         fgets(msg, sizeof(msg), stdin);
         write(clnt_sock, msg, sizeof(msg));
         printf("wait read...\n");
         str_len = read(clnt_sock, msg, sizeof(msg));
         if (str_len == -1)
         {
            error_handling("read() error");
            continue;
         }
         else if (!strcmp(msg, "exit"))
         {
            printf("** disconnect **\n");
            break;
         }
         printf("from client : %s\n", msg);
      }
      close(clnt_sock);
      clnt_sock = -2;
   }

   printf("== finish ==\n");
   close(serv_sock);

   return (0);
}