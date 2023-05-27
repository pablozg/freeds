/*****************************************************
This program was produced by the
CodeWizardAVR V2.04.4a Advanced
Automatic Program Generator
© Copyright 1998-2009 Pavel Haiduc, HP InfoTech s.r.l.
http://www.hpinfotech.com

Project : 
Version : 
Date    : 12.05.2021
Author  : KRIDA Electronics / Olegs Bugrovs
Company : 
Comments: 


Chip type               : ATmega328P
Program type            : Application
AVR Core Clock frequency: 16,000000 MHz
Memory model            : Small
External RAM size       : 0
Data Stack size         : 512


CKSEL0 - 
CKSEL1 - 
CKSEL2 -
CKSEL3 - 
SUT0   - \/ 
SUT1   -

Program size: 1353 words (2706 bytes), 8,3% of FLASH

FIRMWARE V 2.0            12 MAY 2021

*****************************************************/

#include <mega328p.h>
#include <ds18b20.h>     // in this library should be removed delay in 750ms
#include <delay.h>

// 1 Wire Bus functions
#asm
   .equ __w1_port=0x05 ;PORTB
   .equ __w1_bit=5
#endasm
#include <1wire.h>


#define LED_33 PINB.3
#define LED_50 PINB.1
#define LED_50HZ PORTD.6
#define LED_60HZ PORTD.7
#define LED_OVERHEATING PORTC.0
#define LED_FAN PORTB.2
#define FAN PORTD.3
#define GATE PORTD.4

#define MAX_DS18b20 8
#define BUTTON PINC.5

#define ADC_VREF_TYPE 0x20


unsigned char ds18b20_devices;
unsigned char STATUS, PROTECT;
unsigned char blink_cn;
unsigned char TEMP_READ_FLAG;
unsigned char TEMP_COUNT;
unsigned char F;
unsigned char delay;
unsigned char LED_COUNT;


unsigned char adc_count;
unsigned int TIMER_1_DELAY, TIMER_1_BUF_DELAY, ;
unsigned int TIMER_1_IMPULSE, TIMER_1_BUF_IMPULSE, ;
unsigned int VARIABLE;
unsigned int adc_symm;
unsigned char adc_data_BUF;
unsigned char DATA_READY;
unsigned char adc_data;
unsigned char TRIAC;

unsigned int frequency;

int temp0;

unsigned char ds18b20_rom_codes[MAX_DS18b20][9];


// External Interrupt 0 service routine
interrupt [EXT_INT0] void ext_int0_isr(void)
{
 if (TRIAC==1 && PROTECT==0)
 {
  TIMER_1_DELAY = TIMER_1_BUF_DELAY; 
  
  TCNT1H=TIMER_1_DELAY >> 8;
  TCNT1L=TIMER_1_DELAY & 0xff;
  
  TCCR1B=0x02;
 }
 
 frequency=TCNT0;  
 
 TCNT0=0x00;  
 
 if (frequency>149 && frequency<163)  { LED_50HZ=1; 
                                        LED_60HZ=0; 
                                        STATUS=1;
                                        F=100;     } 
                                    
 
 if (frequency>122 && frequency<136)  { LED_50HZ=0; 
                                        LED_60HZ=1;
                                        STATUS=1;
                                        F=83;       }                            
}

// Timer 0 overflow interrupt service routine
interrupt [TIM0_OVF] void timer0_ovf_isr(void)
{
 TCNT0=0x00;

 STATUS=0;
 
 TCCR1B=0x00; 

 blink_cn++;  
                         
 if(blink_cn==10) { LED_50HZ=0; LED_60HZ=0;}
 if(blink_cn==20) { blink_cn=0; LED_50HZ=1; LED_60HZ=1;}

}

// Timer1 overflow interrupt service routine
interrupt [TIM1_OVF] void timer1_ovf_isr(void)
{
TIMER_1_IMPULSE = TIMER_1_BUF_IMPULSE; 

TCNT1H=TIMER_1_IMPULSE >> 8;
TCNT1L=TIMER_1_IMPULSE & 0xff;
// Place your code here

GATE=1;

delay++;

 if (delay==2)
 {
  GATE=0;
  delay=0;
  TCCR1B=0x00;
 }
}

// Timer2 overflow interrupt service routine
interrupt [TIM2_OVF] void timer2_ovf_isr(void)
{
  TCNT2=0x63; 
  
    TEMP_COUNT++;
    LED_COUNT++;   
    
    if (LED_COUNT==25 && PROTECT==1)
    {
     LED_OVERHEATING=~LED_OVERHEATING;     
     LED_COUNT=0;
    }
    
  
    if (TEMP_COUNT==150)
    {       
     TEMP_READ_FLAG=1;
    }   
    
    
}


// ADC interrupt service routine
interrupt [ADC_INT] void adc_isr(void)
{
   adc_symm+=ADCH;

   adc_count++;

if (adc_count==200)
 {
  adc_count=0;
  adc_data=adc_symm/200;  
  
  if (LED_33==0) 
  {
   if (adc_data>168) {adc_data=168;}
   adc_data=adc_data*1.51;
  }
    
  adc_data=255-adc_data;
  adc_symm=0;  
  
  
  if (adc_data<235)  //  origin value 235      
   {
    TRIAC=1;
    if (adc_data<15)  { adc_data=15;}       // origin value 15
   }
    
  if (adc_data>=235  )   // origin value 235
   {    
    TRIAC=0;  
    TCCR1B=0x00;  
    GATE=0;      
   } 
   
   DATA_READY=1;      
 }
 
  ADCSRA|=0x40;

}

