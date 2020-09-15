// license:BSD-3-Clause
// copyright-holders:R. Belmont
/*
Adapted from  ES5503 - Ensoniq ES5503 "DOC" emulator v2.1.1
  By R. Belmont.
  Copyright R. Belmont.
For use with Teensyduino, modified  by Alessandro Fasan July 2020

DAC E408 to E41F

00 to 07: ILLEGAL: 4051s CANNOT BE BOTH ON AT THE SAME TIME!
08 to 0F: Resonance
10 to 17: Cut off
18 to 1F: Filters OFF

We may consider including managing the filters as part of this model.

CA3 CA2 CA1 CA0 **** CHANNEL ASSIGNMENT ****
Final voice Multiplexor
16 channels are possible, 8 are normally used


OSCILLATOR MODE
M2 	M1      MODE 
0	0   - Free Run
0	1   - Address One cycle, reset OSC accumulator, set Halt bit
1	0   - Sync OSC2 to OSC1, or Amplitude Modulate OSC1 by OSC0
1 	1   -  Address One cycle, reset OSC accumulator, set Halt bit then reset Halt bit

M1 * H = 1 causes the oscillator to reset: This  needs to be evaluated during doc_wreg!

E4_0001_0000 (E410) to E4_0001_0111 (E417): CUT OFF
E4_0000_1000 (E408) to E4_0000_1111 (E40F): Resonance
E418 ==> E4_0001_1000 --> FILTERS OFF
This is what OS3.2 does:
E418 1_1000 - FILTERS OFF
E410 1_0000 - F -> 0
E418 1_1000 - FILTERS OFF
E408 0_1000 - Q -> 0
E419 1_1001 - FILTERS OFF
E411 1_0001 - F -> 1
E419 1_1001 - Filters OFF
E409 0_1001 - Q -> 1
E41A 1_1010 - FILTERS OFF
E412 1_0010 - F->2
E41A 1_1010 - FILTERS OFF
E40A 0_1010 - Q -> 2
E41B 1_1011 - FILTERS OFF
E413 1_0011 - F -> 3
E41B 1_1011 - FILTERS OFF
E40B 0_1011 - Q -> 3
E41C 1_1100 - FILTERS OFF
E414 1_0100 - F -> 4
E41C 1_1100 - FILTERS OFF
E40C 0_1100 - Q -> 4
E41D 1_1101 - FILTERS OFF
E415 1_0101 - F -> 5
E41D 1_1101 - FILTERS OFF
E40D 0_1101 - Q -> 5
E41E 1_1110 - FILTERS OFF
E416 1_0110 - F -> 6
E41E 1_1110 - FILTERS OFF
E40E 0_1110 - Q -> 6
E41F 1_1111 - FILTERS OFF
E417 1_0111 - F -> 7
E41F 1_1111 - FILTERS OFF
E40F 0_1111 - Q -> 7
 --> after these operations, IRQ Fired address AE40, which will be serviced later
 --> write to DOC value 03 A0 to A3: 0000_0011; Voice 0  IE = 0;  {M2,M1} = {0,1}; H = 1;
 --> write to DOC value 13 A4 to A7: 0001_0011; Voice 1
 --> write to DOC value 23 A8 to AB: 0010_0011; Voice 2
 --> write to DOC value 33 AC to AF: 0011_0011; Voice 3
 --> after these operations, write to DOC value 43 B0 to B3: 0100_0011; Voice 4 
 --> after these operations, write to DOC value 53 B4 to B7: 0101_0011; Voice 5
 --> after these operations, write to DOC value 63 B8 to B9: 0110_0011; Voice 6a?
 --> after these operations, write to DOC value e3 BA to BB: 1110_0011; Voice 6b?
 --> after these operations, write to DOC value f3 BC to BF: 1111_0011: Voice 7
 --> Then PORTB 10 (DOC CA3 = 0)
Then it goes into the calibrating routing swponfilter (which I patched)
E418 -> FF
E408 -> FF -> MAX  VALUE RESONANCE channel 0
E418 -> B0
E410 -> B0 -> CUTOFF channel 0
E408 -> 00
-> VIA PORTB <- 0001_0000

E419 -> FF
E409 -> FF -> MAX  VALUE RESONANCE channel 0
E419 -> B0
E411 -> B0 -> CUTOFF channel 0
E409 -> 00
-> VIA PORTB <- 0001_0000

E41A -> FF
E40A -> FF -> MAX  VALUE RESONANCE channel 0
E41A -> B0
E412 -> B0 -> CUTOFF channel 0
E40A -> 00
-> VIA PORTB <- 0001_0000

E41B -> FF
E40B -> FF -> MAX  VALUE RESONANCE channel 0
E41B -> B0
E413 -> B0 -> CUTOFF channel 0
E40B -> 00
-> VIA PORTB <- 0001_0000

E41C -> FF
E40C -> FF -> MAX  VALUE RESONANCE channel 0
E41C -> B0
E414 -> B0 -> CUTOFF channel 0
E40C -> 00
-> VIA PORTB <- 0001_0000

E41D -> FF
E40D -> FF -> MAX  VALUE RESONANCE channel 0
E41D -> B0
E415 -> B0 -> CUTOFF channel 0
E40D -> 00
-> VIA PORTB <- 0001_0000

E41E -> FF
E40E -> FF -> MAX  VALUE RESONANCE channel 0
E41E -> B0
E416 -> B0 -> CUTOFF channel 0
E40E -> 00
-> VIA PORTB <- 0001_0000

E41F -> FF
E40F -> FF -> MAX  VALUE RESONANCE channel 0
E41F -> B0
E417 -> B0 -> CUTOFF channel 0
E40F -> 00
-> VIA PORTB <- 0001_1000 *PITCH WHEEL* SELECTED
Read 4 times from the A/D, maybe to average the A2D read?
Then:
*** VIA6522 READ: via_rreg IFR = a0
Reading from VIA 6522 register 0c a0
Writing to VIA 6522 register 000c a0
*** VIA6522 WRITE:  pcr<=a0
Writing to VIA 6522 register 000c ae
*** VIA6522 WRITE:  pcr<=ae
*** VIA6522 READ: via_rreg read shift register unhandled
Reading from VIA 6522 register 0a 00

Loop begin
***  IRQ INTERRUPT ROUTINE ENTRY POINT ***  IRQ INTERRUPT ROUTINE ENTRY POINT ***   IRQ INTERRUPT ROUTINE ENTRY POINT  ***
*** VIA6522 READ: via_rreg IER = 26     IER = 0010_0110
Reading from VIA 6522 register 0d 26
DOC5503 READ INTERRUPT STATUS:  ff
Loop End

*/


