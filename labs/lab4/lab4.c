// lab3_skel.c 
// Sorawis Nilparuk
// 10/29/2015

//  HARDWARE SETUP:
//  PORTA is connected to the segments of the LED display. and to the pushbuttons.
//  PORTA.0 corresponds to segment a, PORTA.1 corresponds to segement b, etc.
//  PORTB bits 4-6 go to a,b,c inputs of the 74HC138.
//  PORTB bit 7 goes to the PWM transistor base and OE_N of BAR_GRAPH
//  PORTB bit 3 goes to OE_n of BAR_GRAPH
//  PORTB bit 2 goes to SDIN of BAR_GRAPH
//  PORTB bit 1 goes to SRCLK of BAR_GRAPH and SCK of Encoder
//  PORTD bit 2 goes to REGCLK of BAR_GRAPH
//  PORTE bit 6 goes to SHIFT_LD_N on Encoder
//  PORTE bit 7 goes to CLK_INT on Encoder  (always driven low)

#define F_CPU 16000000 // cpu speed in hertz 
#define TRUE 1
#define FALSE 0
#define INPUT 0xFF
#define SELECT_BIT_BUTTON_BOARD 0x70
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <math.h>
#define ONE	 0xf9
#define TWO	 0xa4
#define THREE	 0xb0
#define FOUR     0x99
#define FIVE     0x92
#define SIX 	 0x82
#define SEVEN 	 0xf8
#define EIGHT 	 0x80
#define NINE 	 0x90
#define ZERO  	 0xc0
#define OFF	 0xff


#define MAX_SEGMENT 5
#define BUTTON_COUNT 3
#define MAX_SUM 1023
#define ENCODE_LEFT_KNOB(read)   (read & 0x0C) >> 2
#define ENCODE_RIGHT_KNOB(read)  (read & 0x03) 
#define CHECK_LEFT_KNOB   0x03
#define CHECK_RIGHT_KNOB  0x0c


#define CW  1
#define CCW 2

//holds data to be sent to the segments. logic zero turns segment on
uint8_t segment_data[5];

//decimal to 7-segment LED display encodings, logic "0" turns on segment
uint8_t dec_to_7seg[12];
uint8_t dif = 1;
uint16_t value = 0;
uint8_t rotate_7seg = 0;
static uint8_t modeA = 0;
static uint8_t modeB = 0;
static uint8_t sw_table[] = {0, 1, 2, 0, 2, 0, 0, 1, 1, 0, 0, 2, 0, 2, 1, 0};
//******************************************************************************
//                            chk_buttons                                      
//Checks the state of the button number passed to it. It shifts in ones till   
//the button is pushed. Function returns a 1 only once per debounced button    
//push so a debounce and toggle function can be implemented at the same time.  
//Adapted to check all buttons from Ganssel's "Guide to Debouncing"            
//Expects active low pushbuttons on PINA port.  Debounce time is determined by 
//external loop delay times 12. 



int8_t chk_buttons(uint8_t button){
    static uint16_t state = 0;
    state = (state << 1) | (bit_is_clear(PINA, button) | 0xE000);
    if (state == 0xF000){
	return 1;
    }
    return 0;

}
//***********************************************************************************
// int2seg
// return the 7-segment code for each digit
uint8_t int2seg(uint8_t number){
    if(number == 0 ){
	return ZERO;
    }
    else if(number == 1 ){
	return ONE;
    }
    else if(number == 2 ){
	return TWO;
    }
    else if(number == 3 ){
	return THREE;
    }
    else if(number == 4 ){
	return FOUR;
    }
    else if(number == 5 ){
	return FIVE;
    }
    else if(number == 6 ){
	return  SIX;
    }
    else if(number == 7 ){
	return SEVEN;
    }
    else if(number == 8 ){
	return EIGHT;
    }
    else if(number == 9 ){
	return NINE;
    }
    else{ 
	return 0;
    }
}
//*******************************************************************
//                                   segment_sum                                    
//takes a 16-bit binary input value and places the appropriate equivalent 4 digit 
//BCD segment code in the array segment_data for display.                       
//array is loaded at exit as:  |digit3|digit2|colon|digit1|digit0|

