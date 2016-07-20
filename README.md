===
What is GP2IO?
===

My goal is to create an interface between Apple II and the library of peripherals and devices that exist for the modern hobbyist, a list that has exploded recently due to the popularity of Arduino, Raspberry Pi, and other programmable single board computers.

These peripherals - displays, LED arrays, sensors, motor controllers and more – generally communicate via one of a handful of protocols like SPI, I2C or serial. Out of the box, an Apple II can't talk on these channels except serial. Even serial has its limits, though, since the Super Serial Card and later models with built-in serial ports use the older RS232 standard. This requires an adapter to convert the signal voltages to TTL for more modern UART serial, that's built into Arduino, Pi, etc.

The "glue" between the Apple II and the newer devices is the GP2IO. This stands for "General Purpose Game Port I/O"

GP2IO is based on sending serial data from an Apple II game port annunciator pin to an Atmel AVR microcontroller. An Arduino "sketch" on the AVR interprets the data and then acts on it accordingly – triggering LEDs, sending or receiving data to peripherals via I2C, serial, SPI, etc.

Communication is bidirectional, as well. Data is received on the Apple II via reading data shifted into one of the paddle button pins from the AVR, similar to the way data is read through the cassette input. 


Reference: 

                               =========
    Pushbutton 3 (GS) / NC 9  | *     * | 8  Ground
     Gm Ctrl 1 (Stick-1 Y) 10 | *     * | 7  Gm Ctrl 2 (Stick-2 X)
     Gm Ctrl 3 (Stick-2 Y) 11 | *     * | 6  Gm Ctrl 0 (Stick-1 X)
            Annunciator 3  12 | *     * | 5  +5V (GS ONLY) / $C040 Strobe (II-IIe)
            Annunciator 2  13 | *     * | 4  Pushbutton 2
            Annunciator 1  14 | *     * | 3  Pushbutton 1
            Annunciator 0  15 | *     * | 2  Pushbutton 0
            No Connection  16 | *     * | 1  +5V
                               ===| |===
                                   ^
                             Notch on socket
                     (faces toward front of computer)

