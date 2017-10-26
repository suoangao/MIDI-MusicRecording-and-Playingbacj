include<io.h>
#include<avr/interrupt.h>
#include<util/delay.h>
#include<avr/iom32.h>


/*
*This is lab 2 for ece 353, by SuoAn Gao, Yujia Huo, and Siyu Wu
*/

unsigned char FPnote;// first part of the note
unsigned char SPnote;// second part of the note
unsigned char TPnote;// third part of the nore
unsigned char TimeDel1; // timeDelay, 1st part
unsigned char TimeDel2; // timeDelay, 2nd part
unsigned int TimeDel; // the time Delay
unsigned int Test;

// When recieve is enabled

unsigned char USART_Receive(void){
		while(!(UCSRA & (1<<RXC))){
			if((PINA & 0x07)!= (1 << PINA2)){
				return 0;//when the record switch is closed, return 0
			}
		}
		return UDR;
}


void USART_Flush(void){
	unsigned char dummy;
	while(UCSRA&(1<<RXC))
			dummy = UDR;
}

/*This function will implement the methdology you use to write data into eeprom*/
void EEPROM_write(unsigned int writeAddress, unsigned char writeData){
	while(EECR & (1<<EEWE)) ;
	EEAR=writeAddress;
	EEDR=writeData;
	EECR |=(1<<EEMWE);
	EECR |=(1<<EEWE);
}

/*This function will implement the methdology you use to read data from the eeprom*/
unsigned char EEPROM_Read(unsigned int readAddress){
	while(EECR & (1<<EEWE));
	EEAR = readAddress;
	EECR |= (1<<EERE);
	return EEDR;
}

/*This methdology will transfer data to UDR*/
void USART_Transmit(unsigned char data){
	while(!(UCSRA & (1<<UDRE)));
	UDR = data;
}

/*This function will make the led shine based on the music note*/
void LED(unsigned char notec)
{
	PORTB=notec;			// output note value
}

/*LED light off*/
void set_pb0_high(void){
PORTB = 0x00;
}

/*Enable timer1 compare match*/
void timer1_comp_init(void)
{
  TCCR1B = 0x05;   //normal mode
  TIMSK |= 0x10;  //compare match interrupt
  OCR1A = 0x7A1;     //TOP values
  TCNT1 = 0x00;
}

//Check the LED statue
// if led is on, return true; if led is off, retruen false
unsigned char pb0_is_low(void)
{
  return (PORTB != 0x00);
}

ISR(TIMER1_COMPA_vect)
{
	if (pb0_is_low()){
	set_pb0_high();
	}
}


void init_ADC(void)
{
  ADMUX = ADMUX|0x40;         //reference voltage is external voltage

  ADCSRA = 0x06;              //prescaler is 64
  ADCSRA = ADCSRA|0x80;        // enable the ADC
}


uint16_t read_ADC(uint8_t ch)
{
	
  ch&=0b00000111;// select pin 7
  ADMUX |= ch;
  ADCSRA = ADCSRA|0x40;       //start a conversion
  while(!(ADCSRA&0x10));   //wait for complete
 
  ADCSRA=ADCSRA|0x10;         //clear the flag
 
  return (ADC);

}


