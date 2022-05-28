#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "header/gpioRW.h"

#define IN 0
#define OUT 1
#define LOW 0
#define HIGH 1
#define PIN20 20
#define POUT 17
#define PIN21 21

int main(int argc, char *argv[])
{
  int repeat = 100;
  int state = 1;
  int prev_state = 1;
  int light = 0;
  int value;
  int prev = 0;

  //Enable GPIO pins
  if (-1 == GPIOExport(PIN21) || -1 == GPIOExport(PIN20) || -1 == GPIOExport(POUT))
    return(1);

  //Set GPIO directions
  if (-1 == GPIODirection(PIN21, OUT) || -1 == GPIODirection(PIN20, IN) || -1 == GPIODirection(POUT, OUT)) // PIN21 21, PIN20 20
    return(2);

  if ( -1 == GPIOWrite (PIN21, 1)) // 처음 pin 21에 1을 write하면 이 값은 계속 유지된다.
    return(3);
  do {
    // if ( -1 == GPIOWrite (PIN21, 1))
    //   return(3);

    /**
     * 21(PIN21)에 계속 전류가 흐르게 하고 그 전류는 button 눌러져 ground와 연결되지 않는 이상
     * 21(PIN21)로 흘러들어와 20(PIN20)으로 그대로 흘러간다. 만약 button을 누르게 되면 21(PIN21)와 연결된
     * button의 한 쪽 pin이 ground와 연결된 button의 다른 한 쪽 pin과 연결되면서 전류가 ground로 흐르게 되어
     * 20(PIN20)에는 전류가 흐르지 않게 된다.
     * 따라서 button을 누르면 20(PIN20)에는 전류가 흐르지 않게 되어 0이 read된다.
     */
    value = GPIORead(PIN20);
    printf ("%2d) GPIORead : %d from pin %d, prev : %d\n", repeat, value, PIN20, prev);

    if(value == 0 && prev == 1){ //button의 (value)값이 1에서 0으로 될 때 <- button을 press할 때
      if(GPIORead(POUT) == 0){
        GPIOWrite(POUT, 1);
      }
      else if(GPIORead(POUT) == 1){
        GPIOWrite(POUT, 0);
      }
      else{
        fprintf(stderr, "Failed to read LED value!\n");
        return(5);
      }
    }

    prev = value;
    usleep(100000);
  }
  while (repeat--);

  //Disable GPIO pins
  if (-1 == GPIOUnexport(PIN21) || -1 == GPIOUnexport(PIN20) || -1 == GPIOUnexport(POUT))
    return(4);

  return(0);
}