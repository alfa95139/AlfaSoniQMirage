// license:BSD-3-Clause
// copyright-holders:R. Belmont


//Adapted from  ES5503 - Ensoniq ES5503 "DOC" emulator v2.1.1
// By R. Belmont.
// Copyright R. Belmont.
// For use with Teensyduino, modified  by Alessandro Fasan July 2020

//CA3 CA2 CA1 CA0 **** CHANNEL ASSIGNMENT ****
//Final voice Multiplexer
//16 channels are possible, The Mirage uses 8. CA3 is used to synchronize operations (See PB5 VIA6522).
// 
// CLK = 8Mhz is the clock frequency of the DOC in the Ensoniq Mirage
//
// SAMPLE RATE SR
// SR = CLK / ((OSC + 2) * 8)
// CLK is input clock
// OSC is number of total oscillators enabled
// Fundamental Output Frequency = [ SR / 2 ^(17 + RES)] * FC
// where FC is Frequency High and Frequency Low registers concatenated
// The waveform fundamental frequency length is equal to on page of memory

//OSCILLATOR MODES
// M2 	M1      MODE 
// 0	  0   - Free Run
// 0	  1   - Address One cycle, reset OSC accumulator, set Halt bit
// 1	  0   - Sync OSC2 to OSC1, or Amplitude Modulate OSC1 by OSC0
// 1  	1   - Address One cycle, reset OSC accumulator, set Halt bit then reset Halt bit
//
// M1 * H = 1 causes the oscillator to reset: This  needs to be evaluated during doc_wreg!
// 
// OS 3.2 initializes the Oscillator Control Register with:
// Oscillators  0-3 : value 0000_0011 (03)
//                  CA3, CA2, CA1, CA0 = 0000
// Oscillators  4-7 : value 0001_0011 (13)
//                  CA3, CA2, CA1, CA0 = 0001
// Oscillators  8-11: value 0010_0011 (23)
//                  CA3, CA2, CA1, CA0 = 0010
// Oscillators 12-15: value 0011_0011 (33)
//                  CA3, CA2, CA1, CA0 = 0011
// Oscillators 16-19: value 0100_0011 (43)
//                  CA3, CA2, CA1, CA0 = 0100
// Oscillators 20-23: value 0101_0011 (53)
//                  CA3, CA2, CA1, CA0 = 0101
// Oscillators 24-25: value 0110_0011 (63)
//                  CA3, CA2, CA1, CA0 = 0110
// Oscillators 26-27: value 1110_0011 (e3)
//                  CA3, CA2, CA1, CA0 = 1110
// Oscillators 28-31: value 1111_0011 (f3)
//                  CA3, CA2, CA1, CA0 = 1111
//  All the 32 Oscillator Control Registers are set with:
//                  Interrupt Enable --->: Disabled
//                  OSCILLATOR MODE ---->: ONE-SHOT
//                  Halt Bit ----------->: Enabled

//
// We may consider including the filters as part of this model
// E4_0001_0000 (E410) to E4_0001_0111 (E417): CUT OFF
// E4_0000_1000 (E408) to E4_0000_1111 (E40F): Resonance

#define DOC5503_DEBUG 0

#include <iterator>     // For std::fill_n
#include <vector>



#include "Arduino.h"
#include "log.h"
#include "doc5503.h"
#include "via.h"
#include "extern.h"  // For access to control 5503 DAC with VIA PB2, PB3

#include "arm_math.h"
//#include "utility/dspinst.h" //  multiply_32x32_rshift32, etc.

bool nomore = false;

//Functions: 
// getBuffer(); - Returns a pointer to an array of 128 int16. The buffer is within the audio lib memory pool, most efficient way to input data to the audio system. Request only one buffer at a time. NULL if no mem is avail.
// playBuffer(); - Tranmsit the buffer previously obtained from getBuffer();
// Examples
//https://forum.pjrc.com/threads/52745-Custom-Audio-Source
// Audio Library - END

extern uint8_t WAV_RAM[4][WAV_END - WAV_START+1]; 
extern uint8_t PRG_RAM[RAM_END - RAM_START+1];  // we will remove once we know it works

extern unsigned long get_cpu_cycle_count();
unsigned long doc_cycles;

DOC5503Osc oscillator[32];



uint8_t  oscsenabled;                     // # of oscillators enabled
uint8_t  regE0, regE1, regE2;            // contents of register 0xe0

uint8_t m_channel_strobe;
uint32_t CLK = 8000000; // 8Mhz
uint32_t output_rate;

 uint32_t altram;


uint8_t doc_irq;

// useful constants
static constexpr uint16_t wavesizes[8] = { 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 };        // T2, T1, T0
static constexpr uint32_t wavemasks[8] = { 0x1ff00, 0x1fe00, 0x1fc00, 0x1f800, 0x1f000, 0x1e000, 0x1c000, 0x18000 };
static constexpr uint32_t accmasks[8]  = { 0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff, 0x3fff, 0x7fff };
static constexpr int      resshifts[8] = { 9, 10, 11, 12, 13, 14, 15, 16 };



