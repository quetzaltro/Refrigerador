/*
 * File:   main.c
 * Author: Troyan
 *
 * Created on 16 de diciembre de 2020, 11:00 PM
 */

/*
 * Interfacing DHT11 sensor with PIC16F887
 * C Code for MPLAB XC8 compiler
 * Internal oscillator used @ 4MHz
 * This is a free software with NO WARRANTY
 * http://simple-circuit.com/
 */

/*Original config in the webpage*/
//INTOSCIO oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN
//Watchdog Timer (WDT) disabled
//Power-up Timer (PWRT) disabled
//RE3/MCLR pin function is digital input, MCLR internally tied to VDD
//Brown-out Reset (BOR) disabled
//Low voltage programming disabled
//Data memory code protection disabled
//Program memory code protection disabled

// CONFIG
#pragma config FOSC = INTOSCIO  // Oscillator Selection bits (INTOSC oscillator: I/O function on RA6/OSC2/CLKOUT pin, I/O function on RA7/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = OFF      // RA5/MCLR/VPP Pin Function Select bit (RA5/MCLR/VPP pin function is digital input, MCLR internally tied to VDD)
#pragma config BOREN = OFF      // Brown-out Detect Enable bit (BOD disabled)
#pragma config LVP = OFF        // Low-Voltage Programming Enable bit (RB4/PGM pin has digital I/O function, HV on MCLR must be used for programming)
#pragma config CPD = OFF        // Data EE Memory Code Protection bit (Data memory code protection off)
#pragma config CP = OFF         // Flash Program Memory Code Protection bit (Code protection off)



// #pragma config statements should precede project file includes.
// Use project enums instead of #define for ON and OFF.


//In-Circuit Debugger disabled
//Fail-Safe Clock Monitor enabled
//Internal/External Switchover mode enabled
//Flash Program Memory Self Write disabled
//Brown-out Reset set to 4.0V

#define DHT11_PIN      RB0
#define DHT11_PIN_DIR  TRISB0

#define LED_WORKING     RA4
#define LED_ERROR       RA3

#define LED_NUMBER_5    RA6
#define LED_NUMBER_4    RA7
#define LED_NUMBER_3    RA0
#define LED_NUMBER_2    RA1
#define LED_NUMBER_1    RA2

#define TIME_MILI_LED       400
#define TIME_BETW_READ      1000

#define ERROR_TIME_OUT      0x01
#define ERROR_NO_RESPONSE   0x02
#define ERROR_CHECKSUM      0x03

#include <xc.h>
#define _XTAL_FREQ 4000000



//Init leds
void initLeds(void);

//set number
void setNumberLeds(unsigned short int pError, unsigned char pNumber);

// send start signal to the sensor
void startSignal(void);

// Check sensor response
__bit checkResponse(void);

// Data read function
__bit readData(unsigned char* dht_data);

void main(void) {
    // variables declaration
    unsigned char TempUnidad, TempDecimal, Humidity_H, Humidity_L, CheckSum;
    
    //Init ports
    TRISA = 0x20; //solo dejo el puerto RB5 entrada para la resistencia que evita interferencias
    TRISB = 0xFF; //configuracion puerto B
    CMCON = 0x07; //Desabilita los puertos analogicos
    //T1CON  = 0x10;        //el pic original era de 8 MHz, set Timer1 clock source to internal with 1:2 prescaler (Timer1 clock = 1MHz)
    T1CON = 0x00; //set Timer1 clock source to internal with 1:1 prescaler (Timer1 clock = 1MHz)


    while (1) {
        initLeds();
        LED_WORKING = 1;
        
        startSignal(); // send start signal to the sensor

        if (checkResponse()) // check if there is a response from sensor (If OK start reading humidity and temperature data)
        {
            // read (and save) data from the DHT11 sensor and check time out errors
            if (readData(&Humidity_H) || readData(&Humidity_L) || readData(&TempUnidad) || readData(&TempDecimal) || readData(&CheckSum)) {
                // display "Time out!"
                setNumberLeds(1, ERROR_TIME_OUT);
            }
            else{
                // if there is no time out error
                if (CheckSum == ((Humidity_H + Humidity_L + TempUnidad + TempDecimal) )) { 
                    // if there is no checksum error
                    setNumberLeds(0, TempUnidad);
                }else {
                    // if there is a checksum error
                    setNumberLeds(1, ERROR_CHECKSUM);
                }

            }
        }
        else {
            // if there is a response (from the sensor) problem
            setNumberLeds(1, ERROR_NO_RESPONSE);
        }
        
        //turning off the interface
        LED_WORKING = 0;
        TMR1ON = 0; // disable Timer1 module
        __delay_ms(TIME_BETW_READ); // wait 1 second

    }

}

