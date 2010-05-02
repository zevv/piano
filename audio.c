#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "audio.h"
#include "seq.h"
#include "sintab.h"

#define SINTAB_LEN 256
#define NOTETAB_LEN 12
#define NUM_OSCS 4

#define SRATE (F_CPU / 512.0 / 2)
#define F_NOTE0 43.653 // Lowest note we can play is F2
#define STEP_NOTE0 (F_NOTE0 * (256.0 * 256.0) / SRATE)


enum osc_type {
	OSC_TYPE_FM,
	OSC_TYPE_WAVE
};

uint16_t notetab[NOTETAB_LEN] = { 
	1.0000 * STEP_NOTE0,
	1.0594 * STEP_NOTE0,
	1.1224 * STEP_NOTE0,
	1.1892 * STEP_NOTE0,
	1.2599 * STEP_NOTE0,
	1.3348 * STEP_NOTE0,
	1.4142 * STEP_NOTE0,
	1.4983 * STEP_NOTE0,
	1.5874 * STEP_NOTE0,
	1.6817 * STEP_NOTE0,
	1.7817 * STEP_NOTE0,
	1.8877 * STEP_NOTE0,
};

struct instr {
	uint32_t size;
	uint32_t loop_a;
	uint32_t loop_b;
	int8_t sample[];
};


PROGMEM 

#include "instr-piano.c"

struct adsr {
	uint8_t a;
	uint8_t d;
	uint8_t s;
	uint8_t r;
	uint8_t vel;
	uint8_t state;
};

struct osc {
	enum osc_type type;	/* Osc type */
	uint8_t note;		/* Note number */
	uint16_t ticks;		/* Note clock ticks */

	/* FM */

	uint16_t off;		/* Osc offset in sin table */
	uint16_t step;		/* Osc step size */
	struct adsr adsr;	/* Velocity ADSR */

	uint16_t moff;		/* Modulator osc offset in sin table */
	uint16_t mstep;		/* Modulator osc step size */
	struct adsr madsr;	/* Modulator ADSR */

	/* Wave table */

	uint32_t woff;
	uint16_t wstep;
	uint32_t loop_a;
	uint32_t loop_b;
	uint8_t wvel;
	void *sample;
};


static volatile uint8_t master_vol = 0;
static volatile struct osc oscs[NUM_OSCS];
static volatile uint16_t bip_off = 0;
static volatile uint16_t bip_t = 0;
static volatile uint8_t fm_mul;
static volatile uint8_t fm_mod;

void audio_init(void)
{
	/* Timer 0: ticks timer at 1 Khz */

	TCCR0A = 0;
	TCCR0B = (1<<CS01) | (1<<CS00);
	TIMSK0 |= (1<<TOIE0);

	/* Timer 1: Fast PWM 10 bit */

	TCCR1A = (1<<COM1A1) | (1<<WGM11) | (0<<WGM10);
	TCCR1B = (1<<CS10) | (1<<WGM12);
	DDRD |= (1<<PD5);
	TIMSK1 |= (1<<TOIE1);
}


void osc_set_fm(uint8_t mul, uint8_t mod)
{
	fm_mul = mul;
	fm_mod = 7 - mod;
}


void note_on(uint8_t note)
{
	volatile struct osc *osc;
	uint8_t i;

	/* Find free osc, or just pick one */

	for(i=0; i<NUM_OSCS; i++) {
		osc = &oscs[i];
		if(osc->note == 0) break;
	}


	osc->type = OSC_TYPE_WAVE;
	osc->note = note;
	osc->ticks = 0;

	/* FM */
	
	osc->step = notetab[note % 12] << (note / 12);
	osc->mstep = osc->step * fm_mul / 2;

	osc->adsr.a = 64;
	osc->adsr.d = 5;
	osc->adsr.s = 50;
	osc->adsr.r = 20;
	osc->adsr.vel = 0;
	osc->adsr.state = 0;

	osc->madsr.a = 64;
	osc->madsr.d = 2;
	osc->madsr.s = 5;
	osc->madsr.r = 5;
	osc->madsr.vel = 0;
	osc->madsr.state = 0;

	/* Wave table */

	note -= 24;

	osc->woff = 0;
	osc->wstep = notetab[note % 12] << (note / 12);
	osc->wvel = 127;
	osc->sample = &instr.sample;
	osc->loop_a = pgm_read_dword(&instr.loop_a) * 256;
	osc->loop_b = pgm_read_dword(&instr.loop_b) * 256;
}


