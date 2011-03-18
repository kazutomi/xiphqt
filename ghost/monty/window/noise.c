#include <stdio.h>

main(){
  int i,j;
  for(i=0;i<44100;i++){
    int rand1 = (rand()%41)-20;
    int rand2 = rand()%256;
    putchar(rand2);
    putchar(rand1);
    putchar(rand2);
    putchar(rand1);
  }
}
