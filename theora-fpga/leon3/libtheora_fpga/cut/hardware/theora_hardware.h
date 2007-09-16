#ifndef THEORA_HARDWARE_H_
#define THEORA_HARDWARE_H_

//#include <io.h>
//#include <system.h>

//void write_theoradriver(int pf, int data);

 
void write_theora_apb(int data);

void reset_hw(void);

void set_hw_address(void);

int is_first_time(int set);

#include "theora_hardware.c"

#endif /*THEORA_HARDWARE_H_*/

