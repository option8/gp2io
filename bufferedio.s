				ORG $800
BUFFERLENGTH	EQU $08	
INPUTLENGTH		EQU $07
INPUTPTR		EQU $06		
SENDBYTE		EQU $0300
READBYTE		EQU $034A
WAIT			EQU $FCA8
PRINTCR			EQU $FD8E
PRINTBYTE		EQU $FDED	
ANN2HI			EQU $C05D
ANN2LO			EQU $C05C
KEYBUFFER		EQU $0200

BUFFERIO	    JSR QUERYBUFFER		; start by querying the buffer length - is there anything waiting? returns with buffer length in $08
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

TWIDDLE			LDA   #$7C			; |
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
