#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "keyboard.h"
#include "audio.h"

static uint8_t master_vol = 0;
static int8_t oct = 0;


void handle_key(uint8_t key, uint8_t state)
{
	uint8_t note;

	if(key < 32) {
		note = key + 30 + oct*12;
		if(state) {
			note_on(note);
		} else {
			note_off(note);
		}
		return;
	}


	if(state) {

		bip();

		switch(key) {

			case KEY_CLARINET:
				metronome_set(1);
				break;

			case KEY_MIN:
				if(master_vol < 7) master_vol++;
				master_vol_set(master_vol);
				break;

			case KEY_MAX:
				if(master_vol > 0) master_vol--;
				master_vol_set(master_vol);
				break;
			
			case KEY_OCT_UP:
				if(oct < 2) oct++;
				break;

			case KEY_OCT_DOWN:
				if(oct > -2) oct--;
				break;

		}
	}
}



int main(void)
{
	DDRB |= 1;
	DDRB |= 2;

	keyboard_init();
	audio_init();
	sei();

	for(;;) {
		keyboard_scan();
	}

	return 0;
}








