#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <avr/io.h>
#include <util/delay.h>
#include <stdarg.h>
#include "hd44780.h"
#include "lcd.h"
#include "defines.h"
#include <avr/interrupt.h>

/*
 * 
 */
 
//-------------------------

float measuring(void){
    
    PORTB |= (1<<PB2);
    _delay_us(10);
    PORTB &= ~(1<<PB2);
    TIFR1 |= (1<<ICF1);
    TCCR1B |= (1<<ICES1); 
    
    while(!(TIFR1&(1<<ICF1)));
    uint16_t rising_edge_time = ICR1;
    
    
    TCCR1B &= ~(1<<ICES1);
    TIFR1 |= (1<<ICF1);
    while(!(TIFR1&(1<<ICF1)));
    uint16_t falling_edge_time = ICR1;
    
    
    uint16_t time_took = falling_edge_time - rising_edge_time;
    float distance = (time_took)*0.0093045;
 
    return distance;
    
}

FILE lcd_str = FDEV_SETUP_STREAM (lcd_putchar , NULL , _FDEV_SETUP_WRITE ) ; // to create global variable for LCD stream (before main function)

void lcd_vprintf(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(&lcd_str, format, args); 
    va_end(args);
}


volatile bool stability = false;

ISR(TIMER2_COMPA_vect) {
      if (stability) {
        PORTB |= (1 << PB3); // Turn LED ON
    } else {
        PORTB &= ~(1 << PB3); // Turn LED OFF
    }
    

}

void PWM() {
    DDRD |= (1 << PD6); 

    TCCR0A |= (1 << WGM00) | (1 << WGM01); 
    TCCR0A |= (1 << COM0A1); 
    TCCR0B |= (1 << CS00); 

    OCR0A = 0; 
}

uint8_t mapVelocityToPWM(float v0, float v_min, float v_max) {
    if (v0 < v_min) {
        v0 = v_min;
    }
    if (v0 > v_max) {
        v0 = v_max;
    }

    return (uint8_t)(((v0 - v_min) / (v_max - v_min)) * (255 - 200) + 200);
}


void setMotorSpeed(float distance, float x_min, float x_max, float v_min, float v_max) {
    float v0 = ((distance - x_min) / (x_max - x_min)) * (v_max - v_min) + v_min;

    if (v0 < v_min) v0 = v_min;
    if (v0 > v_max) v0 = v_max;

    uint8_t pwmDutyCycle = mapVelocityToPWM(v0, v_min, v_max);

    OCR0A = pwmDutyCycle;
}

void initilizing_timer2_interrupt() {
 
    DDRB |= (1 << PB3);
    
    TCCR2A |= (1 << WGM21); 
    TCCR2B |= (1 << CS22) | (1 << CS21) | (1 << CS20);   
    OCR2A = 156;                          

    TIMSK2 |= (1 << OCIE2A);               
    sei();
  
}


