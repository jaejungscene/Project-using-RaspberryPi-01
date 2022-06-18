/*****************************************
 * 버튼을 누를 때 전동킥보드가 활성화된다고 가정한다.
 * 정해진 시간동안 알코올 감지센서의 값이 특정 값보다 낮게 나와야지
 * 버튼을 눌러서 다른 모튤들을 활성화시킬 수 있다.
 *****************************************/

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>
#include "header/gpioRW.h"
// #include <sys/wait.h>


#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define SPEAKER_PIN 16 //스피커
#define PIN21 21
#define PIN20 20
#define PIN27 27 //알코올 

int exit_signal = 1;

int sock[2];
struct sockaddr_in slave_addr[2];
char command[2];

void error_handling(char *message);
void *exit_worker(){
   char str_exit[2];
   while(1){
      scanf("%s", str_exit);
      if(!strcmp(str_exit, "e")){
         break;
      }
   }
   write(sock[0], str_exit, strlen(str_exit)+1);
   write(sock[1], str_exit, strlen(str_exit)+1);
   printf("send e(exit command) to two slaves\n");
   exit_signal = 0;
}

// ./Main_Alcohol <slaves IP> <slave1 port> <slave2 port>
int main(int argc, char*argv[]){
   printf("========== main alcohol start ==========\n");
   
   /************** 버튼 활성화 ***************/
   int state = 1;
   int light = 0;
   int prev = 1; //button을 press하고 땔 때
   int value;  

   if (-1 == GPIOExport(PIN21) || -1 == GPIOExport(PIN20))
      return(1);
   if (-1 == GPIODirection(PIN21, OUT) || -1 == GPIODirection(PIN20, IN))
      return(2);
   if ( -1 == GPIOWrite (PIN21, 1)) // 처음 pin 21에 1을 write하면 이 값은 계속 유지된다.
      return(3);

   /************** 스피커 활성화 ***************/
   GPIOExport(SPEAKER_PIN);
   GPIODirection(SPEAKER_PIN, OUT);

   /************** 알코올센서 활성화 ***************/
   GPIOExport(PIN27);
   GPIODirection(PIN27, IN);
   /*************************************************/

   /******************** 모듈간 통신 활성화 ***********************/
   sock[0] = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP // SOCK_DGRAM = UDP
   if(sock[0] == -1) error_handling("socket() error");
   memset(&slave_addr[0], 0, sizeof(slave_addr[0]));
   slave_addr[0].sin_family = AF_INET; //IPv4
   slave_addr[0].sin_addr.s_addr = inet_addr(argv[1]);
   slave_addr[0].sin_port = htons(atoi(argv[2]));
   if(connect(sock[0], (struct sockaddr*)&slave_addr[0], sizeof(slave_addr[0])) == -1)
      error_handling("connect() error");

   sock[1] = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP // SOCK_DGRAM = UDP
   if(sock[1] == -1) error_handling("socket() error");
   memset(&slave_addr[1], 0, sizeof(slave_addr[1]));
   slave_addr[1].sin_family = AF_INET; //IPv4
   slave_addr[1].sin_addr.s_addr = inet_addr(argv[1]);
   slave_addr[1].sin_port = htons(atoi(argv[3]));
   if(connect(sock[1], (struct sockaddr*)&slave_addr[1], sizeof(slave_addr[1])) == -1)
      error_handling("connect() error");

   printf("** complete connecting with two slaves **\n");
   /***********************************************************/

   pthread_t exit_thd;
   pthread_create(&exit_thd, NULL, exit_worker, NULL);

   while(exit_signal)
   {
      printf("#################################\n");
      /********** 알코올 감지센서 알고리즘 ***********/
      printf("button wait for Alcohol Check...\n");
      while(exit_signal){
         value = GPIORead(PIN20);
         if(value == 0 && prev == 1) // press
            break;
         prev = value;
         usleep(10000);
      }

      if(exit_signal == 0) // 종료
         break;

      //스피커 울리기
      GPIOWrite(SPEAKER_PIN, HIGH);
      sleep(1);
      GPIOWrite(SPEAKER_PIN, LOW);

      int cnt = 0;
      int state = 0;
      // printf("alcohol checking...\n");
      // while(cnt < 5) { // 5초
      //    if( (value = GPIORead(PIN27)) == 0){
      //       state = 1; //alcohol detected!!
      //       break;
      //    }
      //    cnt++;
      //    sleep(1);
      // }

      if(exit_signal == 0) // 종료
         break;

      if(state){
      /****** alcoholdl detect되어 5초간 스피커 울림 ******/
         printf("**** Alcohol is detected!!! ****\n");

         int repeat = 10; // 반복 횟수
         int period = 300000; // 반복 term
         for(int i=0; i<repeat; i++){
            GPIOWrite(SPEAKER_PIN, i%2);
            usleep(period);
         }
         GPIOWrite(SPEAKER_PIN, LOW);
      }
      else{
      /******* 버튼을 누르면 나머지 두 모듈에게 1(active)명령어 전달 <QR코드를 찍는 행위> *******/
         printf("** Alcohol is not detected **\n");
         printf("button(active) wait...\n");
         cnt = 0;
         while (exit_signal)
         {
            if(cnt >= 10000){ // 10초 지나면
               break;
            }
            value = GPIORead(PIN20);
            if(value == 0 && prev == 1) // press 할때
               break;
            prev = value;
            usleep(1000);
            cnt++;
         }
         
         if(exit_signal == 0) // 종료
            break;

         if(cnt >= 10000){ // 10초 동안 press하지 않으면 다시 alcohol감사
            printf("** timeout!! **\n");
            continue;
         }

         strcpy(command, "1"); // 1 is "active"
         write(sock[0], command, strlen(command)+1);
         write(sock[1], command, strlen(command)+1);
         printf("send 1(active command)\n");
         usleep(1000000);
         /*************************************************/


         /******* 버튼을 한번 더 누르면 나머지 두 모듈에게 0(inactive)명령어 전달 <킥보드를 반납하는 행위> *******/
         printf("button(inactive) wait...\n");
         while (exit_signal)
         {
            value = GPIORead(PIN20);
            if(value == 0 && prev == 1) // press 할때
               break;
            prev = value;
            usleep(10000);
         }

         if(exit_signal == 0) // 종료
            break;

         strcpy(command, "0"); // 0 is "inactive"
         write(sock[0], command, strlen(command)+1);
         write(sock[1], command, strlen(command)+1);
         printf("send 0(inactive command)\n");

         prev = 1;
         usleep(1000000);
         /*************************************************/
      }

   }

   if (-1 == GPIOUnexport(PIN21) || -1 == GPIOUnexport(PIN20) \
      -1 == GPIOUnexport(SPEAKER_PIN)|| -1 == GPIOUnexport(PIN27))
            return(4);
   close(sock[0]);
   close(sock[1]);

   printf("========= finish main alcohol ========\n");
   
   return 0;
}

void error_handling(char *message){
  fputs(message,stderr);
  fputc('\n',stderr);
  exit(1);
}