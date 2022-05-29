#include "header/spiRW.h"
#include "header/gpioRW.h"
#include "header/pwmRW.h"

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1

#define PERIOD 20000000
#define MAX_VALUE 900.0 // 실험 후 나온 조도센서의 최대값 + 특정값
#define MIN_VALUE 0.0 // 실험 후 나온 조도센서의 최소값 - 특정값

#define WRITE_VALUE(light)  (int)((light-MIN_VALUE)*(PERIOD/(MAX_VALUE-MIN_VALUE)))  // (PERIOD*(1-(light/MAX_VALUE)))// 조도센서의 값이 작을수록 값이 커지도록 설계
#define TIME 300

int fd; // file descriptor for DEVICE

void work(int pwmnum, int channel)
{
  PWMExport(pwmnum);
  PWMWritePeriod(pwmnum, PERIOD);
  PWMWriteDutyCycle(pwmnum, PERIOD);
  PWMEnable(pwmnum);

  int light = 0;
  for(int i=0; i<TIME; i++){
    light = readadc(fd, channel);
    PWMWriteDutyCycle( pwmnum, WRITE_VALUE(light) );
    printf("%d -> %d) light: %3d, written value: %d\n", channel, i, light, WRITE_VALUE(light) );
    // printf("%d -> %3d) light: %3d\n", channel, i, light);
    usleep(90000);
  }

  PWMWriteDutyCycle(pwmnum, PERIOD); /////////// finish
}

int main()
{

  printf("================ !! program start !! ================\n");

  fd = open(DEVICE, O_RDWR); // opening the DEVICE file as read/write
  if (fd <= 0) {
    printf( "Device %s not found\n", DEVICE);
    return (-1);
  }
#define PWM0 0
// 가장 안정  2, 4
// 약간 안정  3
// 약간 불안정 0
// 가장 불안정 1
#define CHANNEL 3
  if(prepare(fd) == -1){
    exit(1);
  }
  work(PWM0, CHANNEL);
  close(fd);
  
  printf("================ !! program finish !! ================\n");
  return 0;
}