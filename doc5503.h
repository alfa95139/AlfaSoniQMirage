
#ifndef _DOC5503_H
#define _DOC5503_H

#include <Arduino.h>
#include "bus.h"
#include "AudioStream.h"
#include "arm_math.h"

class Q : public AudioStream
{
public:
  audio_block_t *block_ch0;
  audio_block_t *block_ch1; 
  bool QinitDone = false;

 
  Q(void) : AudioStream(0,NULL) { 
                begin();  
               
            }  
  virtual void update(void);
  void init();
 
  void doc_halt_osc(int onum, int type, uint32_t *accumulator, int resshift);

private:
void begin(){   
  }
  



 
 const int samples =  AUDIO_BLOCK_SAMPLES ; // determines how many samples the audio library processes per update.
 const int output_channels = 2; // stereo

public:

 virtual ~Q() {

        };

};

void doc_run(CPU6809* cpu);
uint8_t doc_rreg(uint8_t reg);
void doc_wreg(uint8_t reg, uint8_t val);
 //void doc_halt_osc(int onum, int type, uint32_t *accumulator, int resshift);
 //int16_t *buffer_0;
 //int16_t *buffer_1;


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
  bool     bankselect;
  uint32_t accumulator;
  uint8_t  irqpend;
} DOC5503Osc;



#endif
