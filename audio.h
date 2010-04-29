#ifndef audio_h
#define audio_h

void audio_init(void);
void osc_set_fm(uint8_t mul, uint8_t vel);
void note_on(uint8_t note);
void note_off(uint8_t note);
void all_off(void);
void bip(uint8_t duration);
void metronome_set(uint8_t tempo);
void master_vol_set(uint8_t vol);

#endif
