
#ifndef __CPU_H
#define __CPU_H

#include <stdint.h>
#include "src/usim/mc6809.h"

////////////////////////////////////////////////////////////////////
// 6809 DEFINITIONS
////////////////////////////////////////////////////////////////////
// MEMORY
//
// WAV Memory: 32K (x 4 banks, eventually: 128K total)
// bit   VIA 6522 Port B
// 0     bank 0/1
// 1     upper/lower
//
// b b
// i i     Pages
// t t
// 0 0   1st Page
// 0 1   2nd Page
// 1 0   3rd Page
// 1 1   4th Page
#define WAV_START   0x0000
#define WAV_END     0x7FFF
uint8_t WAV_RAM_0[WAV_END - WAV_START+1];
//byte    WAV_RAM_1[WAV_END - WAV_START+1];
//byte    WAV_RAM_2[WAV_END - WAV_START+1];
//byte    WAV_RAM_3[WAV_END - WAV_START+1];

// PROGRAM MEMORY: 16K
#define RAM_START  0x8000
#define RAM_END   0xBFFF
uint8_t PRG_RAM[RAM_END - RAM_START+1];

// Expansion Cartridge: 8K
#define CART_START  0xC000
#define CART_END    0xDFFF

// Devices
#define DEVICES_START   0xE000
#define DEVICES_END     0xEFFF
#define UART6850cs      0xE100 // Control/Status
#define UART6850_d      0xE101 // Data
#define VIA6522         0xE200 // to 0xE20F
#define VCF3328         0xE400 //   0xE408 to 0xE40F: Filter cut-off Freq. 0xE410 to 0xE417:  Filter Resonance.E418-E41F  Multiplexer address pre-set (no output) 
#define FDC1770         0xE800 // to 0xE803
#define DOC5503         0xEC00 // to 0xECEF

// BOOTSTRAP ROM: 4K  - contains disk I/O, pitch and controller parameter tables and DOC5503 drivers
#define ROM_START   0xF000
#define ROM_END     0xFFFF

class CPU6809: public mc6809
{
  private:
    unsigned long clock_cycle_count;
  public:
    CPU6809();
    void reset();
    void tick();
    virtual void invalid(const char * = 0);
    unsigned long get_cycle_count() { return clock_cycle_count; }
};

// Define these elsewhere
void writeRAM8(uint16_t address, uint8_t data);
void writeRAM16(uint16_t address, uint16_t data);
uint8_t readRAM8(uint16_t address);
uint16_t readRAM16(uint16_t address);
void writeIO8(uint16_t address, uint8_t data);
uint8_t readIO8(uint16_t address);

#endif