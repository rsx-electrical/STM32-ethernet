#ifndef PEC_HEADER
#define PEC_HEADER
#include <stdint.h>
//#ifdef __cplusplus //for c->arduino translation
//extern "C" {
//#endif

#define CRC15_POLY 0x4599
extern uint16_t pec15Table[256];


void init_PEC15_Table(); //initializes a pec lookup table for pec0 & pec1 byes
uint16_t pec15(char *data , int len); //returns 16bit pec for pec[0:1]

//#ifdef __cplusplus
//}
//#endif

#endif //PEC_HEADER