// **************************************************
// ******************* DOC INIT  ********************
// **************************************************
void Q::init() {
  int i;
  
    regE0 = 0xff;
    regE1 = 0;
    regE2 = 0x82; //A/D converter: rom.asm comparing between $90 and $70 (144 and 112)

for (i=0; i<32; i++)
 {
    oscillator[i].freq = 0;
    oscillator[i].wtsize = 0;
    oscillator[i].control = 0;
    oscillator[i].vol = 0;
    oscillator[i].data = 0x80; // Zero of a signed 8 bit value. The Boot Rom attempts to write to this register, but we won't let it.
    oscillator[i].wavetblpointer = 0;
    oscillator[i].wavetblsize = 0;
    oscillator[i].resolution = 0;
    oscillator[i].accumulator = 0;
    oscillator[i].irqpend = 0;
  }
  
    doc_irq = 0;
    oscsenabled = 0;
    m_channel_strobe = 0;
    output_rate = CLK / ((32 +2) *8);  // 1 cycle per 32 oscillators + 2 refresh cycles = 34 cycles = 29,412 Hz
                                     // for 16 oscillators -> 8Mhz/ (18*8) = 55,5556 Hz
    log_debug("*************************************************");
    log_debug("* DOC5503: Sampling Frequency %ld\n", output_rate);
    log_debug("* m_stream = stream_alloc(0, = %d , output_rate = %ld\n", output_channels, output_rate); 
    log_debug("*************************************************");

 


 // we need to allocate this, or equivalent. This is the audio queue, two channels (Check the PJRC Audio Library for implementation ideas)
 //  -------------------------------------------------------------------------------------------------------------------------------------
 // m_stream = stream_alloc(0, output_channels, output_rate); 

 
block_ch0 = allocate();
block_ch1 = allocate();

if ( (block_ch0 == 0) ) { // || (block_ch1 == 0) )    { // ALWAYS CHECK FOR SUCCESSFUL ALLOCATE()
                    log_debug("DOC5503 - CAN'T ALLOCATE AUDIO MEMORY**********************\n");
                    return;
                   }


//   MAME sets the sampling frequency. 
//  In the Mirage, the sampling frequency is fixed, because OS 3.2 sets oscenabled = 31 (+1)
//  -----------------------------------------------------------------------------------------------------------------
//  m_timer = timer_alloc(0, nullptr);
//  attotime update_rate = output_rate ? attotime::from_hz(output_rate) : attotime::never;
//  m_timer->adjust(update_rate, 0, update_rate);

return;
}
// ***************** END doc_init() ****************

// *************************************************
// ******************* DOC RUN  ********************
// *************************************************

void doc_run(CPU6809* cpu) {
  // this function will append to each audio queue the new associated sample
  // In the Mirage, the DOC runs at 8MHz, while the 6809 runs at 1MHz
  // so for every clock cycle, the DOC calculates for each oscillator the sample to append to the relative queue

if(doc_irq==1) {
//#if DOC5503_DEBUG
//    log_debug("DOC5503: IRQ FIRING");  
//#endif
    cpu->irq();
    return;
    }

if (get_cpu_cycle_count() < doc_cycles) {
          return; // not ready yet
  }
doc_cycles = get_cpu_cycle_count() + 10;  // For Gordon: what's the magic number here?

return;
}
// **************************** END doc_run() *******************************

// **************************************************************************
// ******************************** HALT OSC ********************************
// **************************************************************************
// ***  halt_osc: handle halting an oscillator
// ***  onum = oscillator #
// ***  type = 1 for 0 found in sample data, 0 for hit end of table size
void Q::doc_halt_osc(int onum, int type, uint32_t *accumulator, int resshift) {
//void doc_halt_osc(int onum, int type, uint32_t *accumulator, int resshift) {
  uint16_t wtsize;
  
	DOC5503Osc *pOsc = &oscillator[onum];
	DOC5503Osc *pPartner = &oscillator[onum^1];
  
  const int mode = (pOsc->control>>1) & 3;              // {M2,M1}
  const int partnerMode = (pPartner->control>>1) & 3;

#if DOC5503_DEBUG
log_debug("DOC5503: halt_osc\n");
#endif
  
	// if 0 found in sample data or mode is not free-run, instruct the oscillator to HALT
	if ((mode != MODE_FREE) || (type != 0)) 
	{
		pOsc->control |= 1;   // HALT
	}
	else    // preserve the relative phase of the oscillator when looping
	{
		uint16_t  wtsize = pOsc->wtsize - 1;
    altram = (*accumulator) >> resshift;

		if (altram > wtsize)
		{
			altram -= wtsize;
		}
		else
		{
			altram = 0;
		}

		*accumulator = altram << resshift;
	}

	// if swap mode, start the partner
  if ((mode == MODE_SWAP) || ((partnerMode == MODE_SWAP) && ((onum & 1)==0)))
	{
		pPartner->control &= ~1;    // toggle mode
		pPartner->accumulator = 0;  // and make sure it starts from the top (does this also need phase preservation?)
	}

	// IRQ enabled for this voice?
	if (pOsc->control & 0x08) //  0000_1000 D3 is bit IE of the Control register of the oscillator.
	//                            If IE is set it will cause an interrupt to be passed to the Ocillator Interrupt Register (0xE0) when it completes a cycle, which in turn will interrupt the processor.
  //                            If IE is clear, then the interrupt will be put into the oscillator interrupt table, but not passed to the OIR.
  //                            When more than one oscillator interrupt occurs, the interrupt stack will retain the status and pass interrupts to the OIR as they are serviced by the processor.
  //                            If the interrupt has been stored into the interrupt table while IE = 0, when IE is changed to a 1 the IRQ will be passed to the OIR.
	{
		pOsc->irqpend = 1;
		doc_irq = 1;            //  This will flag the irq in doc_run() **** USE irq.set(0) here with new usim
	}
 
}
// **************************** END doc_alt_osc() *******************************




