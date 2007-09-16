#include "theora_hardware.h"
/*
int write_int_theora(int value) {
  // While cannot write do a busy wait
  while (!IORD(CASCA_AVALON_0_BASE, 0));
  
  // If can write
  if (IORD(CASCA_AVALON_0_BASE, 0)) {
    IOWR( CASCA_AVALON_0_BASE, 0, value );
    return 1;  
  } else {
    return 0;
  }
}


struct _data
{
    int read;	// Driver read on Theora Hardware
    int wrote;	// Driver wrote on Theora Hardware
    int data;	// Read or transmitted data
};

  struct _data dt;  
  static int pf;
*/
  
struct theora_regs_t {
  volatile int * flag_send_data;   // Can the driver send a data to Theora Hardware?
  volatile int * data_transmitted; // Data Transmitted  to Theora Hardware
  volatile int * flag_read_data;   // Can the driver receive a data from Theora Hardware?
  volatile int * data_received;    // Data received from Theora Hardware
};

struct theora_regs_t  theora_regs;

volatile int * reset;    

void set_hw_address() {
  theora_regs.flag_send_data = (volatile int *)0x80000800;
  theora_regs.data_transmitted = (volatile int  *)0x80000804;
  theora_regs.flag_read_data = (volatile int  *)0x80000808;
  theora_regs.data_received = (volatile int  *)0x8000080C;
  reset = (volatile int  *)0x80000810;
}
  
/*
void write_theoradriver(int pf, int data) {
    dt.data = data;        
    do ioctl(pf, 0, &dt); while (dt.read == 0);
    //if (ae <= 10 || !(ae % 500)) printf("[%d] := %d\n", ae++, data);   
}
*/

void write_theora_apb(int data) {
    //for(i=0;i<10;i++) for(a=0;a<12;a++);
    while(!*theora_regs.flag_send_data);
    *theora_regs.data_transmitted = data; 
}


volatile int * reset;    

void reset_hw(void) {
    *reset = 1;
}

  
int is_first_time(int set) {
    static int first_time = 1;
    if (set == 1) 
        first_time = 1;
    else if (set == 0)
        first_time = 0;
        
    return first_time;
}

