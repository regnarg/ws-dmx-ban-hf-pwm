#include <mcs51/lint.h>
#include <mcs51/8051.h>
#include "stc11l04.h"
#include <stdint.h>


#define FOSC  24000000L //System frequency.
#define BAUD  9600L    //UART1 baud-rate
#define RX_INTERRUPT 0

#define AUXR_BRTR (1<<4)
#define AUXR_BRTx12 (1<<2)
#define AUXR_T0x12 (1<<7)
#define AUXR_T1x12 (1<<6)
#define AUXR_S1BRS (1<<0)

uint8_t level0=5, level1=5, level2=5, level3=5;
uint8_t addr;

#define LEVELS 64


#define MSG_LEN 6 //address, 4 channel values, 1B padding
uint8_t last = 0b11111111;
uint8_t msg[MSG_LEN];
uint8_t msgidx = 255;
uint8_t csum = 0;
uint8_t last_csum = 255;

uint8_t need_recompute = 0, recompute_running = 0;


#define STEP (TL1 & (LEVELS-1))
// rotate is faster than shift because the cpu has rotate instructions, shift must be emulated
#define RR(a,k) (((a) >> (k)) | ((a) << (8-(k))))  
#define RL(a,k) (((a) << (k)) | ((a) >> (8-(k))))  


#define ASM_UPDATE_1 \
        mov     a,_TL1 \
        anl     a,#0x3f \
        add     a,#_pwm \
        mov     r1,a \
        mov     a,@r1 \
        rrc a \
        mov     _P3_6,c \
        rrc a \
        mov     _P3_2,c \
        rrc a \
        mov     _P3_3,c \
        rrc a \
        mov     _P3_4,c \
        jb _RI, on_uart

// 37 cycles
#define ASM_UPDATE_2    \
        mov     a,_TL1    ; 1 \
        anl     a,#0x3f   ; 3 \
        mov     r1, a     ; 2 \
        subb    a,_level0 ; 2 \
        mov     _P3_6,c   ; 4 \
        mov     a,r1      ; 1 \
        subb    a,_level1 ; 2 \
        mov     _P3_2,c   ; 4 \
        mov     a,r1      ; 1 \
        subb    a,_level2 ; 2 \
        mov     _P3_3,c   ; 4 \
        mov     a,r1      ; 1 \
        subb    a,_level3 ; 2 \
        mov     _P3_4,c   ; 4

#define ASM_UPDATE_UC \
        ASM_UPDATE_2 \
        jb _RI, on_uart   ; 4

#define ASM_UPDATE_UC2 \
        ASM_UPDATE_2 \
        jnb _RI, 3   ; 4 \
        lcall _handle_uart


void mainloop() {


    __asm
        myloop:
        ASM_UPDATE_UC
        ASM_UPDATE_UC
        ASM_UPDATE_UC
        ASM_UPDATE_UC
        SJMP myloop

        on_uart:
            ASM_UPDATE_2
            ASM_UPDATE_2
            lcall _handle_uart
            ASM_UPDATE_UC
            ASM_UPDATE_UC
            LJMP myloop

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
#if RX_INTERRUPT
    ES = 1;
#endif
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



static inline void send_sync(uint8_t what) {
    ES = 0;  //close serial port interrupt
    TI = 0;  //clear UART transmit interrupt flag
    SBUF = what;
    while (TI == 0); 
    TI = 0; 
#if RX_INTERRUPT
    ES = 1; 
#else
#endif
}


void handle_uart() {
    RI=0;
    uint8_t tmp = SBUF;

    if (SM0) { // framing error
        //TI=0;
        //SBUF = 222;
        SM0 = 0;
        msgidx = 255;
    }


    //TI=0;
    //SBUF=tmp;
    //send_sync(msgidx); //XXX
    //send_sync(tmp); //XXX

    if (tmp == 253) { // start of message
        msgidx = 0;
        csum = 0;
    } else if (tmp == 250) {
        addr = read_addr();
        //TI = 0;
        //SBUF = addr;
    } else if (msgidx == MSG_LEN) {
        //send_sync(221);
        if (tmp == csum && csum != last_csum) { // valid message
            level0  = msg[1];
            level1  = msg[2];
            level2  = msg[3];
            level3  = msg[4];
            last_csum = csum;
            //send_sync(222);
        } else {
            //send_sync(226);
            //send_sync(csum);
            //send_sync(last_csum);
            //send_sync(226);
        }
        msgidx = 255;
    } else if (msgidx != 255) {
        if (msgidx == 0 && tmp != addr && tmp != 251) {
            msgidx = 255;
            return;
        }
        msg[msgidx] = tmp;
        csum = RL(csum, 3);
        csum ^= tmp;
        msgidx++;
    }
}


void main()
{
    P3M0 = 0b1011100; //set P3.6, P3.4, P3.3 and P3.2 to strong push pull output
    ES = 0;
    ET1 = 0;
    EX1 = 0;
    ET0 = 0;
    EX0 = 0;
#if RX_INTERRUPT
    EA  =  1;
#endif
    dip_init();
    addr = read_addr();
    uart_init();
    timer_init();

    //TI = 0;
    //SBUF = 'R';
    //while (TI);

    mainloop();
}
