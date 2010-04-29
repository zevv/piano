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
#include "seq.h"

static uint8_t master_vol = 0;
static int8_t oct = 0;

void handle_key(uint8_t key, uint8_t state)
{
	uint8_t note;

	/* Notes */

	if(key < 32) {
		
		note = key + 30 + oct*12;
		(state ? note_on : note_off)(note);
		seq_note(note, state);
		return;
	}

	/* Control keys */

	if(state) {

		bip(10);

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
				seq_cmd(SEQ_CMD_REC);
				break;
			
			case KEY_PLAY:
				seq_cmd(SEQ_CMD_PLAY);
				break;

			case KEY_STOP:
				seq_cmd(SEQ_CMD_STOP);
				all_off();
				break;

			case KEY_FIRST:
				seq_cmd(SEQ_CMD_FIRST);
				break;
			
			case KEY_PREV:
				seq_cmd(SEQ_CMD_PREV);
				break;
			
			case KEY_NEXT:
				seq_cmd(SEQ_CMD_NEXT);
				break;

			case KEY_LAST:
				seq_cmd(SEQ_CMD_LAST);
				break;

			case KEY_TEMPO_UP:
				seq_cmd(SEQ_CMD_TEMPO_UP);
				break;

			case KEY_TEMPO_DOWN:
				seq_cmd(SEQ_CMD_TEMPO_DOWN);
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
	seq_init();
	sei();
				
	for(;;) {
		keyboard_scan();
	}

	return 0;
}


/*
 * End
 */