// for each oscillator
//          is the oscillator HALT bit set? -> skip to next oscillator
//              else
//          compute the next sample
//          assign the sample to the queue identified by m_channel_strobe

// *****************************************************
// ******************* AUDIO UPDATE ********************
// *****************************************************


void Q::update (void) { //sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples) {

  int osc, snum, i;
  uint32_t ramptr;

  uint32_t  wtptr;
  uint32_t  acc;
  uint16_t  wtsize;
  uint8_t   ctrl;
  uint16_t  freq;
  int16_t   vol;
  int8_t    data; 
  int       resshift;
  uint32_t  sizemask;

int16_t *buffer_0;
int16_t *buffer_1;

// for now here, per Gordon we will move to ::init or ::reset
//block_ch0 = allocate();
//block_ch1 = allocate();

if ( (block_ch0 == 0) || (block_ch1 == 0) )    { // ALWAYS CHECK FOR SUCCESSFUL ALLOCATE()
                    log_debug("DOC5503 - CAN'T ALLOCATE AUDIO MEMORY**********************\n");
                    return;
                    }
 /*
                     else {
                          if (!(nomore))
                              log_debug("DOC5503 - ALLOCATE() OK!\n");
                          nomore = true;
                     }
*/

buffer_0 = block_ch0->data;
buffer_1 = block_ch1->data;

for(i=0; i<AUDIO_BLOCK_SAMPLES; i++) {  // ALWAYS INITIALIZE W ZEROS
        *buffer_0++ = 0;
        *buffer_1++ = 0;
        }


  for (int chan = 0; chan < output_channels; chan++)      // for each channel, chan = 0, or chan = 1
  {
    for (osc = 0; osc < (oscsenabled); osc++)           // for each oscillator that is enabled
    {
      DOC5503Osc *pOsc = &oscillator[osc];               // for each enabled oscillator, get its handle 

// Explanation of ((pOsc->control >> 4) & (output_channels - 1)) == chan
//                ------------------------------------------------------
// (pOsc->control >> 4) is (CA3, CA2, CA1, CA0) of Control Register (A0-BF). Theese bits control the final voice multiplexor, so we have 16 choices for the DOC5503
// The Mirage only uses 8 voices, so will only look for CA2, CA1, CA0. This is important because we need to connect the filters here later.
// Also, not clear yet (1/7/21) how we are going to use CA3. 
// For us output_channels is 2. So (output_channels - 1) is always 1
// The operation   (CA3 CA2 CA1 CA0) & (0001)) always return CA0, which can be either 0 or 1. chan can also be only 0 or 1
// Basicall for each oscillator, it is mapping the voice to either the right or left channel (in this case), depending whether the voice # is even or odd
// Explanation of (pOsc->control & 1)
//                -------------------
// This checks the HALT BIT. !(pOsc->control & 1) means "halt bit is not set".

      if (!(pOsc->control & 1) && ((pOsc->control >> 4) & (output_channels - 1)) == chan)
      {
        
        wtptr     = pOsc->wavetblpointer & wavemasks[pOsc->wavetblsize], altram; //WHAT DOES altram DO?
        acc       = pOsc->accumulator;
        wtsize    = pOsc->wtsize - 1;
        ctrl      = pOsc->control;
        freq      = pOsc->freq;
        vol       = pOsc->vol;
        data      = -128;
        resshift  = resshifts[pOsc->resolution] - pOsc->wavetblsize;
        sizemask  = accmasks[pOsc->wavetblsize];
       // mixp = &m_mix_buffer[0] + chan;  // is  &m_mix_buffer[0] for Left Channel, or  &m_mix_buffer[0] + 1 for Right Channel
        
        for (snum = 0; snum < AUDIO_BLOCK_SAMPLES; snum++)
        {
          altram = acc >> resshift;
          ramptr = altram & sizemask;

          acc += freq;

          // In MAME it is defined but not used by the MIRAGE Driver uint8_t get_channel_strobe() { return m_channel_strobe; }
          // channel strobe is always valid when reading; this allows potentially banking per voice
          m_channel_strobe = (ctrl>>4) & 0xf;           // it is 1 when (CA3 CA2 CA1 CA0) == 4'b1111;

         // data = (int32_t) WAV_RAM[via.orb & 0x03][ramptr + wtptr] ^ 0x80;  // normalize waveform
          
          data = WAV_RAM[via.orb & 0x03][ramptr + wtptr] ^ 0x80; 

          if (data == 0)  // If encountering "stop" 
          {                                                     //
            doc_halt_osc(osc, 1, &acc, resshift);               // we will stop the oscillator
          }
          else
          {
           // *mixp += data * vol;                                // otherwise we store to mixp
           // mixp += output_channels;                            // We are advancing to the next mixp location
            if(chan == 0) 
              *buffer_0 += data * 0xff ;//* vol;
               else           
              *buffer_1 += data * 0xff ;//* vol;

            
            if (altram >= wtsize)   // End of the table has been reached - halt the oscillator
            {
              doc_halt_osc(osc, 0, &acc, resshift);
            }
          }

          // if oscillator halted, we are done generating samples for that oscillator, so we exit the loop
          if (pOsc->control & 1)
          {
            ctrl |= 1;
            break;
          }
        }   // for all the samples in the buffer (128)

        pOsc->control     = ctrl;
        pOsc->accumulator = acc;
        pOsc->data        = data ^ 0x80;
      }     // for each active oscillator in the respective channel
    }      // for each oscillator 
  }       // for each channel (2)


transmit(block_ch0, 0);
transmit(block_ch1, 1);
//release(block_ch0);
//release(block_ch1);




}

