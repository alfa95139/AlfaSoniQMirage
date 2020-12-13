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
// 1  	1   -  Address One cycle, reset OSC accumulator, set Halt bit then reset Halt bit
//
// M1 * H = 1 causes the oscillator to reset: This  needs to be evaluated during doc_wreg!
// 
// The Mirage initializes the Oscillator Control Register with:
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

#define DOC5503_DEBUG 1

#include "Arduino.h"
#include "log.h"
#include "doc5503.h"

extern uint8_t WAV_RAM[4][WAV_END - WAV_START+1]; 
extern uint8_t PRG_RAM[RAM_END - RAM_START+1];  // we will remove once we know it works

extern unsigned long get_cpu_cycle_count();
unsigned long doc_cycles;

DOC5503Osc oscillators[32];

uint32_t altram;
uint8_t  oscsenabled;      // # of oscillators enabled
uint8_t     regE0, regE1, regE2;            // contents of register 0xe0

uint8_t m_channel_strobe;
uint32_t CLK = 8000000; // 8Mhz
uint32_t output_rate;
int output_channels;


uint8_t doc_irq;

// useful constants
static constexpr uint16_t wavesizes[8] = { 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 }; // T2, T1, T0
static constexpr uint32_t wavemasks[8] = { 0x1ff00, 0x1fe00, 0x1fc00, 0x1f800, 0x1f000, 0x1e000, 0x1c000, 0x18000 };
static constexpr uint32_t accmasks[8]  = { 0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff, 0x3fff, 0x7fff };
static constexpr int      resshifts[8] = { 9, 10, 11, 12, 13, 14, 15, 16 };

void doc_init() {
  
    regE0 = 0xff;
    regE1 = 0;
    regE2 = 0x82; //A/D converter: rom.asm comparing between $90 and $70 (144 and 112)

    doc_irq = 0;
    oscsenabled = 1;
    m_channel_strobe = 0;
    output_rate = CLK / ((32 +2) *8);  // 1 cycle per 32 oscillators + 2 refresh cycles = 34 cycles = 29,412 Hz
                                     // for 16 oscillators -> 8Mhz/ (18*8) = 55,5556 Hz

  for (auto & elem : oscillators)
  {
    elem.freq = 0;
    elem.wtsize = 0;
    elem.control = 0;
    elem.vol = 0;
    elem.data = 0x80; // Zero of a signed 8 bit value. The Boot Rom attempts to write to this register, but we won't let it.
    elem.wavetblpointer = 0;
    elem.wavetblsize = 0;
    elem.resolution = 0;
    elem.accumulator = 0;
    elem.irqpend = 0;
  }
  
return;
}

// *************************************************
// ******************* DOC RUN  ********************
// *************************************************
void doc_run(CPU6809* cpu) {
  // this function will append to each audio queue the new associated sample
  // In the Mirage, the DOC runs at 8MHz, while the 6809 runs at 1MHz
  // so for every clock cycle, the DOC calculates for each oscillator the sample to append to the relative queue

  if (get_cpu_cycle_count() < doc_cycles) {
          //log_debug(" DOC WAITS - %ld\n", get_cpu_cycle_count());
          return; // not ready yet
  }
  //log_debug(" DOC DID NOT WAIT - %ld\n", get_cpu_cycle_count());
  doc_cycles = get_cpu_cycle_count() + 2;

  if(doc_irq==1) cpu->irq();
// for each oscillator
//          is the oscillator HALT bit set? -> skip to next oscillator
//              else
//          compute the next sample
//          assign the sample to the queue identified by m_channel_strobe

return;
}

