/*
 * final_project_master.c
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
	
	// Set SS, MOSI, and SCK as outputs [ DDRx |= (1 << Pxn) ]
	DDRB |= (1 << PB0);		// SS
	DDRB |= (1 << PB3);		// MOSI
	DDRB |= (1 << PB1);		// SCK
	
	// Set SPI enable, Set as master, Set SPI clock rate to 1 MHz	(Mega datasheet p.197 SPCR)
	SPCR |= (1 << SPE);		// SPI enable
	SPCR |= (1 << MSTR);	// set as master
	SPCR |= (1 << SPR0);	// set SPI clock rate to 1 MHz
	
	int STATE = 1;         // start state machine in "alarm is armed" state
	
	
	
	while(1)
	{	
		/* motion sensor detects movement (from 0 to 1), state "MOVEMENT DETECTED", 10 s timer starts */
		
		switch(STATE)
		{
			case 1:		// "ALARM_ARMED"  /  default when the system is started
			
				if( /* motion sensor 1 */ )
				{
					// case 2: "MOVEMENT_DETECTED"		// set STATE to 2, handle states in separate case function?
					// timer start
					// NOTE: input password on master, send data to slave
					// slave validates the password and sends the data back to master
					// (password is hardcoded in slave e.g. 1234, slave compares the received value to the correct one)
					
					if ( /* correct password */)
					{
						// case 3: "ALARM_DISARMED"
						// stop the timer!
						// NOTE: both boards idle at this state
						
						//   TODO: maybe add rearm functionality here
					}
					
					else if ( /* wrong password */ )
					{
						// notify the user using LED? on slave's side?
						
						// TODO: let user input again?
					}
					
					else if ( /* timer runs out */)
					{
						// case 4: "BUZZER_ON"
						// send data to slave, turn buzzer on
						
						
						// NOTE: ask for the correct password again
						// -->  case 3: "ALARM_DISARMED"
					}
					
					else
					{
						// print error message?
						// NOTE: state machine has STATE 0 as FAULT
					}
				}
				
				else	// motion sensor is idle
				{
					// do nothing?
				}	
		}
	}
}

