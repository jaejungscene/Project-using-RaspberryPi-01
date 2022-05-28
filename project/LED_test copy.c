#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define PIN 20
#define POUT 17
#define POUT2 21
#define VALUE_MAX 40

static int GPIOExport(int pin)
{
#define BUFFER_MAX 3
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open export for writing!\n");
    return(-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return(0);
}


static int GPIODirection(int pin, int dir) {
/***************************************** 
 *  너무 GPIODirection이 빨리 실행되면, 
 *  GPIOExport로 인해 해당 pin의 gpio가 export 되기도 전에 
 *  open() system call을 사용하여 에러발생
 *****************************************/
  usleep(40000);
/****************************************
 *  usleep - suspend execution for microsecond intervals
 *  1 msec = 1000 usec
 ****************************************/
  static const char s_directions_str[] = "in\0out";

#define DIRECTION_MAX 35
  //char path[DIRECTION _MAX]="/sys/class/gpio/gpio24/direction":
  char path[DIRECTION_MAX];
  int fd;

  snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr,"Failed to open gpio direction for writing!\n");
  }
  if (-1 == write(fd, &s_directions_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)) {
    fprintf(stderr,"Failed to set direction!\n");
    close(fd);
    return(-1);
  }

  close(fd);
  return(0);
}


static int GPIOUnexport(int pin)
{
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd=open("/sys/class/gpio/unexport", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open unexport for writing!\n");
    return(-1);
  }
  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written); //write는 정확히 쓰고 싶은 만큼을 크기로 지정해야 함
  close(fd);
  return(0);
}


static int GPIORead(int pin)
{
  char path[VALUE_MAX];
  char value_str[3];
  int fd;

  snprintf(path, VALUE_MAX,"/sys/class/gpio/gpio%d/value", pin);
  fd = open (path, O_RDONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open gpio value for reading!\n");
    return(-1);
  }

  if (-1 == read(fd, value_str, 3)) {
    fprintf(stderr, "Failed to read value!\n");
    close(fd);
    return(-1);
  }

  close(fd);
  return(atoi(value_str));
}


static int GPIOWrite(int pin, int value)
{
  static const char s_values_str[] = "01";
  char path[VALUE_MAX];
  int fd;

  snprintf(path, VALUE_MAX,"/sys/class/gpio/gpio%d/value",pin);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr,"Failed to open gpio value for writing!\n");
    return(-1);
  }
  if (1 != write(fd, &s_values_str[LOW == value ? 0: 1], 1)){
    fprintf(stderr,"Failed to write value!\n");
    close(fd);
    return(-1);
  }

  close(fd);
  return(0);
}


int main(int argc, char *argv[])
{
  int repeat = 100;
  int state = 1;
  int prev_state = 1;
  int light = 0;
  int value;
  int prev = 0;

  //Enable GPIO pins
  if (-1 == GPIOExport(POUT2) || -1 == GPIOExport(PIN) || -1 == GPIOExport(POUT))
    return(1);

  //Set GPIO directions
  if (-1 == GPIODirection(POUT2, OUT) || -1 == GPIODirection(PIN, IN) || -1 == GPIODirection(POUT, OUT)) // POUT2 21, PIN 20
    return(2);

  if ( -1 == GPIOWrite (POUT2, 1)) // 처음 pin 21에 1을 write하면 이 값은 계속 유지된다.
    return(3);
  do {
    // if ( -1 == GPIOWrite (POUT2, 1))
    //   return(3);

    /**
     * 21(POUT2)에 계속 전류가 흐르게 하고 그 전류는 button 눌러져 ground와 연결되지 않는 이상
     * 21(POUT2)로 흘러들어와 20(PIN)으로 그대로 흘러간다. 만약 button을 누르게 되면 21(POUT2)와 연결된
     * button의 한 쪽 pin이 ground와 연결된 button의 다른 한 쪽 pin과 연결되면서 전류가 ground로 흐르게 되어
     * 20(PIN)에는 전류가 흐르지 않게 된다.
     * 따라서 button을 누르면 20(PIN)에는 전류가 흐르지 않게 되어 0이 read된다.
     */
    value = GPIORead(PIN);
    printf ("%2d) GPIORead : %d from pin %d, prev : %d\n", repeat, value, PIN, prev);

    if(value == 0 && prev == 1){ //button의 (value)값이 1에서 0으로 될 때 <- button을 press할 때
      if(GPIORead(POUT) == 0){
        GPIOWrite(POUT, 1);
      }
      else if(GPIORead(POUT) == 1){
        GPIOWrite(POUT, 0);
      }
      else{
        fprintf(stderr, "Failed to read LED value!\n");
        return(5);
      }
    }

    prev = value;
    usleep(100000);
  }
  while (repeat--);

  //Disable GPIO pins
  if (-1 == GPIOUnexport(POUT2) || -1 == GPIOUnexport(PIN) || -1 == GPIOUnexport(POUT))
    return(4);

  return(0);
}