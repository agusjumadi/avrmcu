#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define USART_BAUDRATE 9600
#define UBRR_VALUE (((F_CPU/(USART_BAUDRATE*16UL)))-1)

#define bitSet(value, bit) ((value) |= (1UL << (bit)))
#define bitClear(value, bit) ((value) &= ~(1UL << (bit)))

volatile uint16_t nec_ok = 0, t_lastval = 0;

volatile uint8_t  i, nec_state = 0, command, inv_command;
unsigned int address;
unsigned long nec_code;

void serial_init(){
	// initialize USART
	UBRRL = UBRR_VALUE & 255; 
	UBRRH = UBRR_VALUE >> 8;
	UCSRB = (1 << TXEN)|(1<<RXEN); // fire-up USART
	//UCSRB |=(1<<RXCIE);
	UCSRC = (1 << UCSZ1) | (1 << UCSZ0); // fire-up USART
	//UCSRC = (1 << USBS) | (3 << UCSZ0);	// asynchron 8n1
}

void serial_send(uint8_t data){
	// send a single character via USART
	while(!(UCSRA&(1<<UDRE))){}; //wait while previous byte is completed
	UDR = data; // Transmit data
}

void serial_sendstring(const char *str)					/* Send string of USART data function */
{
	int i=0;
	while (str[i]!=0)
	{
		serial_send(str[i]);						/* Send each char of string till the NULL */
		i++;
	}
}
void serial_puts(char *str, int ln)					/* Send string of USART data function */
{
	int i=0;
	for (i=0;i<ln;i++)
	{
		serial_send(str[i]);						/* Send each char of string till the NULL */
		//i++;
	}
}
uint8_t serial_receive(void)
{
	while((UCSRA &(1<<RXC)) == 0);
	return UDR;
}
void serial_break(){
	serial_send(10); // new line 
	serial_send(13); // carriage return
}

void serial_number(long val){
	// send a number as ASCII text
	long divby=100000000; // change by dataType
	while (divby>=1){
		serial_send('0'+val/divby);
		val-=(val/divby)*divby;
		divby/=10;
	}
}
 
void remote_read() {
  unsigned int timer_value;
  //PORTB ^=~(1<<PB5);
  if(nec_state != 0){
    timer_value = TCNT1;                         // Store Timer1 value
    TCNT1 = 0;                                   // Reset Timer1
    t_lastval = timer_value;
    //serial_number((long)timer_value);serial_send('\n');
  }
  switch(nec_state){
   case 0 :                                      // Start receiving IR data (we're at the beginning of 9ms pulse)
    TCNT1  = 0;                                  // Reset Timer1
    TCCR1B = 2;                                  // Enable Timer1 module with 1/8 prescaler, F_CPU 8MHz ( 1 ticks every 1 us)
    nec_state = 1;                               // Next state: end of 9ms pulse (start of 4.5ms space)
    i = 0;
    //serial_sendstring("Start init\r\n");
    //serial_number((long)timer_value);serial_send('\n');
    return;
   case 1 :                                      // End of 9ms pulse
    if((timer_value > 9800) || (timer_value < 8500)){         // Invalid interval ==> stop decoding and reset
      nec_state = 0;                             // Reset decoding process
      TCCR1B = 0;                                // Disable Timer1 module
      serial_number((long)timer_value);serial_sendstring(" fail state 1\r\n");
    }
    else
      nec_state = 2;                             // Next state: end of 4.5ms space (start of 562µs pulse)
        
    return;
   case 2 :                                      // End of 4.5ms space
    if((timer_value > 5000) || (timer_value < 4000)){
      nec_state = 0;                             // Reset decoding process
      TCCR1B = 0;                                // Disable Timer1 module
      serial_number((long)timer_value);serial_sendstring(" fail state 2\r\n");
    }
    else
      nec_state = 3;                             // Next state: end of 562µs pulse (start of 562µs or 1687µs space)
    return;
   case 3 :                                      // End of 562µs pulse
    if((timer_value > 700) || (timer_value < 400)){           // Invalid interval ==> stop decoding and reset
      TCCR1B = 0;                                // Disable Timer1 module
      nec_state = 0;                             // Reset decoding process
      serial_number((long)timer_value);serial_sendstring(" fail state 3\r\n");
    }
    else
      nec_state = 4;                             // Next state: end of 562µs or 1687µs space
    return;
   case 4 :                                      // End of 562µs or 1687µs space
    if((timer_value > 1800) || (timer_value < 400)){           // Time interval invalid ==> stop decoding
      TCCR1B = 0;                                // Disable Timer1 module
      nec_state = 0;                             // Reset decoding process
      serial_number((long)timer_value);serial_sendstring(" fail state 4\r\n");
      return;
    }
    if( timer_value > 1000)                      // If space width > 1ms (short space)
      bitSet(nec_code, (31 - i));                // Write 1 to bit (31 - i)
    else                                         // If space width < 1ms (long space)
      bitClear(nec_code, (31 - i));              // Write 0 to bit (31 - i)
    i++;
    if(i > 31){                                  // If all bits are received
      nec_ok = 1;                                // Decoding process OK
      //detachInterrupt(0);                        // Disable external interrupt (INT0)
      GIMSK &= ~(1<<INT0);
      return;
    }
    nec_state = 3;                               // Next state: end of 562µs pulse (start of 562µs or 1687µs space)
  }
}
 
