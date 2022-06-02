#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include "header/spiRW.h"
#include "header/mySocket.h"
#include "header/gpioRW.h"

#define MAX_MSG 30
#define FRONT_PIN 17
#define BACK_PIN 22
#define SPEAKER_PIN 27
#define WAIT_TIME 300000
#define IN 0
#define OUT 1
#define LOW 0


int signal_from_main = 0;
int fd; //adc fd
int front_value; // value of left pressure sensor
int back_value; // value of right pressur esensor
int accident  = 0; // 1이 되면 accident 발생
int speaker_flag = 0;


void *alert_to_server(void *argv){
   printf("alert thread start\n");

   int sock;
   struct sockaddr_in serv_addr;
   int str_len;

   sock = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP // SOCK_DGRAM = UDP
   if(sock == -1) {
      error_handling("socket() error");
   }
   memset(&serv_addr, 0, sizeof(serv_addr));
   serv_addr.sin_family = AF_INET; //IPv4
   serv_addr.sin_addr.s_addr = inet_addr(((char**)argv)[2]); // <-- 변경!!!!!!!!!
   serv_addr.sin_port = htons(atoi(((char**)argv)[3])); // <-- 변경!!!!!!!!

   printf("%s : %s\n", ((char**)argv)[2], ((char**)argv)[3]);
   while(signal_from_main == 1){
      if(accident){
         connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr));
         // if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) == -1){
         //    error_handling("connect() error");
         // }
         printf("** send msg to server **\n");
         write(sock, "two", sizeof("two"));
         close(sock);
      }
   }
   int flag = 0;

   pthread_exit(NULL);
}

void *speaker_warning(void *param){
   GPIOExport((long)param);
   GPIODirection((long)param, OUT);

   while(signal_from_main){
   if(speaker_flag && ((long)param == SPEAKER_PIN)){
      int repeat = 4; //반복 횟수
      int period = 3000; //반복 term
      for(int i=0; i<repeat; i++){
         GPIOWrite((long)param, i%2);
         usleep(period);
         }
      }
   }

   GPIOWrite((long)param, LOW);
   GPIOUnexport((long)param);
}

void *weight_sensor_worker(void *param){
   printf("weight thread start\n");
   GPIOExport((long)param);
   GPIODirection((long)param, IN);

   while(signal_from_main == 1){
      if((long)param == FRONT_PIN){
         front_value = GPIORead(FRONT_PIN);   // 앞쪽 무게센서가 감지되면, front_value == 1
         printf("Front weight sensor Detected!\n");
      }
      if((long)param == BACK_PIN){
         back_value = GPIORead(BACK_PIN);    // 뒨쪽 무게센서가 감지되면, front_value == 1
         printf("Back weight sensor Detected!"\n);
      }
      if(front_value && back_value){            // 앞쪽과 뒤쪽 무게센서가 감지되면 accident 발생
         accident = 1;  //Server에 정보 날리기 - alert_to_server() 함수에서 multi-thread로 실행됨.
         printf("2인이상 탑승중!");
      }
      while(accident){
         speaker_flag = 1;  //스피커 출력
         if((GPIORead(FRONT_PIN) + GPIORead(BACK_PIN)) < 2){  // 다시 1인 이하탑승일시 경고음 중지
            accident = 0;     // 중지
            speaker_flag = 0; // 스피커 중지
         }
         usleep(WAIT_TIME);
      }
      usleep(WAIT_TIME);
   }


   GPIOWrite((long)param, LOW);
   GPIOUnexport((long)param);

   printf("====== finish weight_thd ======\n");
   pthread_exit(NULL);
}

// ./More_Than_Two_Warning01 <port> <Server ip> <Server port>
int main(int argc, char *argv[]){
   printf("== automatic report start ==\n");
   pthread_t alert, front_weight_sensor, back_weight_sensor, speaker;

   int serv_sock, clnt_sock = -1; // socket filedescriptor
   struct sockaddr_in serv_addr, clnt_addr;
   socklen_t clnt_addr_size = sizeof(clnt_addr);
   char msg[MAX_MSG];

   serverPrepare(&serv_sock, &serv_addr, &argv[1]);
   clnt_sock = accept(serv_sock, (struct sockaddr *)&clnt_addr, &clnt_addr_size);
   if (clnt_sock == -1) error_handling("accept() error");
   printf("=== complete connection with %s ===\n", inet_ntoa(clnt_addr.sin_addr));

   int str_len = -1;
   while(1)
   {
      printf("wait read...\n");
      str_len = read(clnt_sock, msg, sizeof(msg));
      if (str_len == -1){
         error_handling("Err : read() error");
      }
      else if (!strcmp(msg, "1")){
         printf("** active **\n");
         signal_from_main = 1;
      }
      else if (!strcmp(msg, "0")){
         printf("** inactive **\n");
         signal_from_main = 0;
         continue;
         // close(clnt_sock);
      }
      
      pthread_create(&alert, NULL, alert_to_server, (void*)argv);
      pthread_create(&front_weight_sensor, NULL, weight_sensor_worker, (void*)FRONT_PIN);
      pthread_create(&back_weight_sensor, NULL, weight_sensor_worker, (void*)BACK_PIN);
      pthread_create(&speaker, NULL, speaker_warning, (void*)SPEAKER_PIN);
      printf("============= check03 ===============\n");
      // pthread_create(&high_vibration_sensor, NULL, vibration_sensor_worker, NULL);
      // pthread_create(&low_vibration_sensor, NULL, vibration_sensor_worker, NULL);
      // pthread_join(alert, NULL);
      // pthread_join(front_weight_sensor, NULL);
      // pthread_join(back_weight_sensor, NULL);
      // pthread_join(high_vibration_sensor, NULL);
      // pthread_join(low_vibration_sensor, NULL);
   }

   

   printf("== automatic report finish ==\n");
   return 0;
}