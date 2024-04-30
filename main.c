/*
 * final_project_master.c
 * 
 *		BL40A1812 Introduction to Embedded Systems
 *		final project - master side
 *
 * Created: 3.4.2024 12.24.39
 * Author : Group 14
 */ 


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
int g_timer = 0; // Used in ISR to count time after movement is detected.

/* keypad defines */
#define RowColDirection DDRB	// Data Direction Configuration for keypad
#define ROW PORTB				// Lower four bits of PORTC are used as ROWs
#define COL PINB				// Higher four bits of PORTC are used as COLs

#define PASSWORD "1234"
#define CORRECT_PW_LENGTH strlen(PASSWORD)
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
#include "keypad/keypad.h"
#include "keypad/delay.h"
#include "LCD/lcd.h"


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

// Assign stdout and stdin as USART
FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);

// Source: http://www.arduinoslovakia.eu/application/timer-calculator
void SetupTimer1() {
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

void StopTimer1() {
	// Clear prescaler bits to stop the timer
	TCCR1B &= ~((1 << CS12) | (1 << CS10));
}

ISR(TIMER1_COMPA_vect) {
	g_timer++;
	if(g_timer == BUZZER_START_TIME) {
		printf("\r\nUNO has turned on the buzzer!\n\r");
		
		lcd_clrscr();
		lcd_puts("TIME HAS RUN");
		lcd_gotoxy(0,1);
		lcd_puts("OUT!");
		_delay_ms(2000);
		
		lcd_clrscr();
		lcd_puts("ALARM TURNED ON!");
		_delay_ms(1000);
		
		lcd_clrscr();
		lcd_puts("Enter password:");
		StopTimer1();
	}
}

void SetupLCD() {
	EICRA |= (1 << ISC01);
	EIMSK |= (1 << INT0);
	SMCR |= (1 << SM1);
	lcd_init(LCD_DISP_ON);
	lcd_clrscr();
}

int main(void)
{
	//Enable interrupts
	sei(); 
	
	// Setting up LCD
	SetupLCD();
	 
	// Initialize the UART with 9600 BAUD
	USART_Init(MYUBRR);
    
	// Redirect the stdin and stdout to UART functions
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
	
	// Password the user inputs
	char inputPassword[20];
	
	// Keypad pressed key
	char pressedKey;
	
	// User input password length
	int pwLength = 0;
	
	/* Motion sensor */
	DDRG &= ~(1 << PG0);
	int8_t motion_sensor_value = 0;
	
	/* initialize the keypad */
	KEYPAD_Init();
	
	lcd_clrscr();
	lcd_puts("Starting");
	lcd_gotoxy(0,1);
	lcd_puts("application...");
	_delay_ms(2000);
	
	// Start state machine in ALARM_ARMED state
	int STATE = ALARM_ARMED; 
	
	while(1)
	{	
		switch(STATE)
		{
			case ALARM_ARMED:		 //  default when the system is started
			
				lcd_clrscr();
				printf("DETECTING MOTION...\n\r");
				lcd_puts("DETECTING");
				lcd_gotoxy(0,1);
				lcd_puts("MOTION...");
				
				// Detecting motion with the motion sensor
				while(1)
				{
					motion_sensor_value = (PING & (1 << PG0));
					_delay_ms(10);
					if(motion_sensor_value) {
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
						STATE = MOVEMENT_DETECTED;
						break;		// goto "MOVEMENT_DETECTED"
					}		
				}	
				break;
			case MOVEMENT_DETECTED:		// Motion sensor triggered
				// password is hardcoded e.g. 1234
				
				// Starts timer
				SetupTimer1();
				
				// Resets values
				pwLength = 0;
				*inputPassword = '\0';
				pressedKey = NULL;
				
				printf("Type the password ('#' enter, '*' backspace): \n\r");
				lcd_clrscr();
				lcd_puts("Enter password:");
				while (1)
				{
					// user inputs the password on the keypad					
					pressedKey = KEYPAD_GetKey();
					
					// If the timer has ran out
					if(g_timer >= BUZZER_START_TIME) {
						STATE = BUZZER_ON;
						break;
					}
					pwLength = strlen(inputPassword);
					
					// Press ENTER to check if password is correct
					if(pressedKey == ENTER) {
						if(!strcmp(inputPassword, PASSWORD)) {
							printf("Password correct!\n\r");
							lcd_clrscr();
							lcd_puts("Password");
							lcd_gotoxy(0,1);
							lcd_puts("CORRECT!");
							_delay_ms(2000);
							STATE = ALARM_DISARMED;
						} 
						if (strcmp(inputPassword, PASSWORD)){
							printf("Password incorrect!\n\r");
							lcd_clrscr();
							lcd_puts("Password");
							lcd_gotoxy(0,1);
							lcd_puts("INCORRECT!");
							_delay_ms(2000);
							lcd_clrscr();
							lcd_puts("ALARM TURNED ON!");
							_delay_ms(2000);
							STATE = BUZZER_ON;
						}
						// Reset values
						pwLength = 0;
						*inputPassword = '\0';
						pressedKey = NULL;
						break;
					// Removes one character from the input password
					} else if ((pressedKey == BACKSPACE) && (pwLength > 0)) {
						inputPassword[pwLength] = NULL;
						inputPassword[pwLength-1] = '\0';
						lcd_clrscr();
						lcd_puts("Enter password:");
					} else if ((pwLength < CORRECT_PW_LENGTH) && isdigit(pressedKey)) {
						inputPassword[pwLength] = pressedKey;
						inputPassword[pwLength+1] = '\0';					
					} else {
						continue;
					}	
					printf("%s\n\r", inputPassword);
					lcd_gotoxy(0,1);
					lcd_puts(inputPassword);		
				}
				break;
			
			case BUZZER_ON:		// start the buzzer when the 10 second timer ran out
				// Stops and resets timer
				StopTimer1();
				g_timer = 0;
				
				// Reset values
				pwLength = 0;
				*inputPassword = '\0';
				pressedKey = NULL;
				
				printf("Type the password to disable the alarm ('#' enter, '*' backspace): \n\r");
				lcd_clrscr();
				lcd_puts("Enter password:");
				while (1)
				{
					// user inputs the password on the keypad					
					pressedKey = KEYPAD_GetKey();
					pwLength = strlen(inputPassword);
					
					// Press ENTER to check the input password
					if(pressedKey == ENTER) {
						if(!strcmp(inputPassword, PASSWORD)) {
							printf("Password correct!\n\r");
							lcd_clrscr();
							lcd_puts("Password");
							lcd_gotoxy(0,1);
							lcd_puts("CORRECT!");
							_delay_ms(2000);
							STATE = ALARM_DISARMED;
							pwLength = 0;
							*inputPassword = '\0';
							pressedKey = NULL;
							break;
						}
						if (strcmp(inputPassword, PASSWORD)) {
							printf("Password incorrect!\n\r");
							lcd_clrscr();
							lcd_puts("Password");
							lcd_gotoxy(0,1);
							lcd_puts("INCORRECT!");
							pwLength = 0;
							*inputPassword = '\0';
							pressedKey = NULL;
							_delay_ms(2000);
							lcd_clrscr();
							lcd_puts("Enter password:");
						}
					// Removes one character from the input password
					} else if ((pressedKey == BACKSPACE) && (pwLength > 0)) {
						inputPassword[pwLength] = NULL;
						inputPassword[pwLength-1] = '\0';
						lcd_clrscr();
						lcd_puts("Enter password:");
					} else if ((pwLength < CORRECT_PW_LENGTH) && isdigit(pressedKey)) {
						inputPassword[pwLength] = pressedKey;
						inputPassword[pwLength+1] = '\0';
					} else {
						continue;
					}
					printf("%s\n\r", inputPassword);
					lcd_gotoxy(0,1);
					lcd_puts(inputPassword);
				}
				break;
			case ALARM_DISARMED: // User has input the correct password
				// Stops and resets timer
				StopTimer1();
				g_timer = 0;
				
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
					if(pressedKey == REARM_BTN) {
						printf("Rearmed.\n\r");
						lcd_clrscr();
						lcd_puts("Rearming...");
						_delay_ms(2000);
						STATE = ALARM_ARMED;
						break;
					} else if (pressedKey == EXIT_BTN) {
						printf("Exiting...\n\r");
						lcd_clrscr();
						lcd_puts("Exiting...");
						exit(0);
					}
				}
				break;
			default:
				printf("ERROR\n");
				break;
		}
	}
}

