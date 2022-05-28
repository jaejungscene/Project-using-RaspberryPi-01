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

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define VALUE_MAX 256
#define DIRECTION_MAX 45
#define IN_FROM_BUTTON 20 // button의 동작을 감지하는 pin
#define OUT_TO_BUTTON 21 // 계속 전류가 흐르는 pin

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
  usleep(40000);
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
#define MAX_VALUE 140.0 // 실험 후 나온 조도센서의 최대값 + 2
#define WRITE_VALUE(light) (int)(PERIOD*(1-(light/MAX_VALUE))) // 조도센서의 값이 작을수록 값이 커지도록 설계
#define TIME 300

static uint8_t MODE0 = SPI_MODE_0;
static uint8_t MODE1 = SPI_MODE_1;
static const char *DEVICE = "/dev/spidev0.0";
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
    if(light <= MAX_VALUE){ // 다른 채널의 조도센서에서 값이 변화하면 light값이 MAX_VALUE보다 커진다.
      PWMWriteDutyCycle( pwmnum, WRITE_VALUE(light) );
      printf("%d -> %d) light: %3d, written value: %d\n", pwmnum, i, light, WRITE_VALUE(light) );
    }
    usleep(90000);
  }

  PWMWriteDutyCycle(pwmnum, PERIOD); /////////// test
}

void* thread0()
{
#define PWM0 0
#define CHANNEL0 0
  if(prepare(fd, MODE0) == -1){
    pthread_exit(NULL);
  }
  printf("-------- thread0 start --------\n");
  work(PWM0, CHANNEL0);
  printf("-------- thread0 finish --------\n");
  pthread_exit(NULL);
}

void* thread1()
{
#define PWM1 1
#define CHANNEL1 1
  if(prepare(fd, MODE1) == -1){
    pthread_exit(NULL);
  }
  printf("-------- thread1 start --------\n");
  work(PWM1, CHANNEL1);
  printf("-------- thread1 finish --------\n");
  pthread_exit(NULL);
}

int main()
{

  printf("================ !! program start !! ================\n");

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

  fd = open(DEVICE, O_RDWR); // opening the DEVICE file as read/write
  if (fd <= 0) {
    printf( "Device %s not found\n", DEVICE);
    return (-1);
  }

  pthread_t threads[2];
  pthread_create(&threads[0], NULL, thread0, NULL);
  pthread_create(&threads[1], NULL, thread1, NULL);
  pthread_join(threads[0], NULL);
  pthread_join(threads[1], NULL);

  printf("================ !! program finish !! ================\n");
  return 0;
}