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
  usleep(100000);
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

  //Enable GPIO pins
  if (-1 == GPIOExport(POUT2) || -1 == GPIOExport(PIN) )
    return(1);

  //Set GPIO directions
  if (-1 == GPIODirection(POUT2, OUT) || -1 == GPIODirection(PIN, IN)) // POUT2 21, PIN 20
    return(2);

  do {
    if ( -1 == GPIOWrite (POUT2, 1))
      return(3);

    // button을 누루면 ground와 연결되어 0이 read됨.
    printf ("%d) GPIORead : %d from pin %d\n", repeat, GPIORead(PIN), PIN);
    usleep(100000);
  }
  while (repeat--);

  //Disable GPIO pins
  if (-1 == GPIOUnexport(POUT2) || -1 == GPIOUnexport(PIN) )
    return(4);

  return(0);
}