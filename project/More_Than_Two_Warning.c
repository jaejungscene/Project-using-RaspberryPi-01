#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "header/spiRW.h"
#include "header/mySocket.h"
#include "header/gpioRW.h"

#define MAX_MSG 30
#define FRONT_PIN 0
#define BACK_PIN 1
#define WAIT_TIME 300000
#define IN 0
#define OUT 1

int signal_from_main = 0;
int fd; //adc fd
int front_weight; // value of left pressure sensor
int back_weight; // value of right pressur esensor
int condition  = 0; // 1이 되면 accident 발생


void *alert_to_server(void *argv){
   printf("alert thread start\n");

   int sock;
   struct sockaddr_in serv_addr;
   int str_len;

   sock = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP // SOCK_DGRAM = UDP
   if(sock == -1) error_handling("socket() error");
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET; //IPv4
   serv_addr.sin_addr.s_addr = inet_addr(((char**)argv)[2]); // <-- 변경!!!!!!!!!
   serv_addr.sin_port = htons(atoi(((char**)argv)[3])); // <-- 변경!!!!!!!!

   printf("%s : %s\n", ((char**)argv)[2], ((char**)argv)[3]);
   while(signal_from_main == 1){
      if(front_weight > 10 && back_weight > 10){
         connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
         // if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
         //    error_handling("connect() error");
         // }
         printf("** send msg to server **\n");
         write(sock, "accident", sizeof("accident"));
         close(sock);
      }
   }

   pthread_exit(NULL);
}


void *weight_sensor_worker(void *param){
   printf("weight thread start\n");
   GPIOExport((long)param);
   GPIODirection((long)param, IN);

   while(signal_from_main == 1){
      if((long)param == FRONT_PIN){
         front_weight = GPIORead(FRONT_PIN);
         printf("%ld -> pressure: %3d\n", (long)FRONT_PIN, front_weight);
      }
      else{
         back_weight = GPIORead(BACK_PIN);
         printf("%ld -> pressure: %3d\n", (long)BACK_PIN, back_weight);
      }
      usleep(WAIT_TIME);
   }

   GPIOUnexport((long)param);

   printf("====== finish weight_thd ======\n");
   pthread_exit(NULL);
}


int main(int argc, char *argv[]){
   printf("== automatic report start ==\n");
   pthread_t alert, front_weight_sensor, back_weight_sensor;

   int serv_sock, clnt_sock = -1; // socket filedescriptor
   struct sockaddr_in serv_addr, clnt_addr;
   socklen_t clnt_addr_size = sizeof(clnt_addr);
   char msg[MAX_MSG];

   serverPrepare(&serv_sock, &serv_addr, &argv[1]);
   clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
   if (clnt_sock == -1) error_handling("accept() error");
   printf("=== complete connection with %s ===\n", inet_ntoa(clnt_addr.sin_addr));

   int str_len = -1;
   while(1)
   {
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
         // close(clnt_sock);
      }
      
      pthread_create(&alert, NULL, alert_to_server, (void*)argv);
      pthread_create(&front_weight_sensor, NULL, weight_sensor_worker, (void*)FRONT_PIN);
      pthread_create(&back_weight_sensor, NULL, weight_sensor_worker, (void*)BACK_PIN);
      printf("============= check03 ===============\n");
      // pthread_create(&high_vibration_sensor, NULL, vibration_sensor_worker, NULL);
      // pthread_create(&low_vibration_sensor, NULL, vibration_sensor_worker, NULL);
      // pthread_join(alert, NULL);
      // pthread_join(front_weight_sensor, NULL);
      // pthread_join(back_weight_sensor, NULL);
      // pthread_join(high_vibration_sensor, NULL);
      // pthread_join(low_vibration_sensor, NULL);
   }

   

   printf("== automatic report finish ==\n");
   return 0;
}