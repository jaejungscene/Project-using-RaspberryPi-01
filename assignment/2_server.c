#include <fcntl.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <string.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <stdint.h>
#include <getopt.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/spi/spidev.h>
#include <sys/stat.h>
#include <sys/types.h>

#define IN 0 
#define OUT 1 
#define LOW 0
#define HIGH 1

#define PIN 20
#define POUT 21

#define ARRAY_SIZE(array) sizeof(array) / sizeof(array[0])
static uint8_t MODE = SPI_MODE_0;
static const char *DEVICE = "/dev/spidev0.0";
static uint8_t BITS = 8;
static uint32_t CLOCK = 1000000;
static uint16_t DELAY = 5;

/* Ensure all settings are correct for the ADC */
static int prepare(int fd) {
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

void error_handling(char *message){
  fputs(message,stderr);
  fputc('\n',stderr);
  exit(1);
}

/* ./server <server Port> */
#define MAX_MSG 30
int main(int argc, char *argv[])
{
  int serv_sock, clnt_sock = -1; // socket filedescriptor
  struct sockaddr_in serv_addr,clnt_addr; 
  socklen_t clnt_addr_size;
  char msg[MAX_MSG];

  if(argc!=1)
    printf("<port> : %s\n",argv[1]);
  serv_sock = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP
  if(serv_sock == -1) error_handling("socket() error");
  memset(&serv_addr, 0 , sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));
  if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1)
    error_handling("bind() error");
  if(listen(serv_sock,5) == -1) error_handling("listen() error");


  int fd = open(DEVICE, O_RDWR); // opening the DEVICE file as read/write
  if (fd <= 0) {
    printf( "Device %s not found\n", DEVICE);
    return -1;
  }
  if(prepare(fd) == -1){
    return -1;
  }


  int light;
  while(1){
    if(clnt_sock<0){
      clnt_addr_size = sizeof(clnt_addr);
      printf("waiting...\n");
      clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
      if(clnt_sock == -1) error_handling("accept() error");
    }

    printf("=== connectiond establish with %s ===\n", inet_ntoa(clnt_addr.sin_addr));

    int str_len = -1;
    printf("wait read...\n");
    while(1){
      str_len = read(clnt_sock, msg, sizeof(msg));
      if(str_len == -1){
        error_handling("read() error");
        continue;
      }
#define CHANNEL 2
      printf("%s\n", msg);
      if(strcmp(msg, "request") == 0){
        light = readadc(fd, CHANNEL);
        printf("write %d\n", light);
        snprintf(msg, MAX_MSG, "%d", light);
        write(clnt_sock, msg, sizeof(msg));
      }
    }
    close(clnt_sock);
    clnt_sock = -2;
  }

  printf("== finish ==\n");
  close(serv_sock);

  return(0);
}