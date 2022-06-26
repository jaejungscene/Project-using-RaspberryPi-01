#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "header/spiRW.h"
#include "header/mySocket.h"
#include "header/gpioRW.h"

#define IN 0
#define OUT 1
#define LOW 0
#define HEIGH 1

#define MAX_MSG 30
#define L_CHANNEL 3
#define R_CHANNEL 4
#define SPEAKER_PIN 27
#define FRONT_PIN 17
#define BACK_PIN 22
#define WAIT_TIME 100000

int signal_from_main = 0;
int fd; // adc fd
int L_pressure; // value of left pressure sensor
int R_pressure; // value of right pressur esensor
int front_weight; // value of left pressure sensor
int back_weight; // value of right pressur esensor


void *alert_to_server(void *argv){
   printf("----- start alert thread -----\n");

   while(signal_from_main == 1){
      if( L_pressure > 10 && R_pressure > 10 && (front_weight == 1 || back_weight == 1) ){
         printf("boarding !\n");
         break;
      }
   }

   int sock;
   struct sockaddr_in serv_addr;
   int str_len;

   sock = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP, SOCK_DGRAM = UDP
   if(sock == -1) error_handling("socket() error");
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET; //IPv4
   serv_addr.sin_addr.s_addr = inet_addr(((char**)argv)[2]);
   serv_addr.sin_port = htons(atoi(((char**)argv)[3]));

   int noise = 5;
   int cnt = 0;
   printf("%s : %s\n", ((char**)argv)[2], ((char**)argv)[3]);
   while(signal_from_main == 1){
      if(L_pressure <= noise && R_pressure <= noise && front_weight==0 && back_weight==0){
         printf("possible accident!\n");
         /**
          * 주행자가 킥보드를 반납할 때 센서 값들이 모두 0일 수도 있으므로
          * 킥보드를 반납하는 시간을 10초 정도로 잡아
          * 10초 후에도 킥보드가 반납이 되지 않고
          * 센서값들이 모두 0이면
          * 사고 발생으로 감지
          **/
         cnt = 0;
         while(signal_from_main==1 && L_pressure <= noise && R_pressure <= noise && front_weight==0 && back_weight==0){
            if(cnt >= 100) // 10 sec
               break;
            usleep(WAIT_TIME); // 0.1 sec
            cnt++;
         }

         if(signal_from_main == 0)
            break;

         if(cnt >= 100){
            if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
               error_handling("connect() error");
            }

            printf("** send \'accident\' msg to server **\n");
            write(sock, "accident", sizeof("accident"));
            close(sock);

            int i=1;
            while(signal_from_main){ // 사이렌
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
            GPIOWrite(SPEAKER_PIN, LOW); // 마지막으로 speaker 끔

            break;
         }
         else{
            printf("not accident!\n");
         }
      }
   }
   
   printf("----- finish alert thread -----\n");
   pthread_exit(NULL);
}


void *pressure_sensor_worker(void *param){
   printf("----- start pressure thread -----\n");

   while(signal_from_main == 1){
      if((long)param == L_CHANNEL){
         L_pressure = readadc(fd, (long)L_CHANNEL);
         // printf("%d -> L_pressure: %3d\n", L_CHANNEL, L_pressure); // <--- test
      }
      else{
         R_pressure = readadc(fd, (long)R_CHANNEL);
         // printf("%d -> R_pressure: %3d\n", R_CHANNEL, R_pressure); // <--- test
      }
      usleep(WAIT_TIME);
   }

   printf("----- finish pressure thread -----\n");
   pthread_exit(NULL);
}



void *weight_sensor_worker(void *param){
   printf("----- start weight thread -----\n");

   int value;

   while(signal_from_main == 1){
      if((long)param == FRONT_PIN){
         front_weight = GPIORead(FRONT_PIN);
         // printf("%d -> F_weight: %3d\n", FRONT_PIN, front_weight); // <--- test
      }
      else{
         back_weight = GPIORead(BACK_PIN);
         // printf("%d -> B_weight: %3d\n", BACK_PIN, back_weight); // <--- test
      }
      usleep(WAIT_TIME);
   }

   printf("----- finish weight thread -----\n");
   pthread_exit(NULL);
}



void *vibration_sensor_worker(){
   printf("thread01!!!\n");
   pthread_exit(NULL);
}


// ./(file name) <port to open> <Server ip> <Server port>
int main(int argc, char *argv[])
{
   printf("===== automatic report start =====\n");
   pthread_t alert, L_pressure_sensor, R_pressure_sensor,
               front_weight_sensor, back_weight_sensor;

   int serv_sock, clnt_sock = -1;
   struct sockaddr_in serv_addr, clnt_addr;
   socklen_t clnt_addr_size = sizeof(clnt_addr);
   char msg[MAX_MSG];

   /**** Main_Alcohol에게 signal을 받기 위해 socket 통신 ****/
   serverPrepare(&serv_sock, &serv_addr, &argv[1]);
   clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
   if (clnt_sock == -1) error_handling("accept() error");
   printf("** complete connection with %s **\n", inet_ntoa(clnt_addr.sin_addr));

   GPIOExport(FRONT_PIN);
   GPIOExport(BACK_PIN);
   GPIODirection(FRONT_PIN, IN);
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

      fd = open(DEVICE, O_RDWR);
      if (fd <= 0) {
         printf( "Device %s not found\n", DEVICE);
         break;
      }
      if(prepare(fd) == -1){
         break;
      }
   
      pthread_create(&alert, NULL, alert_to_server, (void*)argv);
      pthread_create(&L_pressure_sensor, NULL, pressure_sensor_worker, (void*)L_CHANNEL);
      pthread_create(&R_pressure_sensor, NULL, pressure_sensor_worker, (void*)R_CHANNEL);
      pthread_create(&front_weight_sensor, NULL, weight_sensor_worker, (void*)FRONT_PIN);
      pthread_create(&back_weight_sensor, NULL, weight_sensor_worker, (void*)BACK_PIN);
      usleep(1000000);
   }

   GPIOUnexport(FRONT_PIN);
   GPIOUnexport(BACK_PIN);
   GPIOUnexport(SPEAKER_PIN);
   close(serv_sock);
   close(clnt_sock);
   printf("======= finish automatic report =======\n");
   return 0;
}