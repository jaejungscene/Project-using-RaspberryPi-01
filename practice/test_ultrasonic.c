#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include "header/gpioRW.h"

#define BUFFER_MAX 3
#define DIRECTION_MAX 35
#define VALUE_MAX 30

#define IN  0
#define OUT 1
#define LOW  0
#define HIGH 1

#define POUT 23
#define PIN  24
 
int main(int argc, char *argv[]) {
    int repeat = 20;
    clock_t start_t, end_t;
    double time;

    //Enable GPIO pins
    if (-1 == GPIOExport(POUT) || -1 == GPIOExport(PIN)){
        printf("gpio export err\n");    
        return(1);
    }
    //wait for writing to export file
    usleep(100000);

    //Set GPIO directions
    if (-1 == GPIODirection(POUT, OUT) || -1 == GPIODirection(PIN, IN)){
        printf("gpio direction err\n");
        return(2);
    }

    //init ultrawave trigger
    GPIOWrite(POUT,0);
    usleep(10000);
    //start
    do {
      printf("========= repeat %d ========\n", 20-repeat);
        if ( -1 == GPIOWrite(POUT,1)){
            printf("gpio write/trigger err\n");
            return(3);
        }
        
        //1sec == 1000000ultra_sec, 1ms = 1000ultra_sec
        usleep(10);
        GPIOWrite(POUT,0);
        
        while(GPIORead(PIN) == 0){
            start_t = clock();
        }
        while(GPIORead(PIN) == 1){
            end_t = clock();                
        }
        
        time = (double)(end_t-start_t)/CLOCKS_PER_SEC;//ms
        printf("time : %.4lf\n", time);
        printf("distance : %.2lfcm\n", time/2*34000); 
        
        usleep(2000000);
    }
    while (repeat--);
    
    //Disable GPIO pins
    if (-1 == GPIOUnexport(POUT) || -1 == GPIOUnexport(PIN))
        return(4);

    printf("complete\n");

    return(0);
}