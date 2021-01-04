/* 
   KeypadNDisplay.h - emulation of the Mirage Keypad and Display
   (C) 2021 - Alessandro Fasan
   
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

/*
 *    ANODE 1 is PA4 -> input to this model
 *    when LOW Drives LEFT Display
 *   ANODE 2 is PA3 -> input to this model
 *    when LOW Drives RIGHT Display
 *   CAD0, CAD1, CAD2 are (PA0, PA1, PA2) ->  inputs to this model
 *   ROW0, ROW1, ROW2 are (PA5, PA6, PA7) -> outputs to this model
 */
 
 
#ifndef __KD_H
#define __KD_H

#include "bus.h"
//#include <stdint.h>

//               PPPPPPPP
//               AAAAAAAA
//               76543210

#define CAD0   0b00000001
#define CAD1   0b00000010
#define CAD2   0b00000100
#define ANODE2 0b00001000
#define ANODE1 0b00010000
#define ROW0   0b00100000
#define ROW1   0b01000000
#define ROW2   0b10000000

void KeypadNDisplay_init();
void KeypadNDisplay_run();
void render_segment(uint8_t pos_x, uint8_t pos_y, 
                    uint8_t segment, uint16_t color);


#endif
