#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "audio.h"
#include "seq.h"

#define SINTAB_LEN 256
#define NOTETAB_LEN 12
#define NUM_OSCS 4

int16_t sintab[SINTAB_LEN] = { 6, 13, 19, 25, 31, 37, 44, 50, 56, 62, 68, 74, 80, 86, 92, 98, 103, 109, 115, 120, 126, 131, 136, 142, 147, 152, 157, 162, 167, 171, 176, 180, 185, 189, 193, 197, 201, 205, 208, 212, 215, 219, 222, 225, 228, 231, 233, 236, 238, 240, 242, 244, 246, 247, 249, 250, 251, 252, 253, 254, 254, 255, 255, 255, 255, 255, 254, 254, 253, 252, 251, 250, 249, 247, 246, 244, 242, 240, 238, 236, 233, 231, 228, 225, 222, 219, 215, 212, 208, 205, 201, 197, 193, 189, 185, 180, 176, 171, 167, 162, 157, 152, 147, 142, 136, 131, 126, 120, 115, 109, 103, 98, 92, 86, 80, 74, 68, 62, 56, 50, 44, 37, 31, 25, 19, 13, 6, 0, -6, -13, -19, -25, -31, -37, -44, -50, -56, -62, -68, -74, -80, -86, -92, -98, -103, -109, -115, -120, -126, -131, -136, -142, -147, -152, -157, -162, -167, -171, -176, -180, -185, -189, -193, -197, -201, -205, -208, -212, -215, -219, -222, -225, -228, -231, -233, -236, -238, -240, -242, -244, -246, -247, -249, -250, -251, -252, -253, -254, -254, -255, -255, -255, -255, -255, -254, -254, -253, -252, -251, -250, -249, -247, -246, -244, -242, -240, -238, -236, -233, -231, -228, -225, -222, -219, -215, -212, -208, -205, -201, -197, -193, -189, -185, -180, -176, -171, -167, -162, -157, -152, -147, -142, -136, -131, -126, -120, -115, -109, -103, -98, -92, -86, -80, -74, -68, -62, -56, -50, -44, -37, -31, -25, -19, -13, -6, 0 };
uint8_t notetab[NOTETAB_LEN] = { 130, 138, 146, 155, 164, 174, 184, 195, 207, 220, 233, 246 };

struct adsr {
	uint8_t a;
	uint8_t d;
	uint8_t s;
	uint8_t r;
	uint8_t vel;
	uint8_t state;
};

struct osc {
	uint8_t note;
	uint16_t off;
	uint16_t step;
	uint16_t ticks;
	uint8_t vel;
	struct adsr adsr;

	uint16_t mstep;
	uint16_t moff;
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
	fm_mod = mod;
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


	osc->note = note;
	osc->step = notetab[note % 12] << (note / 12);
	osc->mstep = osc->step * fm_mul / 4;
	osc->ticks = 0;

	osc->adsr.a = 64;
	osc->adsr.d = 5;
	osc->adsr.s = 50;
	osc->adsr.r = 20;
	osc->adsr.vel = 0;
	osc->adsr.state = 0;
	
}


void note_off(uint8_t note)
{
	volatile struct osc *osc;
	uint8_t i;

	/* Find free osc, or use oldest */

	for(i=0; i<NUM_OSCS; i++) {
		osc = &oscs[i];
		if(osc->note == note) {
			osc->adsr.state = 3;
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


static void update_adsr(volatile struct osc *osc)
{
	volatile struct adsr *adsr = &osc->adsr;;
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
			if((osc->ticks % adsr->r) == 0) vel-=2;
			if(vel <= 1) adsr->state = 4;
			break;
		case 4:
			osc->note = 0;
			break;
	}

	adsr->vel = vel;

	osc->ticks ++;
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

	if(t++ == 10) {
		t = 0;

		PORTB |=  2;
		for(i=0; i<NUM_OSCS; i++) {
			update_adsr(&oscs[i]);
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

		off = (osc->off >> 8) + (sintab[osc->moff >> 8] >> (7-fm_mod));
		if(osc->note) c = c + osc->adsr.vel * sintab[off] / (128 * NUM_OSCS); 
		osc->off += osc->step;
		osc->moff += osc->mstep;
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