///////////////////////////////////Start Main////////////////////////////
int main(void){
  
  TCCR1B |= ((1<< CS12) + 1); // set prescaler = 1024

  /*This is where we initiate the interrupts*/

//TIMSK |= (1 << OCIE1B); //
  timer1_comp_init(); //Enable timer1 compare interrupt
  sei();

  /*This is where we initate the USART*/
  // USART Register B
  UCSRB |= ( 3 <<TXEN );
  // USART Regisster UBRRL
  UBRRL = 0x07;

  /*Here we initate the ports*/
  DDRA = 0x00; // set Port A as input, so that we could read from sliders
  DDRB = 0xFF; // set ProtB valules as output
  PORTB = 0;
  
  //variables for record
  int countOfNotes;// EEPROM(1kB) can only store 102 notes and the corresponding time (3+2+3+2)
  unsigned int lastAddress;//last valid byte
  unsigned int writeAddress;
  int count;//3 bytes are one group
  int pair;//one note is represented by 2 groups of 3 bytes
  uint16_t interval;
  unsigned char byte[3];
   
  init_ADC();//for modify

  int counter;// counter used to access memory

/*Here is where the function begins*/
  while(1){
	//enter recording new
    if((PINA & 0x07)== (1 << PINA2)){ // If recording, 100, we or a 0100 which is a 0x04
		USART_Flush();//clear UDR before record
		// USART_Receive
		//initialize recording mode
		countOfNotes=0;
		lastAddress=0;
		writeAddress=0;
		count=0;
		pair=0;
		interval=0;

		while((PINA & 0x07)== (1 << PINA2)){
			if(countOfNotes<102){

				byte[0]=USART_Receive();
				if(byte[0] == 0){//it means the record switch is closed
					PORTB=0;
					break;
				}
				byte[1]=USART_Receive();
				byte[2]=USART_Receive();

				pair=!pair;//this group is the start or the end of the note?
				if(pair==0){//this is the end of the note
					countOfNotes++;//pair is 0 means there is a complete note
				}
				if(countOfNotes==0) TCNT1 = 0; //start timer when receive the first group of 3 bytes
				if((countOfNotes!=0)){//timer start?

					interval=TCNT1;//record TCNT1;
					TCNT1 = 0; //Clear timer
					
						//timer into memory
					writeAddress=lastAddress+1;
					EEPROM_write(writeAddress, (uint8_t)interval);	//timer lower byte

					writeAddress=writeAddress+1;
					EEPROM_write(writeAddress, (uint8_t)(interval>>8));	//timer higher byte

					lastAddress=writeAddress;

						//lastAddress into memory
					writeAddress=1022;
					EEPROM_write(writeAddress, (uint8_t)lastAddress);	//lastAddress lower byte

					writeAddress=1023;
					EEPROM_write(writeAddress, (uint8_t)(lastAddress>>8));		//lastAddress higher byte

				}
			
				PORTB=byte[1];

					//UDR into memory
				if(lastAddress!=0){
					writeAddress=lastAddress+1;
				}
				for(count=0; count<3; count++){
					EEPROM_write(writeAddress, byte[count]);
					writeAddress++;
				}
				lastAddress=writeAddress-1;

					//lastAddress into memory
				writeAddress=1022;
				EEPROM_write(writeAddress, (uint8_t)lastAddress);	//lastAddress lower byte

				writeAddress=1023;
				EEPROM_write(writeAddress, (uint8_t)(lastAddress>>8));		//lastAddress higher byte

			}
			else PORTB=0;
		}

    }
    // USART_Receive ends
      
	while( (PINA&0x06)==0x02 ){ // playback mode or modify mode
		pair=0;
		lastAddress = (EEPROM_Read(1023)<<8) + EEPROM_Read(1022);// read the lastAddress
		
		
		counter = 0; // memory access counting
			while( (counter < lastAddress) && ( (PINA&0x06)==0x02 )){
				/*We deal with notes*/

				FPnote = EEPROM_Read(counter);// 1/3 of a group
				TCNT1 = 0; //Clear timer

				counter++;
				SPnote = EEPROM_Read(counter);//  2/3 of a group
				LED(SPnote);
				counter++;
				TPnote = EEPROM_Read(counter); // 3/3 of a group

				USART_Transmit(FPnote);
				USART_Transmit(SPnote);

				USART_Transmit(TPnote);

				/*We deal with delays*/
				counter++;
				if(counter < lastAddress){
				TimeDel1 = EEPROM_Read(counter); // 1/2 of delaytime, least significant

				counter++;
				TimeDel2 = EEPROM_Read(counter); // 2/2 of delaytime, most significant
				counter++;
				
				TimeDel = (TimeDel2<<8) + TimeDel1;
				
				if((PINA&0x07) == 0x03){// if modify the playing, timedel will be changed
				  	
					//init_ADC();
					Test =read_ADC(7);//*TimeDel/100 ;
					if (Test <= 60){
						TimeDel=0.5*TimeDel;
					}
					else if((Test<=800)&(Test>=61))
					{
						TimeDel=2*TimeDel;
						
					}
					else{
						TimeDel=5*TimeDel;
					}
					}

				while(TCNT1<TimeDel) ;

				
				}else{
					TCNT1 = 0;
					while(TCNT1<8192) ;
				}
			}

     }// close brucket to while

	}//close brucket to while(1)
}// close brucket to main

