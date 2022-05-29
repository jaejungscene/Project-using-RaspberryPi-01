#include <stdio.h>
#include <pthread.h>
#include <unistd.h>

void *pressure_sensor_worker(void *param){
   printf("thread01!!!\n");
   pthread_exit(NULL);
}

int main(){
   printf("== main start ==\n");
   sleep(10);
   // pthread_t L_pressure_sensor, R_pressure_sensor,
   //             high_vibration_sensor, low_vibration_sensor;
   // pthread_create(&L_pressure_sensor, NULL, pressure_sensor, );

   printf("== main finish ==\n");
   return 0;
}