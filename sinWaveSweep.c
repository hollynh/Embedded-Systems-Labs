//--------------------------------------------------------------------
// Name:            Holly Hammons
// Date:            Fall 2021
// Purp:            Lab11
//
// Assisted:        No one
// Assisted by:     TA's
//-
//- Academic Integrity Statement: I certify that, while others may have
//- assisted me in brain storming, debugging and validating this program,
//- the program itself is my own work. I understand that submitting code
//- which is the work of other individuals is a violation of the course
//- Academic Integrity Policy and may result in a zero credit for the
//- assignment, or course failure and a report to the Academic Dishonesty
//- Board. I also understand that if I knowingly give my original work to
//- another individual that it could also result in a zero credit for the
//- assignment, or course failure and a report to the Academic Dishonesty
//- Board.
//------------------------------------------------------------------------
#include "mcc_generated_files/mcc.h"
#pragma warning disable 520     // warning: (520) function "xyz" is never called  3
#pragma warning disable 1498    // fputc.c:16:: warning: (1498) pointer (unknown)

#define ROM_LENGTH          16            // Add an extra array entry for interpolation
#define RATE                488          // 


// Build a sine LUT from Excel
// SHOULD THIS BE 8 OR 16 BIT? SINCE INCREASING AMPLITUDE FUNCTION EXISTS
const uint8_t sin[64]	= {128,	140, 152, 165, 176,	188, 198, 208, 218,	226, 234, 240, 245,	250, 253, 254, 255,	254, 253, 250, 245,	240, 234, 226, 218,	208, 198, 188, 176,	165, 152, 140, 128,	115, 103, 90, 79, 67, 57, 47, 37, 29, 21, 15, 10, 5, 2,	1, 0, 1, 2,	5, 10, 15, 21, 29, 37, 47, 57, 67, 79, 90, 103,	115};

void myTMR0ISR (void);
uint16_t convert(void);

uint8_t genSin = false;
uint8_t startSweep = false;


uint8_t incAmp = false;
uint8_t decAmp = false;


uint16_t phaseIncrement = 10;

uint16_t startPhaseIncrement = 100;
uint16_t endPhaseIncrement = 1000;
// how do I use both the sweep period and RATE to control isr overflow time?
uint16_t sweepPeriod = 10;

uint8_t amplitude = 0b00001111;

typedef enum  {IDLE, GENERATE_SIN} myTMR0states_t;

