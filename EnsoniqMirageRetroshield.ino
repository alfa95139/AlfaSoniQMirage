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

#include "bus.h"
#include "via.h"
#include "fdc.h"
#include "doc5503.h"
#include "log.h"

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
  Serial.println();
  Serial.println();
  Serial.flush();

  if (debug_mode)
    log_info("Debug mode enabled.");
  set_debug_enable(debug_mode);

  set_log("via");
  via_init();
  set_log("fdc");
  fdc_init();
  set_log("doc");
  doc_init();

  set_log("setup()");
  log_info("Initializing processor...");

  set_log("cpu");
  // Create & Reset processor
  cpu = new CPU6809();
  cpu->reset();
  cpu->set_stack_overflow(0x8000);

  log_info("Initialized processor");
  cpu->set_debug(debug_mode);
}

void tick_system() {
  set_log("cpu");
  cpu->tick();

  set_log("via");
  via_run(cpu);
  set_log("fdc");
  fdc_run(cpu);
  set_log("doc5503");
  //doc5503_run(cpu);
}

////////////////////////////////////////////////////////////////////
// Loop
////////////////////////////////////////////////////////////////////

void loop()
{
  word j = 0;
  
  // Loop forever
  //  
  while(true)
  {
    if (emergency) {
      do_continue = false;
      debug_mode = true;
      cpu->set_debug(true);
      set_debug_enable(true);
      emergency = false;
      cpu->printLastInstructions();
    }
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
        Serial.print(c);
        if (c == 's') {
          // Step single CPU instruction
          Serial.println();
          tick_system();
          Serial.printf("PC = %04x : %s\n", cpu->pc, address_name(cpu->pc));

        } else if (c == 'c') {
          // Continue to next breakpoint
          // breakpoints are hard-coded for now
          Serial.println();
          do_continue = true;

        } else if (c == 'r') {
          // print CPU registers
          Serial.println();
          cpu->printRegs();

        } else if (c == 'e') {
          // exit debug mode and tell CPU to stop printing
          Serial.println();
          debug_mode = false;
          cpu->set_debug(false);
          set_debug_enable(false);
          break;

        } else if (c == 'E') {
          // exit debug mode but keep CPU printing
          Serial.println();
          debug_mode = false;
          break;
        } else if (c == 'p') {
          // print last instructions executed
          Serial.println();
          cpu->printLastInstructions();
        } else if (c == 'm') {
          // print memory at location
          /* location can be any 16-bit unsigned integer literal...
           *    EX: 0xb920 (hex), 1024 (decimal), 0b1101 (binary)
           * ... or a 16-bit register name
           *    EX: s, u, pc, x, y (use lowercase)
          */
          char s[8];
          int i = 0;
          while (Serial.available()) {
            char c = Serial.read();
            if (c == ' ') {
              if (i == 0) {
                continue;
                Serial.write(' ');
              } else {
                s[i] = 0;
                break;
              }
            }
            if (c == '\n' || c == '\r' || i == 7) {
              s[i] = 0;
              break;
            }
            s[i++] = c;
          }
          Serial.println(s);

          // Decode location
          int location;
          if (!strcmp(s, "pc"))
            location = cpu->pc;
          else if (!strcmp(s, "s"))
            location = cpu->s;
          else if (!strcmp(s, "u"))
            location = cpu->u;
          else if (!strcmp(s, "x"))
            location = cpu->x;
          else if (!strcmp(s, "y"))
            location = cpu->y;
          else
            location = strtol(s, NULL, 0);
          Serial.printf("%s = %04x\n", s, location);

          cpu->print_memory(location);
        }
      }
    }

    tick_system();
    
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
