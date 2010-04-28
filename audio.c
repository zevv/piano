#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "audio.h"

#define SINTAB_LEN 256
#define NOTETAB_LEN 12
#define NUM_OSCS 4

int8_t sintab[SINTAB_LEN] = { 0, 3, 6, 9, 12, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46, 49, 52, 54, 57, 60, 63, 66, 68, 71, 73, 76, 78, 81, 83, 86, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 108, 109, 111, 112, 114, 115, 116, 118, 119, 120, 121, 122, 123, 123, 124, 125, 125, 126, 126, 126, 127, 127, 127, 127, 127, 127, 127, 126, 126, 125, 125, 124, 124, 123, 122, 121, 120, 119, 118, 117, 116, 114, 113, 112, 110, 108, 107, 105, 103, 101, 99, 97, 95, 93, 91, 89, 87, 84, 82, 80, 77, 75, 72, 69, 67, 64, 61, 59, 56, 53, 50, 47, 44, 41, 39, 36, 32, 29, 26, 23, 20, 17, 14, 11, 8, 5, 2, -2, -5, -8, -11, -14, -17, -20, -23, -26, -29, -32, -36, -39, -41, -44, -47, -50, -53, -56, -59, -61, -64, -67, -69, -72, -75, -77, -80, -82, -84, -87, -89, -91, -93, -95, -97, -99, -101, -103, -105, -107, -108, -110, -112, -113, -114, -116, -117, -118, -119, -120, -121, -122, -123, -124, -124, -125, -125, -126, -126, -127, -127, -127, -127, -127, -127, -127, -126, -126, -126, -125, -125, -124, -123, -123, -122, -121, -120, -119, -118, -116, -115, -114, -112, -111, -109, -108, -106, -104, -102, -100, -98, -96, -94, -92, -90, -88, -86, -83, -81, -78, -76, -73, -71, -68, -66, -63, -60, -57, -54, -52, -49, -46, -43, -40, -37, -34, -31, -28, -25, -22, -19, -16, -12, -9, -6, -3, 0 };
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
};


static volatile uint8_t master_vol = 0;
static volatile uint8_t metronome = 0;
static volatile struct osc oscs[NUM_OSCS];
static uint8_t ticks = 0;
static volatile uint16_t bip_off = 0;
static volatile uint16_t bip_t = 0;


void audio_init(void)
{

	/* Timer 2: PWM at 62.5 Khz */

	TCCR2A = (1<<COM2A1) | (1<<WGM21) | (1<<WGM20);
	TCCR2B = (1<<CS20);
	TIMSK2 |= (1<<TOIE2);
	DDRD |= (1<<PD7);

	/* Timer 0: ticks timer at 1 Khz */

	TCCR0A = 0;
	TCCR0B = (1<<CS01) | (1<<CS00);
	TIMSK0 |= (1<<TOIE0);

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
	osc->ticks = 0;

	osc->adsr.a = 70;
	osc->adsr.d = 5;
	osc->adsr.s = 100;
	osc->adsr.r = 10;
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


static void update_adsr(volatile struct osc *osc)
{
	volatile struct adsr *adsr = &osc->adsr;;
	int16_t vel = adsr->vel;

	switch(adsr->state) {
		case 0:
			vel = vel + adsr->a;
			if(vel >= 255) {
				vel = 255;
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


void bip(void)
{
	bip_t = 1000;
}



void metronome_set(uint8_t tempo)
{
	metronome = tempo;
}


void master_vol_set(uint8_t vol)
{
	master_vol = vol;
}


/*
 * Ticks timer
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
		ticks ++;
		PORTB &= ~2;
	}
}



/*
 * Audio timer, highest prio. Mixes active oscillators and sets PWM output to D/A
 * converter value
 */

ISR(TIMER2_OVF_vect)
{
	int8_t c = 0;
	static uint8_t t = 0;
	volatile struct osc *osc;
	uint8_t i;

	t++;
	
	/* Mix all notes and create audio */

	if(t == 3) {
		t = 0;
		PORTB |= 1;

		for(i=0; i<NUM_OSCS; i++) {
			osc = &oscs[i];
			if(osc->note) c = c + osc->adsr.vel * sintab[osc->off >> 8] / (256 * NUM_OSCS); 
			osc->off += osc->step;
		}

		if(bip_t) {
			c = c + sintab[bip_off >> 8] / 32;
			bip_off += 10000;
			bip_t --;
		}


		if(metronome) {
			if((ticks % 32)== 0) c += 16;
			if((ticks % 128)== 0) c += 32;
		}

		c >>= master_vol;
		OCR2A = c + 128;
		PORTB &= ~1;
	}
	
}



/*
 * End
 */


