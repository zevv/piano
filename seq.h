
#ifndef seq_h
#define seq_h

enum seq_cmd {
	SEQ_CMD_CLEAR,
	SEQ_CMD_DEL,
	SEQ_CMD_FIRST,
	SEQ_CMD_LAST,
	SEQ_CMD_PREV,
	SEQ_CMD_NEXT,
	SEQ_CMD_PLAY,
	SEQ_CMD_REC,
	SEQ_CMD_STOP,
	SEQ_CMD_TEMPO_UP,
	SEQ_CMD_TEMPO_DOWN,
	SEQ_CMD_METRONOME_3_4,
	SEQ_CMD_METRONOME_4_4,
	SEQ_CMD_ONEKEY_ON,
	SEQ_CMD_ONEKEY_OFF,
};

void seq_init(void);
void seq_note(uint8_t note, uint8_t state);
void seq_cmd(enum seq_cmd cmd);
void seq_tick(void);

#endif
