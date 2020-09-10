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

#include "bus.h"

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
      digitalWrite(uP_FIRQ_N, false);
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

CPU6809* cpu;
bool emergency = false;
bool debug_mode = true;
bool do_continue = true;

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
  Serial.flush();

  via_init();
  fdc_init();
  // doc_init();

  Serial.println("Initializing processor...");
  Serial.flush();
  // Create & Reset processor
  cpu = new CPU6809();
  cpu->reset();

  Serial.println("Initialized processor");
  Serial.flush();

  Serial.println("\n");

  if (debug_mode)
    Serial.println("Debug mode enabled.");
  cpu->set_debug(debug_mode);
}

////////////////////////////////////////////////////////////////////
// Loop
////////////////////////////////////////////////////////////////////

void loop()
{
  word j = 0;
  bool viairq = false;
  bool old_viairq = false;
  bool fdcirq = false;
  bool old_fdcirq = false;
  bool fdcintrq = false;
  bool old_fdcintrq = false;
  
  // Loop forever
  //  
  while(!emergency)
  {
    //serialEvent0();
    if (debug_mode) {
      const char* s = address_name(cpu->pc);
      if (s[0] == '*') { // && strcmp(s, "countdown")) {
        Serial.printf("BREAKPOINT %04x : %s\n", cpu->pc, s);
        do_continue = false;
      }
      while (!do_continue && !emergency) {
        while (Serial.available()) Serial.read();
        Serial.write("> ");
        while (!Serial.available());
  
        char c = Serial.read();
        Serial.println(c);
        if (c == 's') {
          // Step single CPU instruction
          cpu->tick();
          Serial.printf("PC = %04x : %s\n", cpu->pc, address_name(cpu->pc));

        } else if (c == 'c') {
          // Continue to next breakpoint
          // breakpoints are hard-coded for now
          do_continue = true;

        } else if (c == 'r') {
          // print CPU registers
          cpu->printRegs();

        } else if (c == 'e') {
          // exit debug mode and tell CPU to stop printing
          debug_mode = false;
          cpu->set_debug(false);
          break;

        } else if (c == 'E') {
          // exit debug mode but keep CPU printing
          debug_mode = false;
          break;
        }
      }
    }
    cpu->tick();
    via_run();
    
    if ((viairq = via_irq()) && !old_viairq) {
      bool accepted = cpu->irq();
      if (accepted) {
        Serial.printf(" +++  VIA IRQ FIRED; pc=%04x %s\n", cpu->pc, address_name(cpu->pc));
      }
    }
    old_viairq = viairq;
    //digitalWrite(uP_IRQ_N, viairq);
    //if ( (viairq == LOW) && (old_viairq == HIGH) ) Serial.printf("  +++++++++      VIA IRQ FIRED, address %04x\n", uP_ADDR);
    //if ( (viairq == HIGH) && (old_viairq == LOW) ) Serial.printf("  +++++++++      VIA IRQ de-asserted, address %04x\n", uP_ADDR);
    //old_viairq = viairq;
    
    fdc_run();

    if ((fdcirq = fdc_drq()) && !old_fdcirq) {
      bool accepted = cpu->irq();
      if (accepted) {
        Serial.printf(" +++  FDC IRQ FIRED; pc=%04x %s\n", cpu->pc, address_name(cpu->pc));
      }
    }
    old_fdcirq = fdcirq;
    //digitalWrite(uP_IRQ_N,fdcirq);
    //if ( (fdcirq == LOW) && (old_fdcirq == HIGH) ) Serial.printf("  +++++++++      FDC IRQ FIRED, address %04x\n", uP_ADDR);
    //if ( (fdcirq == HIGH) && (old_fdcirq == LOW) ) Serial.printf("  +++++++++      FDC IRQ de-asserted, address %04x\n", uP_ADDR);
    //old_fdcirq = fdcirq;
    
    if ((fdcintrq = fdc_intrq()) && !old_fdcintrq) {
      if (old_fdcintrq != fdcintrq) {
        bool accepted = cpu->nmi(true);
        if (accepted) {
          Serial.printf(" +++  FDC NMI FIRED; pc=%04x %s\n", cpu->pc, address_name(cpu->pc));
        }
      }
    }
    old_fdcintrq = fdcintrq;
    //digitalWrite(uP_NMI_N, fdcintrq);
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

uint64_t get_cpu_cycle_count() {
  return cpu->get_cycle_count();
}
