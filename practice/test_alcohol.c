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
#define PIN 27

int main(int argc, char* argv[]) {
   GPIOExport(PIN);
   GPIODirection(PIN, IN);
   int cnt = 0;
   while(cnt < 100) {
       if(GPIORead(PIN) == 1){
           printf("%d) Not Detected\n", cnt);
       }
       else{
           printf("%d) Detected!\n", cnt);
       }
       cnt++;
       usleep(500000);
   }

   GPIOUnexport(PIN);
   return 0;
}