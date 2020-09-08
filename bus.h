
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
//byte    WAV_RAM_1[WAV_END - WAV_START+1];
//byte    WAV_RAM_2[WAV_END - WAV_START+1];
//byte    WAV_RAM_3[WAV_END - WAV_START+1];

// PROGRAM MEMORY: 16K
#define RAM_START  0x8000
#define RAM_END   0xBFFF

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

const char* address_name(uint16_t address);

class CPU6809: virtual public mc6809 {

  private:
    unsigned long clock_cycle_count;
    bool debug = false;

  protected:
    virtual uint8_t read(uint16_t offset);
    virtual void    write(uint16_t offset, Byte val);
    
    virtual void    on_branch(char* opcode, uint16_t src, uint16_t dst);
    virtual void    on_branch_subroutine(char* opcode, uint16_t src, uint16_t dst);
    virtual void    on_nmi(uint16_t src, uint16_t dst);
    virtual void    on_irq(uint16_t src, uint16_t dst);
    virtual void    on_firq(uint16_t src, uint16_t dst);

  public:
    CPU6809();
    void tick();
    void invalid(const char * = 0);
    void printRegs();
    unsigned long get_cycle_count() { return clock_cycle_count; }
    void set_debug(bool dbg) { debug = dbg; }

};

#endif