#include "Arduino.h"
#include "doc5503.h"

DOC5503Osc oscillators[32];

uint8_t  oscsenabled;      // # of oscillators enabled
uint8_t     regE0, regE1, regE2;            // contents of register 0xe0

uint8_t m_channel_strobe;

int output_channels;
uint32_t output_rate;

uint8_t doc_irq;

// useful constants
static constexpr uint16_t wavesizes[8] = { 256, 512, 1024, 2048, 4096, 8192, 16384, 32768 }; // T2, T1, T0
static constexpr uint32_t wavemasks[8] = { 0x1ff00, 0x1fe00, 0x1fc00, 0x1f800, 0x1f000, 0x1e000, 0x1c000, 0x18000 };
static constexpr uint32_t accmasks[8]  = { 0xff, 0x1ff, 0x3ff, 0x7ff, 0xfff, 0x1fff, 0x3fff, 0x7fff };
static constexpr int    resshifts[8] = { 9, 10, 11, 12, 13, 14, 15, 16 };

uint8_t doc5503_irq() {
// 
return(doc_irq);
}

void doc_init() {
  
    regE0 = 0xff;
    regE1 = 0;
    regE2 = 0x82; //rom.asm comparing between $90 and $70 (144 and 112)

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

  oscsenabled = 1;

  m_channel_strobe = 0;



  //output_rate = (clock()/8)/34;   // (input clock / 8) / # of oscs. enabled + 2

return;
}

