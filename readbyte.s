				ORG $34A
ANN2HI			EQU $C05D
ANN2LO			EQU $C05C
BUTT1			EQU $C062
BYTETRCVD		EQU $EF
BUTT1HILO		EQU $EE


READBYTE		LDX   #$09		; reading 8 bits requires 9 transitions.
				LDA   #$00		; clear the accumulator
				STA   BYTETRCVD	; $EF is staging for received byte, set 0
				STA   BUTT1HILO	; $EE is staging for each bit, set 0
CTS				STA   ANN2HI	; set ANN2 HIGH, indicate to AVR "Clear to Send"
LOOPSTART		LDY   #$FF		;	start wait loop
LOOPCOUNT		INY				; Increment Y - rolls over to 0 on first run (loopcount)
				LDA   BUTT1		;	check PB1 status hi/low
				AND   #$80		;	clear 0-6 bits (just need bit 7, all others float)
				CMP   BUTT1HILO	; compare Accumulator 7 bit with $EE, previous PB2 value
				BNE   BITCHANGE	; if PB2 has changed state, store in $EE (bitchange)
				BEQ   LOOPCOUNT	; bit hasn't changed yet, return to (loopcount)
BITCHANGE		STA   BUTT1HILO	; (bitchange)
				CPY   #$44		; if the loop count is more than 68, bit is one. Bit is set in Carry
				ROL   BYTETRCVD	; rotate the new bit into $EF, our result byte.
				DEX				; decrement X, our bit count
				BNE   LOOPSTART	; if bit count is not yet full, loop back to (loopcount)
CTSOFF			STA   ANN2LO	; if bit count is full, set ANN2 LOW, CTS off
				LDA   BYTETRCVD ; puts received byte into Accumulator
				RTS				; return with byte in Accumulator
