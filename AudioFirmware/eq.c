/*
 * eq.c
 *
 * Copyright 2011 Thomas Buck <xythobuz@me.com>
 * Copyright 2011 Max Nuding <max.nuding@gmail.com>
 * Copyright 2011 Felix Bäder <baeder.felix@gmail.com>
 *
 * This file is part of LED-Cube.
 *
 * LED-Cube is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * LED-Cube is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with LED-Cube.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <avr/io.h>
#include <stdint.h>
#include <util/delay.h>

#include "eq.h"
#include "adc.h"

#define Trs 72
#define Ts 19
#define To 36

uint8_t result[7] = {128, 128, 128, 128, 128, 128, 128};

#define RESETPORT PORTC
#define STROBEPORT PORTC
#define RESET PC3
#define STROBE PC2

#define RESETON RESETPORT |= (1 << RESET)
#define RESETOFF RESETPORT &= ~(1 << RESET)
#define STROBEON STROBEPORT |= (1 << STROBE)
#define STROBEOFF STROBEPORT &= ~(1 << STROBE)

void equalizerInit(void);
uint8_t *equalizerGet(void);
void calcMultiplicator(uint8_t *d);
void heightenTreble(uint8_t *d);
void filterNoise(uint8_t *data);
uint8_t getOffset(void);

void equalizerInit(void) {
	RESETON;
	STROBEOFF;
}

uint8_t *equalizerGet(void) {
	uint8_t i;

	RESETOFF;
	_delay_us(Trs);

	for (i = 0; i < 7; i++) {
		STROBEON;
		_delay_us(Ts);
		STROBEOFF;
		_delay_us(To); // Wait for result

		adcStartConversion(0);
		result[i] = adcGetByte();
	}

	RESETON;
	asm volatile ("nop"); // Ensure minimal reset pulse width
	asm volatile ("nop"); // 2 NOPs at 16MHz are enough...

	filterNoise(result); // Filter lower values
	heightenTreble(result); // Heighten higher frequencies, cause they seem damped
	calcMultiplicator(result); // Multiply with Poti Position
	filterNoise(result); // Filter lower values

	return result;
}

void calcMultiplicator(uint8_t *d) {
	uint8_t i;
	uint16_t multiplicator = ((uint16_t)getOffset() + 100) / 10;

	for (i = 0; i < 7; i++) {
		d[i] = (d[i] * multiplicator) / 10;
	}
}

void heightenTreble(uint8_t *d) {
	d[6] = d[5] * 10 / 15; // Get practically nothing on this channel. So fake something...
}

void filterNoise(uint8_t *data) {
	uint8_t i;
	for (i = 0; i < 7; i++) {
		if (data[i] < 60) {
			data[i] = data[i] * 10 / 15; // / 1.5
		}
	}
}

uint8_t getOffset(void) {
	adcStartConversion(0x01);
	return adcGetByte();
}
