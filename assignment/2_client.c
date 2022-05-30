#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <time.h>
#include <pthread.h>

#define LOW 0
#define HIGH 1

#define TRIG 23 //초음파센서
#define ECHO 24 //초음파센서
#define PIN 18  // LED

#define VALUE_MAX 256
#define DIRECTION_MAX 45

void error_handling(char *message)
{
  fputs(message, stderr);
  fputc('\n', stderr);
  exit(1);
}

static int PWMExport(int pwmnum)
{
#define BUFFER_MAX 3

  char buffer[BUFFER_MAX];
  int bytes_written;
  int fd;

  fd = open("/sys/class/pwm/pwmchip0/unexport", O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Faild to open in unexport\n");
    return (-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
  write(fd, buffer, bytes_written); // echo 0 > unexport
  close(fd);

  sleep(1);
  fd = open("/sys/class/pwm/pwmchip0/export", O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open in export!\n");
    return (-1);
  }

  bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pwmnum);
  write(fd, buffer, bytes_written); // echo 0 > export
  close(fd);
  sleep(1);
  return (0);
}

static int PWMEnable(int pwmnum)
{
  static const char s_unenable_str[] = "0";
  static const char s_enable_str[] = "1";

  char path[DIRECTION_MAX];
  int fd;

  snprintf(path, DIRECTION_MAX, "/sys/class/pwm/pwmchip0/pwm%d/enable", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open in enable for echo 0!\n");
    return -1;
  }
  write(fd, s_unenable_str, strlen(s_unenable_str)); // echo 0 > pwm(pwmnum)/enable
  close(fd);

  fd = open(path, O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open in enable for echo 1!\n");
    return -1;
  }
  write(fd, s_enable_str, strlen(s_enable_str)); // echo 1 > pwm(pwmnum)/enable
  close(fd);

  return (0);
}

static int PWMWritePeriod(int pwmnum, int value)
{
  char s_values_str[VALUE_MAX];
  char path[VALUE_MAX];
  int fd, byte;

  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/period", pwmnum);

  fd = open(path, O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open in period!\n");
    return (-1);
  }

  byte = snprintf(s_values_str, VALUE_MAX, "%d", value);

  if (-1 == write(fd, s_values_str, byte))
  { // echo (value) > pwm(pwmnum)/period
    fprintf(stderr, "Failed to write value in period\n");
    close(fd);
    return (-1);
  }
  close(fd);

  return (0);
}

static int PWMWriteDutyCycle(int pwmnum, int value)
{
  char path[VALUE_MAX];
  char s_values_str[VALUE_MAX];
  int fd, byte;

  snprintf(path, VALUE_MAX, "/sys/class/pwm/pwmchip0/pwm%d/duty_cycle", pwmnum);
  fd = open(path, O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open in duty_cycle!\n");
    return (-1);
  }

  byte = snprintf(s_values_str, VALUE_MAX, "%d", value);

  if (-1 == write(fd, s_values_str, byte))
  { // echo (value) > pwm(pwmnum)/duty_cycle
    fprintf(stderr, "Failed to write value! in duty_cycle!\n");
    close(fd);
    return (-1);
  }
  close(fd);

  return (0);
}

static int GPIOExport(int pin)
{
  int Buffer_Max = 3;
  char buffer[Buffer_Max];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/export", O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open export for writing!\n");
    return (-1);
  }

  bytes_written = snprintf(buffer, Buffer_Max, "%d", pin);
  write(fd, buffer, bytes_written);
  close(fd);
  return (0);
}

static int GPIODirection(int pin, int dir)
{
  usleep(40000);
  static const char s_directions_str[] = "in\0out";

  int Direction_Max = 35;
  char path[Direction_Max];
  int fd;
  int in = 0;

  snprintf(path, Direction_Max, "/sys/class/gpio/gpio%d/direction", pin);

  fd = open(path, O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open gpio direction for writing!\n");
  }
  if (-1 == write(fd, &s_directions_str[in == dir ? 0 : 3], in == dir ? 2 : 3))
  {
    fprintf(stderr, "Failed to set direction!\n");
    close(fd);
    return (-1);
  }

  close(fd);
  return (0);
}

static int GPIOUnexport(int pin)
{
  int Buffer_Max = 3;
  char buffer[Buffer_Max];
  ssize_t bytes_written;
  int fd;

  fd = open("/sys/class/gpio/unexport", O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open unexport for writing!\n");
    return (-1);
  }
  bytes_written = snprintf(buffer, Buffer_Max, "%d", pin);
  write(fd, buffer, bytes_written); // write는 정확히 쓰고 싶은 만큼을 크기로 지정해야 함
  close(fd);
  return (0);
}

static int GPIORead(int pin)
{
  int Value_Max = 35;
  char path[Value_Max];
  char value_str[3];
  int fd;

  snprintf(path, Value_Max, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_RDONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open gpio value for reading!\n");
    return (-1);
  }

  if (-1 == read(fd, value_str, 3))
  {
    fprintf(stderr, "Failed to read value!\n");
    close(fd);
    return (-1);
  }

  close(fd);
  return (atoi(value_str));
}

