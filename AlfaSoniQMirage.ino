////////////////////////////////////////////////////////////////////
// Ensoniq Mirage Retroshield 6809 Emulator for Teensy 3.5 
// 2020/06/14 Version 1.0 by A. Fasan
// Contributions from Gordon J. Pierce, Erturk Kocalar (8bitforce.com)
// 2020/11/2 Version 2.0 by D. Brophy, A. Fasan
// Ensoniq Mirage with 6809 Emulator for Teensy 4.1
// Made possible by Dylan Brophy (6809 sw emulator, which enabled to upgrade to Teensy 4.1 (speed + full 128K WAV memory))
//
// 2021/1/1 Version 2.1: boots Mirage OS 3.2 after redesigning acia.cpp and fdc.cpp
// 2021/1/2 First integration of Touch Display (ILI ILI9341/9488 and XPP2046) to emulate keypad and display. Display is successfully emulated.
// 2021/1/4 Keypad is successfully emulated. Response is kind of slow, we will address later.
//
// The MIT License (MIT)
//
// Copyright (C) 2012 Gordon JC Pearce <gordonjcp@gjcp.net>
// Copyright (c) 2019 Erturk Kocalar, 8Bitforce.com
// Copyright (c) 2020 Alessandro Fasan, ALFASoniQ  / Dylan Brophy, Nuclare, emulation based on 6809 Usim (by Ray Bellis) integration & enhancements 
// Copyright (c) 2021 Alessandro Fasan, ALFASoniQ        

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
// 2020/11/1    Porting/Developing implementation of DOC 5503             Alessandro

#include "bus.h"
#include "via.h"
#include "fdc.h"
#include "doc5503.h"
#include "acia.h" // AF 11.28.20
#include "KeypadNDisplay.h" // AF 1.2.21
#include "log.h"
#include "TeensyTimerTool.h"
using namespace TeensyTimerTool;

CPU6809* cpu;

Timer Tacia;  // generate a timer for the ACIA
Timer T2;     // VIA T2
Timer T_AudioStream; // Audio Timer: 1ms -> 2 x 1kbytes buffer

typedef struct reg_save_s {
  uint16_t u, s, x, y, d;
  uint8_t dp, cc;
} reg_save_t;

reg_save_t register_slots[10];

void save_regs(int slot) {
  reg_save_t& save = register_slots[slot];
  save.u = cpu->u;
  save.s = cpu->s;
  save.x = cpu->x;
  save.y = cpu->y;
  save.d = cpu->d;
  save.dp = cpu->dp;
  save.cc = cpu->cc.all;
}

void load_regs(int slot) {
  reg_save_t& save = register_slots[slot];
  cpu->u = save.u;
  cpu->s = save.s;
  cpu->x = save.x;
  cpu->y = save.y;
  cpu->d = save.d;
  cpu->dp = save.dp;
  cpu->cc.all = save.cc;
}

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

