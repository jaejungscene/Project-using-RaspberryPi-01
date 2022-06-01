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
#define PIN27 27

int button_switch = 0;

void *siren_thd(){
   printf("======= siren_thd start =======\n");
   GPIOExport(PIN27);
   GPIODirection(PIN27, OUT);
   int term = 500000; //반복 term
   int flag = 0;

   while(button_switch == 0){
      if(flag == 0){
         flag = 1;
         GPIOWrite(PIN27, flag);
      }
      else{
         flag = 0;
         GPIOWrite(PIN27, flag);
      }
      usleep(term);
      printf("button_switch %d\n", button_switch);
   }

   GPIOWrite(PIN27, LOW);
   GPIOUnexport(PIN27);
   printf("====== finish siren_thd ======\n");
   pthread_exit(NULL);
}

void *button_thd(){
   printf("======= button_thd start =======\n");
   int value;
   int prev = 1;
   if (-1 == GPIOExport(PIN21) || -1 == GPIOExport(PIN20))
      exit(1);
   if (-1 == GPIODirection(PIN21, OUT) || -1 == GPIODirection(PIN20, IN)) // PIN21 21, PIN20 20
      exit(2);
   if ( -1 == GPIOWrite (PIN21, 1)) // 처음 pin 21에 1을 write하면 이 값은 계속 유지된다.
      exit(3);

   while(1)
   {
      value = GPIORead(PIN20);
      printf ("value : %d, prev : %d\n", value, prev);
      if(prev == 1 && value == 0){ //button press
         if(button_switch == 0){
            button_switch = 1;
            break;
         }
      }
      prev = value;
      usleep(100000);
   }

   if (-1 == GPIOUnexport(PIN21) || -1 == GPIOUnexport(PIN20))
      exit(4);
   printf("====== finish button_thd ======\n");
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

   if (argc > 1)
      printf("<port> : %s\n", argv[1]);

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

      printf("=== complete connection with %s ===\n", inet_ntoa(clnt_addr.sin_addr));

      printf("wait read...\n");
      str_len = read(clnt_sock, msg, sizeof(msg));
      printf("from client : %s\n", msg); // <------------------------check
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
            if((fd = open("/home/pi/workspace/project/log.txt", O_WRONLY)) == -1){
               error_handling("open() error in emergency_log");
            }
            else{
               time(&t);
               snprintf(log, MAX_STR, ">>> %s user, at %s\n", inet_ntoa(clnt_addr.sin_addr), ctime(&t));
               printf("%s", log); // <--------------------- check
               lseek(fd, 0, SEEK_END);
               write(fd, log, strlen(log));
               close(fd);
            }
         }
      }
      close(clnt_sock);
      clnt_sock = -1;
      button_switch = 0;
   }

   printf("=========== server end ===========\n");
   close(serv_sock);

   return (0);
}