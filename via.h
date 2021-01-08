/* vim: set noexpandtab ai ts=4 sw=4 tw=4:
   via.h -- emulation of 6522 VIA
   Copyright (C) 2012 Gordon JC Pearce
   Copyright (C) 2020 Alessandro Fasan

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2, or (at your option)
   any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.  */

#ifndef __VIA_H
#define __VIA_H

#include "bus.h"
#include <stdint.h>

 typedef struct {
  uint8_t orb, irb;
  uint8_t ora, ira;
  uint8_t ddrb, ddra;
  uint8_t t1LL, t1HL; // t1 Latches (Low and High) 
  uint8_t t1l, t1h;   // t1 Counter (Low and High)
  uint8_t t2l, t2h;   // t2 Counter (Low and High)
  uint8_t ier, ifr;
  uint8_t acr;
  uint8_t pcr;
  uint8_t sr;

} via_struct;


uint8_t via_rreg(uint8_t reg);
void via_wreg(uint8_t reg, uint8_t val);
void via_run(CPU6809* cpu);
void via_irq_callback();//CPU6809* cpu);
void fire_via_irq(); //CPU6809* cpu);
void via_init();

#endif
