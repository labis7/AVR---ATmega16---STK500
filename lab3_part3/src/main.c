/*
***	Lab 3  -  Part 3  Control Input ***
 
 * Lab Team  : LAB41140558
 * Authors	 : Gialitakis - Skoufis 
 * Date		 : Thursday 4/4/19 
 * Atmel Studio 7 - Windows 10 Pro
 * AVR Model : STK500 - ATmega16
 
 */ 

#include <stdio.h>
#include <asf.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>

#define F_CPU   8000000
#define BAUD    9600
#define BRC     ((F_CPU/16/BAUD) - 1)
#define BUFFER_SIZE  256
#include <util/delay.h>
#include <avr/iom16.h>

char mytxbuffer[BUFFER_SIZE];
uint8_t txReadPos=0;
uint8_t txWritePos=0;
uint8_t MEM[256];

uint8_t scan_pointer=0;

char myrxbuffer[BUFFER_SIZE];
uint8_t rxReadPos = 0;
uint8_t rxWritePos = 0;


uint8_t flag;
uint8_t Space_num;
uint8_t Number_num;
uint8_t par1;
uint8_t par2;
uint8_t cliflag;
uint8_t state;


void init_serial(void);
void Transmit(char data[],uint8_t x,uint8_t y);
void Receive(void);
void Sendmsg(char* data);



void Check_Input(char data[]);

//Char definitions of the control inputs
//char SPACE[1];
uint8_t SPACE = 32;
char CR[1];
unsigned char USART_Receive(void);


int main (void)
{
	board_init();
	init_serial();
	
	// question 2, accessing RAM
	volatile char *a = (char *)malloc(sizeof(char)*10);
	a=0x0060;
	
	a[0]= 'H';
	a[1] ='e';
	a[2] ='l';
	a[3] ='l';
	a[4] ='o';
	
	sei();
	
	//String copy
	//strcpy(SPACE,"\x20");
	
	strcpy(CR,"\xD"); 

	
	rxReadPos=0;
	rxWritePos=0;
	
	
	/////////
	DDRB |= (1<<DDB0);
	DDRB |= (1<<DDB1);
	DDRB |= (1<<DDB2);
	DDRB |= (1<<DDB3);

	//
	PORTB |= (1<<PORTB0);
	PORTB |= (1<<PORTB1);
	PORTB |= (1<<PORTB2);
	PORTB |= (1<<PORTB3);

	
	
	PORTD = (1 << PORTD3); // Pull-up resistor enabled, (with low level INT1 , when button is pushed -> low)

	//MCUCR Default values (for ICS11, ICS10) --> low profil

	GICR = (1 << INT1);		//External Interrupt Request 1 Enable
	
	state=0;
	cliflag=0;
	while(1){
		if(cliflag == 0)
		{
			sei();
		}
		else
		{
			cli();
			_delay_ms(4000);
			cliflag = 0;
		}
	}
	
	
	Transmit("\n\rGoodbye",0,strlen("\n\rGoodbye"));
	_delay_ms(500);
}





void Sendmsg(char *data){
	if(UCSRA & (1 << UDRE)) //if UDR is empty(no data transfer at the moment)
		UDR = data;
}


//////////////////////////////////////////  TRANSMIT  ///////////////////////////////////////////////////////////////////////////////

void Transmit(char data[],uint8_t x,uint8_t y){
	

	for (uint8_t i = x ; i < y  ; i++ ){
		while(!(UCSRA & (1 << UDRE))) //if UDR is empty(no data transfer at the moment)
		;
		UDR = data[i];
	}

}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



ISR (USART_TXC_vect) { //  Interrupts for completed transmit data
	
}



