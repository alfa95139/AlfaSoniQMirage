/* vim: set noexpandtab ai ts=4 sw=4 tw=4:
   fdc.c -- emulation of 1772 FDC
   Copyright (C) 2012 Gordon JC Pearce

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

#ifndef __FDC_H
#define __FDC_H

// registers
#define FDC_SR 0
#define FDC_CR 0
#define FDC_TRACK 1
#define FDC_SECTOR 2
#define FDC_DATA 3

#include <stdint.h>



int fdc_init();
void fdc_run();
uint8_t fdc_rreg(uint8_t reg);
void fdc_wreg(uint8_t reg, uint8_t val);
uint8_t fdc_intrq();
uint8_t fdc_drq();


 //UNCOMMENT THESE TWO LINES FOR TEENSY AUDIO BOARD:
 //SPI.setMOSI(7);  // Audio shield has MOSI on pin 7
 //SPI.setSCK(14);  // Audio shield has SCK on pin 14



struct {
	uint8_t sr;       // STATUS REGISTER
	uint8_t cr;       // COMMAND REGISTER
	uint8_t trk_r;    // TRACK REGISTER
	uint8_t sec_r;    // SECTOR REGISTER
	uint8_t data_r;   // DATA REGISTER: holds the data during Read and Write operations
	uint8_t intrq;    // Interrupt Request
} fdc;

#endif