//Tacia.beginPeriodic( acia_clk_CB , 1 );  // make sure that we trigger the ACIA to receive 
 //T2.beginPeriodic( KeypadNDisplay_CB, 2.5);                 //2.5 milliseconds (1 / 400Hz = 2.5 millis)
 //T_AudioStream.beginPeriodic (audio_update_CB, 1); // 1 millisecond
 
  Serial.println("\n");
  Serial.println("=======================================================");
  Serial.println("=           ALFASoniQ Mirage Digital Sampler          =");
  Serial.println("= A Teensyduino-based emulation of the EnsoniQ MIrage =");
  Serial.println("=       - by Alessandro Fasan                         =");
  Serial.println("=                                                     =");
  Serial.println("= Contributors:                                       =");
  Serial.println("= Ray Bellis, Gordon JC Pearce, Erturk Kocalar,       =");
  Serial.println("= Dylan Brophy, Tim Lindner                           =");
  Serial.println("=======================================================");
  /*
  Serial.print  ("SRAM Size:  "); Serial.print(RAM_END - RAM_START + 1, DEC); Serial.println(" Bytes");
  Serial.print  ("SRAM_START: 0x"); Serial.println(RAM_START, HEX);
  Serial.print  ("SRAM_END:   0x"); Serial.println(RAM_END, HEX);
  Serial.println();
  Serial.print  ("WAV RAM Size:  "); Serial.print( ((WAV_END - WAV_START + 1) * 4 )/ 1024, DEC); Serial.println(" KBytes");
  Serial.print  ("BANK 0: WAV RAM START: 0x"); Serial.println(WAV_START, HEX);
  Serial.print  ("        WAV RAM END:   0x"); Serial.println(WAV_END, HEX);
  Serial.print  ("BANK 1: WAV RAM START: 0x"); Serial.println(WAV_START, HEX);
  Serial.print  ("        WAV RAM END:   0x"); Serial.println(WAV_END, HEX);
  Serial.print  ("BANK 2: WAV RAM START: 0x"); Serial.println(WAV_START, HEX);
  Serial.print  ("        WAV RAM END:   0x"); Serial.println(WAV_END, HEX);
  Serial.print  ("BANK 3: WAV RAM START: 0x"); Serial.println(WAV_START, HEX);
  Serial.print  ("        WAV RAM END:   0x"); Serial.println(WAV_END, HEX);
  Serial.println();
  Serial.print  ("ROM Size:  "); Serial.print(ROM_END - ROM_START + 1, DEC); Serial.println(" Bytes");
  Serial.print  ("ROM_START: 0x"); Serial.println(ROM_START, HEX);
  Serial.print  ("ROM_END:   0x"); Serial.println(ROM_END, HEX);
  */
  Serial.println();
  Serial.println();
  Serial.flush();

  set_log("setup()");
  if (debug_mode)
    log_info("Debug mode enabled.");
  set_debug_enable(debug_mode);

  set_log("via");
  via_init();
  set_log("fdc");
  fdc_init();
  set_log("doc5503");
  doc_init();
  set_log("acia6850");
  acia_init();

  KeypadNDisplay_init();

  set_log("setup()");
  log_info("Initializing processor...");

  set_log("cpu");
  // Create & Reset processor
  cpu = new CPU6809();
  cpu->reset();
  cpu->set_stack_overflow(0x8000);

  set_log("setup()");
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
  doc_run(cpu);
  
  set_log("acia6950");
  acia_run(cpu);
  
  set_log("keypad");
  KeypadNDisplay_run();
  
}

uint16_t ask_address() {
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
  uint16_t address;
  if (!strcmp(s, "pc"))
    address = cpu->pc;
  else if (!strcmp(s, "s"))
    address = cpu->s;
  else if (!strcmp(s, "u"))
    address = cpu->u;
  else if (!strcmp(s, "x"))
    address = cpu->x;
  else if (!strcmp(s, "y"))
    address = cpu->y;
  else
    address = (uint16_t)strtol(s, NULL, 0);
  return address;
}

////////////////////////////////////////////////////////////////////
// Loop
////////////////////////////////////////////////////////////////////

void loop()
{
  word j = 0;
  
  // Loop forever
  //  
  if (cpu->pc < 0x8000UL)
    cpu->invalid("Not allowed to execute wave RAM");
  while(true)
  {
    if (was_emergency_triggered())
      emergency = true;
    if (emergency) {
      do_continue = false;
      debug_mode = true;
      cpu->set_debug(true);
      set_debug_enable(true);
      emergency = false;
      cpu->printLastInstructions();
    }

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
          // location can be any 16-bit unsigned integer literal...
          //    EX: 0xb920 (hex), 1024 (decimal), 0b1101 (binary)
          // ... or a 16-bit register name
          //    EX: s, u, pc, x, y (use lowercase)
          //
          int location = ask_address();

          cpu->print_memory(location);
        } else if (c == 'j') {
          // jump to address
          // location can be any 16-bit unsigned integer literal...
          //    EX: 0xb920 (hex), 1024 (decimal), 0b1101 (binary)
          // ... or a 16-bit register name
          //    EX: s, u, pc, x, y (use lowercase)
          //
          uint16_t location = ask_address();
          Serial.printf("pc <= 0x%04hx\n", location);

          cpu->pc = location;
        } else if (c == 'C') {
          // Jump to cartridge ROM
          Serial.println("\nRegisters before jump to cartridge ROM:");
          cpu->printRegs();
          cpu->pc = 0xc010;
        } else if (c == 'S') {
          while (!Serial.available());
          char c = Serial.read();
          Serial.write(c);
          if (c < '0' || c > '9')
            Serial.println("\nSave requires slot specification (0-9), ex: s0 to save to register slot 0");
          else {
            save_regs(c - '0');
            Serial.println("\nSaved register values:");
            cpu->printRegs();
          }
        } else if (c == 'L') {
          while (!Serial.available());
          char c = Serial.read();
          Serial.write(c);
          if (c < '0' || c > '9')
            Serial.println("\nLoad requires slot specification (0-9), ex: s0 to save to register slot 0");
          else {
            load_regs(c - '0');
            Serial.println("\nLoaded register values:");
            cpu->printRegs();
          }
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
