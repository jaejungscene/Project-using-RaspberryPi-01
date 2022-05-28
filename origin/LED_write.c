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
#define POUT 17
#define VALUE_MAX 40

static int GPIOExport(int pin){
#define BUFFER_MAX 3
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if(-1==fd){
    fprintf(stderr, "Failed to open export for writing!\n");
    return -1;
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return 0;
}

static int GPIODirection(int pin, int dir){ //해당 gpio가 in인지 out인지 정함
  /***************************************** 
  *  너무 GPIODirection이 빨리 실행되면, 
  *  GPIOExport로 인해 해당 pin의 gpio가 export 되기도 전에 
  *  open() system call을 사용하여 에러발생
  *****************************************/
  usleep(30000);
  /****************************************
  *  usleep - suspend execution for microsecond intervals
  *  1 msec = 1000 usec
  ****************************************/
  static const char s_direction_str[] = "in\0out";

#define DIRECTION_MAX 35
  char path[DIRECTION_MAX];
  int fd;

  snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);
  // printf("%s\n", path); //check
  fd = open("/sys/class/gpio/gpio17/direction", O_WRONLY);
  if(-1==fd){
    fprintf(stderr, "Failed to open gpio direction for writing!\n");
    return -1;
  }

  // IN과 dir이 같으면 fd의 direction에 in을 보내고 아니면 out을 보냄
  if(-1==write(fd, &s_direction_str[IN == dir ? 0 : 3], IN == dir ? 2 : 3)){
    fprintf(stderr, "Failed to set direction!\n");
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

static int GPIOWrite(int pin, int value){
  static const char s_values_str[] = "01";

  char path[VALUE_MAX];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_WRONLY);
  if(-1==fd){
    fprintf(stderr, "Failed to open gpio value for writing!\n");
    return -1;
  }

  // value값이 LOW(0)이면 0을 전달하여 끄고 1이면 1을 전달하여 켠다.
  if(1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)){
    fprintf(stderr, "Failed to write value!\n");
    close(fd);
    return -1;
  }

  close(fd);
  return 0;
}

static int GPIORead(int pin)
{
  char path[VALUE_MAX];
  char value_str[3];
  int fd;

  snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value", pin);
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

static int GPIOUnexport(int pin){
  char buffer[BUFFER_MAX];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/unexport", O_WRONLY);
  if(-1 == fd){
    fprintf(stderr, "Failed to open unexport for writing!\n");
    return -1;
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return 0;
}


int main(int argc, char *argv[]){
  int repeat = 5;

  if(-1 == GPIOExport(POUT)){
    return 1;
  }

  if(-1 == GPIODirection(POUT, OUT)){ // gpio를 OUT(출력)으로 결정
    return 2;
  }
  
  do {
    printf("%d) value : %d\n", repeat, repeat%2);
    if(-1 == GPIOWrite(POUT, repeat % 2))
      return 3;
    usleep(500 * 1000);
    // printf("%d) value : %d\n", repeat, GPIORead(POUT)); // possible
  }while(repeat--);

  if(-1 == GPIOUnexport(POUT)){
    return 4;
  }
  
  return 0;
}


