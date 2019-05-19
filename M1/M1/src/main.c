/*
***	Milestone 1 ***
 
 * Lab Team  : LAB41140558
 * Authors	 : Gialitakis - Skoufis 
 * Date		 : Thursday 11/4/19 
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

//char mytxbuffer[BUFFER_SIZE];

uint8_t scan_pointer=0;

char myrxbuffer[BUFFER_SIZE];
uint8_t rxReadPos = 0;
uint8_t rxWritePos = 0;

uint8_t ITflag=0;
uint8_t EOT=0;
uint8_t enemy_pass;
uint8_t ok_flag;
uint8_t move_done;
uint8_t MyColor;
uint8_t EndGameFlag;
uint8_t Time;
uint8_t mt=0;
volatile uint8_t ILflag = 0;
volatile uint8_t myTurn = 2;

void AnnounceRes(uint8_t res);
void init_timer(void);
void init_leds(void);
void init_serial(void);
void Transmit(char data[],uint8_t x,uint8_t y);
void Receive(void);
void Sendmsg(char* data);
void RST(void);
void Board(void);
void EndGame(void);
uint8_t CheckMove(uint8_t mi, uint8_t my, uint8_t color);

void Algo(void);
void Waiting(void);
void Check_Input(char data[]);

//Char definitions of the control inputs
//char SPACE[1];
uint8_t SPACE = 32;
char CR[1];
unsigned char USART_Receive(void);

volatile uint8_t *M ;



int main (void)
{
	board_init();
	init_serial();
	init_leds();
	// delimiter carriage return
	strcpy(CR,"\xD");

	//Turning off leds(atmega16)
	PORTB |= (1<<PORTB1); 
	PORTB |= (1<<PORTB2);
	PORTB |= (1<<PORTB3);
		

	//Game board initialization
	M= (uint8_t *)malloc(sizeof(uint8_t)*64);
	
	volatile uint8_t i=0;
	volatile uint8_t y = 0 ;
	for(i = 0 ; i <= 7 ; i++)
	{
		for(y = 0 ; y <= 7 ; y++)
		{
			M[8*i + y] = (uint8_t)2 ; // 0 == black , 1 == white, 2 == empty
		}
	}
	//[ x , y ] ? [ X*8 + Y ](AVR), [ NUM ](AVR) ? [ (NUM)div8 , (NUM)mod8 ]
	M[3*8+3] = 1 ;
	M[3*8+4] = 0 ;
	M[4*8+3] = 0 ;
	M[4*8+4] = 1 ;

	

	//////////////////////////////////////////////////////////////////////
	

	//buffer pointers init
	rxReadPos=0;
	rxWritePos=0;
	//flag Initialization 
	ITflag=0;
	ILflag =0;
	move_done=0;
	myTurn=2;
	MyColor = 1;
	

	sei();
	
	
	while(1){


		//Waiting for PC response - (ILLIGAL request)
		if(ILflag >= 1)
		{
			//Transmit("Waiting OK for announcement\r",0,"Waiting OK for announcement\r");
			_delay_ms(10);
			while(1){		
				if(ILflag == 2){ // we received the response we were waiting
					ILflag = 0;
					break;
				}
			}
			if(myrxbuffer[rxReadPos] == 79 && myrxbuffer[rxReadPos+1] == 75){ // "ok"
			   rxReadPos=rxWritePos;
				AnnounceRes(1); //WIN - LED1
				myTurn=2;
				
			
			}//future update : "else ILflag=1;" , wrong input avoidance

			if(myrxbuffer[rxReadPos] == 80 && myrxbuffer[rxReadPos+1] == 76){ //"PL"
				if(ITflag==1)
				{
					ITflag = 0;
					init_timer();
					
				}
				else
				{
					Transmit("QT\r",0 , strlen("QT\r"));
					myTurn=2;
				}						
					
			}	//future update : "else ILflag=1;" , wrong input avoidance		

			rxReadPos=rxWritePos;
		}
		
		
		if((myTurn == 0)&&(mt == 0)){
			//Possible speculation algo
			mt = 1;
			init_timer();
			myTurn = 0;
		}
			
		
		if(myTurn==1){ //When its avr's turn
			mt = 0; //reset flag for HIS turn			
			init_timer(); //reset timer
			myTurn = 1;  //Important - collision with  init_timer
			Algo();		//The actual algorithm
		}

	}

}
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////



// RESET FUNCTION: initializing game board and turning off leds
void RST(void)
{
	
	ILflag =0;
	move_done=0;
	myTurn=2;
	PORTB |= (1<<PORTB1);
	PORTB |= (1<<PORTB2);
	PORTB |= (1<<PORTB3);

	mt = 0; //flag reset
	myTurn=2;
	//Resetting board 
	for(uint8_t i = 0 ; i <= 7 ; i++)
	{
		for(uint8_t y = 0 ; y <= 7 ; y++)
		{
			M[8*i + y] = 2 ; // 0 == black , 1 == white, 2 == empty
		}
	}
	M[3*8+3] = 1 ;
	M[3*8+4] = 0 ;
	M[4*8+3] = 0 ;
	M[4*8+4] = 1 ;
	enemy_pass = 0;

	TCCR1B = 0x00;         //Clearing timer -- total reset
	TIMSK = (1 << TOIE1) ; //Overflow interrupts -- disabled
	Transmit("OK\r",0 , strlen("OK\r"));
	rxReadPos = rxWritePos;
	
}



//////////////////////////////////////////////////////////////////////////// ALGORITHM   /////////////////////////////////////////////////////////////////////////////
void Algo(void)
{
	uint8_t mi,my,i,j,u,z,ibar,ybar,skip,istep,ystep;
	myTurn = 1;		//Important - collision with  init_timer
	
	char mymove[6];
	
	//The next 2 for loop are responsible to find(scanning whole board) our pawns
	for(mi=0;mi<=7;mi++)
	{
		for(my=0; my<=7; my++)
		{
			 // we found one of our pawns, so we start to check potential move 
			if(M[mi*8+my] == MyColor)  
			{					
				//The next 2 for loops find scanning the 8 slots/positions around the pawn we found.
				for(i = mi - 1; i<=(mi+1); ++i)
				{
					if(i<0||i>7) //Matrix out of border protection 
						continue;
					for(j = my - 1; j<=(my+1); ++j)
					{
						if(j<0||j>7) //Matrix out of border protection 
							continue;

						//our pawn has en enemy pawn adjacent to it,
						//so we will find the borders+direction we need to search
						if((M[i*8 + j] == !MyColor))		
						{

							//Setting up i barrier (board)
							if(i > mi)
							ibar = 7 ;
							else if(i == mi)
							ibar = 10;   //big enough, so the other axis will break while below
							else
							ibar = 0;
							
							//Setting up y barrier
							if(j > mi)
							ybar = 7;
							else if(j == my)
							ybar = 10;
							else
							ybar = 0;

							//setting up steps, (for the loops)
							istep = i - mi;
							ystep = j - my;
							
							//start from the accepted neighbor
							u=i;
							z=j;
							


							skip = 1;//reset

							//Now,its gonna find a valid path(which will get colorized accordingly)
							while((u != (ibar+istep))&&(z != (ybar+ystep))) 
							{
								if(M[u*8 + z] == MyColor ) //Break, because we need empty slot(to put our move in it)
								{
									break;
								}
								
								if( M[u*8 + z] == 2){			//We found a valid empty slot
									skip = 0;
									CheckMove(u, z, MyColor);  //Coloring adjacent paths, according to the rules
									//Building message
									mymove[0] = 'M';
									mymove[1] = 'M';
									mymove[2] = '\x20';
									mymove[3] = u+65;
									mymove[4] = (z+1)+'0';
									mymove[5] = '\r';
										
									break;
								}

								z+= ystep;
								u+= istep;
							}
							//if a solution is found
							if(!skip) 
							{
								u=i;
								z=j;
								//Same iteration, this time we color the path
								while((u!=ibar)&&(z!=ybar))//barriers will never get reached.
								{
									if(M[u*8 + z] == MyColor )
										break;
									M[u*8 + z] = MyColor;
									
									z+= ystep;
									u+= istep;
								}
								
								move_done=1;
								Board(); //Board Visualization
								Transmit(mymove,0,6); //Transmit our Move
								if(move_done)
									break;
							}
							
						}//if check neighbors
					}//j for
					if(move_done)
						break;
				}//i for
			}//if  (find our pawn)
			if(move_done)
				break;
		}//for my
		if(move_done)
			break;
	}//for mi

	
	
	//if move_done == 0 , that means that we cant find solution, we pass
	

	//while loop until 'OK' response
	while(1)
	{		// Check_Input does not support "OK" response so we check it here
		if(move_done >= 1)
		{
			while(1)
			{
				_delay_ms(10);
				if((move_done == 2)||(myTurn==2))  //myturn==2 means that a RST was made. and we can break this loop.
				{ // we received the response we were waiting'
					move_done = 0;
					break;
				}
			}
			
			if(myrxbuffer[rxReadPos] == 79 && myrxbuffer[rxReadPos+1] == 75) //Respone ok for our MM
			{
				init_timer();
				rxReadPos=rxWritePos;
				myTurn = 0;
				break;
			}

		}
		else if(move_done == 0)//No solution was found
		{
			if (enemy_pass == 1) //Enemy sent pass too
			{
				EndGame();		//Go count pawns and announce the winner
				break;
			}
			else
			{
				Transmit("MP\r",0,strlen("MP\r")); //Transmit MYPASS
				break;
			}
		}
	}
	
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////




/////////////////////////////////////////////////////////////////////////////  CHECK MOVE  /////////////////////////////////////////////////////////////////////
//Similar to Algo (less comments)
//Given a slot, it can find all the possible paths that must be colorized(It also returns Legal/Illegal flag)

uint8_t CheckMove(uint8_t mi,uint8_t my,uint8_t color)
{
	volatile int i,j;
	uint8_t u,z,found,ibar,ybar,skip,istep,ystep;
	char mymove[6];

	
	found = 0;//init before main loop
	for(i = mi - 1; i<=(mi+1); ++i)
	{
		if(i<0||i>7)
			continue;
	  for(j = my - 1; j<=(my+1); ++j)		
	  {
		if(j<0||j>7)
			continue;  
		//checking neighbors, we are trying to find opponents color
	    if((M[i*8 + j] == color)||(M[i*8 + j] == 2)) 
			;
		else //opponents color was found
		{	
			//Setting up i barrier
			if(i > mi)
				ibar = 7 ;
			else if(i == mi)
				ibar = 9;
			else
				ibar = 0;
			
			//Setting up y barrier
			if(j > mi)
			ybar = 7;
			else if(j == my)
			ybar = 9;
			else
			ybar = 0;	

			//setting up steps, (for the loops)
			istep = i - mi; 
			ystep = j - my;
			//start from the accepted neighbor
			u=i; 
			z=j;

			skip = 1;
			//Now,its gonna find a valid path(which will get colorized accordingly)
			while((u != (ibar+istep))&&(z != (ybar+ystep)))
			{
				//check	
				if( M[u*8 + z] == 2)
					break;
					
				//IF in the line we are checking, we find a friendly pawn, then the move is legal se place the new pawn 
				if(M[u*8 + z] == color )	
				{ 
					
					M[mi*8 + my] = color;
														
					//mark that it is a legal move
					found = 1;
					skip = 0;
					break;
				}
	    		z+= ystep;
				u+= istep;
			
			}			
			
			if(!skip) //if a solution is found
			{
				u=i;
				z=j;
				while((u!=ibar)&&(z!=ybar))//barriers will never get reached.
				{
					if((M[u*8 + z] == 2)||(M[u*8 + z] == color ))
						break;

					M[u*8 + z] = color;										
					z+= ystep;
					u+= istep;
				}
			}
		
         }//if check
	  }	  //y for	
	}	  //x for
	if(found == 1)
		return 1;//Legal
	return 0;	
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//Visualization of the board
void Board(){
	char mymove[6];
	uint8_t s1,s2;
	Transmit("\x20",0,1);
	for(s1=0 ; s1<8 ; s1++)
	{
		mymove[0] = '|';
		mymove[1] = '\x20';
		mymove[2] = s1+1+'0';
		mymove[3] = '\x20';
		Transmit(mymove,0,4);
	}
	for(s1=0 ; s1<8 ; s1++)
	{
		Transmit("\n\r",0,strlen("\n\r"));
		mymove[0] = s1+65;
		Transmit(mymove,0,1);
		for(s2=0; s2<8; s2++)
		{
			mymove[0] = '|';
			mymove[1] = '\x20';
			mymove[2] = M[s1*8 + s2]+'0';
			mymove[3] = '\x20';
			Transmit(mymove,0,4);
		}
	}
	Transmit("\n\r",0,strlen("\n\r"));
}


/////////////////////////////////////////////////////////////////////////////////////// END GAME & ANNOUNCEMENT /////////////////////////////////////////////////////////////////////

//EG instruction function, blacks and whites counting, and it will announce the winner
void EndGame(){
	uint8_t b=0;
	uint8_t w=0;

	for(uint8_t i = 0 ; i <= 7 ; i++)
	{
		for(uint8_t y = 0 ; y <= 7 ; y++)
		{
			if(M[i*8 + y] == 0)  // 0 == black , 1 == white, 2 == empty
			b++;
			if(M[i*8 + y] == 1)
			w++;
		}
	}
	if(b == w)
	{
		AnnounceRes(2);	//TIE -LED3

	}
	else if(b>w)
	{
		if(MyColor == 0) //black
		AnnounceRes(1);//WIN - LED1
		else
		AnnounceRes(0); //LOST -LED2
	}
	else
	{
		if(MyColor == 1) //white
		AnnounceRes(1);//WIN - LED1
		else
		AnnounceRes(0); //LOST -LED2
	}

	//after announcement wait for ok in while loop(set move_done = 1)
	myTurn = 2;
}



void AnnounceRes(uint8_t res)
{
	//after announcement, timer interrupts are disabled.
	TIMSK &= ~(1 << TOIE1) ;
	if(res == 1)
	{
		Transmit("WN\r",0,strlen("WN\r"));
		PORTB ^= (1<<PORTB1);             //Toggle LED
	}
	else if(res == 0)
	{
		Transmit("LS\r",0,strlen("LS\r"));
		PORTB ^= (1<<PORTB2);			//Toggle LED
	}
	else
	{
		Transmit("TE\r",0,strlen("TE\r"));
		PORTB ^= (1<<PORTB3);			//Toggle LED
	}
	TCCR1B = 0x00;         //Clearing timer -- total reset
	TIMSK = (1 << TOIE1) ; //Overflow interrupts -- disabled

}





///////////////////////////////////////////////////////////////////////////////////////////   CHECK_INPUT    /////////////////////////////////////////////////////////////////////////////////////////

void Check_Input(char data[]){
	
		if(data[rxReadPos]==CR[0]){
			rxReadPos++;
		}
		//flag = 0;
		//Space_num = 0;
		

		//ILFlag will help us spot "OK" or "PL" terminal answer
		if(ILflag == 1)
		{
			ILflag=2;  //answer spotted, you can proceed.
			return;	
		}
		
		//it will help to spot "OK"  terminal answer
		if(move_done == 1)
		{	
			move_done=2; //answer spotted, you can proceed.
			return;	
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

		}
		//Checking for RST<CR> command.
		else if((data[rxReadPos] == 82)&&(data[rxReadPos + 1] == 83)&&(data[rxReadPos + 2] == 84))		
		{
			RST();
		}	
		// SP<SPACE>{B/W}<CR>
		else if((data[rxReadPos] == 83)&&(data[rxReadPos + 1] == 80))
		{
			//(int)c - 65;
			if(data[rxReadPos + 3] == 66)		//B
				MyColor = 0;					//Saving myColor
			else if(data[rxReadPos + 3] == 87)  //W
				MyColor = 1;					//Saving myColor
			Transmit("OK\r",0 , strlen("OK\r"));
			rxReadPos = rxWritePos;
		}
		    //NG  ---- NEW GAME ------
		else if((data[rxReadPos] == 78)&&(data[rxReadPos + 1] == 71))
		{
			//RST();
			move_done=0;
			for(uint8_t i = 0 ; i <= 7 ; i++)
			{
				for(uint8_t y = 0 ; y <= 7 ; y++)
				{
					M[8*i + y] = (uint8_t)2 ; // 0 == black , 1 == white, 2 == empty
				}
			}
			//[ x , y ] ? [ X*8 + Y ](AVR), [ NUM ](AVR) ? [ (NUM)div8 , (NUM)mod8 ]
			M[3*8+3] = 1 ;
			M[3*8+4] = 0 ;
			M[4*8+3] = 0 ;
			M[4*8+4] = 1 ;
			if(MyColor == 0)// BLACK
			{
				myTurn=1;
				Transmit("OK\r",0 , strlen("OK\r"));
			}
			else           //WHITE
			{
				RST();  //FIXING bug with whites(rst contains OK Transmit)
				myTurn=0;
			}
			
			//init_timer();
			//Transmit("OK\r",0 , strlen("OK\r"));           This is the 2nd OK, not important, RST contains OK transmit
			rxReadPos = rxWritePos;
		}
		        //EG<CR>
		else if((data[rxReadPos] == 69)&&(data[rxReadPos + 1] == 71))
		{
			//EndGameFlag = 1; // check this flag during waiting/calculating loop
			Transmit("OK\r",0 , strlen("OK\r"));
			rxReadPos = rxWritePos;
			EndGame();
		}
			 //ST<CR>
        else if((data[rxReadPos] == 83)&&(data[rxReadPos + 1] == 84))
        {
			Time = data[rxReadPos+3] - '0';    //String to Int
			Transmit("OK\r",0 , strlen("OK\r"));
			rxReadPos = rxWritePos;
        }
			//MV<SP>{[A-H],[1-8]}<CR>
		else if((data[rxReadPos] == 77)&&(data[rxReadPos + 1] == 86))
		{
				if((data[rxReadPos+3] >= 65)&&(data[rxReadPos+3] <= 72)&&(data[rxReadPos+4] >= 49)&&(data[rxReadPos+4] <= 56))  // Checking input
				{
					enemy_pass = 0;
					uint8_t moveok = CheckMove(((int)data[rxReadPos+3] - 65),((data[rxReadPos+4] - '0') - 1), !MyColor );  //Check opponents move.
					//If opponent's move is legal, send ok and reset timer, else 
					//send IL and wait for PC response, if response OK --> I win else(PL) --> I LOSE
					if(moveok == 1)		
					{
						// Saving opponent's move in my local game board
						//M[(((int)data[rxReadPos+3] - 65)*8) + (data[rxReadPos+4] - '0')] = !MyColor;  // Saving opponent's move in my local game board
						Transmit("OK\r",0 , strlen("OK\r"));
						init_timer();
						myTurn=1;
					}
					else{
						Transmit("IL\r",0 , strlen("IL\r"));
						ILflag =  1;							//Waiting mode for PC's response
						myTurn=2;
						mt=0;
					}
					rxReadPos = rxWritePos;
						
				} 	
		}

		//PASS
		else if ((data[rxReadPos] == 80)&&(data[rxReadPos + 1] == 83))
		{
			myTurn=1;
			//This flag will help us end game in case we pass after opponent's pass
			enemy_pass = 1;   
			Transmit("OK\r",0,strlen("OK\r"));
		}
		//WN

		else if((data[rxReadPos] == 87)&&(data[rxReadPos + 1] == 78)){
			AnnounceRes(1);  //announce i win with led1
			myTurn=2;			
			Transmit("OK\r",0,strlen("OK\r"));
		}
		else
			NULL;
	rxReadPos=rxWritePos;		
	rxReadPos++;		//Ready for the next command (directs to the next letter, the one after <CR>) 
	

				
}

//////////////////////////////////////////////////////////////////////////////////////////



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



ISR (USART_TXC_vect) { //  Interrupts for completed transmit data
}



 ISR (TIMER1_OVF_vect)    // Timer1 ISR
 {
	 if(myTurn==1){
		 // coming soon 
		 //last sec MM will save the day..eventually.		 
		 myTurn =0;
		// move_done = 1;
	 } 
	 else if(myTurn == 0){
		  Transmit("IT\r",0,strlen("IT\r"));
		  ITflag = 1;
		  ILflag = 1;
	 }	 
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



 //////////////////////////////////////////////// INITIALIZATION ///////////////////////////////////////////////////////////

 void init_leds()
 {
	DDRB |= (1<<DDB1);
	DDRB |= (1<<DDB2);
	DDRB |= (1<<DDB3);
	PORTB |= (1<<PORTB1);
	PORTB |= (1<<PORTB2);
	PORTB |= (1<<PORTB3);
 }

void init_timer(){
	//cli();

	 //////////Timer/Counter Initialization/////////
	 /* Timer starts from a specific value, 
		so we can take advantage of ISR
	 */
	TCNT1 = 3036;//2SECONDS // 34286;//49911  //2^16 = 65536 - (8,000,000/256) 
	TCCR1A = 0x00; //Default - Cleared

	/*	The CLK/64 
	*/
	//TCCR1B &=  ~(1<<CS11);  
	
	TCCR1B |=  (1<<CS12);// |(1<<CS10);
	TIMSK = (1 << TOIE1) ;   // Enable timer1 overflow interrupt(TOIE1)
	//sei();        // Enable global interrupts by setting global interrupt enable bit in SREG
	//sleep();
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