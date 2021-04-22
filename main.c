#include <mcs51/lint.h>
#include <mcs51/8051.h>
#include "stc11l04.h"
#include <stdint.h>


#define FOSC  24000000L //System frequency.
#define BAUD  9600L    //UART1 baud-rate

#define AUXR_BRTR (1<<4)
#define AUXR_BRTx12 (1<<2)
#define AUXR_T0x12 (1<<7)
#define AUXR_T1x12 (1<<6)
#define AUXR_S1BRS (1<<0)

uint8_t level1=32, level2=32, level3=32;
uint8_t addr;

#define LEVELS 64
uint8_t pwm[LEVELS];

#define STEP (TL1 & (LEVELS-1))
// rotate is faster than shift because the cpu has rotate instructions, shift must be emulated
#define RR(a,k) (((a) >> (k)) | ((a) << (8-(k))))  
#define RL(a,k) (((a) << (k)) | ((a) >> (8-(k))))  

//#define UPDATE()  { uint8_t step = STEP; P3_2 = (step < level1); P3_3 = step < level2; P3_4 = step < level3;}
//#define UPDATE()  { uint8_t step = STEP; P3_2 = pwm1[step]; P3_3 = pwm2[step]; P3_4 = pwm3[step];}
//#define UPDATE()  { uint8_t step = TL1; step = (step >> 2); step &= (LEVELS-1); uint8_t val = pwm[step]; P3_2 = val; val >>= 1; P3_3 = val; val >>= 1; P3_4 = val;}
//#define UPDATE()  { uint8_t step = TL1; step = (step >> 2); step &= (LEVELS-1); P3_2 = (step < level1); P3_3 = step < level2; P3_4 = step < level3;}

#define UPDATE()  { uint8_t step = TL1; step &= (LEVELS-1); uint8_t val = pwm[step]; P3_2 = val; val = RR(val, 1); P3_3 = val; val = RR(val, 1); P3_4 = val;}

//static inline void update() {
//    uint8_t step = TL1; step = (step >> 2); step &= (LEVELS-1); uint8_t val = pwm[step]; P3_2 = val; val >>= 1; P3_3 = val; val >>= 1; P3_4 = val;
//}

static  inline void update() {
    // POZOR kde jinde se používá r1 !!
    __asm
	mov	a,_TL1
	anl	a,#0x3f
	add	a,#_pwm
	mov	r1,a
	mov	a,@r1
    rrc a
	mov	_P3_2,c
    rrc a
	mov	_P3_3,c
    rrc a
	mov	_P3_4,c
    __endasm;
}

void uart_init()
{
    //SCON = 0xD8;
    //SCON = 0x48; // 8-bit uart
    SCON = 0xD8; // 8-bit uart

    //calculate timer overlow values based to achieve BAUD rate based on cpu frequency 
    //copied from the example code in offcial documentation
    
    BRT = 178;
    AUXR =  AUXR_BRTR | AUXR_BRTx12 | AUXR_S1BRS;

    //TH1 = 178;
    //TMOD = 0b00100000;
    //TR1 = 1;
    //AUXR =  AUXR_T1x12;


}

void timer_init() {
    TMOD = 0b00100000; // timer 0 8-bit autoreload
    TH1 = 0; // reload value
    TR1 = 1;
    //AUXR |=  AUXR_T1x12;
}

uint8_t read_addr()
{
    //initialize unused bits as 1 (will later be inverted to 0)
    uint8_t result = 0x00;

    result = result | P3_5;
    result = result | (P3_7 << 1);
    result = result | (P1_0 << 2);
    result = result | (P1_1 << 3);
    result = result | (P1_2 << 4);
    result = result | (P1_3 << 5);
    result = result | (P1_4 << 6);
    result = result | (P1_5 << 7);

    result = ~result;
    return result;
}

void recompute() {
    for (int i = 0; i < LEVELS; i++) {
        uint8_t val = 0;
        val |= (i < level3);
        val = RL(val, 1);
        val |= (i < level2);
        val = RL(val, 1);
        val |= (i < level1);
        pwm[i] = val;
        //UPDATE(); //do not forget to update output during recomputation
    }
}


void main()
{
    P3M0 = 0x1C; //set P3.4, P3.3 and P3.2 to strong push pull output
    EA  =  0; //disable interrupts
    addr = read_addr();
    recompute();
    uart_init();
    timer_init();

    while (1) {
        //P3 = (1<<4) | (1<<3) | (1<<2);
        update();
        update();
        update();
        update();
        update();
        update();
        update();
        update();
        update();
        update();
        update();
        update();
        update();
        update();
        update();
        update();


        if (RI==1) {
            RI=0;
            uint8_t tmp = SBUF;
            TI=0;
            SBUF=tmp;
            if (tmp >= '0' && tmp <= '9') {
                uint8_t lev  = (uint8_t) (((uint16_t)LEVELS  * (uint16_t)(tmp - '0')) / 10);
                level1 = lev;
                level2 = lev;
                level3 = lev;
                recompute();
                update();
            }
        }

    }
}