void segsum(uint16_t sum) {
    //determine how many digits there are 
    int digit;
    // Break down the digits
    if(sum >= 1000){
	digit = 4;
    }
    else if (sum >= 100 && sum < 1000){
	digit = 3;
    }
    else if (sum >= 10 && sum < 100){
	digit = 2;
    }
    else if (sum <10){
	digit = 1;
    }
    //break up decimal sum into 4 digit-segments
    segment_data[0] = int2seg(sum % 10); //ones
    segment_data[1] = int2seg((sum % 100)/10); //tens
    //segment_data[2] = 1; //decimal
    segment_data[3] = int2seg((sum % 1000)/100); //hundreds
    segment_data[4] = int2seg(sum/1000); //thousands
    //blank out leading zero digits 
    switch (digit){
	case 3:
	    segment_data[4] = OFF;
	    break;
	case 2:
	    segment_data[4] = OFF;  	
	    segment_data[3] = OFF;  	
	    break;
	case 1:
	    segment_data[4] = OFF;  	
	    segment_data[3] = OFF;  	
	    segment_data[1] = OFF;  	
	    break;
	default:
	    break;
    }
    //now move data to right place for misplaced colon position
}//segment_sum
//***********************************************************************************
void button_routine(){
    uint8_t button;
    DDRA  = 0x00; // PORTA input mode
    PORTA = 0xFF; //Pull ups
    __asm__ __volatile__ ("nop");
    __asm__ __volatile__ ("nop");
    //enable tristate buffer for pushbutton switches
    PORTB |= 0x70; //Set S2,S1,S0 to 111
    __asm__ __volatile__ ("nop");
    __asm__ __volatile__ ("nop");
    //now check each button and increment the count as needed
    for (button = 0 ; button < BUTTON_COUNT ; button++){
	if (chk_buttons(button)){
	    //Check the state of buttons
	    if(button == 0){
		modeA = !modeA;   //Inverse everytime button0 is pressed
		//value = 1;
	    }
	    else if( button == 1){  //Inverse everytime button1 is pressed  
		modeB = !modeB;
		//value = 2;
	    } 
	    //value = value + 100;
	    if (modeA && modeB){    //If both modes are on, add 0
		//value = 4;
		dif = 0;
	    }
	    else if(modeA && !modeB){ //1-on 2-off add 2
		//value = modeA;
		// value = 4;
		dif =  2;
	    }
	    else if (modeB && !modeA){ //1-off 2-on add 4
		//value = modeB;
		dif =  4;
	    }
	    else {               //both off add 1
		dif = 1;
	    }

	}
    }
}
/***************************************************************************
  Interrupt routine: set flag for checking button in main
  (Might cause an issue if button check takes too long to run, it will become
  polling instead of interrupt. Tried putting button routine in the ISR,
  LED dims
 ****************************************************************************/
ISR(TIMER0_OVF_vect){
    check_knobs();
    display_update();
    button_routine();
}



/***************************************************************************
  Initialize SPI 
 ****************************************************************************/
void SPI_init(){
    /* Set MOSI and SCK output, all others input */
    //DDRB = (1<<PB3)|(1<<PB1);

    /* Enable SPI, Master, set clock rate fck/16 */
    SPCR = (1<<SPE)|(1<<MSTR)|(1<<SPR0);
}

/***************************************************************************
  Transmit data to SPI
 ****************************************************************************/
void SPI_Transmit(uint8_t data){

    SPDR = data;    //Write data to SPDR
    while(!(SPSR & (1<<SPIF))){} //SPIN write
}

/***************************************************************************
  Read data from SPI input (SPDR)
 ****************************************************************************/
uint8_t SPI_Receive(void){
    PORTE &= 0;       //Write 0 to PE6 to trigger SPI on radio board
    __asm__ __volatile__ ("nop");
    __asm__ __volatile__ ("nop");
    // Wait until 8 clock cycles are done 
    SPDR = 0x00;     //Write 1 to set the SPI slave input to one (wait for read)
    PORTE |= (1 << PE6);  
    __asm__ __volatile__ ("nop");
    __asm__ __volatile__ ("nop");
    while (bit_is_clear(SPSR,SPIF)){} //SPIN read 
    // Return incoming data from SPDR
    return(SPDR);  
}
void check_knobs(void){
    static uint8_t encoder;
    encoder  = SPI_Receive();
    decode_spi_left_knob(encoder);
    decode_spi_right_knob(encoder);
}
/***************************************************************************
 *void bar_graph()
 *show selected modes on the bar graph
 **************************************************************************/
void bar_graph(){

    uint8_t write = 0;
    //If mode A is selected -> xxxxxxx1
    if(modeA){
	write |= 0x01;  
    }

    //If mode A is not selected -> xxxxxxx0
    else if(!modeA){
	write &= 0xFE;
    }
    //If mode b is selected -> xxxxxxx1x
    if(modeB){
	write |= 0x02;
    }
    //If mode b is not selected -> xxxxxxx0x
    else if(!modeB){
	write &= 0xFD;
    }

    write = 0xFF;
    //Write the bargraph to SPI
    SPI_Transmit(write);
    PORTD = (1 << PD2);  //Push data out of SPI
    __asm__ __volatile__ ("nop"); //Buffer
    __asm__ __volatile__ ("nop");  //Buffer


    PORTD = (2 << PD2);  // Push data out of SPI
    __asm__ __volatile__ ("nop");  //Buffer
    __asm__ __volatile__ ("nop");  //Buffer
}
/************************************************************************
 *Display the number (code from lab1)
 **************************************************************************/