/***************************************************************/

void debug_doc_halt_osc(int onum, int type, uint32_t *accumulator, int resshift) {
//void doc_halt_osc(int onum, int type, uint32_t *accumulator, int resshift) {
  uint16_t wtsize;

//#if DOC5503_DEBUG
log_debug("**** DOC5503: Entering halt_osc. onum = %d, type = %d, accumulator = %d, res shift = %d\n", onum, type, accumulator, resshift);
log_debug("**** DOC5503: onum = %d, onum^1 = %d\n", onum, onum^1);

//#endif
  
  DOC5503Osc *pOsc = &oscillator[onum];
  DOC5503Osc *pPartner = &oscillator[onum^1];
  
  const int mode = (pOsc->control>>1) & 3;              // {M2,M1}
  const int partnerMode = (pPartner->control>>1) & 3;


  
  // if 0 found in sample data or mode is not free-run, instruct the oscillator to HALT
  if ((mode != MODE_FREE) || (type != 0)) 
  {
    pOsc->control |= 1;   // HALT
  }
  else    // preserve the relative phase of the oscillator when looping
  {
     wtsize = pOsc->wtsize - 1;
     altram = (*accumulator) >> resshift;

    if (altram > wtsize)
    {
      altram -= wtsize;
    }
    else
    {
      altram = 0;
    }

    *accumulator = altram << resshift;
  }

  // if swap mode, start the partner
  if ((mode == MODE_SWAP) || ((partnerMode == MODE_SWAP) && ((onum & 1)==0)))
  {
    pPartner->control &= ~1;    // toggle mode
    pPartner->accumulator = 0;  // and make sure it starts from the top (does this also need phase preservation?)
  }

  // IRQ enabled for this voice?
  if (pOsc->control & 0x08) //  0000_1000 D3 is bit IE of the Control register of the oscillator.
  //                            If IE is set it will cause an interrupt to be passed to the Ocillator Interrupt Register (0xE0) when it completes a cycle, which in turn will interrupt the processor.
  //                            If IE is clear, then the interrupt will be put into the oscillator interrupt table, but not passed to the OIR.
  //                            When more than one oscillator interrupt occurs, the interrupt stack will retain the status and pass interrupts to the OIR as they are serviced by the processor.
  //                            If the interrupt has been stored into the interrupt table while IE = 0, when IE is changed to a 1 the IRQ will be passed to the OIR.
  {
    pOsc->irqpend = 1;
//    doc_irq = 1;            //  This will flag the irq in doc_run()
  }
 
}


