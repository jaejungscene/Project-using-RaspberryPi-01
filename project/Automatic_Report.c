#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "header/spiRW.h"
#include "mySocket.h"

#define MAX_MSG 30

int fd; //adc fd

void *pressure_sensor_worker(){
   printf("pressure thread\n");

   fd = open(DEVICE, O_RDWR); // opening the DEVICE file as read/write
   if (fd <= 0) {
      printf( "Device %s not found\n", DEVICE);
      pthread_exit(NULL);
   }
   if(prepare(fd) == -1){
      pthread_exit(NULL);
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

   pthread_t L_pressure_sensor, R_pressure_sensor,
               high_vibration_sensor, low_vibration_sensor;
   long L_channel = 2;
   long R_channel = 3;
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
      
      pthread_create(&L_pressure_sensor, NULL, pressure_sensor_worker, (void*)L_channel);
      pthread_create(&R_pressure_sensor, NULL, pressure_sensor_worker, (void*)R_channel);
      pthread_create(&high_vibration_sensor, NULL, vibration_sensor_worker, NULL);
      pthread_create(&low_vibration_sensor, NULL, vibration_sensor_worker, NULL);
      pthread_join(L_pressure_sensor, NULL);
      pthread_join(R_pressure_sensor, NULL);
      pthread_join(high_vibration_sensor, NULL);
      pthread_join(low_vibration_sensor, NULL);
   }

   

   printf("== automatic report finish ==\n");
   return 0;
}