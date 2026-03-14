/************************************
Copyright 2012 Analog Devices, Inc.
Permission to freely use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies:
THIS SOFTWARE IS PROVIDED “AS IS” AND LTC DISCLAIMS ALL WARRANTIES
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
EVENT SHALL LTC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM ANY USE OF SAME, INCLUDING
ANY LOSS OF USE OR DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
***********************************************************//************************************
Copyright 2012 Analog Devices, Inc.
Permission to freely use, copy, modify, and distribute this software for any
purpose with or without fee is hereby granted, provided that the above
copyright notice and this permission notice appear in all copies:
THIS SOFTWARE IS PROVIDED “AS IS” AND LTC DISCLAIMS ALL WARRANTIES
INCLUDING ALL IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO
EVENT SHALL LTC BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM ANY USE OF SAME, INCLUDING
ANY LOSS OF USE OR DATA OR PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE
OR OTHER TORTUOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
PERFORMANCE OF THIS SOFTWARE.
***********************************************************/
#include "PEC.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
uint16_t pec15Table[256];

/*
int main(){
  init_PEC15_Table();
  char data[2] = {0 , 1};
  uint16_t result = pec15(data, 2);
  printf("result: %X", result);
  return 0;
}
//*/
void init_PEC15_Table() {
  int remainder;
  for (int i = 0; i < 256; i++) {
    remainder = i << 7;
    for (int bit = 8; bit > 0; --bit) {
      if (remainder & 0x4000) {
        remainder = ((remainder << 1));
        remainder = (remainder ^ CRC15_POLY);
      } else {
        remainder = ((remainder << 1));
      }
    }
    pec15Table[i] = remainder & 0xFFFF;
  }
}

uint16_t pec15(char *data, int len) {
  uint16_t remainder, address;
  remainder = 16;  // PEC seed
  for (int i = 0; i < len; i++) {
    address =
        ((remainder >> 7) ^ data[i]) & 0xff;  // calculate PEC table address
    remainder = (remainder << 8) ^ pec15Table[address];
  }
  return (remainder * 2);  // The CRC15 has a 0 in the LSB so the final value
                           // must be multiplied by 2
}

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "PEC.h"
uint16_t pec15Table[256];

/*
int main(){
  init_PEC15_Table();
  char data[2] = {0 , 1};
  uint16_t result = pec15(data, 2);
  printf("result: %X", result);
  return 0;
}
//*/
void init_PEC15_Table() {
  int remainder;
  for (int i = 0; i < 256; i++) {
    remainder = i << 7;
    for (int bit = 8; bit > 0; --bit) {
      if (remainder & 0x4000) {
        remainder = ((remainder << 1));
        remainder = (remainder ^ CRC15_POLY);
      } else {
        remainder = ((remainder << 1));
      }
    }
    pec15Table[i] = remainder & 0xFFFF;
  }
}

uint16_t pec15(char *data, int len) {
  uint16_t remainder, address;
  remainder = 16;  // PEC seed
  for (int i = 0; i < len; i++) {
    address =
        ((remainder >> 7) ^ data[i]) & 0xff;  // calculate PEC table address
    remainder = (remainder << 8) ^ pec15Table[address];
  }
  return (remainder * 2);  // The CRC15 has a 0 in the LSB so the final value
                           // must be multiplied by 2
}
