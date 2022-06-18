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
#define PIN 26

int main(int argc, char* argv[]) {
   GPIOExport(PIN);
   GPIODirection(PIN, OUT);

   int repeat = 10; //반복 횟수
   int period = 300000; //반복 term
   for(int i=0; i<repeat; i++){
      printf("%3d\n",i);
      GPIOWrite(PIN, i%2);
      usleep(period);
   }

   GPIOWrite(PIN, LOW);
   GPIOUnexport(PIN);
   return 0;
}