void debug_update (void) { //sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples) {

  int osc, snum, i;
  uint32_t ramptr;

  uint32_t  wtptr;
  uint32_t  acc;
  uint16_t  wtsize;
  uint8_t   ctrl;
  uint16_t  freq;
  int16_t   vol;
  int8_t    data; 
  int       resshift;
  uint32_t  sizemask;

int16_t buffer_0[128];
int16_t buffer_1[128];


for(i=0; i<AUDIO_BLOCK_SAMPLES; i++) {  // ALWAYS INITIALIZE W ZEROS
        buffer_0[i] = 0;
        buffer_1[i]= 0;
        }

  for (int chan = 0; chan < 2; chan++)      // for each channel, chan = 0, or chan = 1
  {
 #if DOC5503_DEBUG
 #endif
    for (osc = 0; osc < (oscsenabled); osc++)           // for each oscillator that is enabled
    {
 #if DOC5503_DEBUG
 #endif
      DOC5503Osc *pOsc = &oscillator[osc];               // for each enabled oscillator, get its handle 

// Explanation of ((pOsc->control >> 4) & (output_channels - 1)) == chan
//                ------------------------------------------------------
// (pOsc->control >> 4) is (CA3, CA2, CA1, CA0) of Control Register (A0-BF). Theese bits control the final voice multiplexor, so we have 16 choices for the DOC5503
// The Mirage only uses 8 voices, so will only look for CA2, CA1, CA0. This is important because we need to connect the filters here later.
// Also, not clear yet (1/7/21) how we are going to use CA3. 
// For us output_channels is 2. So (output_channels - 1) is always 1
// The operation   (CA3 CA2 CA1 CA0) & (0001)) always return CA0, which can be either 0 or 1. chan can also be only 0 or 1
// Basicall for each oscillator, it is mapping the voice to either the right or left channel (in this case), depending whether the voice # is even or odd
// Explanation of (pOsc->control & 1)
//                -------------------
// This checks the HALT BIT. !(pOsc->control & 1) means "halt bit is not set".

      if (!(pOsc->control & 1) && ((pOsc->control >> 4) & (2 - 1)) == chan)
      {
        
         log_debug("DOC5503: ACTIVE osc = %d\n",osc);
         
        wtptr     = pOsc->wavetblpointer & wavemasks[pOsc->wavetblsize];//, altram; WHAT DOES altram DO?
        acc       = pOsc->accumulator;
        wtsize    = pOsc->wtsize - 1;
        ctrl      = pOsc->control;
        freq      = pOsc->freq;
        vol       = pOsc->vol;
        data      = -128;
        resshift  = resshifts[pOsc->resolution] - pOsc->wavetblsize;
        sizemask  = accmasks[pOsc->wavetblsize];
       // mixp = &m_mix_buffer[0] + chan;  // is  &m_mix_buffer[0] for Left Channel, or  &m_mix_buffer[0] + 1 for Right Channel


        for (snum = 0; snum < AUDIO_BLOCK_SAMPLES; snum++)
        {
          altram = acc >> resshift;
          ramptr = altram & sizemask;

          acc += freq;

          // In MAME it is defined but not used by the MIRAGE Driver uint8_t get_channel_strobe() { return m_channel_strobe; }
          // channel strobe is always valid when reading; this allows potentially banking per voice
          m_channel_strobe = (ctrl>>4) & 0xf;           // it is 1 when (CA3 CA2 CA1 CA0) == 4'b1111;

       //   data = (int32_t) WAV_RAM[via.orb & 0x03][ramptr + wtptr] ^ 0x80;  // normalize waveform
       data =  WAV_RAM[via.orb & 0x03][ramptr + wtptr] ^ 0x80;
 //#if DOC5503_DEBUG
          log_debug("snum = %d Data = %x (%d)\n",snum, data, data);
 //#endif

          if (data == 0) // (WAV_RAM[via.orb & 0x03][ramptr + wtptr] == 0x7f)  // If encountering "stop" data 
          {          
            log_debug("Found STOP Data snum = %x Data = (%x, dec: %d)\n",snum, WAV_RAM[via.orb & 0x03][ramptr + wtptr], WAV_RAM[via.orb & 0x03][ramptr + wtptr]);
            debug_doc_halt_osc(osc, 1, &acc, resshift);               // we will stop the oscillator
          }
          else
          {
           // *mixp += data * vol;                                // otherwise we store to mixp
           // mixp += output_channels;                            // We are advancing to the next mixp location
            if(chan == 0) 
              buffer_0[snum] += data * vol;
               else           
              buffer_1[snum] += data * vol;

            
            if (altram >= wtsize)   // End of the table has been reached - halt the oscillator
            {
              debug_doc_halt_osc(osc, 0, &acc, resshift);
            }
          }

          // if oscillator halted, we are done generating samples for that oscillator, so we exit the loop
          if (pOsc->control & 1)
          {
            ctrl |= 1;
            break;
          }
        }   // for all the samples in the buffer (128)



        pOsc->control     = ctrl;
        pOsc->accumulator = acc;
        pOsc->data        = data ^ 0x80;
        log_debug("DOC5503: Completed Active Oscillator osc = %d\n", osc);
      }     // for each active oscillator in the respective channel
      //log_debug("DCO5503: (END) Oscillator osc = %d\n", osc);
    }      // for each oscillator 
    log_debug("DOC5503: (END) For each Channel chan = %d\n", chan);
  }       // for each channel (2)

for(i=0; i<AUDIO_BLOCK_SAMPLES; i++) {
  log_debug("         buffer_0[%d] = %x (%d)\n", i, buffer_0[i], buffer_0[i]);
  log_debug("         buffer_1[%d] = %x (%d)\n", i, buffer_1[i], buffer_1[i]);
}

}
/**/
/***************************************************************/

