/*
 * uno_SPI_slave_basics.c
 *
 * Created: 4.7.2019 8:50:10
 * Author : Glutex
 */ 

/* ATmega328p (UNO) is slave */

#define F_CPU 16000000UL
#define FOSC 16000000UL // Clock Speed
#define BAUD 9600
#define MYUBRR (FOSC/16/BAUD-1)

#include <avr/io.h>
#include <util/delay.h>
#include <util/setbaud.h>
#include <stdio.h>
#include <avr/interrupt.h>
#include <util/delay.h>

/* functions for the timer/counter1 compare match A interrupt vector */
ISR
(TIMER1_COMPA_vect)
{
    TCNT1 = 0; // reset timer counter
}

int caltop(int f, int n) // lasketaan top - sivu 128 kaava
{
    long f_cpu = F_CPU;
    return ((f_cpu)/(2*n*f));
}

#define BUZZER_OFF 1
#define BUZZER_ON 2

static void buzzerSong()
{
    TCCR1A |= (1 << 6);
    // D3
    OCR1A = caltop(100, 1);
    _delay_ms(10000);
    OCR1A = caltop(147, 1);
    _delay_ms(10000);
    printf("First delay");
    _delay_ms(1000);
    printf("Second delay");
    
    // C3 SHARP
    printf("C3 sharp");
    OCR1A = caltop(139, 1);

    _delay_ms(1000);
    // C3
    OCR1A = caltop(130, 1);
    _delay_ms(1000);
    _delay_ms(10000);
}

static void
USART_init(uint16_t ubrr) // unsigned int
{
    /* Set baud rate in the USART Baud Rate Registers (UBRR) */
    UBRR0H = (unsigned char) (ubrr >> 8);
    UBRR0L = (unsigned char) ubrr;
    
    /* Enable receiver and transmitter on RX0 and TX0 */
    UCSR0B |= (1 << RXEN0) | (1 << TXEN0); //NOTE: the ATmega328p has 1 UART: 0
    
    /* Set frame format: 8 bit data, 2 stop bit */
    UCSR0C |= (1 << USBS0) | (3 << UCSZ00);
    
}

static void
USART_Transmit(unsigned char data, FILE *stream)
{
    /* Wait until the transmit buffer is empty*/
    while(!(UCSR0A & (1 << UDRE0)))
    {
        ;
    }
    
    /* Put the data into a buffer, then send/transmit the data */
    UDR0 = data;
}

static char
USART_Receive(FILE *stream)
{
    /* Wait until the transmit buffer is empty*/
    while(!(UCSR0A & (1 << UDRE0)))
    {
        ;
    }
    
    /* Get the received data from the buffer */
    return UDR0;
}

// Setup the stream functions for UART
FILE uart_output = FDEV_SETUP_STREAM(USART_Transmit, NULL, _FDEV_SETUP_WRITE);
FILE uart_input = FDEV_SETUP_STREAM(NULL, USART_Receive, _FDEV_SETUP_READ);

int 
main(void)
{

    int STATE = 1; 
    /* set MISO as output, pin 12 (PB4)*/
    DDRB  = (1 << PB4);
    /* set SPI enable */
    SPCR  = (1 << 6);
    
    DDRB |= (1 << PB1); // OC1A is located in digital pin 9
    
    // Enable interrupts
    sei();
    
    /* set up the 16-bit timer/counter1, mode 9 used */
    TCCR1B  = 0; // reset timer/counter 1
    TCNT1   = 0;
    TCCR1A |= (1 << 6); // set compare output mode to toggle
        
    // mode 9 phase correct
    TCCR1A |= (1 << 0); // set register A WGM[1:0] bits
    TCCR1B |= (1 << 4); // set register B WBM[3:2] bits
        
    TIMSK1 |= (1 << 1); // enable compare match A interrupt
        
    OCR1A = 15297; // C5 523 Hz, no prescaler
    TCCR1B |= (1 << 0); // set prescaling to 1 (no prescaling)
    
    TCCR1A &= ~(1 << 6);
    
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
        printf("Working program hopefully\n");
        
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
        printf("\n");
        switch(STATE) {
            case BUZZER_OFF :
                //STATE = BUZZER_ON;
                 _delay_ms(1000);
                  if (strstr(spi_receive_data,("ON")))
                  {
                      // ATmega datasheet page 131 Table 16.1
                      printf("Buzzer turning on\n");
                      TCCR1A |= (1 << 6);
                      OCR1A = caltop(100, 1);
                       STATE = BUZZER_ON;
                  }
                  break;
            case BUZZER_ON:
          
             //STATE = BUZZER_OFF;
             if (strstr(spi_receive_data,("OFF")))
                   {
                       // ATmega datasheet page 131 Table 16.1 
                       TCCR1A &= ~(1 << 6);
                       printf("Buzzer turning off\n");
                       STATE = BUZZER_OFF;
                   }
                   break;
        }
    }      
    return 0;
}

