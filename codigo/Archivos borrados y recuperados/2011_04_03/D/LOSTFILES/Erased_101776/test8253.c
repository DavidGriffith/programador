//#define TEST_8253

#ifdef TEST_8253

#include <avr/pgmspace.h>
#include <avr/interrupt.h>
#include "def.h"
#include "usb.h"
#include "macros.h"
#include "usbtinyMod.h"
#include "spi.h"



byte_t programEnable_Cmm[] = {0xAC,0x53,0x00,0x00};
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

void test8253_normal_write()
{


}

// Sends one byte over SPI.
//   Returns the data in.


static byte_t spiOneByte(byte_t dataOut)
{
	byte_t	mask;
	byte_t  dataIn = 0;

	for	( mask = 0x80; mask; mask >>= 1 )
	{

		SET_SCK;
		if	( dataOut & mask )
		{
			SET_MOSI;            
		}
//			if (! status & INVERTED_SCK_MASK )
		delay();

		dataIn <<= 1;
		if	( PIN & MISO_MASK )
		{
			dataIn++;
		}
		CLR_SCK;
        delay();
//			if ( status & INVERTED_SCK_MASK )
//				delay();
		CLR_MOSI;		
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
}

void CheckChipErase()
{
    byte_t result[4] = {0x00,0x00,0x00,0x00} ;
    while(result[3] != 0xFF)
    {
        spiOneCommand(readByte_Cmm,result,4);//Leo la direccion 0x00
    }
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

//El byte anterior me permite asegurar que no perdi la sincronia. En principio no es necesario.
//Tampoco se que se puede hacer en caso de perderla.
void WriteProgramByPage()
{
    byte_t address = 0 ,i, dataInAnterior, dataOutAnterior ;
    byte_t addressTotal = 45;
    byte_t result[4];

    spiOneByte(writePage_Cmm[0]);
    spiOneByte(writePage_Cmm[1]);
    spiOneByte(writePage_Cmm[2]);
    dataOutAnterior = writePage_Cmm[2];
    for (i = 0; i < addressTotal; i++,address++)
    {
        dataInAnterior = spiOneByte(led_program[i]);        
        if (dataInAnterior != dataOutAnterior)
        {   
            return;// Si perdio la sincronia no se que hacer, quiza habilitar una espera y seguir...
        }
        else 
            dataOutAnterior = led_program[i];
    }
    
    //Completo la pagina con 0x00
    for (; address < 64 ; address++)
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
	PORT = POWER_MASK | RESET_MASK;
}

//Fin, bajo el reset, pongo el buffer en alta impedancia, apago el LED y pongo el MOSI como entrada.
void ClosePrograming()
{
    
    DDR  = 0x00;
	PORT = 0x00;
	PORTD |= _BV(4);
}

void ProgramLed()
{
    byte_t result[4];


    PreparePrograming();


    spiOneCommand(programEnable_Cmm,result,4);
    
    spiOneCommand(chipErase_Cmm,result,4);
    CheckChipErase();    

    //WriteProgramByByte();
    WriteProgramByPage();
    
    ClosePrograming();
}

#endif