/*
  PIC18F47Q43 ROM RAM and UART emulation firmware
  This single source file contains all code

  Target: EMUZ80 - The computer with only Z80 and PIC18F47Q43
  Compiler: MPLAB XC8 v2.36
  Written by Tetsuya Suzuki 
  Modify by hayami
 */

#include "mcc_generated_files/mcc.h"

#include <xc.h>
#include <stdio.h>
#include "config.h"

extern const unsigned char rom[]; // Equivalent to ROM, see end of this file
unsigned char ram[RAM_SIZE]; // Equivalent to RAM

static union {
    unsigned int w; // 16 bits Address
    struct {
        unsigned char l; // Address low 8 bits
        unsigned char h; // Address high 8 bits
    };
} address;
    
#define dff_reset() {G3POL = 1; G3POL = 0;}
#define db_setin() (TRISC = 0xff)
#define db_setout() (TRISC = 0x00)

// Never called, logically
//void __interrupt(irq(default),base(8)) Default_ISR(){}

// Called at Z80 MREQ falling edge (PIC18F47Q43 issues WAIT)
void __interrupt(irq(CLC1),base(8)) CLC_ISR(){
    CLC1IF = 0; // Clear interrupt flag
    
    address.h = PORTD; // Read address high
    address.l = PORTB; // Read address low
    
    if(!RA5) { // Z80 memory read cycle (RD active)
        db_setout(); // Set data bus as output
        if(address.w < ROM_SIZE){ // ROM area
            LATC = rom[address.w]; // Out ROM data
        } else if((address.w >= RAM_TOP) && (address.w < (RAM_TOP + RAM_SIZE))){ // RAM area
            LATC = ram[address.w - RAM_TOP]; // RAM data
        } else if(address.w == UART_CREG){ // UART control register
            LATC = PIR9; // U3 flag
        } else if(address.w == UART_DREG){ // UART data register
            LATC = U3RXB; // U3 RX buffer
        } else { // Out of memory
            LATC = 0xff; // Invalid data
        }
    } else { // Z80 memory write cycle (RD inactive)
        if(RE0) while(RE0);
        if((address.w >= RAM_TOP) && (address.w < (RAM_TOP + RAM_SIZE))){ // RAM area
            ram[address.w - RAM_TOP] = PORTC; // Write into RAM
        } else if(address.w == UART_DREG) { // UART data register
            U3TXB = PORTC; // Write into U3 TX buffer
        }
    }
    dff_reset(); // WAIT inactive
}

//  Called at Z80 MREQ rising edge
void __interrupt(irq(INT0),base(8)) INT0_ISR(){
    INT0IF = 0; // Clear interrupt flag
    db_setin(); // Set data bus as input
}

// main routine
void main(void) {
    // Initialize the device
    SYSTEM_Initialize();

    // Z80 start
    INTERRUPT_GlobalInterruptHighEnable();
    LATE1 = 1; // Release reset

    while(1); // All things come to those who wait
}