// ************************************************************
// ******************* DOC READ REGISTERS  ********************
// ************************************************************
uint8_t doc_rreg(uint8_t reg) {
  uint8_t retval;
  int i;

 // here MAME performs a m_stream->update();

  if (reg < 0xe0)
  {
    int osc = reg & 0x1f;

    switch(reg & 0xe0)
    {
      case 0x00:     // freq lo
//#if DOC5503_DEBUG
//        log_debug("DOC5503 READ: Freq LOW %02x  value = %02x\n", reg, oscillators[osc].freq & 0xff);
//#endif
        return (oscillator[osc].freq & 0xff);
        break;
      case 0x20:      // freq hi
//#if DOC5503_DEBUG
//        log_debug("DOC5503 READ: Freq HIGH %02x  value = %02x\n", reg, (oscillators[osc].freq>>8) );
//#endif        
        return (oscillator[osc].freq >> 8);
        break;
      case 0x40:  // volume
//#if DOC5503_DEBUG      
//        log_debug("DOC5503 READ: Volume %02x  value = %02x\n", reg, oscillators[osc].vol);
//#endif        
        return oscillator[osc].vol;
        break;
      case 0x60:  // data
#if DOC5503_DEBUG      
        log_debug("DOC5503 READ: Data %02x  value = %02x\n", reg, oscillator[osc].data);
#endif      
        return oscillator[osc].data;
        break;
      case 0x80:  // wavetable pointer
#if DOC5503_DEBUG      
       log_debug("DOC5503 READ: Wavetable Pointer %02x  value = %02x\n", reg, (oscillator[osc].wavetblpointer >> 8) & 0xff );
#endif	     
	     return ((oscillator[osc].wavetblpointer>>8) & 0xff);
       break;
      case 0xa0:  // oscillator control
#if DOC5503_DEBUG       
        log_debug("DOC5503 READ: Oscillator Control %02x  value = %02x\n", reg, oscillator[osc].control);
#endif        
        return oscillator[osc].control;
        break;
      case 0xc0:  // / N.U. 1bit / Bank Select 1bit / Wavetable Size 3bits / Resolution 3bits/
#if DOC5503_DEBUG      
        log_debug("DOC5503 READ: Remember to implement 4 banks 32Kbytes each. ");
#endif      
        retval = 0;
        if (oscillator[osc].wavetblpointer & 0x10000) // if bit 17 is 1, we are addressing the next 64Kbytes
        {
          retval |= 0x40;
        }

        retval |= (oscillator[osc].wavetblsize<<3);
        retval |=  oscillator[osc].resolution;
#if DOC5503_DEBUG
        log_debug("DOC5503 READ: Bank Select %02x  value = %02x\n", reg, retval);
#endif        
        return retval;
        break;
    }
  }
  else     // global registers
  {
    switch (reg)
    {
      case 0xe0:  // interrupt status
#if DOC5503_DEBUG
        log_debug("DOC5503:       INTERRUPT STATUS IS: %x", regE0);
#endif      
        retval = regE0;

        doc_irq = 0; // clear DOC interrupt  // use irq.set(1) in new usim ****AF 012821 ****

        // scan all oscillators
        for (i = 0; i < oscsenabled; i++)
        {
          if (oscillator[i].irqpend)
          {
            // signal this oscillator has an interrupt
            retval = i<<1;

            regE0 = retval | 0x80;

            // and clear its flag
            oscillator[i].irqpend = 0;
            break;
          }
        }

        // if any oscillators still need to be serviced, assert IRQ again immediately
        for (i = 0; i < oscsenabled; i++)
        {
          if (oscillator[i].irqpend)
          {
#if DOC5503_DEBUG            
            log_debug("DOC5503 -> GENERATE IRQ!!! ");
#endif
            doc_irq=1;
            //cpu->irq();   // use irq.set(0) in new usim ****AF 012821 ****
            break;
          }
        }
#if DOC5503_DEBUG
        log_debug("DOC5503: READ INTERRUPT STATUS WILL RETURN %02x\n", retval);
#endif        
        return regE0 | 0x41; // 0100_0001 fix by Tim Lindner, Jan 4th 2021
        break;
      case 0xe1:  // oscillator enable
#if DOC5503_DEBUG 
  //    log_debug("DOC5503 READ OSCILLATOR ENABLE REGISTER %0x\n", oscsenabled<<1);
#endif        
        return oscsenabled<<1;
        break;
/*
+---------------+-----------------------------------+
|   Port B      |               4052 pins           |
|  PB3  PB2     |       pin 3       |    pin 13     |
|---------------+-------------------+---------------+
|   0   0 -> 0  |   mic (sample)    |  compressed   |
|   0   1 -> 1  |   line            |  uncompressed |
|   1   0 -> 2  |   gnd             |  pitch wheel  | ALFASoniQ Mirage will model this as always connected to GND  (Same as EnsoniQ DMS-1)
|   1   1 -> 3  |   gnd             |  modwheel     | ALFASoniQ MIrage will model this as always connected to +VCC (Same as EnsoniQ DMS-1)
+---------------+-----------------------+-----------+
*/
      case 0xe2:  //A/D converter: reads from: pitch/mod wheel, output signal path, and line-in (sampling) 

          switch( (via.orb >> 2) & 0x3)  {
            case 0b00:
            case 0b01:
               regE2 = 0x88; //A/D converter: rom.asm comparing between $90 and $70 (144 and 112)
            break;
            case 0b10: // Pitch Wheel
              regE2 = 0x7F; // GND - This is equivalent of 0 for the DOC
            break;
            case 0b11: // Mod WHeel
              regE2 = 0xFF; // GND should be correct (DMS-1 shows VCC - But that would cause modulation?)
            break;
        }
#if DOC5503_DEBUG 
//        log_debug("DOC5503 READ A/D Converter, value = %x\n", regE2);
//        log_debug("                            MUX is set to %x", (via.orb >> 2) & 0x3); 
#endif
      return regE2;
    
     break;
    }
  }
return -1; // should never get here
}
// *************************************************************


