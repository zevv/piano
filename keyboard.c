#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "keyboard.h"

extern void handle_key(uint8_t keynum, uint8_t state);

void keyboard_init(void)
{
	DDRA = 0x00;
	PORTA = 0xff;

	PORTC = 0x00;
	DDRC = 0x00;
}


void keyboard_scan(void)
{
	static uint8_t row = 0;
	int8_t col = 0;
	uint8_t key_cur;
	static uint8_t key_prev[8];
	uint8_t diff;
	uint8_t keynum;
	uint8_t bit;

	/* Scan row */

	DDRC = (1<<row);
	_delay_us(15);

	/* Read columns */

	key_cur = PINA;

	/* Check which keys have changed */

	diff = key_cur ^ key_prev[row];
	bit = 0x01;
	for(col=7; col>=0; col--) {
		if(diff & bit) {
			keynum = row * 8 + col;
			handle_key(keynum, (key_cur & bit) ? 0 : 1);
		}
		bit <<= 1;
	}

	key_prev[row] = key_cur;
	row = (row + 1) % 7;
}


/*
 * End
 */

