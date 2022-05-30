#include <arpa/inet.h>
#include <sys/socket.h>

void clientConnecting(int *sock, struct sockaddr_in *dest_addr, char *args[])
{
   *sock = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP // SOCK_DGRAM = UDP
   if(*sock == -1) error_handling("socket() error");
   memset(dest_addr, 0, sizeof(*dest_addr));
   dest_addr->sin_family = AF_INET; //IPv4
   dest_addr->sin_addr.s_addr = inet_addr(args[0]); // The inet_addr(const char *cp) function shall convert the string pointed to by cp, in the standard IPv4 dotted decimal notation, to an integer value suitable for use as an Internet address.
   dest_addr->sin_port = htons(atoi(args[1])); //argv[2] = port number // htonl, htons : host byte order을 따르는 데이터를 network byte order로 변경한다.

   if(connect(*sock, (struct sockaddr*)dest_addr, sizeof(*dest_addr)) == -1)
      error_handling("connect() error");
   printf("** complete connecting with %s **\n", args[0]);
}

void serverPrepare(int *sock, struct sockaddr_in *my_addr, char *args[])
{ // 1) create socket  ,  2) bind ,  3) listen
  *sock = socket(PF_INET, SOCK_STREAM, 0); // PF_INET = IPv4, SOCK_STREAM = TCP
  if(*sock == -1) error_handling("socket() error");
  memset(my_addr, 0 , sizeof(*my_addr));
  my_addr->sin_family = AF_INET;
  my_addr->sin_addr.s_addr = htonl(INADDR_ANY);
  my_addr->sin_port = htons(atoi(args[0]));

  if(bind(*sock, (struct sockaddr*)my_addr, sizeof(*my_addr))==-1)
    error_handling("bind() error");

  if(listen(*sock, 5) == -1) error_handling("listen() error");
  printf("listening...\n");
}