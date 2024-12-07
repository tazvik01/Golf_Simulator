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


//-------------------------
// Function used to measure the distance of the object in front of sensor
float measuring(void) {

    PORTB |= (1 << PB2);
    _delay_us(10);
    PORTB &= ~(1 << PB2);
    TIFR1 |= (1 << ICF1);
    TCCR1B |= (1 << ICES1);

    while (!(TIFR1 & (1 << ICF1)));
    uint16_t rising_edge_time = ICR1;


    TCCR1B &= ~(1 << ICES1);
    TIFR1 |= (1 << ICF1);
    while (!(TIFR1 & (1 << ICF1)));
    uint16_t falling_edge_time = ICR1;


    uint16_t time_took = falling_edge_time - rising_edge_time;
    float distance = (time_took)*0.0093045;

    return distance;

}

//Sets up LCD functionality
FILE lcd_str = FDEV_SETUP_STREAM(lcd_putchar, NULL, _FDEV_SETUP_WRITE); // to create global variable for LCD stream (before main function)

// Function for using vprintf
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

    return (uint8_t) (((v0 - v_min) / (v_max - v_min)) * (240 - 200) + 200);
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

int main(void) {

    initilizing_timer2_interrupt();
    PWM();
    //PWM_2();   
    DDRB &= ~(1 << PB0);
    DDRB |= (1 << PB2);
    TCCR1B |= (1 << CS10);


    DDRD &= ~(1 << PD7);
    PORTD |= (1 << PD7);


    DDRB |= (1 << PB4);
    PORTB &= ~(1 << PB4);

    DDRC |= (1 << PC1);
    PORTC &= ~(1 << PC1);

    DDRB |= (1 << PB2);


    DDRB |= (1 << PB1);
    PORTB |= (1 << PB1);



    // Sets y-component of joystick as an input
    // Sets y-component of joystick as an input
    DDRC &= ~(1 << PC2);
    PORTC &= ~(1 << PC2);


    //   


    float previous_distance = 0.0;

    //previous_distance = measuring();


    while (1) {
        lcd_init();

        float dist_measure = measuring();


        //       
        //PORTB |= (1<<PB5);     
        hd44780_outcmd(HD44780_DDADDR(0x04));
        hd44780_wait_ready(false);

        fprintf(&lcd_str, "%d cm/s", OCR0A);
        
        hd44780_wait_ready(false);
        //_delay_ms(500);



        //Check if Joystick_Switch is pressed
        if (!(PINB & (1 << PB1))) {



            ADMUX |= (1 << REFS0); // Set ADMUX to 0x42 (ADC2 with AVCC)
            ADMUX &= ~(1 << REFS1);

            ADMUX &= ~(1 < MUX3);
            ADMUX &= ~(1 << MUX2);
            ADMUX &= ~(1 << MUX0);
            ADMUX |= (1 << MUX1);



            // Start ADC conversion
            ADCSRA |= (1 << ADSC); // Set ADSC to start conversion
            ADCSRA |= (1 << ADEN);


            DDRB |= (1 << PB5);
            PORTB |= (1 << PB5);



            // Wait for conversion to complete
            while (ADCSRA & (1 << ADSC)) {
                // ADSC is still set to 1, indicating conversion is in progress
                // Loop until ADSC is cleared to 0
            }

            // Read the ADC result
            uint8_t low_byte = ADCL; // Read the low byte first (ADCL)
            uint8_t high_byte = ADCH; // Then read the high byte (ADCH)
            uint16_t adc_value = ((uint16_t) high_byte << 8) | low_byte; // Combine the two bytes to form the 16-bit result

            uint8_t scaled_value = (uint8_t) ((adc_value >> 2) * (240 - 120) / 240 + 120); //135

            // lcd_vprintf("%d", scaled_value);



            // Scale adc_value to match OCR1A range
            //uint16_t mapped_value = adc_value;
            OCR0A = scaled_value; // Max value of 288
            lcd_vprintf("\x1b\xc0  Manual Mode");
            //lcd_vprintf("%d", scaled_value);
        } else {
            PORTB &= ~(1 << PB5);
            if ((dist_measure >= previous_distance - 2.0) && (dist_measure <= previous_distance + 2.0)) {
                stability = true;
                PORTC &= ~(1 << PC1);
                setMotorSpeed(dist_measure, 5.0, 120.0, 5.0, 30.0);
            } else {
                stability = false;
                PORTC |= (1 << PC1);
                //DDRB &= ~(1 << PB5);
                PORTB &= ~(1 << PB5);
            }
            lcd_vprintf("\x1b\xc0   Auto Mode");

        }


        // Release button. To set position, hold position button and release button at same time. For manual mode, change joystick and hit this button at same time 
        //to launch ball
        if (!(PIND & (1 << PD7))) {
            //_delay_ms(100);




            DDRB |= (1 << PB5);
            PORTB |= (1 << PB5);


        } else {
            //DDRB &= ~(1 << PB5);
            PORTB &= ~(1 << PB5);
        }
        previous_distance = measuring();

        _delay_ms(150);
    }






    return 0;
}
