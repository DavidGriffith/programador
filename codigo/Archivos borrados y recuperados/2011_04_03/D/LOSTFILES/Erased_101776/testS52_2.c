#define TEST_S52

#ifdef TEST_S52

#include <avr/pgmspace.h>
//#include <avr/interrupt.h>
#include "def.h"
#include "usb.h"
#include "macros.h"
#include "usbtinyMod.h"
#include "spi.h"


byte_t programEnable_Cmm[] = {0xAC,0x53,0x00,0xFF};
byte_t chipErase_Cmm[]     = {0xAC,0x80,0x00,0x00};
byte_t writeByte_Cmm[]     = {0x40,0x00,0x00,0x00};
byte_t readByte_Cmm[]      = {0x20,0x00,0x00,0x00};
byte_t writePage_Cmm[]     = {0x50,0x00,0x00,0x00};
byte_t readPage_Cmm[]      = {0x03,0x00,0x00,0x00};

#define TEST_LED_1_SEG
//#define TEST_LED_05_SEG

#ifdef TEST_LED_05_SEG

byte_t led_program[] = {
0xB2,
0x80,
0x11,
0x20,
0x80,
0xFA,          
0x7E,
0x08,
0x7A,
0xFF,
0x7B,
0xFF,
0xDB,
0xFE,
0xDA,
0xFA,
0xDE,
0xF6,
0x22,              
0x7E,
0x10,
0x7A,
0xFF,
0x7B,
0xFF,
0xDB,
0xFE,
0xDA,
0xFA,
0xDE,
0xF6,
0x22,          
0x7E,
0x04,
0x7A,
0xFF,
0x7B,
0xFF,
0xDB,
0xFE,
0xDA,
0xFA,
0xDE,
0xF6,
0x22,
};

#elif defined(TEST_LED_1_SEG)

byte_t led_program[] = {
0xB2,
0x80,
0x11,
0x06,
0x80,
0xFA,          
0x7E,
0x08,
0x7A,
0xFF,
0x7B,
0xFF,
0xDB,
0xFE,
0xDA,
0xFA,
0xDE,
0xF6,
0x22,              
0x7E,
0x10,
0x7A,
0xFF,
0x7B,
0xFF,
0xDB,
0xFE,
0xDA,
0xFA,
0xDE,
0xF6,
0x22,          
0x7E,
0x04,
0x7A,
0xFF,
0x7B,
0xFF,
0xDB,
0xFE,
0xDA,
0xFA,
0xDE,
0xF6,
0x22,
};

#else error("NO CODE TO PROGRAM")
#endif


/* Sends one byte over SPI.
   Returns the data in.
*/

void Delay100uS()
{
    uint_t i, limite;
    limite = (100 / sck_period);
    for (i = 0;i < limite;i++)
        delay();
}

void Delay60mS()
{
    uint_t i, limite;
    limite = (60000 / sck_period);
    for (i = 0;i < limite;i++)
        delay();
}

void Delay320mS()
{
    uint_t i, limite;
    limite = (320 / 60);
    for (i = 0;i < limite;i++)
        Delay60mS();
}


static byte_t spiOneByte(byte_t dataOut)
{
	byte_t	mask, j;
	byte_t  dataIn = 0;    

	for	( mask = 0x80; mask; mask >>= 1 )
	{


        if	( dataOut & mask )
        {
    	    SET_MOSI;            
        }
        else
        {
            CLR_MOSI;		        
        }

        delay();  
        SET_SCK;
        		
        delay();

        dataIn <<= 1;
		if	( PIN & MISO_MASK )
		{
			dataIn++;
		}

        delay();

        CLR_SCK;
        
        delay();
 
	}
	return dataIn;
}

	// ----------------------------------------------------------------------
// Issue one SPI command.
// ----------------------------------------------------------------------
//__attribute__((naked))
static	void	spiOneCommand ( byte_t* cmd, byte_t* res, int i )
{
	byte_t	c;
	byte_t	r;
    byte_t  mask;	

	while (i != 0)
	{
	  	i--;
		c = *cmd++;
		r = 0;
		r = spiOneByte(c);        
		*res++ = r;
	}
    //Delay100uS();
}

byte_t CheckProgramEnable(byte_t result[4])
{
    if (result[3] != 0x69)
        return 0x00;
    return 0x01;
}

