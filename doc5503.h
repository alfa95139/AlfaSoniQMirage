
#ifndef _DOC5503_H
#define _DOC5503_H

#include <Arduino.h>
#include "bus.h"
#include "AudioStream.h"
#include "arm_math.h"



class Q : public AudioStream
{
  public:
    Q(void) : AudioStream (2, outputQueueArray) { }   // let's start with stereo
//  Q(void) : AudioStream (8, outputQueueArray) { }   //  8 channels mod
    virtual void update(void);
    void begin(void);
//protected:
private:
  audio_block_t *outputQueueArray[2];
//audio_block_t *outputQueueArray[8]; //  8 channels mod
};




//uint8_t doc5503_irq();
void doc_init();
void doc_run(CPU6809* cpu);
uint8_t doc_rreg(uint8_t reg);
void doc_wreg(uint8_t reg, uint8_t val);
void doc_halt_osc(int onum, int type, uint32_t *accumulator, int resshift);
void audio_update ();
//void audio_update_CB(); // todo: build audioout

enum {
	MODE_FREE = 0,
	MODE_ONESHOT = 1,
	MODE_SYNCAM = 2,
	MODE_SWAP = 3
	};

#define output_channels  2 // stereo
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
