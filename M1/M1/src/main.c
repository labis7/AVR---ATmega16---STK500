/*
***	Lab 3 ***
 
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
uint8_t MyColor;
uint8_t EndGameFlag;
uint8_t Time;

void init_serial(void);
void Transmit(char data[],uint8_t x,uint8_t y);
void Receive(void);
void Sendmsg(char* data);
void RST(void);
uint8_t CheckMove(void);

void Algo(void);
void Waiting(void);
void Check_Input(char data[]);

//Char definitions of the control inputs
//char SPACE[1];
uint8_t SPACE = 32;
char CR[1];
unsigned char USART_Receive(void);

volatile uint8_t *M = (uint8_t *)malloc(sizeof(uint8_t)*64);



int main (void)
{
	board_init();
	init_serial();
	
	// question 2, accessing RAM and determine the position in memory, where the data will be stored.

	//Game board initialization
	
	
	for(uint8_t i = 0 ; i <= 7 ; i++)
	{
		for(uint8_t y = 0 ; y <= 7 ; y++)
		{
			M[i + 8*y] = 2 ; // 0 == black , 1 == white, 2 == empty
		}
	}

	M[3,3] = 1 ;
	M[3,4] = 0 ;
	M[4,3] = 0 ;
	M[4,4] = 1 ;


	
	// question 1 and 3
	sei();
	













	//////////////////////////////////////////////////////////////////////
	// delimiter carriage return
	strcpy(CR,"\xD"); 

	// Initialization of pointers for buffer
	rxReadPos=0;
	rxWritePos=0;
	
	






	
	Transmit("\n\rGoodbye",0,strlen("\n\rGoodbye"));
	_delay_ms(500);
}




/*
* This function transmits a single byte to the terminal
*/
void Sendmsg(char *data){
	if(UCSRA & (1 << UDRE)) //if UDR is empty(no data transfer at the moment)
		UDR = data;
}


// TRANSMIT function : transmits a string

void Transmit(char data[],uint8_t x,uint8_t y){
	

	for (uint8_t i = x ; i < y  ; i++ ){
		while(!(UCSRA & (1 << UDRE))) //if UDR is empty(no data transfer at the moment)
		;
		UDR = data[i];
	}

}


void RST(void)
{
	for(uint8_t i = 0 ; i <= 7 ; i++)
	{
		for(uint8_t y = 0 ; y <= 7 ; y++)
		{
			M[i + 8*y] = 2 ; // 0 == black , 1 == white, 2 == empty
		}
	}
	M[3,3] = 1 ;
	M[3,4] = 0 ;
	M[4,3] = 0 ;
	M[4,4] = 1 ;
	Transmit("OK\r",0 , strlen("OK\r"));
}


void Waiting()
{
	//TIMER SET 
	//WHILE LOOP 
}

void Algo()
{
	//TIMER SET
	//calculating


	//send MOVE
	//while loop until 'OK' response
	//call Waiting
}


uint8_t CheckMove()
{
	
}


ISR (USART_TXC_vect) { //  Interrupts for completed transmit data	
}


/*
*	This function detects and execute the commands
*   list of commands: 
*	AT : XOFF/XON protocol
*	MW : memory write
*	MR : memory read
*	SUM: sum the certain data from memory  
*/

uint8_t ILflag = 0;

void Check_Input(char data[]){
	
		flag = 0;
		Space_num = 0;
		

		//Waiting for PC response - (ILLIGAL request)
		if(ILflag == 1)
		{
			if("OK")
			 //I WIN	
			if("PL")
			//I LOSE	 
		}



		//process
		// command AT
		if((data[rxReadPos] == 65)&&(data[rxReadPos+1] == 84))  // 65 = "A" , 84 = "T"
		{
			//Checking if user finished entering the command by checking if he typed the <CR> character.
			if(data[rxReadPos+2] == CR[0]){
				Transmit("OK\r",0 , strlen("OK\r"));
				rxReadPos = rxWritePos;
			}
			else
				flag = 1;
		}
		//Checking for RST<CR> command.
		else if((data[rxReadPos] == 82)&&(data[rxReadPos + 1] == 83)&&(data[rxReadPos + 2] == 84))		
		{
			RST();
			rxReadPos = rxWritePos; //
		}	
						//// SP<SPACE>{B/W}<CR>
		else if((data[rxReadPos] == 83)&&(data[rxReadPos + 1] == 8O))
		{
			//(int)c - 65;
			if(data[rxReadPos + 3] == 66) //B
				MyColor = 0;
			else if(data[rxReadPos + 3] == 87)  //W
				MyColor = 1;
			Transmit("\r",0 , strlen("\r"));
			rxReadPos = rxWritePos;
		}
		    //NG  ---- NEW GAME ------
		else if((data[rxReadPos] == 78)&&(data[rxReadPos + 1] == 71))
		{
			if(MyColor == 0)// BLACK
				Algo();
			else           //WHITE
				Waiting();
			rxReadPos = rxWritePos;
		}
		        //EG<CR>
		else if((data[rxReadPos] == 69)&&(data[rxReadPos + 1] == 71))
		{
			EndGameFlag = 1; // check this flag during waiting/calculating loop
			Transmit("OK\r",0 , strlen("OK\r"));
			rxReadPos = rxWritePos;
		}
			 //ST<CR>
        else if((data[rxReadPos] == 83)&&(data[rxReadPos + 1] == 84))
        {
			Time = data[rxReadPos] - '0'; //
			Transmit("OK\r",0 , strlen("OK\r"));
			rxReadPos = rxWritePos;
        }
			//MV<SP>{[A-H],[1-8]}<CR>
		else if((data[rxReadPos] == 77)&&(data[rxReadPos + 1] == 86))
		{
				if((data[rxReadPos+3] >= 65)&&(data[rxReadPos+3] <= 72)&&(data[rxReadPos+3] >= 49)&&(data[rxReadPos+4] <= 56))
				{
					uint8_t moveok = CheckMove();
					if(moveok == 1)
					{
						M[(((int)data[rxReadPos+3] - 65)*8) + (data[rxReadPos+4] - '0')] = !MyColor;
						Transmit("OK\r",0 , strlen("OK\r"));
					}
					else
						Transmit("IL\r",0 , strlen("IL\r"));
						while(1)
						{
							ILflag = 1;
						}
				} 	
		}
		else
			flag = 1;
			
		

	if(flag == 1)        // Error found, break while loop (rxreadps --> CR)
	{					
		rxReadPos = rxWritePos;  
		Transmit("\nER\n\r",0,strlen("\nER\n\r"));
	}

	rxReadPos++;		//Ready for the next command (directs to the next letter) 
	

				
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

/*
*   External interrupt handler
*	When the button is pushed, the next led will turn on while the previous one	will turn off
*/

ISR(INT1_vect)
{
	// prevent ring progress until we disable the GICR (external interrupt)
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


