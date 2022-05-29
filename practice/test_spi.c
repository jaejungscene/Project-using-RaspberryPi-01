#include "spiRW.h"
#include "gpioRW.h"
#include "header/pwmRW.h"

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

#define TIME 300

int fd; // file descriptor for DEVICE

void work(int pwmnum, int channel)
{
  int light = 0;
  for(int i=0; i<TIME; i++){
    light = readadc(fd, channel);
    printf("%d -> %3d) light: %3d\n", channel, i, light);
    usleep(90000);
  }
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
#define CHANNEL 3
// 가장 안정  2, 4
// 약간 안정  3
// 약간 불안정 0
// 가장 불안정 1
  work(PWM0, CHANNEL);

  close(fd);
  printf("================ !! program finish !! ================\n");
  return 0;
}