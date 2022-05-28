#include <stdint.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <getopt.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <pthread.h>
#include <wait.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define VALUE_MAX 256
#define DIRECTION_MAX 45
#define IN_FROM_BUTTON 20 // button의 동작을 감지하는 pin
#define OUT_TO_BUTTON 21 // 계속 전류가 흐르는 pin

int fd;
//////////////////////////////////////////////////////////////

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

///////////////////////////////////////////////////////////////

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

////////////////////////////////////////////////////////////////

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])

static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint16_t DELAY = 5;
static uint8_t MODE0 = SPI_MODE_0;
static uint8_t MODE1 = SPI_MODE_1;


/* Ensure all settings are correct for the ADC */
static int prepare(int fd, uint8_t MODE) {
  if (ioctl(fd, SPI_IOC_WR_MODE, &MODE) == -1) {
    perror("Can't set MODE");
    return -1;
  }

  if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &BITS) == -1) {
    perror("Can't set number of BITS");
    return -1;
  }

  if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &CLOCK) == -1) {
    perror("Can't set write CLOCK") ;
    return -1;
  }

  if (ioctl(fd, SPI_IOC_RD_MAX_SPEED_HZ, &CLOCK) == -1) {
    perror("Can't set read CLOCK") ;
    return -1;
  }

  return 0;
}

/* SGL/DIF = 0,  D2=D1=D0=0 */
uint8_t control_bits_differential(uint8_t channel) {
  return (channel & 7) << 4;
}

/* SGL/DIF = 0,  D2=D1=D0=0 */
uint8_t control_bits(uint8_t channel){
  return 0x8 | control_bits_differential(channel);
}

/* Given a prep'd descriptor, and an ADC channel, fetch the raw ADC value for the given channel. */
int readadc (int fd, uint8_t channel) {
  uint8_t tx[] = {1, control_bits(channel), 0};
  uint8_t rx[3];

  struct spi_ioc_transfer tr ={
    .tx_buf = (unsigned long)tx,
    .rx_buf = (unsigned long)rx,
    .len = ARRAY_SIZE(tx),
    .delay_usecs = DELAY,
    .speed_hz = CLOCK,
    .bits_per_word = BITS,
  };

  if (ioctl(fd, SPI_IOC_MESSAGE(1), &tr) == 1) {
    perror ("I0 Error");
    abort();
  }

  return ((rx[1] << 8) & 0x300) | (rx[2] & 0xFF);
}

/////////////////////////////////////////////////////////////

#define PERIOD 20000000
#define TIME 300

static const char *DEVICE = "/dev/spidev0.0";

int main()
{
  printf("======== start =========\n");

  int value;
  int prev = 0; //button을 press하고 땔 때
  //Enable GPIO pins
  if (-1 == GPIOExport(OUT_TO_BUTTON) || -1 == GPIOExport(IN_FROM_BUTTON))
    return(1);
  //Set GPIO directions
  if (-1 == GPIODirection(OUT_TO_BUTTON, OUT) || -1 == GPIODirection(IN_FROM_BUTTON, IN)) // POUT2 21, PIN 20
    return(2);
  if ( -1 == GPIOWrite (OUT_TO_BUTTON, 1)) // 처음 pin 21에 1을 write하면 이 값은 계속 유지된다.
    return(3);

  printf("button wait...\n");
  do{
    value = GPIORead(IN_FROM_BUTTON);
    // printf ("%2d) GPIORead : %d from pin %d, prev : %d\n", repeat, value, IN_FROM_BUTTON, prev);
    if(value == 0 && prev == 1){ //button의 (value)값이 1에서 0으로 될 때 <- button을 !!!press하고 땔 때!!!
      break;
    }
    prev = value;
  } while(1);

  int fd = open(DEVICE, O_RDWR); // opening the DEVICE file as read/write
  if (fd <= 0) {
    printf( "Device %s not found\n", DEVICE);
    pthread_exit(NULL);
  }


  if(fork() == 0){
    /* child */
//     if(prepare(fd, MODE1) == -1){
//       printf( "Fail to prepare()\n");
//     }
// #define PWM1 1
//     PWMExport(PWM1); // pwm0 is gpio18
//     PWMWritePeriod(PWM1, PERIOD);
//     PWMWriteDutyCycle(PWM1, 20000000);
//     PWMEnable(PWM1);

//     printf("========= pwn1 start =========\n");

//     int light = 0;
//     for(int i=0; i<TIME; i++){
//       light = readadc(fd, 0);
//       PWMWriteDutyCycle(PWM1,PERIOD-(light*20000));
//       printf("1 -> %d) light : %d\n", i, light);
//       usleep(100000);
//     }

//     PWMWriteDutyCycle(PWM1, 20000000);
    printf("============= pwm1 finish ============\n");
  }
  else{
    /* parent */
    if(prepare(fd, MODE0) == -1){
      printf( "Fail to prepare()\n");
    }
    #define PWM0 0

    PWMExport(PWM0); // pwm0 is gpio18
    PWMWritePeriod(PWM0, PERIOD);
    PWMWriteDutyCycle(PWM0, 20000000);
    PWMEnable(PWM0);

    printf("======== pwm0 start =========\n");

    int light = 0;
    for(int i=0; i<TIME; i++){
      light = readadc(fd, 0);
      PWMWriteDutyCycle(PWM0,PERIOD-(light*22000));
      printf("0 -> %d) light : %d\n", i, light);
      usleep(100000);
    }

    PWMWriteDutyCycle(PWM0, 20000000);
    printf("============= pwm0 finish ============\n");
  }

  return 0;
}