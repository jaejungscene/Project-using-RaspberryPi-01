#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "header/pwmRW.h"

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

/**
 * duty_cycle이 증가할수록 빛은 더 밝아진다.
 * duty_cycle의 값은 period를 넘을 수 없다.
 * duty_cycle의 값과 period 값이 같다면, 이는 가장 쎈 밝기를 의미한다.
 */
int main(int argc, char* argv[]) {
#define PWMNUM 0
#define PERIOD 20000000
#define WEIGHT 10000
   
   PWMExport(PWMNUM); // pwm0 is gpio18
   PWMWritePeriod(PWMNUM, PERIOD);
   PWMWriteDutyCycle(PWMNUM, 0); // duty_cycle 0으로 초기화(LED가 꺼진 상태와 같다)
   PWMEnable(PWMNUM);

   int max_width = 2000; // 깜빡임의 period를 조절
   int time = 0;  // 몇번 반복할지를 결정

   if(max_width*WEIGHT > PERIOD){ // duty_cycle의 값은 period를 넘을 수 없다. -> 예외처리
      fprintf(stderr, "max_width is invalid value!\n");
      return 1;
   }

   while(1){
      if(time >= 3)
         break;
      for (int i = 0; i < max_width; i++){
         // printf("%-3d", i);
         PWMWriteDutyCycle(PWMNUM, i*WEIGHT);   // i*10000은 period보다 크면 안된다.
         usleep(1000);
      }
      for(int i = max_width; i >= 0; i--){
         // printf("%-3d", i);
         PWMWriteDutyCycle(PWMNUM, i*WEIGHT);
         usleep(1000);
      }
      time++;
      printf("| cycle %d\n", time);
   }
   return 0;
}