static int GPIOWrite(int pin, int value)
{
  int Value_Max = 35;
  static const char s_values_str[] = "01";
  char path[Value_Max];
  int fd;
  int low = 0;

  snprintf(path, Value_Max, "/sys/class/gpio/gpio%d/value", pin);
  fd = open(path, O_WRONLY);
  if (-1 == fd)
  {
    fprintf(stderr, "Failed to open gpio value for writing!\n");
    return (-1);
  }
  if (1 != write(fd, &s_values_str[low == value ? 0 : 1], 1))
  {
    fprintf(stderr, "Failed to write value!\n");
    close(fd);
    return (-1);
  }

  close(fd);
  return (0);
}

/////////////////////////////////////////////////////////////
#define WRITE_VALUE(light) (int)(PERIOD * (1 - (light / MAX_VALUE))) // 조도센서의 값이 작을수록 값이 커지도록 설계
#define PERIOD 20000000
#define MAX_VALUE 900.0 // 실험 후 나온 조도센서의 최대값
#define MAX_MSG 30
#define WAIT_TIME 900000
int sock;
struct sockaddr_in serv_addr;
char msg[MAX_MSG];
int str_len;


void *ultrasonic_thd()
{
  // 초음파센서 세팅
  clock_t start_t, end_t;
  double time;
  double distance;

  // Enable GPIO pins
  if (-1 == GPIOExport(TRIG) || -1 == GPIOExport(ECHO))
  {
    printf("gpio export err\n");
    pthread_exit(NULL);
  }
  // wait for writing to export file
  usleep(100000);

  // Set GPIO directions
  if (-1 == GPIODirection(TRIG, HIGH) || -1 == GPIODirection(ECHO, LOW))
  {
    printf("gpio direction err\n");
    pthread_exit(NULL);
  }

  // init ultrawave trigger
  GPIOWrite(TRIG, 0);
  usleep(10000);

  while (1)
  {
    //초음파센서 인식
    printf("==============================\n");
    if (-1 == GPIOWrite(TRIG, 1))
    {
      printf("gpio write/trigger err\n");
      pthread_exit(NULL);
    }

    // 1sec == 1000000ultra_sec, 1ms = 1000ultra_sec
    usleep(10);
    GPIOWrite(TRIG, 0);

    while (GPIORead(ECHO) == 0)
    {
      start_t = clock();
    }
    while (GPIORead(ECHO) == 1)
    {
      end_t = clock();
    }

    time = (double)(end_t - start_t) / CLOCKS_PER_SEC; // ms
    distance = time / 2 * 34000;
    printf("time : %.4lf\n", time);
    printf("distance : %.2lfcm\n", distance);

    if (distance < 20.0)
    {
      printf("** REQUEST!! **\n");
      write(sock, "request", sizeof("request"));
    }
    usleep(WAIT_TIME); /////////////////
  }
}

void *led_thd()
{
  int light;
  PWMExport(0);
  PWMWritePeriod(0, PERIOD);
  PWMWriteDutyCycle(0, 0);
  PWMEnable(0);

  printf("wait read...\n");
  while (1)
  {
    //조도센서 값 받아오기
    PWMWriteDutyCycle(0, 0);
    str_len = read(sock, msg, sizeof(msg));
    if (str_len == -1)
      error_handling("read() error");
    else if(str_len > 0){
      //조도값
      light = atoi(msg);
      PWMWriteDutyCycle(0, WRITE_VALUE(light)); // light 값이 정확히 정해진 채널에서 왔을 때만 해당되는 LED의 밝기를 변화시킨다.
      printf("**** light: %3d, written value: %d ****\n", light, WRITE_VALUE(light));
    }

    usleep(WAIT_TIME);
  }
}

/////////////////////////////////////////////////////////////

/* ./client <server IP> <server Port> */
int main(int argc, char *argv[])
{

  if (argc != 3)
  {
    printf("<IP> : %s, <port> : %s\n", argv[1], argv[2]);
  }

  sock = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP // SOCK_DGRAM = UDP
  if (sock == -1)
    error_handling("socket() error");
  memset(&serv_addr, 0, sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;                 // IPv4
  serv_addr.sin_addr.s_addr = inet_addr(argv[1]); // The inet_addr(const char *cp) function shall convert the string pointed to by cp, in the standard IPv4 dotted decimal notation, to an integer value suitable for use as an Internet address.
  serv_addr.sin_port = htons(atoi(argv[2]));      // argv[2] = port number // htonl, htons : host byte order을 따르는 데이터를 network byte order로 변경한다.

  if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) == -1)
  {
    error_handling("connect() error");
  }
  printf("** complete connecting **\n");

  pthread_t ultrasonic, led;
  pthread_create(&ultrasonic, NULL, ultrasonic_thd, NULL);
  pthread_create(&led, NULL, led_thd, NULL);
  pthread_join(ultrasonic, NULL);
  pthread_join(led, NULL);

  // Disable GPIO pins
  if (-1 == GPIOUnexport(TRIG) || -1 == GPIOUnexport(ECHO))
    return (4);

  printf("complete\n");

  close(sock);

  return 0;
}
