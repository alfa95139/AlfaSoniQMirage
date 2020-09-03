////////////////////////////////////////////////////////////////////
// Ensoniq Mirage Retroshield 6809 Emulator for Teensy 3.5
//
// 2020/06/14
// Version 1.0

// The MIT License (MIT)

// Copyright (C) 2012 Gordon JC Pearce <gordonjcp@gjcp.net> ---  VIA 6522 Emulation
// Copyright (c) 2019 Erturk Kocalar, 8Bitforce.com
// Copyright (c) 2020 Alessandro Fasan, ALFASoniQ           

// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:

// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.

// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

// Date         Comments                                                 Author
// --------------------------------------------------------------------------------------------
// 9/29/2019    Retroshield 6809 Bring-up on Teensy 3.5                   Erturk
// 2020/06/13   Bring-up Ensoniq Mirage ROM                               Alessandro
// 2020/06/14   Porting GordonJCP implementation of VIA6522 emulation     Alessandro
// 2020/06/14   Porting GordonJCP implementation of FDC emulation         Alessandro

////////////////////////////////////////////////////////////////////
// Options
//   outputDEBUG: Print memory access debugging messages.
////////////////////////////////////////////////////////////////////
#define outputDEBUG     0

//////////////////////
//
/////////////////////
//extern "C++" {

  void via_init();
  uint8_t via_run();
  uint8_t via_rreg(int reg);
  void via_wreg(int reg, uint8_t val);
  uint8_t via_irq();
  void fdc_init();
  void fdc_run(); 
  uint8_t fdc_rreg(uint8_t reg);
  void fdc_wreg(uint8_t reg, uint8_t val);
  uint8_t fdc_intrq();
  uint8_t fdc_drq();
//  void doc_init();
//  uint8_t doc_run();
//  uint8_t doc_rreg(int reg);
//  void doc_wreg(int reg, uint8_t val);
//}

////////////////////////////////////////////////////////////////////
// 6809 DEFINITIONS
////////////////////////////////////////////////////////////////////
// MEMORY
// WAV Memory: 32K (x 4 banks, eventually: 128K total)
//bit   VIA 6522 Port B
//0     bank 0/1
//1     upper/lower
// b b
// i i     Pages
// t t
// 0 0   1st Page
// 0 1   2nd Page
// 1 0   3rd Page
// 1 1   4th Page
#define WAV_START   0x0000
#define WAV_END     0x7FFF
byte    WAV_RAM_0[WAV_END - WAV_START+1];
//byte    WAV_RAM_1[WAV_END - WAV_START+1];
//byte    WAV_RAM_2[WAV_END - WAV_START+1];
//byte    WAV_RAM_3[WAV_END - WAV_START+1];

// PROGRAM MEMORY: 16K
#define RAM_START	0x8000
#define RAM_END		0xBFFF
byte    PRG_RAM[RAM_END - RAM_START+1];

// Expansion Cartridge: 8K
#define CART_START  0xC000
#define CART_END    0xDFFF

// Devices
#define DEVICES_START 	0xE000
#define DEVICES_END	    0xEFFF
#define UART6850cs      0xE100 // Control/Status
#define UART6850_d      0xE101 // Data
#define	VIA6522		      0xE200 // to 0xE20F
#define VCF3328         0xE400 //   0xE408 to 0xE40F: Filter cut-off Freq. 0xE410 to 0xE417:  Filter Resonance.E418-E41F  Multiplexer address pre-set (no output) 
#define FDC1770		      0xE800 // to 0xE803
#define	DOC5503     	  0xEC00 // to 0xECEF

// BOOTSTRAP ROM: 4K  - contains disk I/O, pitch and controller parameter tables and DOC5503 drivers
#define ROM_START   0xF000
#define ROM_END     0xFFFF

////////////////////////////////////////////////////////////////////
// Monitor Code
////////////////////////////////////////////////////////////////////
#include "EnsoniqRom.h"
#include "CartridgeROM.h"

#include "cpu.h"

////////////////////////////////////////////////////////////////////
// Serial Functions
////////////////////////////////////////////////////////////////////

