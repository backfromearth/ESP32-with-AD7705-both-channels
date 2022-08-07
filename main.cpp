// THIS CODE WORKS on BOTH CHANNELS with 3v3 on a ESP32 wroom with PlatformIO and the cheap chinese red module from ali
// The range in unipolar mode is strangely 31230 (0x8000) => 65535 (0xFFFF) on ch1 & ch2 connected to 2 potentiometers on 3v3
// DRDY does'nt work as expected and has been disabled, but some delay() have been added (to be shure...)
// With RESET connected to 3v3, you don't need to reset the chip during setup
// It seems that the same config can be obtained for both channels ONLY in the NORMAL mode, NOT with the self calibration mode !!!
// all routines are in the main file , the timing seems a little critical if you play with the init parameters
// My advise : go buy a ADS1115 !!
//=========================================================================================================================

#include <arduino.h>

enum
{

	REG_COMM	= 0x00,	//Comm register RS2RS1RS0  = 000
	REG_SETUP	= 0x10,//Set register RS2RS1RS0    = 001 0b00010000
	REG_CLOCK	= 0x20,	//Clock register RS2RS1RS0 = 010 ob00100000
	REG_DATA	= 0x30,	//Data register RS2RS1RS0  = 011 0b0011 0000
	REG_ZERO_CH1	= 0x60,	//CH1 offset register
	REG_FULL_CH1	= 0x70,//CH1 full scale register
	REG_ZERO_CH2	= 0x61,	//CH2 offset register
	REG_FULL_CH2	= 0x71,	//CH2 full scale register

	WRITE 		= 0x00,	//Write operation
	READ 		= 0x08,	//Read operation

	CH_1		= 0,
	CH_2		= 1,
	
};

#define ADC_DI GPIO_NUM_23//DIN pin
#define ADC_DO GPIO_NUM_25//DOUT pin
#define ADC_CLK GPIO_NUM_19//SCK pin
//#define ADC_RESET GPIO_NUM_18//RST pin
#define ADC1_CS GPIO_NUM_22//CS pin
//#define ADC1_DRDY1 GPIO_NUM_21//DRDY pin

// Write CLOCK
#define ADC_CLK0   digitalWrite(ADC_CLK, 0) ; 
#define ADC_CLK1   digitalWrite(ADC_CLK, 1) ; 
// write dataIN
#define ADC_DI0   digitalWrite(ADC_DI, 0) ; 
#define ADC_DI1   digitalWrite(ADC_DI, 1) ; 
//write Chip select
#define ADC1_CS0  digitalWrite(ADC1_CS, 0) ; 
#define ADC1_CS1  digitalWrite(ADC1_CS, 1) ; 
// read Data out 
#define ADC_DOR digitalRead(ADC_DO) ; 

void adc_delay(unsigned char NUM)		{		for(;NUM>0;NUM--);		 	}

void write_adc_byte(unsigned char chr)//Write byte function
{
	unsigned char i;
	for(i=0;i<8;i++)//Write eight times in a loop
	{
		if (chr & 0x80)//Bit AND operation, write out data   
		      { ADC_DI1;  }//SPI data MOSI interface 　 bit output		
		else  {	ADC_DI0;  }//SPI data MOSI interface
		ADC_CLK0;//Clock signal
		chr=chr<<1;//Shift one bit of data to the right, and write out the data bit by bit in a loop
		ADC_CLK1;
	}
	//ADC_CLK1;
}

unsigned int read_adc_word()//Read data function
{	unsigned char i;
	unsigned int coder = 0;
	byte dread ; // temporry read buffer 

	for(i=0;i<16;i++)//16-bit data, read in two bytes circularly
	{
		ADC_CLK0;//Clock signal
		adc_delay(1);
		coder = coder<<1;
		dread =  ADC_DOR ;
		if(dread) coder+=1;	//Detect pin level, read in data	
		ADC_CLK1;
	}
	ADC_CLK1;
	return(coder);
}


