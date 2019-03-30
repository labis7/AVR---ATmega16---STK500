/*
***	Lab 3  -  Part 3  Control Input ***
 
 * Lab Team  : LAB41140558
 * Authors	 : Gialitakis - Skoufis 
 * Date		 : Wednesday 20/3/19 
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

uint8_t scan_pointer=0;

char myrxbuffer[BUFFER_SIZE];
uint8_t rxReadPos = 0;
uint8_t rxWritePos = 0;

uint8_t SpecialInputFlag=-1;
uint8_t flag;
uint8_t Space_num;
uint8_t Number_num;
uint8_t par1;
uint8_t par2;

void init_serial(void);
void Transmit(char data[],uint8_t x,uint8_t y);
void Receive(void);
void Sendmsg(char* data);

/*
Normal_program contains the output gen. of the sequence 0-9A-z 
In this function there is some logic to handle the special 
control input(like pause,resume and end program).
*/
void Normal_program();

//We call this function in order to pause the program
void Pause_func();

void Check_Input(char data[]);

//Char definitions of the control inputs
char SPACE[1];
char CR[1];
char Pause_Code[1];
char Resume_Code[1];
char Stop_Code[1];
unsigned char USART_Receive(void);


int main (void)
{
	board_init();
	init_serial();
	
	sei();
	
	//String copy
	strcpy(SPACE,"\x20");
	strcpy(CR,"\xD"); 
	strcpy(Pause_Code,"\x13"); // ^S	
	strcpy(Resume_Code,"\x11"); //^Q
	strcpy(Stop_Code,"\x03");	   // ^C

	//SpecialInputFlag will help at specifying
	//the state of the program according the last input code.
	SpecialInputFlag = -1;
	/*
	while(1){
		Transmit(myrxbuffer,rxReadPos,rxWritePos); //ECHO
		_delay_ms(10);
		if(strcmp(Start_Code[0],(char*)myrxbuffer[rxReadPos-1]) == 0)
			break;
	}


	*/
	//after the 'T' input has been received, we can start the normal program.
	rxReadPos=0;
	rxWritePos=0;
	while(1){
		;
	}
	
	
	//Normal_program();
	
	Transmit("\n\rGoodbye",0,strlen("\n\rGoodbye"));
	_delay_ms(500);
}


void Normal_program(){

	while(1)
	{		
		for (uint8_t i=48 ; i <= 90 ; i++){
			
			////// CHECK INPUT //////
			//(INPUT is changed at RXC interrupt)
			if (SpecialInputFlag == 0)// code 0 = ^S
				Pause_func();
			else if(SpecialInputFlag == 1) // code 1 = ^Q
				NULL;	//Do nothing
			else if(SpecialInputFlag == 2) // code 2 = ^C
				return;
			else NULL;	//Do nothing
					
			if(i == 58 )
				i=65;// jump to the right ASCII Code
			Sendmsg((char)i); //This func is used(easier),because 1 char per time is sent
			_delay_ms(500);
		}
		
	}
}

