/*
 * final_project_master.c
 * 
 *		BL40A1812 Introduction to Embedded Systems
 *		final project - master side
 *
 * Created: 3.4.2024 12.24.39
 * Author : Miika Pynttäri
 */ 


/* DEFINES COME FROM Ex. 9 */
#define F_CPU 16000000UL
#define BAUD 9600
#define FOSC 16000000UL			// clock speed
#define MYUBRR FOSC/16/BAUD-1


/* STATE MACHINE VALUES */
#define ALARM_ARMED 1
#define MOVEMENT_DETECTED 2
#define ALARM_DISARMED 3
#define BUZZER_ON 4
#define FAULT 0

//int STATE = 1;         // start in IDLE state

/* keypad defines */
#define RowColDirection DDRB	// Data Direction Configuration for keypad
#define ROW PORTB				// Lower four bits of PORTC are used as ROWs
#define COL PINB				// Higher four bits of PORTC are used as COLs

#define PASSWORD "1234"

#include <avr/io.h>
#include <stdio.h>
#include <util/setbaud.h>
#include <util/delay.h>
#include "keypad/keypad.h"
#include "keypad/delay.h"
// #include "keypad/stdutils.h"

void USART_Init(unsigned int ubrr)
{
	UBRR0H = (unsigned char)(ubrr>>8);
	UBRR0L = (unsigned char)ubrr;
	UCSR0B = (1<<RXEN0)|(1<<TXEN0);
	UCSR0C = (1<<USBS0)|(3<<UCSZ00);
}

void USART_Transmit( unsigned char data, FILE *stream )
{
	while ( !( UCSR0A & (1<<UDRE0)) );
	UDR0 = data;
}

unsigned char USART_Receive( FILE *stream )
{
	while ( !(UCSR0A & (1<<RXC0)) );
	return UDR0;
}

/* the example code has these outside the main function? */
FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);


int main(void)
{
    /* FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
    FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ); */
 
 
	// initialize the UART with 9600 BAUD
	USART_Init(MYUBRR);
    
	// redirect the stdin and stdout to UART functions
    stdout = &uart_output;
    stdin = &uart_input;
	
	// Set SS, MOSI, and SCK as outputs		[  DDRx |= (1 << Pxn)  ]
	DDRB |= (1 << PB0);		// SS
	DDRB |= (1 << PB3);		// MOSI
	DDRB |= (1 << PB1);		// SCK
	
	// Set SPI enable, Set as master, Set SPI clock rate to 1 MHz	(Mega datasheet p.197 SPCR)
	SPCR |= (1 << SPE);		// SPI enable
	SPCR |= (1 << MSTR);	// set as master
	SPCR |= (1 << SPR0);	// set SPI clock rate to 1 MHz
	
	int STATE = 1;         // start state machine in "alarm is armed" state
	int TIMER = -1;
	
	char send_array[10];			// sending either "turnOnBuzz" or "turnOffBuz"
	char receive_array[6];		// receiving either "pwOkay" or "pwWrng"	// NOTE: not receiving anything?
	
	char inputPassword[20];
	char pressedKey;
	int idx = 0;
	char keypadToText[4] = "";
	
	/* initialize the keypad */
	KEYPAD_Init();
	
	TIMER = 0;
	
	while(1)
	{	
		switch(STATE)
		{
			case 1:		// "ALARM_ARMED"  /  default when the system is started
			
				/* if motion sensor activates */
				if( 0 )
				{	
					TIMER = 0;		/* start the timer */
					STATE = 2;		/* goto "MOVEMENT_DETECTED" */
				}	
				
				
			case 2:		// "MOVEMENT_DETECTED"  /  the sensor is triggered
			
				// NOTE: input password on master, send data to slave
				// slave validates the password and sends the data back to master
				// (password is hardcoded in slave e.g. 1234, slave compares the received value to the correct one)
				printf("Type your password: ");
				int pwLength = 0;
				while (1)
				{
					// user inputs the password on the keypad
						/* USART_Transmit password */
						
					pressedKey = KEYPAD_GetKey();
					printf("%c\r", pressedKey);
					inputPassword[pwLength] = pressedKey;
					inputPassword[pwLength+1] = '\0';
					pwLength = strlen(inputPassword);
					if(strlen(inputPassword) == 4) {
						if(!strcmp(inputPassword, PASSWORD)) {
							printf("Password correct!");
						} 
						if (strcmp(inputPassword, PASSWORD)){
							printf("Password incorrect!");
						}
					}
					
					
					/*
					pwLength = sizeof(inputPassword) / sizeof(inputPassword[0]);
					//printf("%d\n", sizeof(inputPassword) / (inputPassword[0]));
	
					//char keypadToText[4] = "";
					
					printf("%d", pwLength);
					
					if (pwLength < 4)
					{
						pressedKey = KEYPAD_GetKey();
						keypadToText[idx] = pressedKey;			// converting int to char
						//inputPassword[idx] = pressedKey;
						printf("%c", keypadToText[idx]);
						idx += 1;
					}
					
					printf("%s", keypadToText);
					*/
						
					/* PORTB &= ~(1 << PB0);		// set SS low
					
					for (int i = 0; i <= sizeof(send_array); i++)
					{
						SPDR = send_array[i];
						
						while (!(SPSR & (1 << SPIF))) {;}		// Wait until ready
						receive_array[i] = SPDR;				// Receive data using SPDR
					}
					
					PORTB |= (1 << PB0); */
					
					
						
					/* correct password */
					
					//if ( inputPassword == correctPassword )
					//{
						//STATE = 3;		/* goto "ALARM_DISARMED" */
						//TIMER = -1000000;
					//}
					
					/* wrong password */
					//else if ( !inputPassword == correctPassword )
					//{
						// notify the user using LED on slave's side?		/* when slave sends message, flash/turn on led on slave's side */
						// let user input again
					//}
				
					//else if ( TIMER > 10 )
					//{	
						//STATE = 4;		/* goto "BUZZER_ON" */
						
						/* USART_Transmit "bzOn" */
						
						// send data to slave, turn buzzer on
					//}
					
					TIMER += 1;		/* NOTE: has to be asynchronous? how to do this? */
					
				
				}
			
				
			case 3:		// "ALARM_DISARMED"  /  the correct password was input
	
				// stop the timer (?)
				// the program is finished (unless rearm functionality is added)	
				
				
				/* TODO: input to console(?) "system disarmed" */
			
				break;			// no exit function in C?
			
			
			case 4:		// "BUZZER_ON"  /  start the buzzer when the 10 second timer ran out
						/* basically the same as state 2 (movement detected) but without the timer */
						
						
				/* WAIT FOR USER'S PASSWORD */
				
				/* correct password */
				if ( inputPassword == PASSWORD )
				{
					STATE = 3;		/* goto "ALARM_DISARMED" */
					TIMER = -1000000;
				}
				
				/* wrong password */
				else if ( !inputPassword == PASSWORD )
				{
					// notify the user using LED on slave's side?		/* when slave sends message, flash/turn on led on slave's side */
					// let user input again
				}		

		}
	}
}