/*
  SerialEvent occurs whenever a new data comes in the
 hardware serial RX.  This routine is run between each
 time loop() runs, so any delay inside loop will delay
 response.  
 Warning: Multiple bytes of data may be available.
 */

 /*
inline __attribute__((always_inline))
void serialEvent0() 
{
  // If serial data available, assert FIRQ so process can grab it.
  if (Serial.available())
      digitalWrite(uP_FIRQ_N, LOW);
  return;
}


inline __attribute__((always_inline))
byte FTDI_Read()
{
  byte x = Serial.read();
  // If no more characters left, deassert FIRQ to the processor.
  if (Serial.available() == 0)
      digitalWrite(uP_FIRQ_N, HIGH);
  return x;
}


// This function is not used by the timer loop
// because it takes too long to call a subrouting.
inline __attribute__((always_inline))
void FTDI_Write(byte x)
{
  Serial.write(x);
}

*/

CPU6809 cpu;

////////////////////////////////////////////////////////////////////
// Setup
////////////////////////////////////////////////////////////////////

void setup() 
{
  Serial.begin(115200);
  while (!Serial);


  Serial.println("\n");
  Serial.print("Retroshield Debug:   --> "); Serial.println(outputDEBUG, HEX);
  Serial.println("========================================");
  Serial.println("= Ensoniq Mirage Memory Configuration: =");
  Serial.println("========================================");
  Serial.print("SRAM Size:  "); Serial.print(RAM_END - RAM_START + 1, DEC); Serial.println(" Bytes");
  Serial.print("SRAM_START: 0x"); Serial.println(RAM_START, HEX); 
  Serial.print("SRAM_END:   0x"); Serial.println(RAM_END, HEX); 
  Serial.print("WAV RAM Size:  "); Serial.print(WAV_END - WAV_START + 1, DEC); Serial.println(" Bytes");
  Serial.print("WAV RAM START: 0x"); Serial.println(WAV_START, HEX); 
  Serial.print("WAV RAM END:   0x"); Serial.println(WAV_END, HEX); 
  Serial.print("ROM Size:  "); Serial.print(ROM_END - ROM_START + 1, DEC); Serial.println(" Bytes");
  Serial.print("ROM_START: 0x"); Serial.println(ROM_START, HEX); 
  Serial.print("ROM_END:   0x"); Serial.println(ROM_END, HEX); 

  via_init();
  fdc_init();
 // doc_init();


  // Reset processor
  cpu.reset();

  Serial.println("\n");
}

////////////////////////////////////////////////////////////////////
// Loop
////////////////////////////////////////////////////////////////////

void loop()
{
  word j = 0;
  uint8_t viairq = HIGH;
  uint8_t old_viairq = HIGH;
  uint8_t fdcirq = HIGH;
  uint8_t old_fdcirq = HIGH;
  uint8_t fdcintrq = HIGH;
  uint8_t old_fdcintrq = HIGH;
  
  // Loop forever
  //  
  while(1)
  {
    //serialEvent0();
    cpu.tick();
    via_run();
    
    viairq = (via_irq() == 1) ? LOW : HIGH;
    digitalWrite(uP_IRQ_N, viairq);
    if ( (viairq == LOW) && (old_viairq == HIGH) ) Serial.printf("  +++++++++      VIA IRQ FIRED, address %04x\n", uP_ADDR);
    if ( (viairq == HIGH) && (old_viairq == LOW) ) Serial.printf("  +++++++++      VIA IRQ de-asserted, address %04x\n", uP_ADDR);
    old_viairq = viairq;
    
    fdc_run();
    
    fdcirq = (fdc_drq()   == 1) ? LOW : HIGH;
    digitalWrite(uP_IRQ_N,fdcirq);
    //if ( (fdcirq == LOW) && (old_fdcirq == HIGH) ) Serial.printf("  +++++++++      FDC IRQ FIRED, address %04x\n", uP_ADDR);
    //if ( (fdcirq == HIGH) && (old_fdcirq == LOW) ) Serial.printf("  +++++++++      FDC IRQ de-asserted, address %04x\n", uP_ADDR);
    //old_fdcirq = fdcirq;
    
    fdcintrq = (fdc_intrq() == 1) ? LOW : HIGH;
    digitalWrite(uP_NMI_N, fdcintrq);
    //if ( (fdcintrq == LOW) &&  (old_fdcintrq == HIGH) ) Serial.printf("  +++++++++     FDC NMI FIRED, address %04x\n", uP_ADDR);
    //if ( (fdcintrq == HIGH) && (old_fdcintrq == LOW) ) Serial.printf("  +++++++++      FDC NMI de-asserted, address %04x\n", uP_ADDR);
    //old_fdcintrq = fdcintrq;

    //doc5503_run();
    
    if (j-- == 0)
    {
      Serial.flush();
      j = 500;
    }
  }
}
