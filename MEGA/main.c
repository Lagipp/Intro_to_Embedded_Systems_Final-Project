/*
 * final_project_master
 * main.c 
 * 
 *		BL40A1812 Introduction to Embedded Systems
 *		final project - master
 *
 * Created: 4/2024
 * Author : Group 14
 */ 

 /* Code taken from different exercises through out the course unless stated otherwise */
 
/* DEFINES COME FROM Ex. 9 */
#define F_CPU 16000000UL
#define BAUD 9600
#define FOSC 16000000UL			// clock speed
#define MYUBRR FOSC/16/BAUD-1


/* STATE MACHINE VALUES */
#define ALARM_ARMED 1
#define MOVEMENT_DETECTED 2
#define BUZZER_ON 3
#define ALARM_DISARMED 4
#define FAULT 0

#define BUZZER_START_TIME 10 // Time when buzzer is turned on after detecting motion (seconds)
volatile int g_timer = 0; // Used in ISR to count time after movement is detected.

#define STRING_LENGTH 20 // Character array length

/* keypad defines */
#define RowColDirection DDRB	// Data Direction Configuration for keypad
#define ROW PORTB				// Lower four bits of PORTC are used as ROWs
#define COL PINB				// Higher four bits of PORTC are used as COLs

/* Password */
#define PASSWORD "1234" // password is hard coded e.g. 1234
#define CORRECT_PW_LENGTH strlen(PASSWORD)

/* Buttons on the keypad */
#define BACKSPACE '*'
#define ENTER '#'
#define REARM_BTN 'A'
#define EXIT_BTN 'B'

#include <avr/io.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <util/setbaud.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <ctype.h>
#include <avr/sleep.h>
#include "keypad/keypad.h"
#include "keypad/delay.h"
#include "LCD/lcd.h"

/* USART functions used for communication via USB 
Source: exercises */
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
	while ( !(UCSR0A & (1<<UDRE0)) );
	return UDR0;
}

// Assign stdout and stdin as the USART functions
FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);

// Source: http://www.arduinoslovakia.eu/application/timer-calculator
/* Setups interrupt timer1 */
void setup_timer1() 
{
	// Clear registers
	TCCR1A = 0;
	TCCR1B = 0;
	TCNT1 = 0;

	// 1 Hz (16000000/((15624+1)*1024))
	OCR1A = 15624;
	// CTC
	TCCR1B |= (1 << WGM12);
	// Prescaler 1024
	TCCR1B |= (1 << CS12) | (1 << CS10);
	// Output Compare Match A Interrupt Enable
	TIMSK1 |= (1 << OCIE1A);
}

/* Stops timer1 */
void stop_timer1() 
{
	// Clear prescaler bits to stop the timer
	TCCR1B &= ~((1 << CS12) | (1 << CS10));
}

/* Setups lcd */
void setup_lcd() 
{
	EICRA |= (1 << ISC01);
	EIMSK |= (1 << INT0);
	SMCR |= (1 << SM1);
	lcd_init(LCD_DISP_ON);
	lcd_clrscr();
}

/* Sends message/command to slave (UNO) */
void send_message_to_slave(char* message) 
{
	char spi_send_data[STRING_LENGTH];
	strcpy(spi_send_data, message);	
	PORTB &= ~(1 << PB0); // SS LOW
	
	for(int8_t spi_data_index = 0; spi_data_index < STRING_LENGTH; spi_data_index++)
	{
		SPDR = spi_send_data[spi_data_index]; // send byte using SPI data register
		_delay_ms(10);
		while(!(SPSR & (1 << SPIF)))
		{
			/* wait until the transmission is complete */
			;
		}
	}
			
	PORTB |= (1 << PB0); // SS HIGH	
}

ISR(TIMER1_COMPA_vect) 
{
	// counts up the timer 1Hz
	g_timer++;
	// if the timer hits the time limit, trigger the alarm
	if(g_timer == BUZZER_START_TIME) 
	{
		printf("\r\nUNO has turned on the buzzer!\n\r");
		
		lcd_clrscr();
		lcd_puts("TIME HAS RUN");
		lcd_gotoxy(0,1);
		lcd_puts("OUT!");
		_delay_ms(2000);
		
		lcd_clrscr();
		lcd_puts("ALARM TURNED ON!");
		send_message_to_slave("BZR_ON");
		_delay_ms(500);
		
		lcd_clrscr();
		lcd_puts("Enter password:");
		stop_timer1();
	}
}