// *************************************************
// ******************* HALT OSC ********************
// halt_osc: handle halting an oscillator
// chip = chip ptr
// onum = oscillator #
// type = 1 for 0 found in sample data, 0 for hit end of table size
// *************************************************
void doc_halt_osc(int onum, int type, uint32_t *accumulator, int resshift) {
  
	DOC5503Osc *pOsc = &oscillators[onum];
	DOC5503Osc *pPartner = &oscillators[onum^1];
  
	int mode = (pOsc->control>>1) & 3; // {M2,M1}

	// if 0 found in sample data or mode is not free-run, instruct the oscillator to HALT
	if ((mode != MODE_FREE) || (type != 0)) 
	{
		pOsc->control |= 1;   // HALT
	}
	else    // preserve the relative phase of the oscillator when looping
	{
		uint16_t wtsize = pOsc->wtsize - 1;
//		uint32_t altram = (*accumulator) >> resshift;
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
	if (mode == MODE_SWAP)
	{
		pPartner->control &= ~1;    // clear the halt bit
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
		doc_irq = 1;
	}
 
}

// ************************************************************
// ******************* AUDIO UPDATE ********************
// ************************************************************
void audio_update () { //sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples) {
  int32_t *mixp;
  int osc, snum, i;
  uint32_t ramptr;
  
  int samples = 256; // int samples = outputs[0].samples(); THIS IS THE LENGTH OF THE QUEUE

 // assert(samples < (44100/50));
 // std::fill_n(&m_mix_buffer[0], samples*output_channels, 0);

  for (int chan = 0; chan < output_channels; chan++)
  {
    for (osc = 0; osc < (oscsenabled+1); osc++)
    {
      DOC5503Osc *pOsc = &oscillators[osc];

      if (!(pOsc->control & 1) && ((pOsc->control >> 4) & (output_channels - 1)) == chan)
      {
        
        uint32_t wtptr = pOsc->wavetblpointer & wavemasks[pOsc->wavetblsize], altram;
        uint32_t acc = pOsc->accumulator;
        uint16_t wtsize = pOsc->wtsize - 1;
        uint8_t ctrl = pOsc->control;
        uint16_t freq = pOsc->freq;
        int16_t vol = pOsc->vol;
        int8_t data = -128;
        int resshift = resshifts[pOsc->resolution] - pOsc->wavetblsize;
        uint32_t sizemask = accmasks[pOsc->wavetblsize];
 //     mixp = &m_mix_buffer[0] + chan;
        
        for (snum = 0; snum < samples; snum++)
        {
          altram = acc >> resshift;
          ramptr = altram & sizemask;

          acc += freq;

          // channel strobe is always valid when reading; this allows potentially banking per voice
          m_channel_strobe = (ctrl>>4) & 0xf;
 //       data = (int32_t)read_byte(ramptr + wtptr) ^ 0x80;
 //       data = (int32_t) WAV_RAM[BANK][ramptr + wtptr] ^ 0x80;
          data = (int32_t) WAV_RAM[0][ramptr + wtptr] ^ 0x80;

//        if (WAV_RAM[BANK][ramptr + wtptr] == 0x00)
          if (WAV_RAM[0][ramptr + wtptr] == 0x00)
          {
            doc_halt_osc(osc, 1, &acc, resshift);
          }
          else
          {
            *mixp += data * vol;
            mixp += output_channels;

            if (altram >= wtsize)
            {
              doc_halt_osc(osc, 0, &acc, resshift);
            }
          }

          // if oscillator halted, we've got no more samples to generate
          if (pOsc->control & 1)
          {
            ctrl |= 1;
            break;
          }
        }

        pOsc->control     = ctrl;
        pOsc->accumulator = acc;
        pOsc->data        = data ^ 0x80;
      }
    }
  }

/*

  mixp = &m_mix_buffer[0];
  
  for (int chan = 0; chan < output_channels; chan++)
  {
    for (i = 0; i < outputs[chan].samples(); i++)
    {
      outputs[chan].put_int(i, *mixp++, 32768*8);
    }
  }
*/

}

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
        log_debug("DOC5503 READ: Freq LOW %02x  value = %02x\n", reg, oscillators[osc].freq & 0xff);
        return (oscillators[osc].freq & 0xff);
        break;
      case 0x20:      // freq hi
        log_debug("DOC5503 READ: Freq HIGH %02x  value = %02x\n", reg, (oscillators[osc].freq>>8) );
        return (oscillators[osc].freq >> 8);
        break;
      case 0x40:  // volume
        log_debug("DOC5503 READ: Volume %02x  value = %02x\n", reg, oscillators[osc].vol);
        return oscillators[osc].vol;
        break;
      case 0x60:  // data
        log_debug("DOC5503 READ: Data %02x  value = %02x\n", reg, oscillators[osc].data);
        return oscillators[osc].data;
        break;
      case 0x80:  // wavetable pointer
       log_debug("DOC5503 READ: Wavetable Pointer %02x  value = %02x\n", reg, (oscillators[osc].wavetblpointer >> 8) & 0xff );
	     return ((oscillators[osc].wavetblpointer>>8) & 0xff);
       break;
      case 0xa0:  // oscillator control
        log_debug("DOC5503 READ: Oscillator Control %02x  value = %02x\n", reg, oscillators[osc].control);
        return oscillators[osc].control;
        break;
      case 0xc0:  // / N.U. 1bit / Bank Select 1bit / Wavetable Size 3bits / Resolution 3bits/
        log_debug("DOC5503 READ: Remember to implement 4 banks 32Kbytes each. ");
        retval = 0;
        if (oscillators[osc].wavetblpointer & 0x10000) // if bit 17 is 1, we are addressing the next 64Kbytes
        {
          retval |= 0x40;
        }

        retval |= (oscillators[osc].wavetblsize<<3);
        retval |= oscillators[osc].resolution;
        log_debug("DOC5503 READ: Bank Select %02x  value = %02x\n", reg, retval);
        return retval;
        break;
    }
  }
  else     // global registers
  {
    switch (reg)
    {
      case 0xe0:  // interrupt status
        log_debug("DOC5503 READ INTERRUPT STATUS: %x", regE0);
        retval = regE0;

        doc_irq = 0; // clear DOC interrupt

        // scan all oscillators
        for (i = 0; i < oscsenabled+1; i++)
        {
          if (oscillators[i].irqpend)
          {
            // signal this oscillator has an interrupt
            retval = i<<1;

            regE0 = retval | 0x80;

            // and clear its flag
            oscillators[i].irqpend = 0;
            break;
          }
        }

        // if any oscillators still need to be serviced, assert IRQ again immediately
        for (i = 0; i < oscsenabled+1; i++)
        {
          if (oscillators[i].irqpend)
          {
            log_debug("DOC5503 -> GENERATE IRQ!!! ");
            doc_irq=1;
            //cpu->irq();
            break;
          }
        }
        log_debug(" %02x\n", retval);
        return retval;
        break;
      case 0xe1:  // oscillator enable
#if DOC5503_DEBUG 
        log_debug("DOC5503 READ OSCILLATOR ENABLE REGISTER%0x\n", oscsenabled<<1);
#endif        
        return oscsenabled<<1;
        break;
      case 0xe2:  //A/D converter: reads from: pitch/mod wheel, output signal path, and line-in (sampling) 
        log_debug("DOC5503 READ A/D Converter, faking %d value\n", regE2);
        return(regE2);
        break; 
    }
  }
}

