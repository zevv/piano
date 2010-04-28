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


struct rec {
	int16_t ticks;
	uint8_t note;
};

struct rec rec_list[200];
uint8_t rec_n = 0;

static uint8_t master_vol = 0;
static int8_t oct = 0;
static uint8_t recording = 0;
static uint8_t playing = 0;

void handle_key(uint8_t key, uint8_t state)
{
	uint8_t note;

	if(key < 32) {
		
		note = key + 30 + oct*12;

		if(recording) {
			struct rec *rec = &rec_list[rec_n];
			rec->ticks = ticks;
			rec->note = note;
			if(state) rec->note |= 0x80;
			rec_n ++;
		}

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

			case KEY_RECORD:
				if(! recording) {
					ticks = 0;
					metronome_set(1);
					recording = 1;
				} else {
					metronome_set(0);
					recording = 0;
				}
				break;
			
			case KEY_PLAY:
				if(recording) {
					metronome_set(0);
					recording = 0;
				}
				if(! playing) {
					playing = 1;
					ticks = 0;
				} else {
					playing = 0;
				}
				break;
			
		}
	}
}



int main(void)
{
	DDRB |= 1;
	DDRB |= 2;

	uint16_t pticks = 0;
	uint16_t i;
	uint8_t note;
	struct rec *rec;

	keyboard_init();
	audio_init();
	sei();

	for(;;) {
		keyboard_scan();

		if(pticks != ticks) {
			pticks = ticks;
			for(i=0; i<rec_n; i++) {
				rec = &rec_list[i];
				if(rec->ticks == ticks) {
					note = rec->note & 0x7f;
					if(rec->note & 0x80) {
						note_on(note);
					} else {
						note_off(note);
					}
				}
			}
		}
	}

	return 0;
}