void doc_run() {
  // this function will update the audio stream, one day...
  // check
  //  https://github.com/mamedev/mame/blob/master/src/devices/sound/es5503.cpp
  // for implementation ideas
return;
}

// halt_osc: handle halting an oscillator
// chip = chip ptr
// onum = oscillator #
// type = 1 for 0 found in sample data, 0 for hit end of table size
void halt_osc(int onum, int type, uint32_t *accumulator, int resshift) {
  /*
	ES5503Osc *pOsc = &oscillators[onum];
	ES5503Osc *pPartner = &oscillators[onum^1];
	int mode = (pOsc->control>>1) & 3;

	// if 0 found in sample data or mode is not free-run, halt this oscillator
	if ((mode != MODE_FREE) || (type != 0))
	{
		pOsc->control |= 1;
	}
	else    // preserve the relative phase of the oscillator when looping
	{
		uint16_t wtsize = pOsc->wtsize - 1;
		uint32_t altram = (*accumulator) >> resshift;

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
	if (pOsc->control & 0x08) // 0000_1000 D3 is IE
	//                           if IE is set it will cause an interrupt
	{
		pOsc->irqpend = 1;
		doc_irq = 1;
	}
 */
}


//void sound_stream_update (sound_stream &stream, stream_sample_t **inputs, stream_sample_t **outputs, int samples) {
//}

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
        Serial.printf("DOC5503 READ: Freq LOW %02x  value = %02x\n", reg, oscillators[osc].freq & 0xff);
        return (oscillators[osc].freq & 0xff);
      case 0x20:      // freq hi
        Serial.printf("DOC5503 READ: Freq HIGH %02x  value = %02x\n", reg, (oscillators[osc].freq>>8) );
        return (oscillators[osc].freq >> 8);
      case 0x40:  // volume
        Serial.printf("DOC5503 READ: Volume %02x  value = %02x\n", reg, oscillators[osc].vol);
        return oscillators[osc].vol;

      case 0x60:  // data
        Serial.printf("DOC5503 READ: Data %02x  value = %02x\n", reg, oscillators[osc].data);
        return oscillators[osc].data;

      case 0x80:  // wavetable pointer
       Serial.printf("DOC5503 READ: Wavetable Pointer %02x  value = %02x\n", reg, (oscillators[osc].wavetblpointer >> 8) & 0xff );
	return ((oscillators[osc].wavetblpointer>>8) & 0xff);

      case 0xa0:  // oscillator control
        Serial.printf("DOC5503 READ: Oscillator Control %02x  value = %02x\n", reg, oscillators[osc].control);
        return oscillators[osc].control;

      case 0xc0:  // / N.U. 1bit / Bank Select 1bit / Wavetable Size 3bits / Resolution 3bits/
        Serial.printf("DOC5503 READ: Remember to implement 4 banks 32Kbytes each. ");
        retval = 0;
        if (oscillators[osc].wavetblpointer & 0x10000) // if bit 17 is 1, we are addressing the next 64Kbytes
        {
          retval |= 0x40;
        }

        retval |= (oscillators[osc].wavetblsize<<3);
        retval |= oscillators[osc].resolution;
        Serial.printf("DOC5503 READ: Bank Select %02x  value = %02x\n", reg, retval);
        return retval;
    }
  }
  else     // global registers
  {
    switch (reg)
    {
      case 0xe0:  // interrupt status
        Serial.printf("DOC5503 READ INTERRUPT STATUS: ");
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
            Serial.printf("DOC5503 -> GENERATE IRQ!!! ");
            doc_irq = 1; // generate interrupt
            break;
          }
        }
        Serial.printf(" %02x\n", retval);
        return retval;

      case 0xe1:  // oscillator enable
        Serial.printf("DOC5503 READ OSCILLATOR ENABLE\n");
        return oscsenabled<<1;

      case 0xe2:  //A/D converter: reads from: pitch/mod wheel, output signal path, and line-in (sampling) 
        Serial.printf("DOC5503 READ A/D Converter, faking %d value\n", regE2);
        return(regE2); 
    }
  }
}

