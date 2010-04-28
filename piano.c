#include <stdio.h>
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#define TIMER_NOTE 1000

#define SINTAB_LEN 256
#define NOTETAB_LEN 12
#define NUM_OSCS 4

int8_t sintab[SINTAB_LEN] = { 0, 3, 6, 9, 12, 16, 19, 22, 25, 28, 31, 34, 37, 40, 43, 46, 49, 52, 54, 57, 60, 63, 66, 68, 71, 73, 76, 78, 81, 83, 86, 88, 90, 92, 94, 96, 98, 100, 102, 104, 106, 108, 109, 111, 112, 114, 115, 116, 118, 119, 120, 121, 122, 123, 123, 124, 125, 125, 126, 126, 126, 127, 127, 127, 127, 127, 127, 127, 126, 126, 125, 125, 124, 124, 123, 122, 121, 120, 119, 118, 117, 116, 114, 113, 112, 110, 108, 107, 105, 103, 101, 99, 97, 95, 93, 91, 89, 87, 84, 82, 80, 77, 75, 72, 69, 67, 64, 61, 59, 56, 53, 50, 47, 44, 41, 39, 36, 32, 29, 26, 23, 20, 17, 14, 11, 8, 5, 2, -2, -5, -8, -11, -14, -17, -20, -23, -26, -29, -32, -36, -39, -41, -44, -47, -50, -53, -56, -59, -61, -64, -67, -69, -72, -75, -77, -80, -82, -84, -87, -89, -91, -93, -95, -97, -99, -101, -103, -105, -107, -108, -110, -112, -113, -114, -116, -117, -118, -119, -120, -121, -122, -123, -124, -124, -125, -125, -126, -126, -127, -127, -127, -127, -127, -127, -127, -126, -126, -126, -125, -125, -124, -123, -123, -122, -121, -120, -119, -118, -116, -115, -114, -112, -111, -109, -108, -106, -104, -102, -100, -98, -96, -94, -92, -90, -88, -86, -83, -81, -78, -76, -73, -71, -68, -66, -63, -60, -57, -54, -52, -49, -46, -43, -40, -37, -34, -31, -28, -25, -22, -19, -16, -12, -9, -6, -3, 0 };
uint8_t notetab[NOTETAB_LEN] = { 130, 138, 146, 155, 164, 174, 184, 195, 207, 220, 233, 246 };

struct osc {
	uint8_t note;
	uint16_t off;
	uint16_t step;
	uint16_t ticks;
	uint8_t vel;
};

static uint8_t cur_osc = 0;

volatile struct osc oscs[NUM_OSCS];


void note_on(uint8_t note, uint8_t vel);


void audio_init(void)
{
	TCCR0A = (1<<COM0A1) | (0<<COM0A0) | (1<<WGM01) | (1<<WGM00);
	TCCR0B = (1<<CS00);
	TIMSK0 |= (1<<TOIE0);
	DDRB |= (1<<PB3);
}


void audio_set(uint8_t v)
{
	OCR0A = v + 128;
}


void update_osc(volatile struct osc *osc)
{
	if(osc->vel >= 80) osc->vel --;
	osc->ticks ++;
}


void note_on(uint8_t note, uint8_t vel)
{
	volatile struct osc *osc;
	uint8_t i;

	/* Find free osc, or use oldest */

	for(i=0; i<NUM_OSCS; i++) {
		osc = &oscs[i];
		if(osc->vel == 0) break;
	}

	osc = &oscs[cur_osc];

	osc->note = note;
	osc->step = notetab[note % 12] << (note / 12); 
	osc->vel = vel;
	osc->ticks = 0;
	
	cur_osc = (cur_osc + 1) % NUM_OSCS;
	
}


void note_off(uint8_t note)
{
	volatile struct osc *osc;
	uint8_t i;

	/* Find free osc, or use oldest */

	for(i=0; i<NUM_OSCS; i++) {
		osc = &oscs[i];
		if(osc->note == note) {
			osc->vel = 0;
		}
	}
}


void keys_init(void)
{
	DDRA = 0x00;
	PORTA = 0xff;

	DDRC = 0xff;
}


void handle_key(uint8_t keynum, uint8_t state)
{
	uint8_t note;

	note = keynum + 20;
	if(state) {
		note_on(note, 255);
	} else {
		note_off(note);
	}
}

void keys_scan(void)
{
	static uint8_t row = 0;
	uint8_t col = 0;
	uint8_t key_cur;
	static uint8_t key_prev[8];
	uint8_t diff;
	uint8_t keynum;
	uint8_t bit;
	
	_delay_us(10);
	PORTC = ~(1<<row);
	_delay_us(10);

	key_cur = PINA;

	diff = key_cur ^ key_prev[row];

	bit = 0x01;
	for(col=0; col<8; col++) {
		if(diff & bit) {
			keynum = row * 8 + (8-col);
			handle_key(keynum, (key_cur & bit) ? 0 : 1);
		}
		bit <<= 1;
	}

	key_prev[row] = key_cur;

	row = (row + 1) % 7;

}


ISR(TIMER0_OVF_vect)
{
	int16_t c = 0;
	static uint8_t t = 0;
	volatile struct osc *osc;
	uint8_t i;

	t++;
	
	PORTB |= 1;

	/* Mix all notes and create audio */

	if((t % 2) == 0) {
		for(i=0; i<NUM_OSCS; i++) {
			osc = &oscs[i];
			c = c + osc->vel * sintab[osc->off >> 8] / (256 * NUM_OSCS); 
			osc->off += osc->step;
		}

		audio_set(c);
	}
	
	/* Update osciallators */

	if(t == 0) {
		PORTB |=  2;
		for(i=0; i<NUM_OSCS; i++) {
			update_osc(&oscs[i]);
		}
		PORTB &= ~2;
	}

	PORTB &= ~1;
}



int main(void)
{
	int i;
	int j;

	DDRB |= 1;
	DDRB |= 2;

	keys_init();
	audio_init();
	sei();

	for(j=0; j<3; j++) {
		note_on(rand() % 30 + 30, 255);
		for(i=0; i<rand() % 20; i++) {
			_delay_ms(10);
		}
	}

	for(;;) {
		keys_scan();
	}

	return 0;
}








