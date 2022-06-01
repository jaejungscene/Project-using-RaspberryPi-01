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
#include "header/gpioRW.h"
// #include <sys/wait.h>


#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define PIN21 21
#define PIN20 20

void error_handling(char *message);
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

int main(int argc, char*argv[]){
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
   /*************************************************/

   /************** 모듈간 통신 활성화 ***************/
   #define MAX_MSG 2
      int sock[2];
      struct sockaddr_in slave_addr[2];
      char command[MAX_MSG];

      if(argc != 3){
         printf("1) <IP> : %s, <port> : %s\n", argv[1], argv[2]);
         printf("2) <IP> : %s, <port> : %s\n", argv[3], argv[4]);
      }

      sock[0] = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP // SOCK_DGRAM = UDP
      if(sock[0] == -1) error_handling("socket() error");
      memset(&slave_addr[0], 0, sizeof(slave_addr[0]));
      slave_addr[0].sin_family = AF_INET; //IPv4
      slave_addr[0].sin_addr.s_addr = inet_addr(argv[1]); // The inet_addr(const char *cp) function shall convert the string pointed to by cp, in the standard IPv4 dotted decimal notation, to an integer value suitable for use as an Internet address.
      slave_addr[0].sin_port = htons(atoi(argv[2])); //argv[2] = port number // htonl, htons : host byte order을 따르는 데이터를 network byte order로 변경한다.
      if(connect(sock[0], (struct sockaddr*)&slave_addr[0], sizeof(slave_addr[0])) == -1)
         error_handling("connect() error");

      // sock[1] = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP // SOCK_DGRAM = UDP
      // if(sock[1] == -1) error_handling("socket() error");
      // memset(&slave_addr[1], 0, sizeof(slave_addr[1]));
      // slave_addr[1].sin_family = AF_INET; //IPv4
      // slave_addr[1].sin_addr.s_addr = inet_addr(argv[3]); // The inet_addr(const char *cp) function shall convert the string pointed to by cp, in the standard IPv4 dotted decimal notation, to an integer value suitable for use as an Internet address.
      // slave_addr[1].sin_port = htons(atoi(argv[4])); //argv[2] = port number // htonl, htons : host byte order을 따르는 데이터를 network byte order로 변경한다.
      // if(connect(sock[1], (struct sockaddr*)&slave_addr[1], sizeof(slave_addr[0])) == -1)
      //    error_handling("connect() error");

      printf("** complete connecting **\n");
   /*************************************************/


   char exit[5];
   while(1)
   {
      printf(">> ");
      scanf("%s\n", exit);
      if(!strcmp(exit, "exit"))   break;


      /********** 알코올 감지센서 알고리즘 ***********/





      /*****************************************/


   
      /******* 버튼을 누르면 나머지 두 모듈에게 1(active)명령어 전달(QR코드를 찍어 킥보드를 탈수 있는 상황) *******/
      while (1)
      {
         value = GPIORead(PIN20);
         if(value == 0 && prev == 1) // press 할때
            break;
         prev = value;
         usleep(10000);
      }

      command[0] = '1'; // 1 is "active"
      write(sock[0], command, sizeof(command));
      // write(sock[1], command, sizeof(command));
      printf("send 1(active command)\n");
      /*************************************************/

      /******* 버튼을 한번 더 누르면 나머지 두 모듈에게 0(inactive)명령어 전달(킥보드를 반납하는 상황) *******/
      while (1)
      {
         value = GPIORead(PIN20);
         if(value == 0 && prev == 1) // press 할때
            break;
         prev = value;
         usleep(10000);
      }

      command[0] = '0'; // 0 is "inactive"
      write(sock[0], command, sizeof(command));
      // write(sock[1], command, sizeof(command));
      printf("send 0(inactive command)\n");
      /*************************************************/
   }

   close(sock[0]);
   // close(sock[1]);
   if (-1 == GPIOUnexport(PIN21) || -1 == GPIOUnexport(PIN20))
      return(4);
   printf("=========== finish ===========\n");
   
   // if(fork() == 0){
   //    if(fork() == 0){
   //       execv("./Two_Warning", NULL);
   //       printf("error\n");
   //       return -1;
   //    }
   //    else{
   //       execv("./Automatic_Report", NULL);
   //       printf("error\n");
   //       return -1;
   //    }
   // }
   // else{
   //    waitpid(-1, NULL, 0);
   //    sleep(10);
   //    printf("============= all finish =============\n");
   //    return(0);
   // }
   return 0;
}

void error_handling(char *message){
  fputs(message,stderr);
  fputc('\n',stderr);
  exit(1);
}