void adc1_init(void)
{
// 1) realign synchro ---------------------------------------------------------------------------------------
	ADC1_CS0;
	write_adc_byte(0xFF); // from hint ; if lost input 32 high pulse cycles to ADIN
	write_adc_byte(0xFF);
	write_adc_byte(0xFF);
	write_adc_byte(0xFF);
	ADC1_CS1;
	delay(1) ; // need to wait at least 500us after a reset 32 peak highs : see datasheet / application guide
	// 2) setup clock --------------------------------------------------------------------------------------------
	ADC1_CS0;
	//write_adc_byte( 0x20 );// 
	write_adc_byte( REG_CLOCK|WRITE|CH_1 );//next operation is write in clock registery for ch1
	ADC1_CS1;
	adc_delay(10); 
	ADC1_CS0;
	//Clock Register
 	//   7      6       5        4        3        2      1      0
	//ZERO(0) ZERO(0) ZERO(0) CLKDIS(0) CLKDIV(0) CLK(1) FS1(0) FS0(1)
	//    0     0       0         										000 : keep them always low
	//					   		 0       								0 :clock disable is false
	//                                    1                             1 = clock division (should be 1 with 4.19Mhz)
	//                                              1                   1 if freq > 2Mhz
	// 													  1       1     filter : 00 =50Hz  01=60hz 10=250hz 11=500hz
	write_adc_byte(0b00001111) ;//  clock div, freq >2Mhz filter = 500hz
	ADC1_CS1;
	delay(20) ; // just try and error ...
	// 3) config setup register ---------------------------------------------------------------------------------
	ADC1_CS0;
	write_adc_byte(REG_SETUP|WRITE|CH_1); // next operation is write in setup registry for Ch1
	ADC1_CS1;
	adc_delay(10);
	ADC1_CS0;
	//Setup Register
	//  7     6     5     4     3      2      1      0
	//MD10) MD0(0) G2(0) G1(0) G0(0) B/U(0) BUF(0) FSYNC(1)
	//  0     0                                              01= self calibration 00 = normal
	//  		    0     0     0                            000 = Gain1 
	//                                  1                    1=unipolar                               
	//                                        0              0=no buff or 1 buff
	//                                               0       0=running
	write_adc_byte(0b00000110) ;                         //  00 000 1 1 0 = normal Gain1 unipolar with_buffer 
	ADC1_CS1;
	delay(20) ; // just to be shure ...
}

unsigned int adc1_read_value(unsigned char ch)
{
	unsigned int  value;
	//ADC_DI1; // ?????????????????????????????????
		ADC1_CS0;
		if(ch == 1)
      		write_adc_byte(0x38);//
   		else if(ch == 2)
      		write_adc_byte(0x39);//
		else    {ADC1_CS1;return 0;}
		ADC1_CS1;
		adc_delay(1);
		ADC1_CS0;
		value = read_adc_word();
		ADC1_CS1;
		//ADC_DI0   ; // ?????????????????????????????????
		return value;
}

int volt1,volt2;
unsigned int temp1,temp2 ;


//===============================================================================================================================
void setup(void) {
	pinMode(ADC_CLK, OUTPUT) ;
	pinMode(ADC1_CS, OUTPUT) ;
	pinMode(ADC_DI, OUTPUT) ;
    pinMode(ADC_DO, INPUT_PULLUP); 
    
	adc1_init();
    
	Serial.begin(115200);
}
//===========================================================================================
void loop(void)
{ 
    delay(7) ; 	// min 7ms required in current mode in order to read a valid next value
   temp1 = adc1_read_value(1);
    volt1=((temp1 * 0.09621) - 3004.5) ; //  y = ax+b... based on my low value : 31228 for 0 volts
    Serial.printf("channel1:   %d    V:%d\n",temp1,volt1);
   delay(7) ; 	// min 7ms required in current mode in order to read a valid next value
    temp2 = adc1_read_value(2);
    volt2=((temp2 * 0.09621) - 3004.5) ; //  
    Serial.printf("channel2:   %d    V:%d\n",temp2,volt2);
}


