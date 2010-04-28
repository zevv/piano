#ifndef audio_h
#define audio_h

void audio_init(void);
void note_on(uint8_t note);
void note_off(uint8_t note);
void bip(void);
void metronome_set(uint8_t tempo);
void master_vol_set(uint8_t vol);

#endif
