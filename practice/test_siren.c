#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "header/gpioRW.h"

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define PIN 17

int main(int argc, char* argv[]) {
   GPIOExport(PIN);
   GPIODirection(PIN, OUT);

   int repeat = 4; //반복 횟수
   int period = 300000; //반복 주기
   int flag = 1;
   for(int i=0; i<repeat; i++){
      GPIOWrite(PIN, flag);
      if(flag == 0)  flag = 1;
      else  flag = 0;
      usleep(period);
   }

   GPIOWrite(PIN, LOW);
   GPIOUnexport(PIN);
   return 0;
}