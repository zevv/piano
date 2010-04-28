
#ifndef keyboard_h
#define keyboard_h

enum key {
	KEY_CLARINET = 32,
	KEY_ELECTRIC_GUITAR,
	KEY_OBOE,
	KEY_VIOLIN,
	KEY_XYLOPHONE,
	KEY_PIANO,
	KEY_BANJO,
	KEY_HARPISCORD,
	KEY_POPS,
	KEY_DISCO,
	KEY_16BEAT,
	KEY_LATIN,
	KEY_SWING,
	KEY_SLOW_ROCK,
	KEY_MARCH,
	KEY_WALZ,
	KEY_MIN,
	KEY_MAX,
	KEY_SLOW,
	KEY_FAST,
	KEY_STOP,
	KEY_RECORD,
	KEY_PLAY,
	KEY_START
};

#define KEY_OCT_UP KEY_POPS
#define KEY_OCT_DOWN KEY_SWING

void keyboard_init(void);
void keyboard_scan(void);

#endif

