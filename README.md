
# ATMega Polyphonic FM piano

This is a tiny project I did some years ago to upgrade a small Yamaha
monophonic toy keyboard to polyphonic sound. The sound generating chip was
replaced by an Atmel AVR ATMega644 with firmware to scan the keyboard and
generate 4 channel polyphonic FM sound. It also contains a sequencer for
recording and playing back of tunes, and of course has a factory demo tune.

Unfortunately I don't have any exact schematics of how things are connected, but
everything is pretty basic: a keyboard scan matrix is connected to some GPIO, and
audio output is generated using pulse width modulation on one of the timers.

![hardware(/media/P100233.JPG)


## piano.c

This file contains the main() function, and handles the scanned keys. A switch
statement handles all the different keys: key 0 to 31 are the notes, which are
sent to the sequencer. All other keys are handle depending on their function:
change volume, FM synthesis parameters or commands for the sequencer.

## keyboard.c

The keyboard is connected to a scan matrix with a few rows and columns; The
scan matrix is left as it was originally, but I needed to install diodes on all
the keys to allow scanning polyphonic keys. The function keyboard_scan() is
called from the main loop, and scans a row each time it is executed.

## audio.c

This file holds the code for the 4 channel polyphonic FM synthesizer. A struct
'osc' is allocated for each channel, and holds the current state of the FM
oscillator: a pointer into the sine table (off), sine table step size for the
current note frequency (step), and an ADSR for modifying the note velocity
during the notes life time. Most of the work is done in the timer1 interrupt:
here the oscillator and ADSR states are updated, and the output samples are
calculated for each osc, summed together and sent to the PWM output which is
connected to the amplifier using a 1st order low pass filter.

## sintab.c

The sine table for FM Synthesis

## seq.c

The sequencer consists of a simple list of notes and timestamps, and can be sent
commands for inserting, deleting, editing and playing back notes.

# Licence

The MIT License (MIT)

Copyright (c) Ico Doornekamp

Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the "Software"), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.