// *************************************************************
// ******************* DOC WRITE REGISTERS  ********************
// *************************************************************
void doc_wreg(uint8_t reg, uint8_t val) {
uint8_t osc;
uint16_t verifyaddr, verifycol;
 // here MAME performs a m_stream->update();


if (reg < 0xe0)
  {
    osc = reg & 0x1f;

    switch(reg & 0xe0)
    {
      case 0x0:     // freq lo
        oscillators[osc].freq &= 0xff00;
        oscillators[osc].freq |= val;
        log_debug("DOC5503 WRITE: Freq LOW %02x with value = %02x\n", reg, oscillators[osc].freq );
        break;

      case 0x20:      // freq hi
        oscillators[osc].freq &= 0x00ff;
        oscillators[osc].freq |= (val<<8);
        log_debug("DOC5503 WRITE: Freq High %02x with value = %02x\n", reg, oscillators[osc].freq );
        break;

      case 0x40:  // volume
        log_debug("DOC5503 WRITE: Volume Register %02x with value = %02x\n", reg, val);
        oscillators[osc].vol = val;
        break;

      case 0x60:  // data - ignore writes
      log_debug("DOC5503 WRITE: ERROR ATTEMTPING TO Write READ-ONLY DATA REGISTER %02x with value = %02x\n", reg, val);

        break;

      case 0x80:  // wavetable pointer
        log_debug("DOC5503 WRITE: Waveteble Pointer Register %02x with value (%02x << 8) == %02x\n", reg, val, val<<8);
        oscillators[osc].wavetblpointer = (val<<8);
        break;

      case 0xa0:  // oscillator control
        // if a fresh key-on, reset the accumulator
        if ((oscillators[osc].control & 1) && (!(val & 1)))
        {
          oscillators[osc].accumulator = 0;
        }

        oscillators[osc].control = val;
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
        break;

      case 0xc0:  // bank select / wavetable size / resolution
        log_debug("DOC5503 WRITE: Bank Select Register [REMEMBER TO IMPLEMENT BANK SELECT] %02x with value = %02x\n", reg, val);
        if (val & 0x40)    
        {
          oscillators[osc].wavetblpointer |= 0x10000;
        }
        else
        {
          oscillators[osc].wavetblpointer &= 0xffff;
        }

        oscillators[osc].wavetblsize = ((val>>3) & 7);
        oscillators[osc].wtsize = wavesizes[oscillators[osc].wavetblsize];
        oscillators[osc].resolution = (val & 7);
        break;
    }
  }
  else     // global DOC registers
  {
    switch (reg)
    {
      case 0xe0:  // interrupt status
        log_debug("DOC5503 WRITE not implemented: Attempting to write Interrupt Status Register %02x with value = %02x\n", reg, val);
        break;

      case 0xe1:  // oscillator enable
      /* This  register determines  the  number of  active oscillators on  the DOC. 
       *  Load  the register with  the  number of  oscillators  to be  enabled, multiplied by 2,  
       *  e.g.,  to  enable all  oscillators, load register with a  62.
       */
      {
        oscsenabled = (val>>1) & 0x1f; //[H H E4 E3 E2 E1 E0 H] >> 1 -> [ X H H E4 E3 E2 E1 E0] & 0x1F -> [0 0 0 E4 E3 E2 E1 E0]
        output_rate = (CLK / 8) / (2 + oscsenabled);
        
#if DOC5503_DEBUG  
        log_debug("DOC5503 WRITE: Oscillator Enable Register [H H E4 E3 E2 E1 E0 H] %02x with value = %02x -> oscenabled = %d\n", reg, val, oscsenabled);
        log_debug("DOC5503 WRITE: Updated output rate value = %ld (Hz)\n", output_rate);
#endif
	// The number of oscillators selected will cause changes to the output_rate and sampling rate
	// as such this will cause a change in timer that we will use to sample the data out
        //m_stream->set_sample_rate(output_rate);  // how will output_rate affect the pjrc audio library? 

        //attotime update_rate = output_rate ? attotime::from_hz(output_rate) : attotime::never;  // This may not be necessary with the pjrc audio lib
        //m_timer->adjust(update_rate, 0, update_rate);                                           // This may not be necessary with the pjrc audio lib
        break;
      }

      case 0xe2:  // A/D converter
         log_debug("DOC5503 ILLEGAL WRITE: Attempting to write to A2D Converter (READ ONLY!!!) with value = %02x\n", val);
        break;
    }
  }
}
