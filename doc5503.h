
#ifndef _DOC5503_H
#define _DOC5503_H

#include "bus.h"

uint8_t doc5503_irq();
void doc_init();
void doc_run(CPU6809* cpu);
uint8_t doc_rreg(uint8_t reg);
void doc_wreg(uint8_t reg, uint8_t val);


enum {
	MODE_FREE = 0,
	MODE_ONESHOT = 1,
	MODE_SYNCAM = 2,
	MODE_SWAP = 3
	};


typedef struct
{
  uint16_t freq;
  uint16_t wtsize;
  uint8_t  control;
  uint8_t  vol;
  uint8_t  data;
  uint32_t wavetblpointer; // DOC5503 implementation uses 8bits, but we need 17 bits!!!
  uint8_t  wavetblsize;
  uint8_t  resolution;

  uint32_t accumulator;
  uint8_t  irqpend;
} DOC5503Osc;

#endif