//----------------------------------------------
// Main "function"
//----------------------------------------------
void main(void) {

    
    uint8_t i;
    char cmd;

    uint16_t temp;
    

    SYSTEM_Initialize();
    
    //ADC_SelectChannel(MIC_PIN);
    
    EPWM1_LoadDutyValue(0);
    
    // Provide Baud rate generator time to stabilize before splash screen
    TMR0_WriteTimer(0x0000);
    INTCONbits.TMR0IF = 0;
    while (INTCONbits.TMR0IF == 0);

    TMR0_SetInterruptHandler(myTMR0ISR);
    
    INTERRUPT_GlobalInterruptEnable();    
    INTERRUPT_PeripheralInterruptEnable();

    printf("Lab 12\r\n");
    printf("DDS waveform generator\r\n");
    printf("Dev'21 Board wiring\r\n");
    printf("Connect jumper between RC2 and LPF_in.\r\n");
    printf("Connect jumper between LPF_out and AMP_in.\r\n");
    printf("Connect Channel 1 scope probe to RC2.\r\n");
    printf("Connect Channel 2 scope probe to LPF_out.\r\n");
    printf("\r\n> "); // print a nice command prompt

    for (;;) {

        if (EUSART1_DataReady) { // wait for incoming data on USART
            cmd = EUSART1_Read();
            switch (cmd) { // and do what it tells you to do

                    //--------------------------------------------
                    // Reply with help menu
                    //--------------------------------------------
                case '?':
                    printf("-------------------------------------------------\r\n");
                    printf("--	Fixed                       \r\n");
                    printf("--      Frequency:	%d Hz        \r\n", phaseIncrement >> 1);
                    
                    // edit below
                    printf("--      Amplitude:      %d / 16                        \r\n", amplitude);
                    printf("--	Sweep                       \r\n");
                    printf("--	    Starting:   %d                         \r\n", startPhaseIncrement);
                    printf("--	    Ending:     %d                         \r\n", endPhaseIncrement);
                    printf("--	    Duration:   %d                         \r\n", sweepPeriod);
                    
                    printf("-------------------------------------------------\r\n");
                    printf("?:   help menu\r\n");
                    printf("o:   k\r\n");
                    printf("Z:   reset processor\r\n");
                    printf("z:   clear the terminal\r\n");
                    printf("S/s: Start/stop fixed frequency sine wave \r\n");
                    printf("f: enter Frequency \r\n");
                    printf("A/a: increase/decrease Amplitude select amplitude \r\n");
                    printf("w:   sWeep sine wave \r\n");
                    printf("t: enter sTarting frequency \r\n");
                    printf("e: enter Ending frequency \r\n");
                    printf("d: enter Duration \r\n");
                    printf("-------------------------------------------------\r\n");
                    break;

                    //--------------------------------------------
                    // Reply with "k", used for PC to PIC test
                    //--------------------------------------------
                case 'o':
                    printf("o:	ok\r\n");
                    break;

                    //--------------------------------------------
                    // Reset the processor after clearing the terminal
                    //--------------------------------------------                      
                case 'Z':
                    for (i = 0; i < 40; i++) printf("\n");
                    RESET();
                    break;

                    //--------------------------------------------
                    // Clear the terminal
                    //--------------------------------------------                      
                case 'z':
                    for (i = 0; i < 40; i++) printf("\n");
                    break;

                case 'S':
                    printf("Generating a %d Hz sine wave\r\n", phaseIncrement >> 1);
                    genSin = true;
                    break;
                                  
                case 's':
                    genSin = false;
                    printf("Stopped generating a sine wave \r\n");
                    break;
                    
                case 'f':
                    temp = convert();
                    printf("The frequency is now %d Hz. \r\n", temp);
                    phaseIncrement = temp << 1;
                    break;
                
                
                case 'A':
                    if(amplitude < 0b00001111){
                        amplitude = amplitude + 0b00000001;
                        printf("Increased amplitude to %d / 16 \r\n", amplitude);
                    }
                    else{
                        printf("Can't increment further. \r\n");
                    }
                    break;
                    
                case 'a':
                    if(amplitude > 0b00000001){
                        amplitude = amplitude - 0b00000001;
                        printf("Decreased amplitude to %d / 16 \r\n", amplitude);
                    }
                    else{
                        printf("Can't decrement further. \r\n");
                    }
                    break;

                    

                //---------------------------------------------------------------
                // start a frequency sweep 
                //---------------------------------------------------------------  
                case 'w':
                    printf("Generating sweep. Press any key to exit.\r\n");
                    
                    uint32_t sweep = sweepPeriod;
                    uint32_t start = startPhaseIncrement;
                    uint32_t end = endPhaseIncrement;
                   
                    // The following lines of code took far too long to figure out
                    uint32_t a32 = ((uint32_t) sweepPeriod) * ((uint32_t) 500);
                    //printf("a32=0x%04x:%04x\r\n",((uint16_t) (a32>>16)), ((uint16_t) a32));      

                    uint32_t b32 = (uint32_t)endPhaseIncrement - (uint32_t)startPhaseIncrement;
                    //printf("b32=0x%04x:%04x \r\n",((uint16_t) (b32>>16)), ((uint16_t) b32));                

                    uint32_t c32 = a32 / b32;
                    //printf("c32=0x%04x:%04x\r\n",((uint16_t) (c32>>16)), ((uint16_t) c32));   
                    
                    
                    uint32_t updateRate = c32 << 2;
                    // maybe change to 16 later
                    //uint16_t timerCount = (updateRate * 500);
                    //genSin = true;
                    while(!EUSART1_DataReady){
                        genSin = true;
                        for (phaseIncrement = startPhaseIncrement; phaseIncrement < endPhaseIncrement; phaseIncrement++) {
                            
                            TMR1_WriteTimer(0xFFFF - updateRate); 
                            PIR1bits.TMR1IF = 0;
                            while (TMR1_HasOverflowOccured() == false){
                                genSin = true;
                            }
                            
                            //genSin = false;
                        }
                        genSin = false;
                    }
                    genSin = false;
                    
                    /*
                    while(genSin == true){
                        for (phaseIncrement = startPhaseIncrement; phaseIncrement < endPhaseIncrement; phaseIncrement++) {
                            TMR1_WriteTimer(0xFFFF - timerCount); 
                            PIR1bits.TMR1IF = 0;
                            while (TMR1_HasOverflowOccured() == false);
                            if(EUSART1_DataReady){
                                uint8_t temp1 = EUSART1_Read();
                                genSin = false;
                            }
                        }        
                    } 
                     */
                    uint8_t temp1 = EUSART1_Read();
                    printf("Exited sweep. \r\n");
                    
                    break;

                case 't':
                    temp = convert();
                    startPhaseIncrement = temp << 1;
                    printf("The starting frequency is now %d Hz. \r\n", temp);
                    break;
                    
                case 'e':
                    temp = convert();
                    endPhaseIncrement = temp << 1;
                    printf("The ending frequency is now %d Hz. \r\n", temp);
                    break;
                    
                case 'd':
                    temp = convert();
                    // how to use in milliseconds?
                    sweepPeriod = temp;
                    printf("The sweep period is now %d. \r\n", temp);
                    break;

                    //--------------------------------------------
                    // If something unknown is hit, tell user
                    //--------------------------------------------
                default:
                    printf("Unknown key %c\r\n", cmd);
                    break;
            } // end switch

        } // end if
    } // end while 
} // end main

