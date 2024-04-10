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

#define FOSC 16000000UL
#define MYUBRR FOSC/16/BAUD-1


/* STATE MACHINE VALUES */
#define ALARM_ARMED 1
#define MOVEMENT_DETECTED 2
#define ALARM_DISARMED 3
#define BUZZER_ON 4
#define FAULT 0

//int STATE = 1;         // start in IDLE state



#include <avr/io.h>
#include <stdio.h>
#include <util/setbaud.h>
#include <util/delay.h>

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



int main(void)
{
    USART_Init(MYUBRR);
    
    FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
    FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);
    
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
	
	
	
	while(1)
	{	
		
		switch(STATE)
		{
			case 1:		// "ALARM_ARMED"  /  default when the system is started
			
				if( /* motion sensor 1 */ )
				{	
					TIMER = 0;		/* start the timer */
					STATE = 2		/* goto "MOVEMENT_DETECTED" */
				}	
				
				
			case 2:		// "MOVEMENT_DETECTED"  /  the sensor is triggered
			
				// NOTE: input password on master, send data to slave
				// slave validates the password and sends the data back to master
				// (password is hardcoded in slave e.g. 1234, slave compares the received value to the correct one)
			
				while (TIMER < 100)
				{
					// user inputs the password on the keypad
						/* USART_Transmit password */
				
					if ( /* correct password */)		// USART_Receive correctPassword
					{
						STATE = 3;		/* goto "ALARM_DISARMED" */
						TIMER = -1000000;
					}
				
					else if ( /* wrong password */ )	// USART_Receive wrongPassword
					{
						// notify the user using LED on slave's side?		/* when slave sends message, flash/turn on led on slave's side */
						// let user input again
					}
				
					else if ( TIMER > 10 )
					{
						STATE = 4		/* goto "BUZZER_ON" */
						// send data to slave, turn buzzer on
							/* USART_Transmit turnOnBuzzer */			// NOTE: this first before state change?
					}
				
					TIMER += 1;		/* NOTE: has to be asynchronous? how to do this? */
				}
			
				
			case 3:		// "ALARM_DISARMED"  /  the correct password was input
	
				// stop the timer (?)
				// the program is finished (unless rearm functionality is added)	
			
				break;			// no exit function in C?
			
			
			case 4:		// "BUZZER_ON"  /  start the buzzer when the 10 second timer ran out
						/* basically the same as state 2 (movement detected) but without the timer */
						
						
				/* WAIT FOR USER'S PASSWORD */
				
				if ( /* correct password */)		/* USART_Receive correctPassword */
				{
					STATE = 3;		// goto "ALARM_DISARMED"
				}
				
				else if ( /* wrong password */ )	/* USART_Receive wrongPassword */
				{
					// notify the user using LED? on slave's side?
					// let user input again
				}		

		}
	}
}

