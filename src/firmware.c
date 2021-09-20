#include <string.h>
#include <avr/interrupt.h>
#include <avr/io.h>
#include <util/delay.h>

#define BIT(r, b) (r & (1 << b))
#define BIT_ON(r, b) (r |= (1 << b))
#define BIT_OFF(r, b) (r &= ~(1 << b))


static void timer_disable()
{
  TCCR0B = 0;
}

static void timer_enable()
{
  TCCR0B = (1 << CS02); //256us period
}

static void pwm_enable(unsigned char duty)
{
  BIT_OFF(PORTB, PB4);
  TCCR1 = (1 << CS12);  //16us period 
  GTCCR = (1 << PWM1B) | (1 << COM1B1);
  OCR1B = duty;
}

static void init()
{
  BIT_OFF(MCUCR, PUD);
  BIT_ON(DDRB, PB4);
  //BIT_ON(PORTB, PB4);

  BIT_OFF(DDRB, PB3);

  BIT_ON(GIMSK, PCIE); //enable Pin Change Interrupt
  BIT_ON(PCMSK, PCINT3);

  GTCCR = 0;
  TCCR0A = 0;

  timer_enable();
  sei();
}     

volatile uint64_t code = 0;
volatile uint64_t last_code = 0;
volatile uint8_t ignore = 0;

static void uart_send(unsigned char b)
{
#define D 98
  BIT_OFF(PORTB, PB4);
  _delay_us(D + 2);  
  for(unsigned char i = 0; i < 8; ++i)
  {
    unsigned char v = (b & 1) << PB4;
    PORTB = v;
    b >>= 1;
    _delay_us(D);
  }
  BIT_ON(PORTB, PB4);
  _delay_us(D);
}

ISR(PCINT0_vect)
{
  timer_disable();
  if(BIT(TIFR, TOV0))
  {
    BIT_ON(TIFR, TOV0);
    last_code = code;
    code = 0;
    ignore = 0;
  }
  else
  {
    if(TCNT0 < 5)
    {
      if(!ignore)
      {
        code <<= 1;
        ignore = 1;
      }
      else
        ignore = 0;
    }
    else if(TCNT0 < 10)
    {
      code = (code << 2) | 1;
      ignore = 0;
    }
    else
    {
      last_code = code;
      code = 0;
      ignore = 0;
    }
  }
  TCNT0 = 0;    
  timer_enable();
}

void uart_send_hex(uint8_t v)
{
  v &= 0xf;
  if(v < 10)
    uart_send(v + '0');
  else
    uart_send(v + 'A' - 10);
}

#define MIN_DUTY 5
#define MAX_DUTY 230
#define FAST_THRESHOLD 100

void __attribute__((noreturn)) main()
{    
  init();
  uint8_t light_on = 1;
  uint8_t duty = 255;
  pwm_enable(duty);
  for(;;)
  {
    if(light_on && duty < 255)
    {
      ++duty;
      if(duty < MIN_DUTY)
        duty = MIN_DUTY;
      if(duty > MAX_DUTY)
        duty = 255;
      pwm_enable(duty);
      if(duty > 128)
        sei();
    }
    else if(!light_on && duty > 0)
    {
      --duty;
      if(duty > MAX_DUTY)
        duty = MAX_DUTY;
      if(duty < MIN_DUTY)
        duty = 0;
      pwm_enable(duty);
      if(duty < 128)
        sei();
    }
    else
      sei();
    
    if((uint32_t)last_code == 0x41089224)
    {
       cli();
       light_on = !light_on;
       last_code = 0;
    }
    _delay_ms(4);
/*
    if(last_code)
    {
      cli();
      for(int i = 0; i < 16; ++i)
      {
        uart_send_hex(last_code >> 60);
        last_code <<= 4;
      }
      uart_send('\r');
      uart_send('\n');
      sei();
    }*/
  }
}

