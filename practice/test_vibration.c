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
#define PIN 22

int main(int argc, char* argv[]) {
   GPIOExport(PIN);
   GPIODirection(PIN, IN);

   int repeat = 100; //반복 횟수
   int value = -1;

   for(int i=0; i<repeat; i++){
      value = GPIORead(PIN);
      if(value == -1){
         printf("Fail to read!!\n");
      }
      else{
         printf("%d) read value : %4d\n", i, value);
      }
      usleep(100000);
   }

   GPIOUnexport(PIN);
   return 0;
}