void doc_wreg(uint8_t reg, uint8_t val) {
uint8_t osc;

 // here MAME performs a m_stream->update();


if (reg < 0xe0)
  {
    osc = reg & 0x1f;

    switch(reg & 0xe0)
    {
      case 0x0:     // freq lo
        oscillators[osc].freq &= 0xff00;
        oscillators[osc].freq |= val;
        Serial.printf("DOC5503 WRITE: Freq LOW %02x with value = %02x\n", reg, oscillators[osc].freq );
        break;

      case 0x20:      // freq hi
        oscillators[osc].freq &= 0x00ff;
        oscillators[osc].freq |= (val<<8);
        Serial.printf("DOC5503 WRITE: Freq High %02x with value = %02x\n", reg, oscillators[osc].freq );
        break;

      case 0x40:  // volume
        Serial.printf("DOC5503 WRITE: Volume Register %02x with value = %02x\n", reg, val);
        oscillators[osc].vol = val;
        break;

      case 0x60:  // data - ignore writes
      Serial.printf("DOC5503 WRITE: ERROR ATTEMTPING TO Write READ-ONLY DATA REGISTER %02x with value = %02x\n", reg, val);

        break;

      case 0x80:  // wavetable pointer
        Serial.printf("DOC5503 WRITE: Waveteable Pointer Register %02x with value %02x <<8 == %02x\n", reg, val, val<<8);
        oscillators[osc].wavetblpointer = (val<<8);
        break;

      case 0xa0:  // oscillator control
        // if a fresh key-on, reset the accumulator
        if ((oscillators[osc].control & 1) && (!(val & 1)))
        {
          oscillators[osc].accumulator = 0;
        }

        oscillators[osc].control = val;
        Serial.printf("DOC5503 WRITE: Oscillator Control Register %02x with value = %02x\n", reg, val);
        break;

      case 0xc0:  // bank select / wavetable size / resolution
        Serial.printf("DOC5503 WRITE: Bank Select Register [REMEMBER TO IMPLEMENT BANK SELECT] %02x with value = %02x\n", reg, val);
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
        Serial.printf("DOC5503 WRITE not implemented: Attempting to write Interrupt Status Register %02x with value = %02x\n", reg, val);
        break;

      case 0xe1:  // oscillator enable
      /* This  register determines  the  number of  active oscillators on  the DOC. 
       *  Load  the register with  the  number of  oscillators  to be  enabled, multiplied by 2,  
       *  e.g.,  to  enable all  oscillators, load register with a  62.
       */
      {
        oscsenabled = (val>>1) & 0x1f;
        Serial.printf("DOC5503 WRITE: Oscillator Enable Register %02x with value = %02x -> %02x\n", reg, val, oscsenabled);
	// The number of oscillators selected will cause changes to the output_rate and sampling rate
	// as such this will cause a change in timer that we will use to sample the data out
        //output_rate = (clock()/8)/(2+oscsenabled);
        //m_stream->set_sample_rate(output_rate);

        //attotime update_rate = output_rate ? attotime::from_hz(output_rate) : attotime::never;
        //m_timer->adjust(update_rate, 0, update_rate);
        break;
      }

      case 0xe2:  // A/D converter
         Serial.printf("DOC5503 ILLEGAL WRITE: Attempting to write to A2D Converter (READ ONLY!!!) with value = %02x\n", val);
        break;
    }
  }
}
