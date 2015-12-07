#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "twi_master.h"
#include "uart_functions.h"
#include "si4734.h"


//set to KRKT Albany at 99.9Mhz
uint8_t si4734_wr_buf[9];          //buffer for holding data to send to the si4734 
uint8_t si4734_rd_buf[15];         //buffer for holding data recieved from the si4734
uint8_t si4734_tune_status_buf[8]; //buffer for holding tune_status data  
uint8_t si4734_revision_buf[16];   //buffer for holding revision  data  

volatile uint16_t  current_fm_freq =  9990; //0x2706, arg2, arg3; 99.9Mhz, 200khz steps
volatile uint8_t STC_interrupt;     //indicates tune or seek is done

uint8_t rssi;
uint16_t eeprom_fm_freq;
uint16_t eeprom_am_freq;
uint16_t eeprom_sw_freq;
uint8_t  eeprom_volume;

enum radio_band{FM, AM, SW};
volatile enum radio_band current_radio_band = FM;

//uint16_t current_fm_freq;
uint16_t current_am_freq;
uint16_t current_sw_freq;
uint8_t  current_volume;


char uart_tx_buf[40];      //holds string to send to crt
char uart_rx_buf[40];      //holds string that recieves data from uart


//You will also need this:
//******************************************************************************
//                          External Interrupt 7 ISR
// Handles the interrupts from the radio that tells us when a command is done.
// The interrupt can come from either a "clear to send" (CTS) following most
// commands or a "seek tune complete" interrupt (STC) when a scan or tune command
// like fm_tune_freq is issued. The GPIO2/INT pin on the Si4734 emits a low
// pulse to indicate the interrupt. I have measured but the datasheet does not
// confirm a width of 3uS for CTS and 1.5uS for STC interrupts.
//
// I am presently using the Si4734 so that its only interrupting when the
// scan_tune_complete is pulsing. Seems to work fine. (12.2014)
//
// External interrupt 7 is on Port E bit 7. The interrupt is triggered on the
// rising edge of Port E bit 7.  The i/o clock must be running to detect the
// edge (not asynchronouslly triggered)
//******************************************************************************
ISR(INT7_vect){STC_interrupt = TRUE;}
//******************************************************************************


//Port E inital values and setup.  This may be different from yours for bits 0,1,6.

//                           DDRE:  0 0 0 0 1 0 1 1
//   (^ edge int from radio) bit 7--| | | | | | | |--bit 0 USART0 RX
//(shift/load_n for 74HC165) bit 6----| | | | | |----bit 1 USART0 TX
//                           bit 5------| | | |------bit 2 (new radio reset, active high)
//                  (unused) bit 4--------| |--------bit 3 (TCNT3 PWM output for volume control)
void radio_init(void){
	DDRE  |= 0x04; //Port E bit 2 is active high reset for radio
	DDRE  |= 0x40; //Port E bit 6 is shift/load_n for encoder 74HC165
	DDRE  |= 0x08; //Port E bit 3 is TCNT3 PWM output for volume
	PORTE |= 0x04; //radio reset is on at powerup (active high)
	PORTE |= 0x40; //pulse low to load switch values, else its in shift mode
}
//Given the hardware setup reflected above, here is the radio reset sequence.
//hardware reset of Si4734
void radio_reset(void){
	PORTE &= ~(1<<PE7); //int2 initially low to sense TWI mode
	DDRE  |= 0x80;      //turn on Port E bit 7 to drive it low
	PORTE |=  (1<<PE2); //hardware reset Si4734
	_delay_us(200);     //hold for 200us, 100us by spec
	PORTE &= ~(1<<PE2); //release reset
	_delay_us(30);      //5us required because of my slow I2C translators I suspect
	//Si code in "low" has 30us delay...no explaination in documentation
	DDRE  &= ~(0x80);   //now Port E bit 7 becomes input from the radio interrupt
}

//Once its setup, you can tune the radio and get the received signal strength.
void radio_powerUp(void){
	while(twi_busy()){} //spin while TWI is busy
	fm_pwr_up();        //power up radio
}
void radio_tune_freq(){
	while(twi_busy()){} //spin while TWI is busy
	fm_tune_freq();     //tune to frequency
}
//retrive the receive strength and display on the bargraph display
void radio_get_strengh(){
	while(twi_busy()){}                //spin while TWI is busy
	fm_rsq_status();                   //get status of radio tuning operation
	rssi =  si4734_tune_status_buf[4]; //get tune status
}
//redefine rssi to be a bar graph
void redefine_rssi(){
	if(rssi<= 8) {rssi = 0x00;} 
	else if(rssi<=16) {rssi = 0x01;} 
	else if(rssi<=24) {rssi = 0x03;} 
	else if(rssi<=32) {rssi = 0x07;} 
	else if(rssi<=40) {rssi = 0x0F;} 
	else if(rssi<=48) {rssi = 0x1F;} 
	else if(rssi<=56) {rssi = 0x3F;} 
	else if(rssi<=64) {rssi = 0x7F;}
	else if(rssi>=64) {rssi = 0xFF;}
}
void interrupt_init(){
	EIMSK |= (1<<INT7);
	EICRB |= (1<<ISC71)|(1<<ISC70);
	EIFR |= (1 << INTF7);
}


int main(void){
	//redefine_rssi();
	PORTE |= (0<<PE3);
	interrupt_init();
	sei();
	init_twi();
	lm73_init();
	uart_init();
	radio_init();
	radio_reset();
	radio_powerUp();
	radio_tune_freq();

	while(1){

	};
}