void display_update(){
    update_number();
    segsum(value);
    if(rotate_7seg > 4){
	rotate_7seg = 0;
    }
    DDRA = 0xFF;  //switch PORTA to output
    __asm__ __volatile__ ("nop"); //Buffer
    __asm__ __volatile__ ("nop"); //Buffer 
    PORTB &= 0x8F;
    PORTB |= rotate_7seg << 4;
    PORTA = segment_data[rotate_7seg];	
    _delay_us(100);
    rotate_7seg++;
    /*    
	  for(display_segment = 0 ; display_segment < MAX_SEGMENT ; display_segment++){
    //send PORTB the digit to display
    //value = 1;
    //segsum(value);
    PORTB &= 0x8F;
    PORTB |= display_segment << 4;
    //send 7 segment code to LED segments
    //update digit to display
    PORTA = segment_data[display_segment];	
    _delay_us(200);
    }
     */
}
/**************************************************************************
 *Decode the knobs encoder using table method
 *Track the last phase and current phase
 **************************************************************************/
void decode_spi_left_knob(uint8_t encoder1){
    //Set up the table
    //Set up index
    uint8_t sw_index = 0;
    //Counter for preventing unneccessary reset    
    static uint8_t acount1 = 0;
    static uint8_t previous_encoder1 = 0; //Initialize previous    
    uint8_t direction = 0;                    //Direction variable
    encoder1 = ENCODE_LEFT_KNOB(encoder1);  //Mask the bit for decoding left know
    sw_index = (previous_encoder1 << 2) | encoder1; 
    /*shift previous to the left use it as an index Since
      we know the pattern of the knob when it is turning
      Use that data to compare with the table to determine
      Which way it is turning*/
    direction = sw_table[sw_index];
    //Read out the direction from table
    //value = acount2;
    if(direction == CW){  //If CW, add counter
	acount1++;
    }	
    if(direction == CCW){ //If CCW, decrement counter
	acount1--;
    }
    if(encoder1 == 3){    //encoder1 = 3 (stop spinning)
	if((acount1 > 1) && (acount1 < 10)){   //Check if the counter for CW
	    value = value + dif;
	}
	if ((acount1 <= 0xFF) && (acount1 > 0xF0)){    //Check counter for CCW
	    value = value - dif;
	}
	//update_number();                 //Check number in the routine
	acount1 = 0;                     //Reset counter
    }
    previous_encoder1 = encoder1;
}
/*************************************************************************
  Exactly the same with decode_spi_left_knob(), only mask different bits 
 **************************************************************************/
void decode_spi_right_knob(uint8_t encoder2){
    uint8_t sw_index = 0;
    static uint8_t acount2 = 0;
    static uint8_t previous_encoder2 = 0;
    uint8_t direction = 0;
    encoder2 = ENCODE_RIGHT_KNOB(encoder2);
    sw_index = (previous_encoder2 << 2) | encoder2;
    direction = sw_table[sw_index];
    //value = modeA;
    if(direction == CW){
	acount2++;
    }	
    if(direction == CCW){
	acount2--;
    }
    if(encoder2 == 3){
	if((acount2 > 1) && (acount2 < 10)){
	    value = value + dif;
	}
	if ((acount2 <= 0xFF) && (acount2 > 0xF0)){
	    value = value - dif;
	}
	//update_number();
	acount2 = 0;
    }
    previous_encoder2 = encoder2;
}

void update_number(void){
    if (value > (0-MAX_SUM)){
	value = MAX_SUM - (dif-1);                       	
    }
    else if (value > MAX_SUM){
	value = value - MAX_SUM;
    }
}

//***********************************************************************************
int main()
{
    //set port bits 4-7 B as outputs
    DDRE = 0xc0;
    PORTE &= 0x7F;
    DDRB = 0xF7;
    DDRD |= (1 << PB2);

    segment_data[2] = OFF;

    TIMSK |= (1<<TOIE0);             //enable interrupts
    TCCR0 |= (1<<CS00) | (1<<CS10);  //normal mode, prescale by 128
    SPI_init();
    //sei();
    while(1){
	//display_update();
	bar_graph();
    }//while
    return 0;
}//main