byte_t CheckChipErase()
{
    byte_t result[4] = {0x00,0x00,0x00,0x00} ;
    byte_t i = 0;
    while( (result[3] != 0xFF) && (i <30) )
    {
        spiOneCommand(readByte_Cmm,result,4);//Leo la direccion 0x00
        i++;
    }
    if (i == 30)
       return 0x00;
    return 0x01;
}

void WaitDateWriteCompletion(byte_t writeByte, byte_t address, byte_t size)
{
    byte_t result[4] = {0x00,0x00,0x00,(~writeByte) };
    readByte_Cmm [2] = address;
    while (result[3] != writeByte)
    {
        spiOneCommand(readByte_Cmm,result,size);
    }
}

void WriteProgramByByte()
{
    byte_t address = 0 ,i ;
    byte_t addressTotal = 45;
    byte_t result[4];

    for (i = 0; i < addressTotal; i++,address++)
    {
        writeByte_Cmm[2] = address;
        writeByte_Cmm[3] = led_program[i];        
        spiOneCommand(writeByte_Cmm,result,4);
        WaitDateWriteCompletion(writeByte_Cmm[3], address, 4);
    }
}

//NO VALIDADO PARA S52
//El byte anterior me permite asegurar que no perdi la sincronia. En principio no es necesario.
//Tampoco se que se puede hacer en caso de perderla.
void WriteProgramByPage()
{
    byte_t address = 0 ,i, dataInAnterior, dataOutAnterior ;
    byte_t addressTotal = 45;
    byte_t result[4];

    spiOneByte(writePage_Cmm[0]);
    spiOneByte(writePage_Cmm[1]);    
    dataOutAnterior = writePage_Cmm[1];
    for (i = 0; i < addressTotal; i++,address++)
    {
        dataInAnterior = spiOneByte(led_program[i]);        
        /*if (dataInAnterior != dataOutAnterior) NO VALIDADO PARA S52
        {   
            return;// Si perdio la sincronia no se que hacer, quiza habilitar una espera y seguir...
        }
        else 
            dataOutAnterior = led_program[i];*/
    }
    
    //Completo la pagina con 0x00
    for (; address != 0x00 ; address++)
    {   
        spiOneByte(0x00);
    }

    WaitDateWriteCompletion(led_program[0], 0x00, 4);    //Chequeeo la primer posicion
}

//Inicio, activo el buffer, subo el reset, prendo el LED y configuro el MOSI como salida.
void PreparePrograming()
{
    byte_t mask;
    PORTD &= ~_BV(4);   //Enable Buffer
	DDR  = POWER_MASK | RESET_MASK | SCK_MASK | MOSI_MASK;
	//PORT = POWER_MASK | RESET_MASK;
    PORT = POWER_MASK + MISO_MASK;  //El reset ya lo pone el ResetCycle. Pongo el Pull up del miso.   
    CLR_SCK;
    SET_MOSI;
    Delay60mS();
    ChangeResetPin(0x01);

}

//Fin, bajo el reset, pongo el buffer en alta impedancia, apago el LED y pongo el MOSI como entrada.
void ClosePrograming()
{
    
    DDR  = 0x00;
	PORT = 0x00;
	PORTD |= _BV(4);
}

void ChangeResetPin (byte_t reset)
{
    if (reset)
        PORT |= RESET_MASK;
    else
    {
        PORT &= ~RESET_MASK;
        CLR_SCK;
    }
}

void ResetCycle()
{
    ChangeResetPin(0x00);
    Delay60mS();
    ChangeResetPin(0x01);
    Delay60mS();
    ChangeResetPin(0x00);
    Delay60mS();
    ChangeResetPin(0x01);
    Delay320mS();
}


void ProgramLed()
{
    byte_t result[4], i;

    sck_period = 250;

    PreparePrograming();
    Delay320mS();


    for(i = 0;(CheckProgramEnable(result) == 0x00) && (i < 5);i++)
    {
        //ResetCycle();
        spiOneCommand(programEnable_Cmm,result,4);
    }

    if (i == 5)
    {
        ClosePrograming();
        return;
    }

    Delay100uS();

    spiOneCommand(chipErase_Cmm,result,4);

    Delay320mS();
    Delay320mS();

    if ( CheckChipErase() == 0x01 )    
        WriteProgramByByte();
    //WriteProgramByPage();
    
    ClosePrograming();
}

#endif