/*
 * Slave.c
 *
 * Created: 17/04/2024
 * Author : Group_14
 * Used examples: Exercise 10, Exercise 7
 * used pins: Buzzer (GNR, 9 "PB1" --> Buzzer) USART = (10, 11, 12, 13, GNR --> Mega)
 */ 

/* ATmega328p "UNO" */

#define F_CPU 16000000UL
#define FOSC 16000000UL // Clock Speed
#define BAUD 9600
#define MYUBRR (FOSC/16/BAUD-1)

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <stdlib.h> // turha?
#include <string.h>

static void USART_init(uint16_t ubrr) // unsigned int
{
    /* Set baud rate in the USART Baud Rate Registers (UBRR) */
    UBRR0H = (unsigned char) (ubrr >> 8);
    UBRR0L = (unsigned char) ubrr;
    
    /* Enable receiver and transmitter on RX0 and TX0 */
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0); //NOTE: the ATmega328p has 1 UART: 0
    
    /* Set frame format: 8 bit data, 2 stop bit */
    UCSR0C |= (1 << USBS0) | (3 << UCSZ00);
    
}

/* functions for the timer/counter1 compare match A interrupt vector */
ISR (TIMER1_COMPA_vect)
{
    TCNT1 = 0; // reset timer counter
}

static void USART_Transmit(unsigned char data, FILE *stream)
{
    /* Wait until the transmit buffer is empty*/
    while(!(UCSR0A & (1 << UDRE0))) {;}
    
    /* Put the data into a buffer, then send/transmit the data */
    UDR0 = data;
}

static char USART_Receive(FILE *stream)
{
    /* Wait until the transmit buffer is empty - Wait for data to be received */
    while(!(UCSR0A & (1 << UDRE0))) {;}
    
    /* Get the received data from the buffer */
    return UDR0;
}

// Setup the stream functions for UART
FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);

int main(void)
{
    
    /*-----------Buzzer--------------*/
    /* set up the ports and pins */
    DDRB |= (1 << PB1); // OC1A is located in digital pin 9
    
    // Enable interrupts
    sei();
    
    /* set up the 16-bit timer/counter1, mode 9 used */
    TCCR1B  = 0; // reset timer/counter 1
    TCNT1   = 0;
    TCCR1A |= (1 << 6); // set compare output mode to toggle
    // TCCR1A |= 0b01000000;
    // TCCR1A |= 0x40;
    
    // mode 9 phase correct
    TCCR1A |= (1 << 0); // set register A WGM[1:0] bits
    // TCCR1A |= 0b00000001;
    TCCR1B |= (1 << 4); // set register B WBM[3:2] bits
    // TCCR1B |= 0b00010000;
    
    TIMSK1 |= (1 << 1); // enable compare match A interrupt
    // TIMSK1 |= 0b00000100;
    
    OCR1A = 15297; // C5 523 Hz, no prescaler
    //OCR1A = 2462;   // A7 3250 Hz, no prescaler, calculated
    //OCR1A = 2440;   // A7 3250 Hz, no prescaler, empirical
    //OCR1A = 1016;   // B2  123 Hz, 64 prescaler
    
    /*---------------------------------*/
    
    
    /* set MISO as output, pin 12 (PB4)*/
    DDRB  = (1 << PB4);
    /* set SPI enable */
    SPCR  = (1 << 6);
    
    // initialize the UART with 9600 BAUD
    USART_init(MYUBRR);
    
    // redirect the stdin and stdout to UART functions
    stdout = &uart_output;
    stdin = &uart_input;
    
    char spi_send_data[20] = "slave to master\n";
    char spi_receive_data[20];
    
    /* send message to master and receive message from master */
    while (1)
    {
        
        for(int8_t spi_data_index = 0; spi_data_index < sizeof(spi_send_data); spi_data_index++)
        {
            
            SPDR = spi_send_data[spi_data_index]; // send byte using SPI data register
            
            while(!(SPSR & (1 << SPIF)))
            {
                /* wait until the transmission is complete */
                ;
            }
            spi_receive_data[spi_data_index] = SPDR; // receive byte from the SPI data register

        }
        
        printf(spi_receive_data);
        if (!strcmp(spi_receive_data,("1")))
        {
            TCCR1B |= (1 << 0); // set prescaling to 1 (no prescaling)
        }else{
            TCCR1B |= (0 << 0); // Buzzer off???
        }
    }
    
    return 0;
}
