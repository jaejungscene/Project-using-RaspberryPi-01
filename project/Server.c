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
#include <time.h>
#include "header/mySocket.h"
#include "header/gpioRW.h"

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

#define PIN20 20
#define PIN21 21
#define SPEAKER_PIN 26

int button_switch = 0;

void *siren_thd(){
   printf("------ siren_thd start ------\n");
   GPIOExport(SPEAKER_PIN);
   GPIODirection(SPEAKER_PIN, OUT);
   int term = 500000; //반복 term
   int flag = 0;

   while(button_switch == 0){
      if(flag == 0){
         flag = 1;
         GPIOWrite(SPEAKER_PIN, flag);
      }
      else{
         flag = 0;
         GPIOWrite(SPEAKER_PIN, flag);
      }
      usleep(term);
      // printf("button_switch %d\n", button_switch);
   }
   GPIOWrite(SPEAKER_PIN, LOW);
   GPIOUnexport(SPEAKER_PIN);
   printf("------ finish siren_thd ------\n");
   pthread_exit(NULL);
}

void *button_thd(){
   printf("------ button_thd start ------\n");
   int value;
   int prev = 1;
   if (-1 == GPIOExport(PIN21) || -1 == GPIOExport(PIN20))  exit(1);
   if (-1 == GPIODirection(PIN21, OUT) || -1 == GPIODirection(PIN20, IN)) exit(2);
   if (-1 == GPIOWrite (PIN21, 1)) exit(3);

   while(1)
   {
      value = GPIORead(PIN20);
      // printf ("value : %d, prev : %d\n", value, prev);
      if(prev == 1 && value == 0){ //button press
         printf ("button press - 신고 확인 !\n", value, prev);
         if(button_switch == 0){
            button_switch = 1;
            break;
         }
      }
      prev = value;
      usleep(100000);
   }

   if (-1 == GPIOUnexport(PIN21) || -1 == GPIOUnexport(PIN20)) exit(4);
   printf("------ finish button_thd ------\n");
   pthread_exit(NULL);
}

/* ./server <server Port> */
#define MAX_STR 100
int main(int argc, char *argv[])
{
   int serv_sock, clnt_sock = -1; // socket filedescriptor
   struct sockaddr_in serv_addr, clnt_addr;
   socklen_t clnt_addr_size = sizeof(clnt_addr);
   char msg[MAX_STR];
   char log[MAX_STR];

   serverPrepare(&serv_sock, &serv_addr, &argv[1]);

   int str_len;
   int fd;
   time_t t;
   while (1)
   {
      if (clnt_sock < 0){
         printf("wait client...\n");
         clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
         if (clnt_sock == -1)
            error_handling("accept() error");
      }

      printf("** complete connection with %s **\n", inet_ntoa(clnt_addr.sin_addr));

      printf("wait read...\n");
      str_len = read(clnt_sock, msg, sizeof(msg));
      printf("received msg : %s\n", msg);
      if (str_len == -1){
         error_handling("read() error");
         continue;
      }
      else{
         if(!strcmp(msg, "accident")){ 
            /*** 안전사고 자동신고 신호 수신시 ***/
            pthread_t siren, button;
            printf("*** in %s, an emergency accident occurs!! ***\n", inet_ntoa(clnt_addr.sin_addr));
            pthread_create(&button, NULL, button_thd, NULL);
            pthread_create(&siren, NULL, siren_thd, NULL);
            pthread_join(button, NULL);
            pthread_join(siren, NULL);
         }
         else if(!strcmp(msg, "two")){
            /*** 2인 이상 신고 감지 신호 수신시 ***/
            if((fd = open("./log.txt", O_WRONLY)) == -1){
               error_handling("open() error in emergency_log");
            }
            else{
               time(&t);
               snprintf(log, MAX_STR, ">>> %s user, at %s\n", inet_ntoa(clnt_addr.sin_addr), ctime(&t));
               lseek(fd, 0, SEEK_END);
               write(fd, log, strlen(log));
               printf("write log in log file\n");
               close(fd);
            }
         }
      }
      close(clnt_sock); // 신호를 한번 받고 연결은 끊음
      clnt_sock = -1;
      button_switch = 0;
      printf("#######################################\n");
   }

   printf("=========== server end ===========\n");
   close(serv_sock);

   return (0);
}