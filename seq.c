
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <util/delay.h>

#include "audio.h"
#include "seq.h"

#define BIP_ALERT 50

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
volatile uint8_t seq_metro = 0;
volatile uint8_t seq_measures = 4;

volatile struct seq seq_list[SEQ_NOTES];

volatile struct seq *seq_play;	/* Current play pointer */
volatile struct seq *seq_rec;	/* Current rec pointer */
volatile struct seq *seq_last;	/* Last note in recording */


static int seq_cmp(const void *a, const void *b)
{
	const struct seq *seq_a = a;
	const struct seq *seq_b = b;
	if (seq_a->ticks > seq_b->ticks) return +1;
	if (seq_a->ticks < seq_b->ticks) return -1;
	return 0;
}


static void seq_sort(void)
{
	uint16_t size = sizeof *seq_list;
	uint16_t nmemb = seq_last - seq_list;
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
			bip(BIP_ALERT);
			seq_state = SEQ_STATE_IDLE;
		}
	}
}


void play_one(uint8_t note)
{
	uint8_t i;
	note_on(note & 0x7f);
	for(i=0; i<20; i++) _delay_ms(1);
	note_off(note & 0x7f);
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
			memset((void *)seq_list, 0, sizeof seq_list);
			seq_ticks = 0;
			break;
			
		case SEQ_CMD_DEL:
			if(seq_play != seq_list) {
				seq_play->note = 0;
				seq_play->ticks = 0;
				seq_last --;
				seq_sort();
			} else {
				bip(BIP_ALERT);
			}

		case SEQ_CMD_FIRST:
			seq_play = seq_list;
			seq_ticks = 0;
			seq_metro = 0;
			if(seq_last != seq_list) {
				play_one(seq_play->note);
			} else {
				bip(BIP_ALERT);
			}
			break;

		case SEQ_CMD_LAST:
			seq_play = seq_last;
			seq_metro = 0;
			while(!(seq_play->note & 0x80) && seq_play > seq_list) seq_play --;
			seq_ticks = seq_last->ticks;
			if(seq_play != seq_list) {
				play_one(seq_play->note);
			} else {
				bip(BIP_ALERT);
			}
			break;

		case SEQ_CMD_PREV:
			if(seq_play > seq_list) seq_play --;
			while(seq_play > seq_list && !(seq_play->note & 0x80)) seq_play --;
			play_one(seq_play->note);
			seq_ticks = seq_play->ticks;
			seq_metro = 0;
			break;

		case SEQ_CMD_NEXT:
			if(seq_play < seq_last) seq_play ++;
			while(seq_play < seq_last && !(seq_play->note & 0x80)) seq_play ++;
			play_one(seq_play->note);
			seq_ticks = seq_play->ticks;
			seq_metro = 0;
			break;

		case SEQ_CMD_PLAY:
			if(seq_state == SEQ_STATE_IDLE) {
				seq_state = SEQ_STATE_PLAY;
			}
			break;

		case SEQ_CMD_REC:
			seq_rec = seq_last;
			seq_state = SEQ_STATE_REC;
			break;

		case SEQ_CMD_STOP:
			if(seq_state == SEQ_STATE_REC) {
				seq_last = seq_rec;
				seq_sort();
			}
			seq_state = SEQ_STATE_IDLE;
			seq_metro = 0;
			break;

		case SEQ_CMD_TEMPO_DOWN:
			if(seq_tempo <= 240) seq_tempo += 10;
			break;
		
		case SEQ_CMD_TEMPO_UP:
			if(seq_tempo >= 10) seq_tempo -= 10;
			break;

		case SEQ_CMD_METRONOME:
			seq_metro = !seq_metro;
			break;
		
		case SEQ_CMD_MEASURES:
			seq_measures = 7 - seq_measures;
			break;
		
	}
}


void seq_tick(void)
{
	static uint8_t t = 0;

	if(t-- == 0) {
		t = seq_tempo;

		if(seq_metro || seq_state == SEQ_STATE_REC) {
			if((seq_ticks % 64) == 0) bip(5);
			if((seq_ticks % (64*seq_measures)) == 0) bip(25);
		}

		if(seq_state != SEQ_STATE_IDLE) {
			while(seq_ticks == seq_play->ticks) {
				uint8_t state = seq_play->note & 0x80;
				uint8_t note = seq_play->note & 0x7f;
				(state ? note_on : note_off)(note);
				seq_play ++;
			}

			if(seq_state == SEQ_STATE_PLAY) {
				if(seq_play >= seq_last) {
					bip(BIP_ALERT);
					seq_state = SEQ_STATE_IDLE;
				}
			}
		}
		
		if(seq_state != SEQ_STATE_IDLE || seq_metro) { 
			seq_ticks ++;
		}
	}
}


/*
 * End
 */


