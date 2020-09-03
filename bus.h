
#ifndef __CPU_H
#define __CPU_H

#include <stdint.h>
#include "src/usim/mc6809.h"

class CPU6809: public mc6809
{
  private:
    unsigned long clock_cycle_count;
  public:
    CPU6809();
    void reset();
    void tick();
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