For input, I use the joystick/paddle button pins on the game port. Setting these to TTL HIGH (pressed) or LOW (not pressed) and "bit banging" in rapid succession gives them the capacity to carry serial data. Elsewhere, this is referred to as the Track & Field Protocol (http://atomsandelectrons.com/blog/2010/04/apple-t/)

The first two buttons addresses are shared with the Apple keys, so it's best not to fiddle with those, if your program uses the Apple keys for keyboard input, macros, or for the PEW PEW!

	PB0 / OPNAPPLE	= $C061	; open apple (command) key data (read) 
	PB1 / CLSAPPLE	= $C062	; closed apple (option) key data (read) 
	PB2 			= $C063	; game Pushbutton 2 (read) <- Our victim

	PB3 (GS only)	= $C060	; game Pushbutton 3 (read) 
                          $C060 bit 7 = data from cassette on Apple II, II+, IIe

Button 3 and the cassette share the same memory address $C060. So, we could toggle pin 9 on the game port (PB3) to load data directly into the memory address for the built-in cassette routines. But that pin is only on the GS. Poo. And the GS, having no cassette input, has no cassette routine in ROM. Double poo.

Luckily, there are already examples of reading in data on those pins. 

I dug into [Michael Mahon's NadaNet](http://michaeljmahon.com/NadaNetPaper.html) implementation for inspiration, and found proof of speedy and reliable communication on the game port. This was encouraging.

But, rather than require developers to implement NadaNet network packet reads just to get a few bytes into and out of an AVR, we can continue to take inspiration from the cassette routines. Using those as a starting point, I look for a transition from LOW to HIGH in pushbutton 1, then count the loops until the transition from HIGH to LOW. If it's a short time, the bit is zero, if it's longer, a one.

===

The game port also has four output pins: Annunciators 0,1,2,3. Each has a softswitch and memory location for "setting" and "clearing", counterintuitively these are 0 and 1 respectively. For AVR comms, I'll refer to HIGH (1) and LOW (0).

	SETAN0 =   $C058 ;Set annunciator-0 output to 0 
	CLRAN0 =   $C059 ;Set annunciator-0 output to 1 

	SETAN1 =   $C05A ;Set annunciator-1 output to 0 
	CLRAN1 =   $C05B ;Set annunciator-1 output to 1 

	SETAN2 =   $C05C ;Set annunciator-2 output to 0 
	CLRAN2 =   $C05D ;Set annunciator-2 output to 1 

	SETAN3 =   $C05E ;Set annunciator-3 output to 0 
	CLRAN3 =   $C05F ;Set annunciator-3 output to 1 

NOTE: You won't likely be able to use the GP2IO in this mode with a double hi-res program. Double hi-res is incompatible with the annunciators since it clobbers those memory locations:

	SETIOUDIS= $C07E ;enable DHIRES & disable $C058-5F (W) 
	CLRIOUDIS= $C07E ;disable DHIRES & enable $C058-5F (W) 

	SETDHIRES= $C05E ;if IOUDIS Set, turn on double-hires 
	CLRDHIRES= $C05F ;if IOUDIS Set, turn off double-hires 

Sorry.

Other peripherals, like the ZIP Chip, use the annunciators' memory locations for setting configuration flags, so, timing matters aside, there are probably other incompatible setups as well.

Sorry again.


===
Sending data from Apple II to GP2IO
===

Considering it's available in ROM on a bare II or II+, repurposing the cassette output seemed like a good place to start. There's no handshaking (other than the user pressing Record on the tape recorder) and it requires minimal effort on the Apple II to send a stream of bytes. The source for the ROM cassette output, via 
			https://github.com/cmosher01/Apple-II-Source/blob/master/src/system/monitor/common/cassette.m4

     TAPEOUT  =     $C020   

   	HEADR    LDY   #$4B       ;WRITE A*256 'LONG 1'   
   			 JSR   ZERDLY     ;  HALF CYCLES   
   			 BNE   HEADR      ;  (650 USEC EACH)   
   			 ADC   #$FE   
   			 BCS   HEADR      ;THEN A 'SHORT 0'   
   			 LDY   #$21       ;  (400 USEC)   
   	WRBIT    JSR   ZERDLY     ;WRITE TWO HALF CYCLES   
   			 INY              ;  OF 250 USEC ('0')   
   			 INY              ;  OR 500 USEC ('0')   
   	ZERDLY   DEY   
   			 BNE   ZERDLY   
   			 BCC   WRTAPE     ;Y IS COUNT FOR   
   			 LDY   #$32       ;  TIMING LOOP   
   	ONEDLY   DEY   
   			 BNE   ONEDLY   
   	WRTAPE   LDY   TAPEOUT   
   			 LDY   #$2C   
   			 DEX   
   			 RTS   

Sending bytes from the Apple II via GP2IO is similar to writing to the cassette port. Instead of writing to the $C020 casette output for each bit, annunciator 1 is set to HIGH, then back to LOW after a short wait (zero) or longer wait (one). 

There is also a short "reset" transition from LOW to HIGH between bits, which are ignored by the AVR, to remove the necessity of keeping track of the ANN1 value between bits and bytes.

I've also added in rudimentary handshaking with "Ready to Send" signals. Before sending, to tell the AVR to be ready to receive, I set annunciator 0 HIGH ("clear ANN0"), then LOW again when the byte is done. For speed, and since we have two-way communications, we can dispense with the long header write and sync bit of the cassette routine. 

Here is the source, which should be fairly relocatable (thanks, Mark Pilgrim):

SENDBYTE
entry point $0303, or $0300 for sending single bit
uses $EF for storing outgoing byte

    0300-   A0 01       LDY   #$01	; load 1, for a 1 bit send - OR -   
    0302-   2C          BIT  		;    
    0303-   A0 08       LDY   #$08	; load 8 for a full byte, loop counter   
    0305-   85 EF       STA   $EF	; put byte in zero page for safe keeping   
    0307-   8D 59 C0    STA   $C059	; annunciator 0 high, RTS   
    030A-   26 EF       ROL   $EF	; rotate byte left, high/MSB out to carry (sendbit)   
    030C-   90 03       BCC   $0311	; "branch on carry clear" - if carry/bit = 0, goto #311 (short loop)   
    030E-   A2 14       LDX   #$14	; if carry/bit = 1, load X with 20 (long loop) - OR -   
    0310-   2C          BIT   
    0311-   A2 0A       LDX   #$0A	; if carry/bit = 0, load X with 10 (short loop)   
    0313-   8D 5B C0    STA   $C05B	; set annunciator 1 HIGH   
    0316-   CA          DEX			; decrement, countdown to setting ANN1 low (countdown)   
    0317-   D0 FD       BNE   $0316	; if X > 0, keep counting (countdown)   
    0319-   8D 5A C0    STA   $C05A	; if done, set ANN1 LOW, countdown to getting next bit   
    031C-   A2 05       LDX   #$05	; reset counter for short "reset" transition. This lets the GP2IO stage the bit before the next interrupt   
    031E-   CA          DEX			; count down again (countdown2)   
    031F-   D0 FD       BNE   $031E	; if X > 0, keep counting (countdown2)   
    0321-   88          DEY			; decrement y, countdown bits sent   
    0322-   D0 E6       BNE   $030A	; if y > 0, next bit (sendbit)   
    0324-   8D 58 C0    STA   $C058	; annunciator 0 low, sending RTS OFF   
    0327-   60          RTS			; return   


     ___---------------------___ 	RTS line goes high
     ____--_-_--_--_-_--_-_-____		as Apple sends 10110100 (0xB4, ASCII 4)


On the GP2IO side, the AVR has two interrupts set on ANN0 and ANN1: as the RTS pin goes HIGH (rising), and on changes to the data pin. If the data pin changes state in less than 70 microseconds (.07 milliseconds) the bit is read as zero. Longer than 70 microseconds, a one.

~~~
     attachInterrupt(0, APPLERTS, RISING); // ANNUNCIATOR 0, Apple sending byte
     attachInterrupt(1, RECEIVINGBITS, CHANGE); // ANNUNCIATOR 1, Apple sending bits
   void APPLERTS() {
     // signal to start receiving bits from Apple II
     bitCount = 0;
     changeCount = 0;
     returnByte = B00000000;
   }
   void RECEIVINGBITS()
   {
     // ignore short "reset" transitions
     currentMicros = micros();
     if (changeCount % 2 == 1) {
       if ((currentMicros - lastMicros) > 70) {
         receivedBit = 1;
       } else {
         receivedBit = 0;
       }
       byteArray[7 - bitCount] = receivedBit;
       bitCount++;
     }
     changeCount++;
     if (bitCount == 8) { // got a BYTE
       receivedByte = arrayToByte(byteArray, 8);
       PROCESSBYTE( byte(receivedByte) );
       TIMEOUTCLOCK = millis();
     }
     lastMicros = currentMicros;
   }
~~~


===
Practical example SENDKEY
===
Read input from the keyboard, send each byte to GP2IO

    20 1B FD    JSR   $FD1B	; wait for keyboard input, byte now in Accumulator
    C9 9B       CMP   #$9B	; check for ESC
    F0 0B       BEQ   $0812	; return on ESC
    20 ED FD    JSR   $FDF0	; print byte to screen, local echo
    09 80       ORA   #$80	; clear bit 7 (for "low" vs "high" ASCII, display purposes)
    20 03 03    JSR   $0303	; send the byte to GP2IO (SENDBYTE)
    4C 00 08    JMP   $0800	; repeat
    60          RTS			; return on ESC




===
Processing Bytes on the GP2IO
===

The AVR starts in a null "standby" state, waiting for a "control" byte from the Apple II to set its function. Subsequent bytes after the function is set are the "message". The value of the first byte from the Apple determines the function according to this table:

        $01		Sets the RGB LED on the GP2IO to the color value of the next three bytes, in order Red, Green and Blue. Requires 3 byte message
        
        $02		Sets the RGB LED to white, intensity based on the 1 byte message that follows (0-255).
        
        $04		Write the following message to an internal buffer, for retrieval later.	First byte is message length (0-255 bytes follow)
        
        $08		Write the following message to UART serial (buffered). First byte is message length (0-255 bytes follow)
        
        $10		Write the following 8 bytes to I2C bus. For the demo, this is connected to either an 8x8 LED matrix or a 4 character 7-segment display. Requires 8 byte message. 
        
        $20		SPI (not yet implemented)
        
        $40		Query the buffer. Triggers the AVR to respond with one byte, containing the length of the current buffer. 
        
        $80		Send the buffer. Triggers the AVR to respond with the first N bytes of the buffer, where N is the byte following the $80 trigger.
        
        $00		Debug mode. At the moment, simply echoes the bytes received from the Apple side into the serial buffer, and prints them to USB serial out. Also sends one byte from the GP2IO buffer to the Apple whenever the APPLECTS signal (Annunciator 2) goes high.
	

Each function, as outlined, expects a certain number of bytes per message. For example, to set an RGB LED requires three bytes - one each for Red, Green, and Blue values, plus the first $01 byte to set the function, for a total of four. 

The AVR then shuttles those bytes to the proper output, sends results back to the Apple, or waits for further instruction.

After each message is done, the GP2IO returns to standby for the next function, so before setting the LED to a different color, you need to send the $01 control byte again. In case a message gets cut off, or something goes awry, the GP2IO will also go to standby if nothing is received for roughly 5 seconds.

The LED on the GP2IO board goes to blue to indicate it has timed out or is in standby mode waiting for a control byte.


EXAMPLE: RGB LED Pulse white

    0800-   A9 FF       LDA   #$FF	; set accumulator = 255, starts with LED off after one INC
    0802-   48          PHA			; push FF to stack
    0803-   A9 02       LDA   #$02	; set accumulator = 2, one byte LED mode
    0805-   20 03 03    JSR   $0303	; SENDBYTE, sets the mode (setmode)
    0808-   68          PLA			; pull current intensity from stack
    0809-   18          CLC			; clear carry
    080A-   69 01       ADC   #$01	; increment accumulator/intensity value, $FF rolls to $00
    080C-   48          PHA			; push new intensity to stack
    080D-   20 03 03    JSR   $0303	; SENDBYTE, sets the LED intensity
    0810-   4C 03 08    JMP   $0803	; jump to (setmode) for next pass



SLIGHTLY MORE COMPLEX EXAMPLE: RGB LED pulsing 


SETRGB:
	one-shot, sets the LED with red value in $06, green at $07, blue at $08

    0800-   A9 01       LDA   #$01	; set accumulator to 1, three byte RGB mode
    0802-   20 03 03    JSR   $0303	; $0303 = SENDBYTE routine
    0805-   A5 06       LDA   $06	; read in red value from zero page
    0807-   20 03 03    JSR   $0303	; SENDBYTE
    080A-   A5 07       LDA   $07	; read green value
    080C-   20 03 03    JSR   $0303	; SENDBYTE
    080F-   A5 08       LDA   $08	; read blue value
    0811-   20 03 03    JSR   $0303	; SENDBYTE
    0814-   60          RTS			; return.


CYCLERGB

    0816-   A9 00       LDA   #$00	; set accumulator to 0
    0818-   85 06       STA   $06	; zero to red value,
    081A-   85 07       STA   $07	; zero green
    081C-   85 08       STA   $08	; zero blue
    081E-   20 00 08    JSR   $0800	; SETRGB
    0821-   A5 06       LDA   $06	; load red value
    0823-   69 02       ADC   #$02	; add 2 to current red value
    0825-   85 06       STA   $06	; push it to zero page $06
    0827-   B0 F5       BCS   $081E	; if still less than $FF, return and SETRGB
    0829-   A5 07       LDA   $07	; else, red has rolled over, load green
    082B-   69 02       ADC   #$02	; increment green x2
    082D-   85 07       STA   $07	; store green
    082F-   B0 ED       BCS   $081E	; if less than $FF, return and SETRGB
    0831-   A5 08       LDA   $08	; else, green has rolled over, load blue
    0833-   69 02       ADC   #$02	; increment blue
    0835-   85 08       STA   $08	; store blue
    0837-   4C 1E 08    JMP   $081E	; return and SETRGB
    083A-   00          BRK





===
Receiving Data from the GP2IO to the Apple
===

The plan, again, is to mimic data coming in on the cassette port, but with the pushbutton signal going high and low instead of tones on the casette. About the cassette input:

	"Routines in the monitor scan bit 7 at location $CO6O every 12.8 microseconds looking for a transition. By measuring the time between transitions, the routines can distinguish between header, sync, 0, and 1 bits."
	
	From Apple II Circuit Description by Winston Gaylor
	©1983 by Howard and Sams & Co, Inc.

Here's the source for those calls, again via https://github.com/cmosher01/Apple-II-Source/blob/master/src/system/monitor/common/cassette.m4

	TAPEOUT  =     $C020
	TAPEIN   =     $C060

	RDBYTE   LDX   #$08       ;8 BITS TO READ
	RDBYT2   PHA              ;READ TWO TRANSITIONS
			 JSR   RD2BIT     ;  (FIND EDGE)
			 PLA
			 ROL              ;NEXT BIT
			 LDY   #$3A       ;COUNT FOR SAMPLES
			 DEX
			 BNE   RDBYT2
			 RTS

	RD2BIT   JSR   RDBIT
	RDBIT    DEY              ;DECR Y UNTIL
			 LDA   TAPEIN     ; TAPE TRANSITION
			 EOR   LASTIN
			 BPL   RDBIT
			 EOR   LASTIN
			 STA   LASTIN
			 CPY   #$80       ;SET CARRY ON Y
			 RTS



Here is my GP2IO READ BYTE routines:

$EF = byte as it's read
$EE = pushbutton 1 value stored for comparison
$C05D = set annunciator 2 = 1
$C05C = set annunciator 2 = 0
Entry point at $34A. Received byte is returned in Accumulator 

READCOMBINED

    034A-   A2 09       LDX   #$09	; reading 8 bits requires 9 transitions.
    034C-   A9 00       LDA   #$00	; clear the accumulator
    034E-   85 EF       STA   $EF	; $EF is staging for received byte, set 0
    0350-   85 EE       STA   $EE	; $EE is staging for each bit, set 0
    0352-   8D 5D C0    STA   $C05D	; set ANN2 HIGH, indicate to AVR "Clear to Send"
    0355-   A0 FF       LDY   #$FF	;	start wait loop
    0357-   C8          INY			; Increment Y - rolls over to 0 on first run (loopcount)
    0358-   AD 62 C0    LDA   $C062	;	check PB1 status hi/low
    035B-   29 80       AND   #$80	;	clear 0-6 bits (just need bit 7, all others float)
    035D-   C5 EE       CMP   $EE	; compare Accumulator 7 bit with $EE, previous PB2 value
    035F-   D0 02       BNE   $0363	; if PB2 has changed state, store in $EE (bitchange)
    0361-   F0 F4       BEQ   $0357	; bit hasn't changed yet, return to (loopcount)
    0363-   85 EE       STA   $EE	; (bitchange)
    0365-   C0 44       CPY   #$44	; if the loop count is more than 68, bit is one. Bit is set in Carry
    0367-   26 EF       ROL   $EF	; rotate the new bit into $EF, our result byte.
    0369-   CA          DEX			; decrement X, our bit count
    036A-   D0 E9       BNE   $0355	; if bit count is not yet full, loop back to (loopcount)
    036C-   8D 5C C0    STA   $C05C	; if bit count is full, set ANN2 LOW, CTS off
    036F-   A5 EF       LDA   $EF	; puts received byte into Accumulator
    0371-   60          RTS			; return with byte in Accumulator


And a demo: this reads one byte from the AVR output buffer to the screen. It gets one byte at a time, triggered by a keypress. The AVR will need to be ready to send one byte at a time when it sees ANN2 go HIGH ("SEND BUFFER CONTENT" mode, set by sending $80 as the function switch first).

GETBYTEONKEY

    0800-   20 1B FD    JSR   $FD1B	; wait for keypress
    0803-   20 4A 03    JSR   $034A	; READBYTE
    0806-   20 DA FD    JSR   $FDDA	; print hex to screen
    0809-   20 8B FD    JSR   $FD8B	; return carriage
    080C-   4C 00 08    JMP   $0800	; repeat


So, how to receive usefully large chunks of data at once? With no interrupts on the game port to work with on incoming data, and better things to do with your program than poll the game port constantly, we need to tell the AVR when to send, and how much data you're ready to store.

The Apple sends a "Send Buffer Data" control byte (0x80) followed by another byte indicating how long the message should be (1-255 bytes).

The GP2IO then sends the requested number of bytes from its internal output buffer, zero-padded if necessary. Future versions could optionally include a checksum. The checksum could be a simple AND of each byte, like what's done with the original cassette routine, or something more complicated. This is not yet implemented, and I leave that as an exercise for other programmers.

Here is a buffered I/O routine for minimally interactive data transmission between the Apple II and a serial device connected to the GP2IO.

    0800-   20 4D 08    JSR   $084D	; start by querying the buffer length - is there anything waiting? returns with buffer length in $08
    0804-   A9 80       LDA   #$80	; load control byte $80 "send buffer bytes"
    0806-   20 03 03    JSR   $0303	; send control byte
    0809-   A5 08       LDA   $08	; load buffer length (or however much buffer you want sent)
    080B-   20 03 03    JSR   $0303	; send message length byte
    080E-   A9 28       LDA   #$28	; wait a moment (betweenbytes)
    0810-   20 A8 FC    JSR   $FCA8	; wait
    0813-   20 4A 03    JSR   $034A	; CTS - send byte
    0816-   09 80       ORA   #$80	; clear byte 7 (for ASCII)
    0818-   20 ED FD    JSR   $FDED	; display byte on screen
    081B-   C6 08       DEC   $08	; count down bytes (countdownbuffer)
    081D-   A5 08       LDA   $08	; load Accumulator with new buffer length
    0820-   D0 EC       BNE   $080E	; if there's more to come, loop (betweenbytes)
    0822-   20 8E FD    JSR   $FD8E	; last byte. print CF/LF
    0825-   60		RTS		; return


GETLINE

    0826-   20 6A FD    JSR   $FD6A	; get line of input
    0829-   8A          TXA		; get input length into Accumulator
    082A-   85 07       STA   $07	; put input length into $07
    082C-   A9 00       LDA   #$00	; load zero into Accumulator
    082E-   85 06       STA   $06	; put zero into $06, keyboard buffer pointer
    0830-   A9 04       LDA   #$04	; load Accumulator with control byte "write to buffer"
    0832-   20 03 03    JSR   $0303	; send control byte
    0835-   A5 07       LDA   $07	; load input length from $07
    0837-   20 03 03    JSR   $0303	; send message length 
    083A-   A6 06       LDX   $06	; load X with keyboard buffer pointer (keybuffer)
    083C-   BD 00 02    LDA   $0200,X	; load Accumulator with bytes from keyboard buffer (start at $200)
    083F-   29 7F       AND   #$7F	; clear high bit (for ASCII)  
    0841-   20 03 03    JSR   $0303	; send byte
    0844-   E6 06       INC   $06	; increment keyboard buffer pointer
    0846-   A5 06       LDA   $06	; load buffer pointer into Accumulator
    0848-   C5 07       CMP   $07	; compare with message length
    084A-   D0 EE       BNE   $083A	; if not at end of message, loop to (keybuffer)
    084C-   60          RTS		; return

QUERY BUFFER LENGTH
returns buffer length in $08

    084D-   A9 40       LDA   #$40	;	control byte for query buffer length
    084F-   20 03 03    JSR   $0303	;	send control byte
    0852-   20 28 03    JSR   $0328	;	wait routine - gives GP2IO time to receive bytes from serial
    0855-   8D 5C C0    STA   $C05C	;	set ANN2 to LOW (just in case it's floating high)
    0858-   20 4A 03    JSR   $034A	;	CTS - ready for response byte with buffer length
    085B-   F0 F0       BEQ   $084D	;	if there's nothing in the buffer, loop until there is
    085D-   85 08       STA   $08	;	put the byte in $08
    085F-   60          RTS		;	return  



Used Zero Page locations:

06 07 08: SetRGB routine, Red Green Blue bytes 

EE: ReadBit
EF: ReadByte, SendByte



===
So, What Can It Do?
===

With a general purpose gateway to I2C, SPI and UART devices (among others), GP2IO lets the Apple II access a variety of different inputs and outputs, from sensors and alternate controllers to external data displays.

Some use case ideas include:
 -	light indicator for disk activity, RWTS function
 -	trackstar-type indicator for bootloaders
 -	game status indicators - blink on collision, red for damage, etc
 -	get current internet time via NTP for real time clock function
	

===
So, What Can't It Do?
===

The main limitation is the platforms the GP2IO can connect to. The IIc and IIc+ have no 16-pin game port, and the 9-pin game port on those models lacks connections to the annunciator pins. The GS has the 16-pin port, but the CPU speed makes the timing-based loops run much too quickly without modification.

Those models also include serial on-board, so why not use serial connection?

===
So, Why Not Use A Serial Connection?
===

Seriously, why not?

The GP2IO includes RS232 9-pin connector that's adapted on the board to UART levels, so yes, you can connect to it directly with any Apple II with serial. 

Did I forget to mention that?


===
Big thanks to 
===

 - Mark Pilgrim for checking and optimizing my code, and helping out testing the various use cases. 

 - David Schmidt for sanity checking my serial implementation and proof-of-concept.

 - KansasFest committee for allowing me to present, public beta test the GP2IO at KansasFest 2016.

 - Chris Torrence and Roger Wagner for Assembly Lines, both the book and searchable PDF.

 - Michael Mahon, for inventing such a nice wheel that I had to go and reinvent it.



=== 
The Inaugural, First Annual, and Probably Last, RetroConnector Mini-Hackfest
===

Each participant will be given a beta GP2IO board and available components as requested, on a first-come-first-served basis. Participants may keep the board, so long as he or she submits a completed entry, and agrees to post instructions publicly. Participants are encouraged to build or purchase additional hardware as needed, so long as they do not spend more than $50. The GP2IO board, its software and components can be altered, repurposed and hacked, but not outright destroyed. Okay, fine, you can destroy whatever you want, as long as your entry is cooler because of it.

All entries must be documented or demonstrated to a degree they could be reproduced by someone of reasonable intelligence, and submitted or demonstrated to the judges prior to the Vendor Faire. The judges will have to decide amongst themselves what constitutes "reasonable intelligence."

Entries will be judged on the following criteria:
 -	Is it cool?
 -	Does it do something the Apple II couldn't do before?
 -	Is it *really* cool?
 -	Originality.
 -	No, seriously. How cool is that? I'm angry I didn't think of it myself.
	
Judges:
 -	Charles Mangin
 -	Mark Pilgrim
 -	Bill Martens
	
		