void Pause_func(){
	
	while(1){
	Sendmsg('\0');
	_delay_ms(2);
		//if input means 'resume'
		if(SpecialInputFlag == 1)
		{
			//Reset flag,so we can continue without 
			//a problem and return back to normal program		
			SpecialInputFlag = -1;
			return;
		}
	    if(SpecialInputFlag == 2)//end of program input
	    {
			Transmit("\n\rGoodbye",0,strlen("\n\rGoodbye"));
			_delay_ms(15);
			//Disable transmit(See report for the reason)
			UCSRB &= ~(1 << TXEN); 
		    return;
	    }
	}
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
	//if(UCSRA & (1 << UDRE)) //if UDR is empty(no data transfer at the moment)
	//	UDR = 0; 
	//rxReadPos = rxWritePos;            //          CARE!!!!!! mporei ligo prohgoumenws na erthei interrupt 
	
/*
	for (uint8_t i = x ; i < y  ; i++ ){
		mytxbuffer[txWritePos] = data[i];
		txWritePos++;
	}
	if(txWritePos >= BUFFER_SIZE)
		txWritePos = 0;
	rxReadPos = rxWritePos; // Transmit Received Data
	if(UCSRA & (1 << UDRE)) //if UDR is empty(no data transfer at the moment)
		UDR = 0;			//Start with empty data, so interrupts can handle the remaining items in mytxbuffer
*/
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////



ISR (USART_TXC_vect) { //  Interrupts for completed transmit data
	/*
	if(txWritePos != txReadPos)
	{
		UDR = mytxbuffer[txReadPos];
		txReadPos++;
	}
	if(txReadPos >= BUFFER_SIZE)
		txReadPos = 0;
		*/
}



void Check_Input(char data[]){
	
	//while(data[rxReadPos] != CR[0]){
		//
		flag=0;
		Space_num = 0;
		Number_num=0;
		//process
		Transmit(myrxbuffer,0,rxWritePos);
		//Checking for AT<CR> command.
		/*
		if((data[rxReadPos] == 65)&&(data[rxReadPos+1] == 84))  // 65 = "A" , 84 = "T"
		{
			Transmit("MPIKA",0,strlen("MPIKA"));
			if(data[rxReadPos+2] == CR[0]){
				Transmit("OK\r",0 , strlen("OK\r"));
				rxReadPos = rxWritePos;
			}
			else
				flag = -1;
		}
			//Checking for MW<SP>?<SP>?<CR> and SUM<SP>?<SP>?<CR> command.
		else if((data[rxReadPos] == 77)&&(data[rxReadPos + 1] == 87))		// "M" , "W"
		{
			while(data[rxReadPos] != CR[0])
			{	
				if(Space_num == 2)
				{
					flag=-1;
					break;
				}

				++rxReadPos;
				if(data[rxReadPos] == SPACE[0])
				{
					++rxReadPos;
					++Space_num;
				}
				else
				{
					flag=-1;
					break;
				}

				uint8_t k = 0;
				while((Number_num < 3)&&(data[rxReadPos] != CR[0])&&(data[rxReadPos] != SPACE[0]))
				{
					
					if( (data[rxReadPos] >= 48)&&(data[rxReadPos] <= 57))	 // checking number parameter
					{
						Number_num++;

						k = 10 * k + (data[rxReadPos]);
						Transmit(data[rxReadPos],0,1);
						rxReadPos++;
					}
					else
					{
						flag = -1;
						break;
					}	
				}

				if((data[rxReadPos] == SPACE[0]))  //the above while has broken bcs of space and we cancel the rxreadpos increase(must be counted in the next loop)
					rxReadPos--;
				if(Number_num == 0)				//if not valid number parameter
				{
					flag = -1;
					break;
				}
				if(k > 255)
				{
					flag = -1;
					break;
				}
				if(Space_num == 1)
					par1 = k;
				else if(Space_num == 2)
					par2 = k;
				else
					NULL;
			}//WHILE LOOP END
			if((Space_num == 1)||(Space_num == 0))
				flag = -1;
		}
		else
			flag = -1;




	if(flag == -1)        // Error found, break while loop (rxreadps --> CR)
	{					
		rxReadPos = rxWritePos;  
		Transmit("ER\r",0,strlen("ER\r"));
	} 


	rxReadPos++;		//Ready for the next command (deixnei sto 1o gramma) 
	*/

				
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
	/*
	if(strcmp(myrxbuffer[rxWritePos],Pause_Code[0]) == 0) //^S
		SpecialInputFlag = 0;
	else if (strcmp(myrxbuffer[rxWritePos],Resume_Code[0]) == 0) //^Q
		SpecialInputFlag = 1;
	else if (strcmp(myrxbuffer[rxWritePos],Stop_Code[0]) == 0) //^C
		SpecialInputFlag = 2;
	else
		SpecialInputFlag = -1;
	*/	
	
	
	
	if(rxWritePos >= BUFFER_SIZE )
		rxWritePos = 0;
		
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
