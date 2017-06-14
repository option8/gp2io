				ORG $800

BUFFERLENGTH	EQU $08	
INPUTLENGTH		EQU $07
INPUTPTR		EQU $06		

BYTETOSEND		EQU $ED
BYTETRCVD		EQU $EF
BUTT1HILO		EQU $EE

ANN0HI			EQU $C059
ANN0LO			EQU $C058
ANN1HI			EQU $C05B
ANN1LO			EQU $C05A
ANN2HI			EQU $C05D
ANN2LO			EQU $C05C
BUTT1			EQU $C062

WAIT			EQU $FCA8
PRINTCR			EQU $FD8E
PRINTBYTE		EQU $FDED	
KEYBUFFER		EQU $0200

BUFFERIO	    JSR   QUERYBUFFER	; start by querying the buffer length - is there anything waiting? returns with buffer length in $08
				LDA   #$80			; load control byte $80 "send buffer bytes"
				JSR   SENDBYTE		; send control byte
				LDA   BUFFERLENGTH			; load buffer length (or however much buffer you want sent)
				JSR   SENDBYTE		; send message length byte
BETWEENBYTES	LDA   #$28			; wait a moment (betweenbytes)
				JSR   WAIT			; wait
				JSR   READBYTE		; CTS - send byte
				ORA   #$80			; clear byte 7 (for ASCII)
				JSR   PRINTBYTE		; display byte on screen
				DEC   BUFFERLENGTH	; count down bytes (countdownbuffer)
				LDA   BUFFERLENGTH	; load Accumulator with new buffer length
				BNE   BETWEENBYTES	; if there's more to come, loop (betweenbytes)
				JSR   PRINTCR		; last byte. print CF/LF
				RTS					; return
 
GETLINE   	    JSR   $FD6A			; get line of input, input length in X reg
				TXA					; get input length into Accumulator
				STA   INPUTLENGTH	; put input length into $07
CLEARPTR		LDA   #$00			; load zero into Accumulator
				STA   INPUTPTR		; put zero into $06, keyboard buffer pointer
				LDA   #$04			; load Accumulator with control byte "write to buffer"
				JSR   SENDBYTE		; send control byte
				LDA   INPUTLENGTH	; load input length from $07
				JSR   SENDBYTE		; send message length 
LOADBUFFER		LDX   INPUTPTR		; load X with keyboard buffer pointer (LOADBUFFER)
				LDA   KEYBUFFER,X	; load Accumulator with bytes from keyboard buffer (start at $200)
				AND   #$7F			; clear high bit (for ASCII on other end)  
				JSR   SENDBYTE		; send byte
				INC   INPUTPTR		; increment keyboard buffer pointer
				LDA   INPUTPTR		; load buffer pointer into Accumulator
				CMP   INPUTLENGTH	; compare with message length
				BNE   LOADBUFFER	; if not at end of message, loop to (LOADBUFFER)
				RTS					; return

QUERYBUFFER     LDA   #$40			;	control byte for query buffer length
				JSR   SENDBYTE		;	send control byte
				JSR   TWIDDLE		;	wait routine - gives GP2IO time to receive bytes from serial
CTS				STA   ANN2LO		;	set ANN2 to LOW (just in case it's floating high)
				JSR   READBYTE		;	CTS - ready for response byte with buffer length
				BEQ   QUERYBUFFER	;	if there's nothing in the buffer, loop until there is
				STA   BUFFERLENGTH	;	put the byte in $08
				RTS					;	returns buffer length in $08 (BUFFERLENGTH)

TWIDDLE			LDA   #$21			; ! (inverse)
				JSR   PRINTCHAR		;
				LDA   #$2F			; /
				JSR   PRINTCHAR		;
				LDA   #$2D			; -
				JSR   PRINTCHAR		;
				LDA   #$1C			; \
PRINTCHAR 		JSR   PRINTBYTE		; prints character to screen, high byte not set, inverse text	
				LDA   #$AA			; speed of animation
				JSR   WAIT			; waits for a while
				LDA   #$88			;  (backspace)
				JSR   PRINTBYTE		; backspaces over current char to overwrite
				RTS					; RETURN

SENDBYTE		LDY   #$08			; load 8 for a full byte, loop counter   
				STA   BYTETOSEND	; put byte in zero page for safe keeping   
RTS				STA   ANN0HI		; annunciator 0 high, RTS   
SENDBIT			ROL   BYTETOSEND	; rotate byte left, high/MSB out to carry (sendbit)   
				BCC   SHORTLOOP		; "branch on carry clear" - if carry/bit = 0, goto #311 (short loop)   
LONGLOOP		LDX   #$14			; if carry/bit = 1, load X with 20 (long loop) 
				JMP   SETANN1		; jump over SHORTLOOP
SHORTLOOP		LDX   #$0A			; if carry/bit = 0, load X with 10 (short loop)   
SETANN1			STA   ANN1HI		; set annunciator 1 HIGH   
COUNTDOWN		DEX					; decrement, countdown to setting ANN1 low (COUNTDOWN)   
				BNE   COUNTDOWN		; if X > 0, keep counting (goto COUNTDOWN)   
				STA   ANN1LO		; if done, set ANN1 LOW, countdown to getting next bit   
				LDX   #$05			; reset counter for short "reset" transition. This lets the GP2IO stage the bit before the next interrupt  
COUNTDOWN2		DEX					; count down again (COUNTDOWN2)   
				BNE   COUNTDOWN2	; if X > 0, keep counting (goto COUNTDOWN2)   
				DEY					; decrement y, countdown bits sent   
				BNE   SENDBIT		; if y > 0, next bit (SENDBIT)   
RTSOFF			STA   ANN0LO		; annunciator 0 low, sending RTS OFF   
				RTS					; return   

READBYTE		LDX   #$09			; reading 8 bits requires 9 transitions.
				LDA   #$00			; clear the accumulator
				STA   BYTETRCVD		; $EF is staging for received byte, set 0
				STA   BUTT1HILO		; $EE is staging for each bit, set 0
CTSON			STA   ANN2HI		; set ANN2 HIGH, indicate to AVR "Clear to Send"
LOOPSTART		LDY   #$FF			;	start wait loop
LOOPCOUNT		INY					; Increment Y - rolls over to 0 on first run (loopcount)
				LDA   BUTT1			;	check PB1 status hi/low
				AND   #$80			;	clear 0-6 bits (just need bit 7, all others float)
				CMP   BUTT1HILO		; compare Accumulator 7 bit with $EE, previous PB2 value
				BNE   BITCHANGE		; if PB2 has changed state, store in $EE (bitchange)
				BEQ   LOOPCOUNT		; bit hasn't changed yet, return to (loopcount)
BITCHANGE		STA   BUTT1HILO		; (bitchange)
				CPY   #$44			; if the loop count is more than 68, bit is one. Bit is set in Carry
				ROL   BYTETRCVD		; rotate the new bit into $EF, our result byte.
				DEX					; decrement X, our bit count
				BNE   LOOPSTART		; if bit count is not yet full, loop back to (loopcount)
CTSOFF			STA   ANN2LO		; if bit count is full, set ANN2 LOW, CTS off
				LDA   BYTETRCVD	 	; puts received byte into Accumulator
				RTS					; return with byte in Accumulator
