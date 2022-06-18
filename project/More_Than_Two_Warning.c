#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "header/spiRW.h"
#include "header/mySocket.h"
#include "header/gpioRW.h"

#define MAX_MSG 30
#define FRONT_PIN 17
#define BACK_PIN 22
#define SPEAKER_PIN 27
#define WAIT_TIME 300000
#define IN 0
#define OUT 1
#define LOW 0


int signal_from_main = 0;
int fd; //adc fd
int front_value; // value of left pressure sensor
int back_value; // value of right pressur esensor


void *alert_to_server(void *argv){
   printf("----- alert thread start -----\n");

   int sock;
   struct sockaddr_in serv_addr;
   int str_len;

   sock = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP // SOCK_DGRAM = UDP
   if(sock == -1) {
      error_handling("socket() error");
   }
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET; //IPv4
   serv_addr.sin_addr.s_addr = inet_addr(((char**)argv)[2]); // <-- 변경!!!!!!!!!
   serv_addr.sin_port = htons(atoi(((char**)argv)[3])); // <-- 변경!!!!!!!!

   printf("%s : %s\n", ((char**)argv)[2], ((char**)argv)[3]);
   while(signal_from_main == 1){
      if(front_value && back_value){
         if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
            error_handling("connect() error");
         }

         printf("2인이상 탑승중!");

         printf("** send \'two\' msg to server **\n");
         write(sock, "two", sizeof("two"));
         close(sock);
         break;
      }
   }
   printf("----- finish alert thread -----\n");
   pthread_exit(NULL);
}


void *speaker_warning(){
   printf("----- speaker thread start -----\n");

   int i=1;
   while(signal_from_main){
      if(front_value && back_value){
         GPIOWrite(SPEAKER_PIN, i);
         if(i == 1){
            i = 0;
            usleep(100000);
         }
         else{
            i = 1;
            usleep(50000);
         }
      }
      else
         GPIOWrite(SPEAKER_PIN, LOW);
   }

   GPIOWrite(SPEAKER_PIN, LOW);

   printf("----- finish speaker thread -----\n");
   pthread_exit(NULL);
}

void *weight_sensor_worker(void *param){
   printf("----- weight thread start -----\n");


   while(signal_from_main == 1){
      if((long)param == FRONT_PIN){
         front_value = GPIORead(FRONT_PIN);   // 앞쪽 무게센서가 감지되면, front_value == 1
         // printf("front value : %d\n", front_value);
      }
      if((long)param == BACK_PIN){
         back_value = GPIORead(BACK_PIN);    // 뒨쪽 무게센서가 감지되면, front_value == 1
         // printf("back value : %d\n", back_value);
      }

      // while(front_value && back_value){
      //    speaker_flag = 1;  //스피커 출력
      //    if((GPIORead(FRONT_PIN) + GPIORead(BACK_PIN)) < 2){  // 다시 1인 이하탑승일시 경고음 중지
      //       speaker_flag = 0; // 스피커 중지
      //    }
      //    usleep(WAIT_TIME);
      // }
      usleep(WAIT_TIME);
   }

   printf("----- finish weight thread -----\n");
   pthread_exit(NULL);
}

// ./(file name) <port to open> <Server ip> <Server port>
int main(int argc, char *argv[])
{
   printf("===== more than two warning start =====\n");
   pthread_t alert, front_weight_sensor, back_weight_sensor, speaker;

   int serv_sock, clnt_sock = -1; // socket filedescriptor
   struct sockaddr_in serv_addr, clnt_addr;
   socklen_t clnt_addr_size = sizeof(clnt_addr);
   char msg[MAX_MSG];

   serverPrepare(&serv_sock, &serv_addr, &argv[1]);
   clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
   if (clnt_sock == -1) error_handling("accept() error");
   printf("** complete connection with %s **\n", inet_ntoa(clnt_addr.sin_addr));

   /**
    * Automatic_Report와 겹치는 센서이다보니
    * GPIO Unexport와 Export가 겹치는 상황이
    * 생겨 thread 내에 작성하지 않음
    **/
   GPIOExport(FRONT_PIN);
   GPIODirection(FRONT_PIN, IN);
   GPIOExport(BACK_PIN);
   GPIODirection(BACK_PIN, IN);
   GPIOExport(SPEAKER_PIN);
   GPIODirection(SPEAKER_PIN, OUT);

   int str_len = -1;
   while(1)
   {
      printf("#######################################\n");
      printf("wait read...\n");
      str_len = read(clnt_sock, msg, sizeof(msg));
      if (str_len == -1){
         error_handling("Err : read() error");
      }
      else if (!strcmp(msg, "1")){
         printf("** active **\n");
         signal_from_main = 1;
      }
      else if (!strcmp(msg, "0")){
         printf("** inactive **\n");
         signal_from_main = 0;
         continue;
      }
      else if (!strcmp(msg, "e")){
         printf("## exit ##\n");
         signal_from_main = 0;
         break;
      }
      
      pthread_create(&alert, NULL, alert_to_server, (void*)argv);
      pthread_create(&front_weight_sensor, NULL, weight_sensor_worker, (void*)FRONT_PIN);
      pthread_create(&back_weight_sensor, NULL, weight_sensor_worker, (void*)BACK_PIN);
      pthread_create(&speaker, NULL, speaker_warning, NULL);
      usleep(1000000); // console 출력들을 보기 좋게 하기 위해
   }

   GPIOUnexport(FRONT_PIN);
   GPIOUnexport(BACK_PIN);
   GPIOUnexport(SPEAKER_PIN);
   close(serv_sock);
   close(clnt_sock);
   printf("======= finish more than two warning =======\n");
   return 0;
}