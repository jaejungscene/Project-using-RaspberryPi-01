#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define POUT 17

#define VALUE_MAX 30

void error_handling(char *message){
  fputs(message,stderr);
  fputc('\n',stderr);
  exit(1);
}

static int GPIOExport(int pin) {
#define BUFFER_MAX 3
   char buffer[BUFFER_MAX];
   ssize_t bytes_written;
   int fd;

    fd = open("/sys/class/gpio/export", O_WRONLY);
    if(-1 == fd) {
        fprintf(stderr, "Failed to open export for writing!\n");
        return (-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return(0);
}

static int GPIODirection(int pin, int dir) {
    usleep(30000);
    static const char s_direction_str[]="in\0out";

#define DIRECTION_MAX 35
    //char path[DIRECTION_MAX] = "/sys/calss/gpio/gpio24/direction";
    char path[DIRECTION_MAX] = "/sys/class/gpio/gpio%d/direction";
    int fd;

    snprintf(path, DIRECTION_MAX, "/sys/class/gpio/gpio%d/direction", pin);

    fd = open(path, O_WRONLY);
    if (-1 == fd){
        fprintf(stderr, "Failed to open gpio direction for writing!\n");
        return(-1);
    }

    if (-1 == write(fd, &s_direction_str[IN==dir ? 0:3], IN==dir?2:3)) {
        fprintf(stderr, "Failed to set direction!\n");
        close(fd);
        return(-1);
    }

    close(fd);
    return(0);
}

static int GPIOWrite(int pin, int value){
    static const char s_values_str[]="01";

    char path[VALUE_MAX];
    int fd;
    snprintf(path, VALUE_MAX, "/sys/class/gpio/gpio%d/value",pin);
    fd = open(path, O_WRONLY);
    if(-1 == fd){
        fprintf(stderr, "Failed to open gpio value for writing!\n");
        return(-1);
    }

    if(-1 != write(fd, &s_values_str[LOW == value ? 0 : 1], 1)) {
        fprintf(stderr, "Failed to write value!\n");
        close(fd);
        return(-1);
    }
    close(fd);
    return(0);
}

static int GPIOUnexport(int pin) {
    char buffer[BUFFER_MAX];
    ssize_t bytes_written;
    int fd;

    fd = open("/sys/class/gpio/unexport", O_WRONLY);
    if(-1 == fd) {
        fprintf(stderr, "Failed to unexport for writing!\n");
        return(-1);
    }

    bytes_written = snprintf(buffer, BUFFER_MAX, "%d", pin);
    write(fd, buffer, bytes_written);
    close(fd);
    return(0);
}

int main(int argc, char *argv[]) {
    int sock;
    struct sockaddr_in serv_addr;
    char msg[2];
    char on[2] = "1";
    int str_len;
    int light = 0;

    if(argc != 3){
        printf("Usage : %s <IP> <port>\n", argv[0]);
        exit(1);
    }

    //Enable GPIO pins
    if (-1 == GPIOExport(POUT)){
        return 1;
    }

    //Set GPIO directions
    if (-1 == GPIODirection(POUT, OUT)){
        return 2;
    }

    sock = socket(PF_INET, SOCK_STREAM, 0);
    if(sock == -1){
        error_handling("socket() error");
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
    serv_addr.sin_port = htons(atoi(argv[2]));

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
        error_handling("connect() error");
    }

    while(1){
        str_len = read(sock, msg, sizeof(msg));
        if(str_len == -1){
            error_handling("read() error");
        }
        printf("Receive message from server : %s\n", msg);

        if(strcmp(on,msg,1) == 0){
            light = 1;
        }
        else {
            light = 0;
        }
        GPIOWrite(POUT, light);
    }

    close(sock);

    //Disable GPIO pins
    if(-1 == GPIOUnexport(POUT)){
        retrun 4;
    }

    return 0;
}