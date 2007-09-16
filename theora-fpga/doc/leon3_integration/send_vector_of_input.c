#include "input.h"

// André Costa - andre.lnc@gmail.com


struct theora_regs_t {
  volatile int flag_send_data;   // Can the driver send a data to Theora Hardware?
  volatile int data_transmitted; // Data Transmitted  to Theora Hardware
  volatile int flag_read_data;   // Can the driver receive a data from Theora Hardware?
  volatile int data_received;    // Data received from Theora Hardware
};

struct theora_regs_t  * theora_regs = (struct theora_regs_t  *)0x80000800;

main()
{

int i = 0, j=0, d = 500;

  while(1) {
    

    if (i <= INPUT_SIZE) {
    while(theora_regs->flag_send_data) { 
     
     if (d == 500) {
         printf( "input[%d] = %d, 0x%08X\n", i, input[i], input[i]);
         d = 0;
      }
      d++;      
      theora_regs->data_transmitted = input[i];
      i++;
      if (i > INPUT_SIZE) break;
    }
    
   }
   
    while(theora_regs->flag_read_data) {
       printf( "output[%d] =  %d\n",  j++, theora_regs->data_received);
    }

  }
}