int main(void)
{
	//Enable interrupts
	sei(); 
	
	// Setting up LCD
	setup_lcd();
	 
	// Redirect the stdin and stdout to UART functions
    stdout = &uart_output;
    stdin = &uart_input;
	
	/* set SS, MOSI and SCK as output, pins 53 (PB0), 51 (PB2) and 52 (PB1) */
	DDRB |= (1 << PB0) | (1 << PB1) | (1 << PB2); // SS as output
	/* set SPI enable and master/slave select, making MEGA the master */
	SPCR |= (1 << 6) | (1 << 4);
	/* set SPI clock rate to 1 MHz */
	SPCR |= (1 << 0);
	
	// Initialize the UART with 9600 BAUD
	USART_Init(MYUBRR);
	
	// Password the user inputs
	char inputPassword[STRING_LENGTH];
	
	// Keypad pressed key
	char pressedKey;
	
	// User input password length
	int pwLength = 0;
	
	/* Motion sensor */
	DDRB &= ~(1 << PB7);
	int8_t motionSensorValue = 0;
	
	/* initialize the keypad */
	KEYPAD_Init();
	
	lcd_clrscr();
	lcd_puts("Starting");
	lcd_gotoxy(0,1);
	lcd_puts("application...");
	_delay_ms(1000);
	
	// Start state machine in ALARM_ARMED state
	int state = ALARM_ARMED; 
	
	while(1)
	{	
		switch(state)
		{
			case ALARM_ARMED:		 //  default when the system is started
				motionSensorValue = 0;
				lcd_clrscr();
				printf("DETECTING MOTION...\n\r");
				lcd_puts("DETECTING");
				lcd_gotoxy(0,1);
				lcd_puts("MOTION...");
				
				// Detecting motion with the motion sensor
				while(1)
				{
					motionSensorValue = (PINB & (1 << PB7));
					_delay_ms(10);
					// If motion sensor value is 1 (motion detected)
					if(motionSensorValue) 
					{
						printf("\nMOTION DETECTED!\n\n\r");
						lcd_clrscr();
						lcd_puts("MOTION DETECTED!");
						_delay_ms(1500);
						lcd_clrscr();
						lcd_puts("Backspace = '*'");
						lcd_gotoxy(0,1);
						lcd_puts("Enter = '#'");
						_delay_ms(3000);
						lcd_clrscr();
						lcd_puts("You have 10");
						lcd_gotoxy(0,1);
						lcd_puts("seconds.");
						_delay_ms(2000);
						state = MOVEMENT_DETECTED;
						break;		// goto "MOVEMENT_DETECTED"
					}		
				}	
				break;
			case MOVEMENT_DETECTED:		// Motion sensor triggered
				// Starts timer
				setup_timer1();
				
				// Resets values
				pwLength = 0;
				*inputPassword = '\0';
				pressedKey = '\0';
				
				printf("Type the password ('#' enter, '*' backspace): \n\r");
				lcd_clrscr();
				lcd_puts("Enter password:");
				while (1)
				{
					// user inputs the password on the keypad					
					pressedKey = KEYPAD_GetKey();
					
					// if the timer hits the time limit, change state to BUZZER_ON
					if(g_timer >= BUZZER_START_TIME) 
					{
						state = BUZZER_ON; // Change state to BUZZER_ON
						break;
					}
					pwLength = strlen(inputPassword);
					
					// Press ENTER to check if password is correct
					if((pressedKey == ENTER) && (strcmp(inputPassword, ""))) 
					{
						if(!strcmp(inputPassword, PASSWORD)) 
						{
							stop_timer1();
							printf("Password correct!\n\r");
							lcd_clrscr();
							lcd_puts("Password");
							lcd_gotoxy(0,1);
							lcd_puts("CORRECT!");
							_delay_ms(2000);
							state = ALARM_DISARMED; // Change state to ALARM_DISARMED
						} 
						if (strcmp(inputPassword, PASSWORD))
						{							
							stop_timer1();
							printf("Password incorrect!\n\r");
							lcd_clrscr();
							lcd_puts("Password");
							lcd_gotoxy(0,1);
							lcd_puts("INCORRECT!");
							_delay_ms(2000);
							lcd_clrscr();
							lcd_puts("ALARM TURNED ON!");
							_delay_ms(500);
							state = BUZZER_ON; // Change state to BUZZER_ON
						}
						// Reset values
						pwLength = 0;
						*inputPassword = '\0';
						pressedKey = '\0';
						break;
					// Removes one character from the input password if there are characters
					} else if ((pressedKey == BACKSPACE) && (pwLength > 0)) 
					{
						inputPassword[pwLength] = '\0';
						inputPassword[pwLength-1] = '\0';
						lcd_clrscr();
						lcd_puts("Enter password:");
					// Accepts only number as password inputs. If the user tries to put more than the password length amount of characters, they're not accepted.
					} else if ((pwLength < CORRECT_PW_LENGTH) && isdigit(pressedKey)) 
					{
						inputPassword[pwLength] = pressedKey;
						inputPassword[pwLength+1] = '\0';					
					} else 
					{
						continue; // If any other button is pressed, uses 'continue' so the character won't be printed at the end
					}	
					printf("%s\n\r", inputPassword);
					lcd_gotoxy(0,1);
					lcd_puts(inputPassword);		
				}
				break;
			
			case BUZZER_ON:		// start the buzzer when the 10 second timer ran out
			
				// Stops and resets timer
				stop_timer1();
				g_timer = 0;
				
				// Tells the slave to turn on the buzzer
				send_message_to_slave("BZR_ON");

				// Reset values
				pwLength = 0;
				*inputPassword = '\0';
				pressedKey = '\0';
				
				printf("Type the password to disable the alarm ('#' enter, '*' backspace): \n\r");
				lcd_clrscr();
				lcd_puts("Enter password:");
				while (1)
				{
					// user inputs the password on the keypad					
					pressedKey = KEYPAD_GetKey();
					pwLength = strlen(inputPassword);
					
					// Press ENTER to check the input password
					if((pressedKey == ENTER) && (strcmp(inputPassword, ""))) 
					{
						if(!strcmp(inputPassword, PASSWORD)) 
						{
							stop_timer1();
							printf("Password correct!\n\r");
							lcd_clrscr();
							lcd_puts("Password");
							lcd_gotoxy(0,1);
							lcd_puts("CORRECT!");
							_delay_ms(2000);
							state = ALARM_DISARMED; // Change state to ALARM_DISARMED
							pwLength = 0;
							*inputPassword = '\0';
							pressedKey = '\0';
							break;
						}
						if (strcmp(inputPassword, PASSWORD)) 
						{
							printf("Password incorrect!\n\r");
							lcd_clrscr();
							lcd_puts("Password");
							lcd_gotoxy(0,1);
							lcd_puts("INCORRECT!");
							pwLength = 0;
							*inputPassword = '\0';
							pressedKey = '\0';
							_delay_ms(2000);
							lcd_clrscr();
							lcd_puts("Enter password:");
						}
					// Removes one character from the input password if there are characters
					} else if ((pressedKey == BACKSPACE) && (pwLength > 0)) 
					{
						inputPassword[pwLength] = '\0';
						inputPassword[pwLength-1] = '\0';
						lcd_clrscr();
						lcd_puts("Enter password:");
					// Accepts only number as password inputs. If the user tries to put more than the password length amount of characters, they're not accepted.
					} else if ((pwLength < CORRECT_PW_LENGTH) && isdigit(pressedKey)) 
					{
						inputPassword[pwLength] = pressedKey;
						inputPassword[pwLength+1] = '\0';
					} else 
					{
						continue; // If any other button is pressed, uses 'continue' so the character won't be printed at the end
					}
					printf("%s\n\r", inputPassword);
					lcd_gotoxy(0,1);
					lcd_puts(inputPassword);
				}
				break;
			case ALARM_DISARMED: // User has input the correct password
			
				// Stops and resets timer
				stop_timer1();
				g_timer = 0;
				
				// Tells the slave to turn off the buzzer
				send_message_to_slave("BZR_OFF");
				
				printf("Alarm has been disarmed.\n\r");
				lcd_clrscr();
				lcd_puts("ALARM DISARMED!");
				_delay_ms(2000);
				
				printf("Press %c to rearm. Press %c to exit.\n\r", REARM_BTN, EXIT_BTN);
				lcd_clrscr();
				lcd_puts("'A' to rearm.");
				lcd_gotoxy(0,1);
				lcd_puts("'B' to exit.");
				// Waits for user to rearm or exit.
				while(1) {
					pressedKey = KEYPAD_GetKey();
					if(pressedKey == REARM_BTN) 
					{
						printf("Rearmed.\n\r");
						lcd_clrscr();
						lcd_puts("Rearming...");
						_delay_ms(2000);
						state = ALARM_ARMED; // Change state to ALARM_ARMED
						break;
					} else if (pressedKey == EXIT_BTN) 
					{
						send_message_to_slave("SLEEP");
						printf("Sleep mode...\n\r");
						lcd_clrscr();
						lcd_puts("Sleep mode...");
						
						// Sleep mode
						SMCR |= (1 << SM1);
						_delay_ms(100);
						SMCR |= (1 << SE);
						sleep_cpu();
						// Requires system reset
					}
				}
				break;
			default:
				printf("ERROR\n"); // Unknown state, should not go here
				break;
		}
	}
}