// Declare your global variables here

void main(void)
{
// Declare your local variables here

// Crystal Oscillator division factor: 1
#pragma optsize-
CLKPR=0x80;
CLKPR=0x00;
#ifdef _OPTIMIZE_SIZE_
#pragma optsize+
#endif

// Input/Output Ports initialization
// Port B initialization
// Func7=In Func6=In Func5=In Func4=In Func3=Out Func2=Out Func1=Out Func0=In 
// State7=T State6=T State5=T State4=T State3=1 State2=1 State1=1 State0=T 
PORTB=0x0E;
DDRB=0x04;

// Port C initialization
// Func6=In Func5=In Func4=In Func3=In Func2=In Func1=In Func0=Out 
// State6=T State5=P State4=T State3=T State2=T State1=T State0=1 
PORTC=0x21;
DDRC=0x01;

// Port D initialization
// Func7=Out Func6=Out Func5=In Func4=Out Func3=Out Func2=In Func1=In Func0=In 
// State7=0 State6=0 State5=T State4=0 State3=0 State2=T State1=T State0=T 
PORTD=0x00;
DDRD=0xD8;

// Timer/Counter 0 initialization
// Clock source: System Clock
// Clock value: 15,625 kHz
// Mode: Normal top=FFh
// OC0A output: Disconnected
// OC0B output: Disconnected
TCCR0A=0x00;
TCCR0B=0x05;
TCNT0=0x00;
OCR0A=0x00;
OCR0B=0x00;

// Timer/Counter 1 initialization
// Clock source: System Clock
// Clock value: 2 MHZ
// Mode: Normal top=FFFFh
// OC1A output: Discon.
// OC1B output: Discon.
// Noise Canceler: Off
// Input Capture on Falling Edge
// Timer1 Overflow Interrupt: On
// Input Capture Interrupt: Off
// Compare A Match Interrupt: Off
// Compare B Match Interrupt: Off
TCCR1A=0x00;
TCCR1B=0x00;  // 0x02 PRESCALLER 8, CLOCK 2 MHZ
TCNT1H=0x00;
TCNT1L=0x55;
ICR1H=0x00;
ICR1L=0x00;
OCR1AH=0x00;
OCR1AL=0x00;
OCR1BH=0x00;
OCR1BL=0x00;

// Timer/Counter 2 initialization
// Clock source: System Clock
// Clock value: 15,625 kHz
// Mode: Normal top=FFh
// OC2A output: Disconnected
// OC2B output: Disconnected
ASSR=0x00;
TCCR2A=0x00;
TCCR2B=0x07;
TCNT2=0x63;      
OCR2A=0x00;
OCR2B=0x00;


// External Interrupt(s) initialization
// INT0: On
// INT0 Mode: Falling Edge
// INT1: Off
// Interrupt on any change on pins PCINT0-7: Off
// Interrupt on any change on pins PCINT8-14: Off
// Interrupt on any change on pins PCINT16-23: Off
EICRA=0x02;
EIMSK=0x01;
EIFR=0x01;
PCICR=0x00;

// Timer/Counter 0 Interrupt(s) initialization
TIMSK0=0x01;
// Timer/Counter 1 Interrupt(s) initialization
TIMSK1=0x01;
// Timer/Counter 2 Interrupt(s) initialization
TIMSK2=0x01;

// Analog Comparator initialization
// Analog Comparator: Off
// Analog Comparator Input Capture by Timer/Counter 1: Off
ACSR=0x80;
ADCSRB=0x00;

// ADC initialization
// ADC Clock frequency: 250,000 kHz
// ADC Voltage Reference: AREF pin
// ADC Auto Trigger Source: Free Running
// Only the 8 most significant bits of
// the AD conversion result are used
// Digital input buffers on ADC0: Off, ADC1: On, ADC2: Off, ADC3: Off
// ADC4: Off, ADC5: Off
DIDR0=0x00;
ADMUX=0b00100001;
ADCSRA=0x8F;

ADCSRA|=0x40;

// 1 Wire Bus initialization
w1_init();

ds18b20_devices=w1_search(0xf0,ds18b20_rom_codes);

// Global enable interrupts
#asm("sei")

delay_ms(100);

TEMP_READ_FLAG=1;

PROTECT=0;


while (1)
      {                          
         if (TEMP_READ_FLAG)
        {
         temp0=(ds18b20_temperature(&ds18b20_rom_codes[0][0]));    
                       
         if (temp0 != 85) 
           {                      
             if (temp0>49) 
             {
              LED_FAN=0; FAN=1;    
              
              if (temp0>59) {LED_FAN=0; FAN=1; PROTECT=1;}    
              
               else 
               {
                if (temp0<55 && temp0>0) {PROTECT=0; LED_OVERHEATING=1;}
               }      
             } 
              else 
              {
               if (temp0<40 && temp0>0) { LED_FAN=1; FAN=0; PROTECT=0;}  
              }                       
           } 
                  
             TEMP_READ_FLAG=0;  
             TEMP_COUNT=0; 
             LED_COUNT=0;         
        }    
        
        if(DATA_READY)
        {
         adc_data_BUF=adc_data;     
         
         VARIABLE = ((((adc_data_BUF*F)/256)*100)*2);     
         TIMER_1_BUF_DELAY = 65535 - VARIABLE;  
         TIMER_1_BUF_IMPULSE = 65535 - ( (F*100)*2 - (VARIABLE+1000) );   
         
         DATA_READY=0;             
        }  
            
      };
}
