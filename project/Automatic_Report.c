#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "header/spiRW.h"
#include "mySocket.h"

#define MAX_MSG 30
#define L_CHANNEL = 2
#define R_CHANNEL = 3

int fd; //adc fd
int L_pressure; // value of left pressure sensor
int R_pressure; // value of right pressur esensor

void *alert_to_server(void *channel){
   int sock;
   struct sockaddr_in serv_addr;
   int str_len;

   sock = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP // SOCK_DGRAM = UDP
   if(sock == -1) error_handling("socket() error");
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET; //IPv4
   // serv_addr.sin_addr.s_addr = inet_addr(argv[1]); // <-- 변경!!!!!!!!!
   // serv_addr.sin_port = htons(atoi(argv[2])); // <-- 변경!!!!!!!!

   while(1){
      if(L_pressure > 10 && R_pressure > 10){
         sleep(60);
         if(L_pressure > 10 && R_pressure > 10){
            if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
               error_handling("connect() error");
            }
            write(sock, "accident", sizeof("accident"));
            close(sock);
         }
         
      }
   }
}

void *pressure_sensor_worker(void *param){
   printf("pressure thread\n");

   while(1){
      if((long)param == L_CHANNEL){
         L_pressure = readadc(fd, (long)L_CHANNEL);
         printf("%ld -> pressure: %3d\n", (long)L_CHANNEL, L_pressure);
      }
      else{
         R_pressure = readadc(fd, (long)R_CHANNEL);
         printf("%ld -> pressure: %3d\n", (long)R_CHANNEL, R_pressure);
      }
      usleep(90000);
   }

   pthread_exit(NULL);
}

void *vibration_sensor_worker(){
   printf("thread01!!!\n");
   pthread_exit(NULL);
}

int main(){
   printf("== automatic report start ==\n");

   int serv_sock, clnt_sock = -1; // socket filedescriptor
   struct sockaddr_in serv_addr, clnt_addr;
   socklen_t clnt_addr_size;
   char msg[MAX_MSG];

   serverPrepare(&serv_sock, &serv_addr, &argv[1]);

   pthread_t alert, L_pressure_sensor, R_pressure_sensor,
               high_vibration_sensor, low_vibration_sensor;

   int str_len = -1;
   while(1)
   {
      clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
      if (clnt_sock == -1) error_handling("accept() error");
      printf("=== complete connection with %s ===\n", inet_ntoa(clnt_addr.sin_addr));

      printf("wait read...\n");
      while(1){
         str_len = read(clnt_sock, msg, sizeof(msg));
         if (str_len == -1){
            error_handling("Err : read() error");
            continue;
         }
         else if (!strcmp(msg, "active")){
            printf("** active **\n");
            break;
         }
      }
      

      fd = open(DEVICE, O_RDWR); // opening the DEVICE file as read/write
      if (fd <= 0) {
         printf( "Device %s not found\n", DEVICE);
         break;
      }
      if(prepare(fd) == -1){
         break;
      }

      pthread_create(&alert, NULL, sor_worker, NULL);
      pthread_create(&L_pressure_sensor, NULL, pressure_sensor_worker, (void*)L_CHANNEL);
      pthread_create(&R_pressure_sensor, NULL, pressure_sensor_worker, (void*)R_CHANNEL);
      pthread_create(&high_vibration_sensor, NULL, vibration_sensor_worker, NULL);
      pthread_create(&low_vibration_sensor, NULL, vibration_sensor_worker, NULL);
      pthread_join(alert, NULL);
      pthread_join(L_pressure_sensor, NULL);
      pthread_join(R_pressure_sensor, NULL);
      pthread_join(high_vibration_sensor, NULL);
      pthread_join(low_vibration_sensor, NULL);
   }

   

   printf("== automatic report finish ==\n");
   return 0;
}