ISR(TIMER1_OVF_vect) {                           // Timer1 interrupt service routine (ISR)
    TCCR1B = 0;                                    // Disable Timer1 module
    //serial_number((long)t_lastval);serial_sendstring("\r\n");
    //serial_number((long)nec_state);serial_sendstring("\r\n");
    nec_state = 0;                                 // Reset decoding process
    
    
  
}

void blink()
{
  // ON when Low
  PORTB &=~(1<<PB7);
  _delay_ms(500);
  PORTB |=(1<<PB7);
}
void loop() {
  if(nec_ok){                                    // If the mcu receives NEC message with successful
    nec_ok = 0;                                  // Reset decoding process
    nec_state = 0;
    TCCR1B = 0;                                  // Disable Timer1 module
    address = nec_code >> 16;
    command = nec_code >> 8;
    inv_command = nec_code;
    blink();
    //serial_number(command);serial_send(' ');serial_number(inv_command);
    //ON -> signal LOW
    if(command == 162 && inv_command == 93)
    {
        serial_sendstring("OFF\r\n");
        PORTB |=(1<<PB5);
        PORTB |=(1<<PB6);
    }
    else if(command == 48 && inv_command == 207)
    {
        serial_sendstring("ON 1\r\n");
        PORTB &=~(1<<PB5);
        PORTB |=(1<<PB6);
    }
    else if(command == 24 && inv_command == 231)
    {
        serial_sendstring("ON 2\r\n");
        PORTB &=~(1<<PB5);
        PORTB &=~(1<<PB6);
    }
    
    GIMSK = 1<<INT0;
  }
}

ISR(INT0_vect)
{
  PORTB &=~(1<<PB7);
  remote_read();
  PORTB |=(1<<PB7);
}

int main(void)
{
  DDRB = 255; //output all
  PORTB = 255; //high all

  PORTB &=~(1<<PB7);//pb7
  _delay_ms(500);
  PORTB |=(1<<PB7);
  _delay_ms(500);
  PORTB &=~(1<<PB7);
  _delay_ms(500);
  PORTB |=(1<<PB7);
  _delay_ms(500);
  PORTB &=~(1<<PB7);
  _delay_ms(500);
  PORTB |=(1<<PB7);

  serial_init();
  // Timer1 module configuration
  TCCR1A = 0;
  TCCR1B = 0;                                    // Disable Timer1 module
  TCNT1  = 0;                                    // Set Timer1 preload value to 0 (reset)
  TIMSK = 0x80;                                    // enable Timer1 overflow interrupt

  // Enable external interrupt (INT0)
  MCUCR &= ~_BV(ISC01); // trigger INTO interrupt on raising
  MCUCR |= _BV(ISC00); // and falling edge
  GIMSK = 1 << INT0;
  //MCUCR = 1<<ISC01 | 1<<ISC00;
  sei(); // enable global interrupts
  serial_sendstring("Start IR...\r\n");
  while(1)
  {
      loop();
  }
  return 0;
}