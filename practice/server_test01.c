#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h> 
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <string.h> 
#include <arpa/inet.h>
#include <sys/socket.h>
#include <pthread.h>

#define IN 0 
#define OUT 1 
#define LOW 0
#define HIGH 1
#define PIN 20
#define POUT 21
#define VALUE_MAX 40

void error_handling(char *message){
  fputs(message,stderr);
  fputc('\n',stderr);
  exit(1);
}

void *receive(){
  pthread_exit(NULL);
}

/* ./server <server Port> */
#define MAX_MSG 30
int main(int argc, char *argv[])
{ 
  int serv_sock, clnt_sock=-1; // socket filedescriptor
  struct sockaddr_in serv_addr,clnt_addr; 
  socklen_t clnt_addr_size;
  char msg[MAX_MSG];

  if(argc!=1){
    printf("<port> : %s\n",argv[1]);
  }


  serv_sock = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP
  if(serv_sock == -1) error_handling("socket() error");
  memset(&serv_addr, 0 , sizeof(serv_addr));
  serv_addr.sin_family = AF_INET;
  serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  serv_addr.sin_port = htons(atoi(argv[1]));


  /**
   * bind(2)
   * int bind(int sockfd, const struct sockaddr *addr,
                socklen_t addrlen);
   * sockfd 는 socket(2) 함수를 통해서 만들어진 소켓지정번호이다.
   * bind 는 이 sockfd 에 my_addr 로 대변되는 특성을 부여한다.
   * my_addr 에는 sockfd 가 통신을 하기 위해서 필요한 정보인 
   * "port", "인터넷주소", "소켓 흐름종류" 등을 포함하고 있다.
   */
  if(bind(serv_sock, (struct sockaddr*) &serv_addr, sizeof(serv_addr))==-1)
    error_handling("bind() error");


  /**
   * https://www.joinc.co.kr/w/man/2/listen
   * listen(2)
   * int listen(int sockfd, int backlog);
   * 서버(:12)측 프로그램은 socket(2)함수를 이용해서 클라이언트(:12)의 연결을 받아들일 듣기소켓을 만들게 된다. 클라이언트의 연결은 듣기소켓(:12)을 통해서 이루어지는데 클라이언트는 connect(2)를 호출해서 서버에 연결을 시도하고, 3번:::악수기법(:12)이 성공하면 서버와 완전한 연결이 만들어 진다.
   * 만들어진 연결은 queue(:12)에 들어가게 되고 서버측에서 accept(2)를 호출하면 비로서 서버는 연결소켓을 만들고 만들어진 연결소켓(:12)을 이용해서 클라이언트와 통신하게 된다.
   * listen(2) 시스템호출(:12)은 SOCK_STREAM과 SOCK_SEQPACKET에만 사용된다.
   * a는 socket(2)에 의해서 만들어진 듣기 소켓이다. backlog는 연결이 대기할 수 있는 큐의 갯수이다. 만약 backlog에 연결이 모두 찬 상태에서 새로운 연결을 시도한다면, 클라이언트는 ECONNREFUSED 에러를 받게될 것이다. 만약 재전송을 지원하는 프로토콜을 사용한다면 에러를 무시하고 성공할 때까지 재시도를 하게 된다.
   */
  if(listen(serv_sock,5) == -1) error_handling("listen() error");


  while(1){
  /**
   * accept(2)
   * int accept(int s, struct sockaddr *addr, socklen_t *addrlen);
   * accept() 함수는 연결지향 소켓 타입 (SOCK_STREAM, SOCK_SEQPACKET, SOCK_RDM)에 사용된다. 이것은 아직 처리되지 않은 연결들이 대기하고 있는 큐에서 제일 처음 연결된 연결을 가져와서 새로운 연결된 소켓을 만든다. 그리고 소켓을 가르키는 파일 지정자를 할당하고 이것을 리턴한다.
   * 인자 s 는 socket() 로 만들어진 end-point(듣기 소켓)을 위한 파일지정자이다.
   * 인자 addr 는 sockaddr 구조체에 대한 포인터이다. 연결이 성공되면 이 구조체를 채워서 되돌려 주게 되고, 우리는 이구조체의 정보를 이용해서 연결된 클라이언트의 인터넷 정보를 알아낼수 있다. addrlen 인자는 addr의 크기 이다.
   */
  if(clnt_sock<0){
    clnt_addr_size = sizeof(clnt_addr);
    printf("waiting...\n");
    if(clnt_sock == -2){
      printf(">> ");
      fgets(msg, sizeof(msg), stdin);
      msg[strlen(msg)-1] = '\0';
      if(!strcmp(msg, "exit")){
        break;
      }
    }
    clnt_sock = accept(serv_sock, (struct sockaddr*)&clnt_addr, &clnt_addr_size);
    if(clnt_sock == -1)
      error_handling("accept() error");
  }

  printf("=== connectiond establish with %s ===\n", inet_ntoa(clnt_addr.sin_addr));

  // pthread_t receiver, sender;

  int str_len = -1;
    while(1){
      printf(">> ");
      fgets(msg, sizeof(msg), stdin);
      write(clnt_sock, msg, sizeof(msg));
      printf("wait read...\n");
      str_len = read(clnt_sock, msg, sizeof(msg));
      if(str_len == -1){
        error_handling("read() error");
        continue;
      }
      else if(!strcmp(msg, "exit")){
        printf("** disconnect **\n");
        break;
      }
      printf("from client : %s\n", msg);
    }
    close(clnt_sock);
    clnt_sock = -2;
  }

  printf("== finish ==\n");
  close(serv_sock);

  return(0);
}