// *************************************************************
// ******************* DOC WRITE REGISTERS  ********************
// *************************************************************
void doc_wreg(uint8_t reg, uint8_t val) {
uint8_t osc;
uint16_t verifyaddr, verifycol;
 // here MAME performs a m_stream->update();


if (reg < 0xe0)
  {
    osc = reg & 0x1f;  //<<<< OSC is selected here

    switch(reg & 0xe0)
    {
      case 0x0:     // freq lo
        oscillator[osc].freq &= 0xff00;
        oscillator[osc].freq |= val;
#if DOC5503_DEBUG 
 //       log_debug("DOC5503 WRITE: Freq LOW %02x with value = %02x\n", reg, oscillator[osc].freq );
#endif            
        break;
      case 0x20:      // freq hi
        oscillator[osc].freq &= 0x00ff;
        oscillator[osc].freq |= (val<<8);
#if DOC5503_DEBUG 
 //       log_debug("DOC5503 WRITE: Freq High %02x with value = %02x\n", reg, oscillator[osc].freq );
#endif          
        break;
      case 0x40:  // volume
#if DOC5503_DEBUG 
if (val ==0x08)
        log_debug("DOC5503 WRITE: Volume Register %02x with value = %02x\n", reg, val);
#endif          
        oscillator[osc].vol = val;  
        break;
      case 0x60:  // data - ignore writes
#if DOC5503_DEBUG 
      log_debug("DOC5503 WRITE: ERROR ATTEMTPING TO Write READ-ONLY DATA REGISTER %02x with value = %02x\n", reg, val);
#endif  
        break;
      case 0x80:  // wavetable pointer
#if DOC5503_DEBUG 
        log_debug("DOC5503 WRITE: Waveteble Pointer Register %02x with value (%02x << 8) == %04x\n", reg, val, val<<8);
#endif  
        oscillator[osc].wavetblpointer = (val<<8);
        break;
      case 0xa0:  // oscillator control
//           [     current osc is off     ] && [ the new control value turns ON the oscillator] 
        if ((oscillator[osc].control & 1) && (!(val & 1)))
        {
          oscillator[osc].accumulator = 0;
//#if DOC5503_DEBUG
        log_debug("DOC5503 WRITE: Turning ON oscillator %d\n", osc);
//#endif
        }
        oscillator[osc].control = val;
#if DOC5503_DEBUG 
        log_debug("DOC5503 WRITE: Oscillator Control Register %02x with value = %02x\n", reg, val);
        log_debug("DOC5503        (CA3, CA2, CA1, CA0) = %x%x%x%x\n", ((val & 0xFF) >> 7) & 0x01, ((val & 0xFF) >> 6) & 0x01, ((val & 0xFF) >> 5) & 0x01,  ((val & 0xFF) >> 4) & 0x01  );
        log_debug("DOC5503        Interrupt Enable is %s\n", ((val & 0x08) == 0x08) ? "Enabled" : "Disabled");
        switch((val& 0x06)>>1) {
          case 0x0: log_debug("DOC5503        OSCILLATOR MODE: FREE RUN\n");
          break;
          case 0x1: log_debug("DOC5503        OSCILLATOR MODE: ONE-SHOT\n");
          break;
          case 0x2: log_debug("DOC5503        OSCILLATOR MODE: SYNC/AM \n");
          break;
          case 0x3: log_debug("DOC5503        OSCILLATOR MODE: SWAP MODE\n");
          break;
        }
          log_debug("DOC5503        Halt Bit is %s\n", ((val & 0x01) == 0x01) ? "Enabled" : "Disabled");
//        if ( reg == 0xbf) { // take advantage of this to check if we can access correctly PRGM_RAM from here
//          for(verifyaddr=0; verifyaddr < 1024; verifyaddr++) {
//             log_debug("%X , %X, %X, %X, %X, %X, %X, %X, %X , %X, %X, %X, %X, %X, %X, %X\n", PRG_RAM[verifyaddr*16 + 0c], PRG_RAM[verifyaddr*16 + 1] ,PRG_RAM[verifyaddr*16 + 2], PRG_RAM[verifyaddr*16 + 3],PRG_RAM[verifyaddr*16 + 4],PRG_RAM[verifyaddr*16 + 05],PRG_RAM[verifyaddr*16 + 6],PRG_RAM[verifyaddr*16 + 7],PRG_RAM[verifyaddr*16 + 8],PRG_RAM[verifyaddr*16 + 9],PRG_RAM[verifyaddr*16 + 10],PRG_RAM[verifyaddr*16 + 11],PRG_RAM[verifyaddr*16 + 12],PRG_RAM[verifyaddr*16 + 13],PRG_RAM[verifyaddr*16 + 14],PRG_RAM[verifyaddr*16 + 15]);
//            }
//        }
#endif
        debug_update();  //<<< remove when you get sound out!!!
        break;
      case 0xc0:  // bank select / wavetable size / resolution
#if DOC5503_DEBUG 
        log_debug("DOC5503 WRITE: Bank Select Register [REMEMBER TO IMPLEMENT BANK SELECT] %02x with value = %02x\n", reg, val);
#endif          
        if (val & 0x40)    
        {
          oscillator[osc].wavetblpointer |= 0x10000;
        }
        else
        {
          oscillator[osc].wavetblpointer &= 0xffff;
        }

        oscillator[osc].wavetblsize = ((val>>3) & 7);
        oscillator[osc].wtsize = wavesizes[oscillator[osc].wavetblsize];
        oscillator[osc].resolution = (val & 7);
        break;
    }
  }
  else     // global DOC registers
  {
    switch (reg)
    {
      case 0xe0:  // interrupt status

#if DOC5503_DEBUG 
        log_debug("DOC5503 WRITE not implemented: Attempting to write Interrupt Status Register %02x with value = %02x\n", reg, val);
#endif            
        break;
      case 0xe1:  // 0xe1 - Oscillator Enable
      // This  register determines  the  number of  active oscillators on  the DOC. 
      //  Load  the register with  the  number of  oscillators  to be  enabled, multiplied by 2,  
      //  e.g.,  to  enable all  oscillators, load register with a  62.
      // AF: 1/28/21: oscenabled + 1, otherwise up to 31. Removed +1 in the boundary checks in the rest of the code.
        oscsenabled = (val>>1) & 0x1f + 1; //[H H E4 E3 E2 E1 E0 H] >> 1 -> [ X H H E4 E3 E2 E1 E0] & 0x1F -> [0 0 0 E4 E3 E2 E1 E0]
        output_rate = (CLK / 8) / (2 + oscsenabled);
        
//#if DOC5503_DEBUG  
        log_debug("DOC5503 WRITE: Oscillator Enable Register [H H E4 E3 E2 E1 E0 H] %02x with value = %02x -> oscenabled = %d\n", reg, val, oscsenabled);
        log_debug("DOC5503 WRITE: Updated output rate value = %ld (Hz) for %d enabled oscillators\n", output_rate,oscsenabled);
//#endif
	// The number of oscillators selected will cause changes to the output_rate and sampling rate
	// as such this will cause a change in timer that we will use to sample the data out
  // HOWEVER: THE SAMPLING FREQUENCY SHOULD NEVER CHANGE, AS THE NUMBER OF ENABLED OSCILLATOR IS FIXED BY THE OS (AF 012821)
        //m_stream->set_sample_rate(output_rate);  // how will output_rate affect the pjrc audio library? 

        //attotime update_rate = output_rate ? attotime::from_hz(output_rate) : attotime::never;  // This may not be necessary with the pjrc audio lib
        //m_timer->adjust(update_rate, 0, update_rate);                                           // This may not be necessary with the pjrc audio lib
    
        break; //break case 0xe1 - Oscillator Enable
      case 0xe2:  //0xe2 - A/D converter 
#if DOC5503_DEBUG 
         log_debug("DOC5503 ILLEGAL WRITE: Attempting to write to A2D Converter (READ ONLY!!!) with value = %02x\n", val);
#endif        
        break;  // break case 0xe2 A/D Converter  
    }
  }
}
