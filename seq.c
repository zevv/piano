
#include <stdint.h>
#include <stdlib.h>

#include "audio.h"
#include "seq.h"

enum seq_state {
	SEQ_STATE_IDLE,
	SEQ_STATE_PLAY,
	SEQ_STATE_REC
};

struct seq {
	uint16_t ticks;
	uint8_t note;
};


#define SEQ_NOTES 200

volatile enum seq_state seq_state;
volatile uint16_t seq_ticks;
volatile uint8_t seq_tempo = 140;

volatile struct seq seq_list[SEQ_NOTES];

volatile struct seq *seq_play;	/* Current play pointer */
volatile struct seq *seq_rec;	/* Current rec pointer */
volatile struct seq *seq_last;	/* Last note in recording */


static int seq_cmp(const void *a, const void *b)
{
	const struct seq *seq_a = a;
	const struct seq *seq_b = b;
	if (seq_a->ticks > seq_b->ticks) return 1;
	if (seq_a->ticks < seq_b->ticks) return -1;
	return 0;
}


static void seq_sort(void)
{
	uint16_t size = sizeof *seq_list;
	uint16_t nmemb = (seq_last - seq_list) / size;

	qsort((void *)seq_list, nmemb, size, seq_cmp);
}


void seq_init(void)
{
	seq_play = seq_list;
	seq_rec = seq_list;
	seq_last = seq_list;

	seq_ticks = 0;
	seq_state = SEQ_STATE_IDLE;
}



void seq_note(uint8_t note, uint8_t state)
{
	if(seq_state == SEQ_STATE_REC) {
		if(seq_rec < seq_list + SEQ_NOTES) {
			seq_rec->ticks = seq_ticks;
			seq_rec->note = note;
			if(state) seq_rec->note |= 0x80;
			seq_rec ++;
		} else {
			bip(100);
			seq_state = SEQ_STATE_IDLE;
		}
	}
}


/*
 * Handle sequencer control command
 */

void seq_cmd(enum seq_cmd cmd)
{
	switch(cmd) {
		case SEQ_CMD_CLEAR:
			seq_play = seq_list;
			seq_rec = seq_list;
			seq_last = seq_list;
			seq_ticks = 0;
			break;

		case SEQ_CMD_FIRST:
			seq_play = seq_list;
			seq_rec = seq_list;
			seq_ticks = 0;
			break;

		case SEQ_CMD_LAST:
			seq_play = seq_last;
			seq_ticks = seq_last->ticks;
			break;

		case SEQ_CMD_PREV:
			if(seq_play > seq_list) seq_play --;
			seq_ticks = seq_play->ticks;
			break;

		case SEQ_CMD_NEXT:
			if(seq_play < seq_last) seq_play ++;
			seq_ticks = seq_play->ticks;
			break;

		case SEQ_CMD_PLAY:
			if(seq_state == SEQ_STATE_IDLE) {
				seq_state = SEQ_STATE_PLAY;
			}
			break;

		case SEQ_CMD_REC:
			if(seq_state == SEQ_STATE_IDLE) {
				seq_rec = seq_last;
				seq_state = SEQ_STATE_REC;
			}
			break;

		case SEQ_CMD_STOP:
			if(seq_state == SEQ_STATE_REC) {
				seq_last = seq_rec;
				seq_sort();
			}
			seq_state = SEQ_STATE_IDLE;
			break;

		case SEQ_CMD_TEMPO_DOWN:
			if(seq_tempo <= 240) seq_tempo += 10;
			break;
		
		case SEQ_CMD_TEMPO_UP:
			if(seq_tempo >= 10) seq_tempo -= 10;
			break;
	}
}


void seq_tick(void)
{
	static uint8_t t = 0;

	if(seq_state == SEQ_STATE_IDLE) return;

	if(t-- == 0) {
		t = seq_tempo;

		if(seq_state == SEQ_STATE_REC) {
			if((seq_ticks % 64) == 0) bip(5);
			if((seq_ticks % 256) == 0) bip(25);
		}

		if(1) {
			if(seq_ticks == seq_play->ticks) {
				uint8_t state = seq_play->note & 0x80;
				uint8_t note = seq_play->note & 0x7f;
				(state ? note_on : note_off)(note);
				seq_play ++;

			}

			if(seq_state == SEQ_STATE_PLAY) {
				if(seq_play >= seq_last) {
					bip(100);
					seq_state = SEQ_STATE_IDLE;
				}
			}
		}

		seq_ticks ++;
	}
}


/*
 * End
 */


