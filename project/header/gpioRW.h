#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

static int GPIOExport(int pin)
{
  int Buffer_Max = 3;
  char buffer[Buffer_Max];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open export for writing at %d pin!\n", pin);
    return(-1);
  }

  bytes_written = snprintf(buffer, Buffer_Max, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return(0);
}


static int GPIODirection(int pin, int dir) {
  usleep(40000);
  static const char s_directions_str[] = "in\0out";

  int Direction_Max = 35;
  char path[Direction_Max];
  int fd;
  int in = 0;

  snprintf(path, Direction_Max, "/sys/class/gpio/gpio%d/direction", pin);

  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr,"Failed to open gpio direction for writing at %d pin!\n", pin);
  }
  if (-1 == write(fd, &s_directions_str[in == dir ? 0 : 3], in == dir ? 2 : 3)) {
    fprintf(stderr,"Failed to set direction at %d pin!\n", pin);
    close(fd);
    return(-1);
  }

  close(fd);
  return(0);
}


static int GPIOUnexport(int pin)
{
  int Buffer_Max = 3;
  char buffer[Buffer_Max];
  ssize_t bytes_written;
  int fd;

  fd=open("/sys/class/gpio/unexport", O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open unexport for writing at %d pin!\n", pin);
    return(-1);
  }
  bytes_written = snprintf(buffer, Buffer_Max, "%d", pin);
  write(fd, buffer, bytes_written); //write는 정확히 쓰고 싶은 만큼을 크기로 지정해야 함
  close(fd);
  return(0);
}

/**
 * @return when failed to read, return -1. when read successfully, return the read value.
 */
static int GPIORead(int pin)
{
  int Value_Max = 35;
  char path[Value_Max];
  char value_str[10];
  int fd;

  snprintf(path, Value_Max, "/sys/class/gpio/gpio%d/value", pin);
  fd = open (path, O_RDONLY);
  if (-1 == fd) {
    fprintf(stderr, "Failed to open gpio value for reading at %d pin!\n", pin);
    return(-1);
  }

  if (-1 == read(fd, value_str, sizeof(value_str))) {
    fprintf(stderr, "Failed to read value at %d pin!\n", pin);
    close(fd);
    return(-1);
  }

  close(fd);
  return(atoi(value_str));
}


static int GPIOWrite(int pin, int value)
{
  int Value_Max = 35;
  static const char s_values_str[] = "01";
  char path[Value_Max];
  int fd;
  int low = 0;

  snprintf(path, Value_Max,"/sys/class/gpio/gpio%d/value",pin);
  fd = open(path, O_WRONLY);
  if (-1 == fd) {
    fprintf(stderr,"Failed to open gpio value for writing at %d pin!\n", pin);
    return(-1);
  }
  if (1 != write(fd, &s_values_str[low == value ? 0: 1], 1)){
    fprintf(stderr,"Failed to write value at %d pin!\n", pin);
    close(fd);
    return(-1);
  }

  close(fd);
  return(0);
}