void Check_Input(char data[]){
	
		flag = 0;
		Space_num = 0;
		
		//process
		//Transmit(myrxbuffer,0,rxWritePos);
		//Checking for AT<CR> command.
		
		if((data[rxReadPos] == 65)&&(data[rxReadPos+1] == 84))  // 65 = "A" , 84 = "T"
		{
			if(data[rxReadPos+2] == CR[0]){
				Transmit("OK\r",0 , strlen("OK\r"));
				rxReadPos = rxWritePos;
			}
			else
				flag = 1;
		}
			//Checking for MW<SP>?<SP>?<CR> and SUM<SP>?<SP>?<CR> command.
		else if((data[rxReadPos] == 77)&&(data[rxReadPos + 1] == 87))		// "M" , "W"
		{
			rxReadPos++;
			while(data[rxReadPos] != CR[0])
			{	
				if(Space_num == 2)
				{
					flag = 1;
					break;
				}

				rxReadPos++;
				if(data[rxReadPos] == SPACE)
				{
					++rxReadPos;
					++Space_num;
				}
				else
				{
					flag = 1;
					break;
				}
				Number_num=0;
				uint16_t k = 0;
				while((Number_num < 3)&&(data[rxReadPos] != CR[0])&&(data[rxReadPos] != 32))
				{
					
					if( (data[rxReadPos] >= 48)&&(data[rxReadPos] <= 57))	 // checking number parameter
					{
						Number_num++;

						k = 10 * k + (data[rxReadPos] - '0');
						rxReadPos++;
					}
					else
					{
						flag = 1;
						break;
					}	
				}

				if((data[rxReadPos] == SPACE))  //the above while has broken bcs of space and we cancel the rxreadpos increase(must be counted in the next loop)
					rxReadPos--;
				if(Number_num == 0)				//if not valid number parameter
				{
					flag = 1;
					break;
				}
				if(k > 255)
				{
					flag = 1;
					break;
				}
				if(Space_num == 1)
					par1 =(uint8_t) k ;
				else if(Space_num == 2)
					par2 =(uint8_t) k ;
				else
					NULL;
			}//WHILE LOOP END
			if((Space_num == 1)||(Space_num == 0)){
				flag = 1;
			}
			
			if (flag != 1)
			{
				MEM[par1]=par2;
				Transmit("OK\r\n",0,strlen("OK\n\r"));
			}
			
		}
		/////////////////////////////////////////// MR ////////////////////////////////////////////////////////////////////
		else if ((data[rxReadPos] == 77)&&(data[rxReadPos + 1] == 82))				// Command : MR		M=77	R=82
		{
			rxReadPos++;
			while(data[rxReadPos] != CR[0])
			{
				if(Space_num == 1)
				{
					flag = 1;
					break;
				}

				rxReadPos++;
				if(data[rxReadPos] == SPACE)
				{
					++rxReadPos;
					++Space_num;
				}
				else
				{
					flag = 1;
					break;
				}
				Number_num=0;
				uint16_t k = 0;
				while((Number_num < 3)&&(data[rxReadPos] != CR[0])&&(data[rxReadPos] != 32))
				{
					
					if( (data[rxReadPos] >= 48)&&(data[rxReadPos] <= 57))	 // checking number parameter
					{
						Number_num++;

						k = 10 * k + (data[rxReadPos] - '0');
						rxReadPos++;
					}
					else
					{
						flag = 1;
						break;
					}
				}

				if((data[rxReadPos] == SPACE))  //the above while has broken bcs of space and we cancel the rxreadpos increase(must be counted in the next loop)
				rxReadPos--;
				if(Number_num == 0)				//if not valid number parameter
				{
					flag = 1;
					break;
				}
				if(k > 255)
				{
					flag = 1;
					break;
				}
				if(Space_num == 1)
				par1 =(uint8_t) k ;
				else
				NULL;
			}//WHILE LOOP END
			if((Space_num == 0)){
				flag = 1;
			}
			
			if (flag != 1)
			{
				par2 = MEM[par1];
				//par2=145;
				char t[3];
				Transmit(itoa( (par2/(100))%(10),t,10),0,strlen(itoa((par2/(100))%(10),t,10)));
				Transmit(itoa( (par2/(10))%(10),t,10),0,strlen(itoa((par2/(100))%(10),t,10)));
				Transmit(itoa( (par2/(1))%(10),t,10),0,strlen(itoa((par2/(100))%(10),t,10)));
				Transmit("\n\r",0,strlen("\n\r"));
			}
			
		}
		//////////////////////////////////////////////// SUM /////////////////////////////////////////////////////
		else if((data[rxReadPos] == 83)&&(data[rxReadPos + 1] == 85) &&(data[rxReadPos + 2] == 77) ){
			rxReadPos+=2;
			while(data[rxReadPos] != CR[0])
			{
				if(Space_num == 2)
				{
					flag = 1;
					break;
				}

				rxReadPos++;
				if(data[rxReadPos] == SPACE)
				{
					++rxReadPos;
					++Space_num;
				}
				else
				{
					flag = 1;
					break;
				}
				Number_num=0;
				uint16_t k = 0;
				while((Number_num < 3)&&(data[rxReadPos] != CR[0])&&(data[rxReadPos] != 32))
				{
					
					if( (data[rxReadPos] >= 48)&&(data[rxReadPos] <= 57))	 // checking number parameter
					{
						Number_num++;

						k = 10 * k + (data[rxReadPos] - '0');
						rxReadPos++;
					}
					else
					{
						flag = 1;
						break;
					}
				}

				if((data[rxReadPos] == SPACE))  //the above while has broken bcs of space and we cancel the rxreadpos increase(must be counted in the next loop)
				rxReadPos--;
				if(Number_num == 0)				//if not valid number parameter
				{
					flag = 1;
					break;
				}
				if(k > 255 && Space_num ==1)
				{
					flag = 1;
					break;
				}
				else if (k > 15 && Space_num ==2)
				{
					flag = 1;
					break;
				}
				if(Space_num == 1)
				par1 =(uint8_t) k ;
				else if(Space_num == 2)
				par2 =(uint8_t)k ;
				else
				NULL;
			}//WHILE LOOP END
			if((Space_num == 1)||(Space_num == 0) || (par1+par2>255)){
				flag = 1;
			}
			
			if (flag!=1)
			{
				uint16_t sum=0;
				for(int i = par1; i<=par2 ; i++)
				{
					sum+=MEM[i];
				}
				char t[4];
				Transmit(itoa( (sum/(1000))%(10),t,10),0,strlen(itoa((par2/(100))%(10),t,10)));
				Transmit(itoa( (sum/(100))%(10),t,10),0,strlen(itoa((par2/(100))%(10),t,10)));
				Transmit(itoa( (sum/(10))%(10),t,10),0,strlen(itoa((par2/(100))%(10),t,10)));
				Transmit(itoa( (sum/(1))%(10),t,10),0,strlen(itoa((par2/(100))%(10),t,10)));
				Transmit("\n\r",0,strlen("\n\r"));
				
			}
		}
		else
			flag = 1;
			
		

	//Transmit("MPIKA",0,strlen("MPIKA"));


	if(flag == 1)        // Error found, break while loop (rxreadps --> CR)
	{					
		rxReadPos = rxWritePos;  
		Transmit("ER\r",0,strlen("ER\r"));
	}
	//char Val[10];
	//Transmit(itoa(par1,Val,16),0,10);


	rxReadPos++;		//Ready for the next command (deixnei sto 1o gramma) 
	

				
}