void note_off(uint8_t note)
{
	volatile struct osc *osc;
	uint8_t i;

	for(i=0; i<NUM_OSCS; i++) {
		osc = &oscs[i];
		if(osc->note == note) {
			osc->adsr.state = 3;
			osc->wvel = 126;
		}
	}
}


void all_off(void)
{
	uint8_t i;
	for(i=0; i<NUM_OSCS; i++) {
		oscs[i].note = 0;
	}
}


static void update_adsr(volatile struct adsr *adsr)
{
	int16_t vel = adsr->vel;

	switch(adsr->state) {
		case 0:
			vel = vel + adsr->a;
			if(vel >= 127) {
				vel = 127;
				adsr->state = 1;
			}
			break;
		case 1:
			vel = vel - adsr->d;
			if(vel <= adsr->s) {
				vel = adsr->s;
				adsr->state = 2;
			}
			break;
		case 2:
			break;
		case 3:
			vel = vel - adsr->r;
			if(vel <= 0) {
				vel = 0;
				adsr->state = 4;
			}
			break;
		case 4:
			break;
	}

	adsr->vel = vel;
}


void bip(uint8_t duration)
{
	bip_t = duration * 100;
}



void master_vol_set(uint8_t vol)
{
	master_vol = vol;
}


/*
 * Oscillator / ADSR update timer
 */

ISR(TIMER0_OVF_vect) __attribute__((interrupt));
ISR(TIMER0_OVF_vect)
{
	uint8_t i;
	static uint8_t t = 0;
	volatile struct osc *osc;

	if(t++ == 10) {
		t = 0;

		PORTB |=  2;
		for(i=0; i<NUM_OSCS; i++) {
			osc = &oscs[i];

			if(osc->type == OSC_TYPE_FM) {
				update_adsr(&osc->adsr);
				update_adsr(&osc->madsr);
				osc->ticks ++;
				if(osc->adsr.state == 4) osc->note = 0;
			} else {
				int8_t vel = osc->wvel;
				if(vel < 127) vel -= 5;
				if(vel <= 0) osc->note = 0;
				osc->wvel = vel;
			}
		}
		PORTB &= ~2;
	}
}



/*
 * Audio timer, highest prio. Mixes active oscillators and sets PWM output to D/A
 * converter value
 */

ISR(TIMER1_OVF_vect)
{
	int16_t c = 0;
	volatile struct osc *osc;
	uint8_t i;
	static uint8_t t = 0;
	uint8_t off;

	if(t++ != 1) return;
	t = 0;

	/* Mix all notes and create audio */

	PORTB |= 1;

	for(i=0; i<NUM_OSCS; i++) {
		osc = &oscs[i];
		if(! osc->note) continue;

		if(osc->type == OSC_TYPE_FM) {

			/* FM */

			int16_t m;

			m = sintab[osc->moff >> 8];
			m = m * osc->madsr.vel / 16;
			m >>= fm_mod;
			off = (osc->off >> 8) + m;
			if(osc->note) c = c + osc->adsr.vel * sintab[off] / (64 * NUM_OSCS); 
			osc->off += osc->step;
			osc->moff += osc->mstep;

		} else {

			/* Wave table */

			c = c + osc->wvel * pgm_read_byte(osc->sample + osc->woff / 256) / ( 128 * NUM_OSCS);
			osc->woff = osc->woff + osc->wstep;
			if(osc->woff >= osc->loop_b) osc->woff = osc->loop_a;
		}
	}

	if(bip_t) {
		c = c + sintab[bip_off >> 8] / 8;
		bip_off += 10000;
		bip_t --;
	}

	c >>= master_vol;
	c += 256;
	OCR1A = c;

	seq_tick();
	
	PORTB &= ~1;
}



/*
 * End
 */


