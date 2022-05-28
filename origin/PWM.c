#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define VALUE_MAX 256

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


/**
 * duty_cycle이 증가할수록 빛은 더 밝아진다.
 * duty_cycle의 값은 period를 넘을 수 없다.
 * duty_cycle의 값과 period 값이 같다면, 이는 가장 쎈 밝기를 의미한다.
 */
int main(int argc, char* argv[]) {
#define PWMNUM 0
#define PERIOD 20000000
#define WEIGHT 10000
   
   PWMExport(PWMNUM); // pwm0 is gpio18
   PWMWritePeriod(PWMNUM, PERIOD);
   PWMWriteDutyCycle(PWMNUM, 0); // duty_cycle 0으로 초기화(LED가 꺼진 상태와 같다)
   PWMEnable(PWMNUM);

   int max_width = 2000; // 깜빡임의 period를 조절
   int time = 0;  // 몇번 반복할지를 결정

   if(max_width*WEIGHT > PERIOD){ // duty_cycle의 값은 period를 넘을 수 없다. -> 예외처리
      fprintf(stderr, "max_width is invalid value!\n");
      return 1;
   }

   while(1){
      if(time >= 3)
         break;
      for (int i = 0; i < max_width; i++){
         // printf("%-3d", i);
         PWMWriteDutyCycle(PWMNUM, i*WEIGHT);   // i*10000은 period보다 크면 안된다.
         usleep(1000);
      }
      for(int i = max_width; i >= 0; i--){
         // printf("%-3d", i);
         PWMWriteDutyCycle(PWMNUM, i*WEIGHT);
         usleep(1000);
      }
      time++;
      printf("| cycle %d\n", time);
   }
   return 0;
}