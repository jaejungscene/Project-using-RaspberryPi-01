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
#include "header/mySocket.h"
#include "header/gpioRW.h"

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define PIN20 20
#define PIN21 21
#define PIN27 27

int button_switch = 0;

void *siren_thd(){
   while(button_switch == 0){
      GPIOExport(PIN27);
      GPIODirection(PIN27, OUT);

      int repeat = 4; //반복 횟수
      int period = 300000; //반복 주기
      for(int i=0; i<repeat; i++){
         GPIOWrite(PIN27, i%2);
         usleep(period);
      }

      GPIOWrite(PIN27, LOW);
      GPIOUnexport(PIN27);
   }
}


/* ./server <server Port> */
#define MAX_STR 50
int main(int argc, char *argv[])
{
   int serv_sock, clnt_sock = -1; // socket filedescriptor
   struct sockaddr_in serv_addr, clnt_addr;
   socklen_t clnt_addr_size;
   char msg[MAX_STR];
   char log[MAX_STR];
   char promp[5] = ">>> ";

   if (argc > 1)
      printf("<port> : %s\n", argv[1]);

   serverPrepare(&serv_sock, &serv_addr, &argv[1]);

   int str_len;
   int fd;
   while (1)
   {
      if (clnt_sock < 0){
         clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
         if (clnt_sock == -1)
            error_handling("accept() error");
      }

      printf("=== complete connection with %s ===\n", inet_ntoa(clnt_addr.sin_addr));

      printf("wait read...\n");
      str_len = read(clnt_sock, msg, sizeof(msg));
      if (str_len == -1){
         error_handling("read() error");
         continue;
      }
      else(str_len){
         if(!strcmp(msg, "accident")){ 
            /*** 안전사고 자동신고 신호 ***/
            printf("*** in %s, an emergency accident occurs!! ***\n", inet_ntoa(clnt_addr.sin_addr));
         }
         if(!strcmp(msg, "two")){
            /** 2인 이상 신고 감지 신호 **/
            if((fd = open("/home/pi/workspace/project/more_than_two_log", O_WRONLY)) == -1){
               error_handling("open() error in emergency_log");
            }
            else{
               printf("*** in %s, an emergency accident occurs!! ***\n", inet_ntoa(clnt_addr.sin_addr));
               lseek(fd, 0, SEEK_END);
               snprintf(log, MAX_STR, "in %s, an emergency accident occurs!!\n", inet_ntoa(clnt_addr.sin_addr));
               write(fd, promp, strlen(promp));
               write(fd, str, strlen(str));
               printf("=== finish ===\n");
               close(fd);
            }
         }
         printf("from client : %s\n", msg);
      }
      close(clnt_sock);
      clnt_sock = -1;
   }

   printf("=========== server end ===========\n");
   close(serv_sock);

   return (0);
}