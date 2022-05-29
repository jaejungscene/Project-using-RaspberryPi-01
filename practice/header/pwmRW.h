#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>

static int PWMExport(int pwmnum) {
   #define BUFFER_MAX 3

   char buffer[BUFFER_MAX];
   int bytes_written;
   int fd;

   fd = open("/sys/class/pwm/pwmchip0/unexport", O_WRONLY);
   if(-1 == fd){
      fprintf(stderr, "Faild to open in unexport\n");
      return(-1);
   }

   bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
   write(fd, buffer, bytes_written);  // echo 0 > unexport
   close(fd);

   sleep(1);
   fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
   if (-1 == fd) {
      fprintf(stderr, "Failed to open in export!\n");
      return(-1);
   }

   bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
   write(fd, buffer, bytes_written);  // echo 0 > export
   close(fd);
   sleep(1);
   return(0);
}

static int PWMEnable(int pwmnum){
   static const char s_unenable_str[] = "0";
   static const char s_enable_str[] = "1";

#define DIRECTION_MAX 45
   char path[DIRECTION_MAX];
   int fd;

   snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum);
   fd = open(path, O_WRONLY);
   if(-1 == fd) {
      fprintf(stderr, "Failed to open in enable for echo 0!\n");
      return -1;
   }
   write(fd, s_unenable_str, strlen(s_unenable_str)); // echo 0 > pwm(pwmnum)/enable
   close(fd);

   fd = open(path, O_WRONLY);
   if(-1 == fd){
      fprintf(stderr, "Failed to open in enable for echo 1!\n");
      return -1;
   }
   write(fd,s_enable_str,strlen(s_enable_str)); // echo 1 > pwm(pwmnum)/enable
   close(fd);

   return(0);
}

static int PWMWritePeriod(int pwmnum, int value){
#define VALUE_MAX 256
   char s_values_str[VALUE_MAX];
   char path[VALUE_MAX];
   int fd,byte;

   snprintf(path, VALUE_MAX,"/sys/class/pwm/pwmchip0/pwm%d/period",pwmnum);

   fd = open(path, O_WRONLY);
   if(-1 == fd) {
      fprintf(stderr, "Failed to open in period!\n");
      return(-1);
   }

   byte = snprintf(s_values_str, VALUE_MAX,"%d",value);

   if(-1 == write(fd, s_values_str, byte)) {  // echo (value) > pwm(pwmnum)/period
      fprintf(stderr, "Failed to write value in period\n");
      close(fd);
      return(-1);
   }
   close(fd);

   return(0);

}

static int PWMWriteDutyCycle(int pwmnum, int value){
   char path[VALUE_MAX];
   char s_values_str[VALUE_MAX];
   int fd,byte;

   snprintf(path, VALUE_MAX,"/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", pwmnum);
   fd = open(path, O_WRONLY);
   if(-1 == fd) {
      fprintf(stderr, "Failed to open in duty_cycle!\n");
      return(-1);
   }

   byte = snprintf(s_values_str, VALUE_MAX, "%d",value);

   if(-1 == write(fd, s_values_str, byte)) {  // echo (value) > pwm(pwmnum)/duty_cycle
      fprintf(stderr, "Failed to write value! in duty_cycle!\n");
      close(fd);
      return(-1);
   }
   close(fd);

   return(0);
}