ISR (USART_RXC_vect) { //  Interrupts : a new element in UDR

	/////// ECHO DRIVER  //////////
	myrxbuffer[rxWritePos] = UDR ;
	
	while(!(UCSRA & (1 << UDRE))) //if UDR is empty(no data transfer at the moment)
	;
	UDR = myrxbuffer[rxWritePos];
	//////////////////////////////
	
	//Flag setting according to input control codes
	if(myrxbuffer[rxWritePos] == CR[0])
		Check_Input(myrxbuffer);
	

	rxWritePos++;
	
	
	if(rxWritePos >= BUFFER_SIZE )
		rxWritePos = 0;
		
}


ISR(INT1_vect)
{
	
	if(cliflag == 0)
	{
		++state;
		if(state >= 5)
		state = 1;

		if(state == 1)
		{
			PORTB |= (1<<PORTB0);
			PORTB ^= (1<<PORTB3);
		}
		else if (state == 2)
		{
			PORTB ^= (1<<PORTB3);
			PORTB ^= (1<<PORTB2);
		}
		else if (state == 3)
		{
			PORTB ^= (1<<PORTB2);
			PORTB ^= (1<<PORTB1);
		}
		else
		{
			PORTB ^= (1<<PORTB1);
			PORTB ^= (1<<PORTB0);
		}
		cliflag = 1;
	}
	//PORTB &= ~(1<<PORTB3);
	

	
}




void init_serial(void){
	// By default URSEL == 0, so we can edit UBRRH regs.
	UBRRH = (unsigned char)(BRC >> 8); //UBRRH has 8 + 4 useful bits
	UBRRL = (unsigned char)BRC;
	
	//UBRRH |= ( 1 << URSEL); // So we can edit UCRSC
	
	UCSRC &= ~(1 << UPM0); //UPM1 is cleared by default
	UCSRC &= ~(1 << UPM1); //UPM1 is cleared by default
	UCSRC &= ~(1 << USBS); // 1 Stop bit

	UCSRC = ((1<<URSEL) | (1 << UCSZ0) | (1 << UCSZ1)) ;
	UCSRB &= ~(1 << UCSZ2); //

	UCSRB |= ((1 << RXEN) | (1 << TXEN)) ;

	UCSRB |= (1 << TXCIE); //TXC interrupts enabled
	UCSRB |= (1 << RXCIE); //RXC interrupts enabled
}



