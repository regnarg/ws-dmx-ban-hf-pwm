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

uint8_t level0=5, level1=5, level2=5, level3=5;
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
	mov	_P3_6,c
    rrc a
	mov	_P3_2,c
    rrc a
	mov	_P3_3,c
    rrc a
	mov	_P3_4,c
    __endasm;
}


#define PCON_SMOD0 (1 << 6)
void uart_init()
{
    SM0 = 0; // 8-bit mode
    SM1 = 1; // variable-rate UART
    
    BRT = 178; // corresponds to 9600bd
    AUXR =  AUXR_BRTR | AUXR_BRTx12 | AUXR_S1BRS;

    PCON |= PCON_SMOD0; // enable frame error bit access

    REN = 1;
}

void timer_init() {
    TMOD = 0b00100000; // timer 0 8-bit autoreload
    TH1 = 0; // reload value
    TR1 = 1;
    //AUXR |=  AUXR_T1x12;
}

void dip_init() {
    P3_5 = 1; // enable pull-ups
    P3_7 = 1;
    P1_0 = 1;
    P1_1 = 1;
    P1_2 = 1;
    P1_3 = 1;   
    P1_4 = 1;
    P1_5 = 1;
    // give some time to settle
    __asm
        nop
        nop
        nop
        nop
        nop
        nop
    __endasm;
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

    result = ~result; // DIP switches are active low

    return result & 0b01111111; // we use only 7bit addresses
}

void recompute() {
    for (uint8_t i = 0; i < LEVELS; i++) {
        uint8_t val = 0;
        val |= (i < level3);
        val = RL(val, 1);
        update();
        val |= (i < level2);
        val = RL(val, 1);
        update();
        val |= (i < level1);
        val = RL(val, 1);
        update();
        val |= (i < level0);
        pwm[i] = val;
        update();
        //UPDATE(); //do not forget to update output during recomputation
    }
}

#define MSG_LEN 6 //address, 4 channel values, 1B padding

void main()
{
    P3M0 = 0b1011100; //set P3.6, P3.4, P3.3 and P3.2 to strong push pull output
    EA  =  0; //disable interrupts
    dip_init();
    addr = read_addr();
    recompute();
    uart_init();
    timer_init();

    TI = 0;
    SBUF = 'R';
    while (TI);

    uint8_t last = 0b11111111;
    uint8_t msg[MSG_LEN];
    uint8_t msgidx = 255;
    uint8_t csum = 0;
    uint8_t last_csum = 255;

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


        if (RI==1) {
            RI=0;
            uint8_t tmp = SBUF;
            update();

            if (SM0) { // framing error
                TI=0;
                SBUF = 222;
                SM0 = 0;
                msgidx = 255;
            }



            if (tmp == 253) { // start of message
                msgidx = 0;
                csum = 0;
                update();
            } else if (tmp == 250) {
                addr = read_addr();
                TI = 0;
                SBUF = addr;
            } else if (msgidx == MSG_LEN) {
                update();
                if (tmp == csum && csum != last_csum) { // valid message
                    level0  = msg[1];
                    level1  = msg[2];
                    update();
                    level2  = msg[3];
                    level3  = msg[4];
                    update();
                    recompute();
                    update();
                    last_csum = csum;
                }
                update();
                msgidx = 255;
            } else if (msgidx != 255) {
                update();
                if (msgidx == 0 && tmp != addr && tmp != 251) {
                    msgidx = 255;
                    update();
                    continue;
                }
                if (msgidx > 0 && tmp > 64) {
                    msgidx = 255;
                    update();
                    continue;
                }
                msg[msgidx] = tmp;
                update();
                csum = RL(csum, 3);
                csum ^= tmp;
                msgidx++;
                update();
            }
        }

        update();
        update();
        update();
        update();
        update();
        update();
        update();
        update();

    }
}
