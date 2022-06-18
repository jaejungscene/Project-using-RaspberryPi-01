// #include <sys/stat.h>
// #include <sys/types.h>
// #include <fcntl.h>
// #include <stdio.h>
// #include <stdlib.h>
// #include <unistd.h>
// #include <string.h>
// #include "header/gpioRW.h"

// #define IN 0
// #define OUT 1
// #define LOW 0
// #define HIGH 1
// #define PIN 27

// int main(int argc, char* argv[]) {
//    GPIOExport(PIN);
//    GPIODirection(PIN, IN);
//    int cnt = 0;
//    while(cnt < 100) {
//        if(GPIORead(PIN) == 1){
//            printf("%d) Not Detected\n", cnt);
//        }
//        else{
//            printf("%d) Detected!\n", cnt);
//        }
//        cnt++;
//        usleep(500000);
//    }

//    GPIOUnexport(PIN);
//    return 0;
// }


#include "header/spiRW.h"
#include "header/gpioRW.h"
#include "header/pwmRW.h"

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define SPEAKER_PIN 27

#define TIME 300

int fd; // file descriptor for DEVICE

void work(int pwmnum, int channel)
{
    GPIOExport(SPEAKER_PIN);
    GPIODirection(SPEAKER_PIN, OUT);

  int value = 0;
  for(int i=0; i<TIME; i++){
    value = readadc(fd, channel);
    if(value >= 1000){
        printf("detected!\n");
        GPIOWrite(SPEAKER_PIN, OUT);
    }
    GPIOWrite(SPEAKER_PIN, IN);
    printf("%d -> %3d) value: %3d\n", channel, i, value);
    usleep(90000);
  }

  GPIOUnexport(SPEAKER_PIN);
}

int main()
{

  printf("================ !! program start !! ================\n");

  fd = open(DEVICE, O_RDWR); // opening the DEVICE file as read/write
  if (fd <= 0) {
    printf( "Device %s not found\n", DEVICE);
    return (-1);
  }
  if(prepare(fd) == -1){
    return -1;

  }

#define PWM0 0
#define CHANNEL 2

// 가장 안정  2, 4
// 약간 안정  3
// 약간 불안정 0
// 가장 불안정 1
  work(PWM0, CHANNEL);
    

  close(fd);
  printf("================ !! program finish !! ================\n");
  return 0;
}