void startSignal(void) {
    DHT11_PIN_DIR = 0; // configure DHT11_PIN as output
    DHT11_PIN = 0; // clear DHT11_PIN output (logic 0)

    __delay_ms(25); // wait 25 ms
    DHT11_PIN = 1; // set DHT11_PIN output (logic 1)

    __delay_us(30); // wait 30 us
    DHT11_PIN_DIR = 1; // configure DHT11_PIN as input
}

__bit checkResponse(void) {
    TMR1H = 0; // reset Timer1
    TMR1L = 0;
    TMR1ON = 1; // enable Timer1 module

    while (!DHT11_PIN && TMR1L < 100); // wait until DHT11_PIN becomes high (checking of 80µs low time response)

    if (TMR1L > 99) // if response time > 99µS  ==> Response error
        return 0; // return 0 (Device has a problem with response)
    else {
        TMR1H = 0; // reset Timer1
        TMR1L = 0;

        while (DHT11_PIN && TMR1L < 100); // wait until DHT11_PIN becomes low (checking of 80µs high time response)

        if (TMR1L > 99) // if response time > 99µS  ==> Response error
            return 0; // return 0 (Device has a problem with response)

        else
            return 1; // return 1 (response OK)
    }
}

__bit readData(unsigned char* dht_data) {
    *dht_data = 0;

    for (char i = 0; i < 8; i++) {
        TMR1H = 0; // reset Timer1
        TMR1L = 0;

        while (!DHT11_PIN) // wait until DHT11_PIN becomes high
            if (TMR1L > 100) { // if low time > 100  ==>  Time out error (Normally it takes 50µs)
                return 1;
            }

        TMR1H = 0; // reset Timer1
        TMR1L = 0;

        while (DHT11_PIN) // wait until DHT11_PIN becomes low
            if (TMR1L > 100) { // if high time > 100  ==>  Time out error (Normally it takes 26-28µs for 0 and 70µs for 1)
                return 1; // return 1 (timeout error)
            }

        if (TMR1L > 50) // if high time > 50  ==>  Sensor sent 1
            *dht_data |= (1 << (7 - i)); // set bit (7 - i)
    }

    return 0; // return 0 (data read OK)
}

//Init leds
void initLeds(void){
    LED_WORKING = 0;    
    LED_ERROR = 0;

    LED_NUMBER_5 = 0;    
    LED_NUMBER_4 = 0;    
    LED_NUMBER_3 = 0;    
    LED_NUMBER_2 = 0;    
    LED_NUMBER_1 = 0; 
    
    __delay_ms(TIME_MILI_LED);
    LED_WORKING = 1;
    __delay_ms(TIME_MILI_LED);
    LED_ERROR = 1; LED_WORKING = 0;
    __delay_ms(TIME_MILI_LED);
    LED_NUMBER_5 = 1; LED_ERROR = 0;
    __delay_ms(TIME_MILI_LED);
    LED_NUMBER_4 = 1; LED_NUMBER_5 = 0;
    __delay_ms(TIME_MILI_LED);
    LED_NUMBER_3 = 1; LED_NUMBER_4 = 0;
    __delay_ms(TIME_MILI_LED);
    LED_NUMBER_2 = 1; LED_NUMBER_3 = 0;
    __delay_ms(TIME_MILI_LED);
    LED_NUMBER_1 = 1; LED_NUMBER_2 = 0;
    __delay_ms(TIME_MILI_LED);
    LED_NUMBER_1 = 0;
    __delay_ms(TIME_MILI_LED);
}

//set number
void setNumberLeds(unsigned short int pError, unsigned char pNumber){
    
    LED_ERROR = pError == 0 ? 0 : 1;
    LED_NUMBER_1 = pNumber & 0x01;
    LED_NUMBER_2 = (pNumber & 0x02) >> 1;
    LED_NUMBER_3 = (pNumber & 0x04) >> 2;
    LED_NUMBER_4 = (pNumber & 0x08) >> 3;
    LED_NUMBER_5 = (pNumber & 0x10) >> 4;
}


// End of code.