void myTMR0ISR (void){
    // will need 6.10 FPF
    static uint16_t phaseAccumulator = 0;
    static uint8_t index;
    uint8_t local, localDiv2, localDiv4, localDiv8, localDiv16, total;
    static myTMR0states_t currState;
    static uint8_t dutyVal;
    static uint8_t count;
    
    //ADCON0bits.GO_NOT_DONE = 1;
    
    EPWM1_LoadDutyValue(dutyVal);
    
    switch(currState){
        case IDLE:
            //printf("idle \r\n");
            phaseAccumulator = 0;
            dutyVal = 0;
            index = 0;
            count = 0;
            total = 0;
            
            if(genSin == true){
                currState = GENERATE_SIN;
            }
            break;
            
        case GENERATE_SIN:
            phaseAccumulator = phaseAccumulator + phaseIncrement;
            index = phaseAccumulator >> 10;
            
            local = sin[index];
            localDiv2  = 0;
            localDiv4  = 0;
            localDiv8  = 0;
            localDiv16 = 0;
            
            // increase/decrease amplitude if needed
            if((amplitude & 0b00000001) != 0){
                localDiv16 = local >> 4;
            }
            if((amplitude & 0b00000010) != 0){
                localDiv8  = local >> 3;
            }
            if((amplitude & 0b00000100) != 0){
                localDiv4  = local >> 2;
            }
            if((amplitude & 0b00001000) != 0){
                localDiv2  = local >> 1;
            }
            total = localDiv2 + localDiv4 + localDiv8 + localDiv16;
            dutyVal = total;

            //dutyVal = sin[index];
            

            if(genSin == false){
                currState = IDLE;
            }
            //printf("Amplitude = %d \r\n", amplitude);
            break;
 
        default:
            currState = IDLE;
            break;
    }    
    TMR0_WriteTimer(TMR0_ReadTimer() + (0xFFFF - RATE));
    INTCONbits.TMR0IF = 0;
}

// CHECK THIS OVER
uint16_t convert(void){
    printf("Enter in a multi-digit number, press enter when done. \r\n");
    uint16_t sum = 0;
    while(!EUSART1_DataReady);
    char cmd = EUSART1_Read();
    
    if((cmd >= 48) && (cmd <= 57)){
        printf("%c", cmd);
        sum = sum + (cmd - 48);
        
    }
    
    while(cmd != '\r'){
        while(!EUSART1_DataReady);
        cmd = EUSART1_Read();
        
        if((cmd >= 48) && (cmd <= 57)){
            printf("%c", cmd);
            sum = (10*sum) + (cmd - 48);
            
        }
    }
    printf("\r\n");
    return(sum);
}

/*
 case SWEEP:
            phaseAccumulator = phaseAccumulator + startPhaseIncrement;
            index = phaseAccumulator >> 4;
            dutyVal = sin[index];
            if(startPhaseIncrement < endPhaseIncrement){
                count++;
                startPhaseIncrement++;
            }
            else if(startPhaseIncrement >= endPhaseIncrement){
                startPhaseIncrement = endPhaseIncrement - count;
            }
            
            if(startSweep == false){
                currState = IDLE;
            }
            TMR0_WriteTimer(TMR0_ReadTimer() + (0xFFFF - sweepPeriod));
            break;
 */