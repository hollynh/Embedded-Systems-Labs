//--------------------------------------------------------------------
// Name:            Holly Hammons
// Date:            Fall 2021
// Purp:            Lab08
//
// Assisted:        no one
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
#include <pic18f26k22.h>

#include "mcc_generated_files/mcc.h"
#pragma warning disable 520     // warning: (520) function "xyz" is never called  3
#pragma warning disable 1498    // fputc.c:16:: warning: (1498) pointer (unknown)

#define     NUM_SAMPLES     16
#define     NUM_OF_MIC_SAM_TO_COLLECT     512
uint8_t NEW_SAMPLE = false;

typedef enum  {MIC_IDLE, MIC_WAIT_FOR_TRIGGER, MIC_ACQUIRE} myTMR0states_t;
uint8_t fillBuffer = false;
uint8_t upperThreshold = 138;
uint8_t lowerThreshold = 118;

uint16_t avFreq;
uint16_t avPeriod;

uint16_t buffer[NUM_OF_MIC_SAM_TO_COLLECT];
uint16_t crossingPoint[NUM_OF_MIC_SAM_TO_COLLECT];

uint8_t bufferFull = false;

void INIT_PIC(void);
void myTMR0ISR(void);

//----------------------------------------------
// Main "function"
//----------------------------------------------

void main(void) {

    uint8_t i, adc_reading[NUM_SAMPLES];
    char cmd;

    SYSTEM_Initialize();

    // BEFORE enabling interrupts, otherwise that while loop becomes an
    // infinite loop.  Doing this to give EUSART1's baud rate generator time
    // to stablize - this will make the splash screen looks better
    TMR0_WriteTimer(0x0000);
    INTCONbits.TMR0IF = 0;
    while (INTCONbits.TMR0IF == 0);

    TMR0_SetInterruptHandler(myTMR0ISR);
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

    printf("inLab 08\r\n");
    printf("Microphone experiments\r\n");
    printf("Dev'21\r\n");
    printf("> "); // print a nice command prompt

    for (;;) {

        if (EUSART1_DataReady) { // wait for incoming data on USART
            cmd = EUSART1_Read();
            switch (cmd) { // and do what it tells you to do

                case '?':
                    printf("------------------------------\r\n");
                    printf("?: Help menu\r\n");
                    printf("o: k\r\n");
                    printf("Z: Reset processor\r\n");
                    printf("z: Clear the terminal\r\n");
                    printf("T/t: Increase/decrease threshold 138 - 118\r\n", NUM_SAMPLES);
                    printf("f: gather 512 samples from the microphone and calculate the frequency\r\n");
                    printf("------------------------------\r\n");
                    break;

                    //--------------------------------------------
                    // Reply with "k", used for PC to PIC test
                    //--------------------------------------------    
                case 'o':
                    printf(" k\r\n>");
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
                    for (i = 0; i < 40; i++) printf("\n>");
                    break;

                    //--------------------------------------------
                    // Continue to collect samples until the user
                    // presses a key on the keyboard
                    //--------------------------------------------                                          
                case'T':
                    upperThreshold = upperThreshold + 5;
                    lowerThreshold = lowerThreshold - 5;
                    printf("upper threshold: %d \r\n", upperThreshold);
                    printf("lower threshold: %d \r\n", lowerThreshold);
                    break;
                case 't':
                    upperThreshold = upperThreshold - 5;
                    lowerThreshold = lowerThreshold + 5;
                    printf("upper threshold: %d \r\n", upperThreshold);
                    printf("lower threshold: %d \r\n", lowerThreshold);
                    break;
                    
                case 'f':
                    printf("collecting samples \r\n");
                    fillBuffer = true;
                    
                    while(bufferFull == false);
                    
                    printf("The last 256 ADC samples from the microphone are: \r\n");
                    for(uint16_t k = 0; k <= NUM_OF_MIC_SAM_TO_COLLECT; k = k + 16){
                        printf("S[%d] ", k);
                        for(uint16_t j = k; j < (k + 16); j++){
                            printf("%d ", buffer[j]);
                        } 
                        printf("\r\n");
                    }
                    printf("\r\n");
                    
                    for(uint16_t i = 0; i < NUM_OF_MIC_SAM_TO_COLLECT; i++){
                        crossingPoint[i] = 0;
                    }
                    
                    uint16_t j = 0;
                    for(uint16_t i = 0; i < (NUM_OF_MIC_SAM_TO_COLLECT - 1); i++){
                        if((buffer[i] <= 128) && (buffer[i + 1] > 128)){
                            crossingPoint[j] = i;
                            j++;
                        }
                    }
                    
                    uint16_t k = 0;
                    while(crossingPoint[k] != 0){
                        k++;
                    }
                    
                    printf("The sound wave crossed at the following indicies \r\n");
                    for(uint16_t i = 0; i < k; i++){
                        printf("%d ", crossingPoint[i]);
                    }
                    printf("\r\n");
                    printf("\r\n");
                    
                    printf("The sound wave had %d periods \r\n", (k - 1));
                    for(uint16_t i = 0; i < (k - 1); i++){
                        printf("%d - %d = %d \r\n", crossingPoint[i + 1], crossingPoint[i], (crossingPoint[i + 1] - crossingPoint[i]));
                    }
                    printf("\r\n");
                    
                    uint16_t sum = 0;
                    for(uint16_t i = 0; i < (k - 1); i++){
                        sum = sum + (crossingPoint[i + 1] - crossingPoint[i]);
                    }
                    
                    avPeriod = (sum * 25) / (k - 1);
                    avFreq = 1000000 / avPeriod;
                    
                    printf("average period = %d us \r\n", avPeriod);
                    printf("average frequency = %d Hz \r\n", avFreq);
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

//-----------------------------------------------------------------------------
// Start an analog to digital conversion every 100uS.  Toggle RC1 so that users
// can check how fast conversions are being performed.
//-----------------------------------------------------------------------------

void myTMR0ISR(void) {
    static myTMR0states_t currState = MIC_IDLE;
    static uint16_t i = 0;
    uint8_t mic;
    ADCON0bits.GO_NOT_DONE = 1;
    
    switch(currState){
        case MIC_IDLE:
            i = 0;
            if(fillBuffer == true){
                currState = MIC_WAIT_FOR_TRIGGER;
            }
            break;
            
        case MIC_WAIT_FOR_TRIGGER:
            mic = ADRESH;
            //printf("mic value: %d \r\n", mic);
            if((mic > upperThreshold) || (mic < lowerThreshold)){
                currState = MIC_ACQUIRE;
            }
            break;
            
        case MIC_ACQUIRE:
            //printf("next \r\n");
            mic = ADRESH;
            buffer[i] = mic;
            i++;
            if(i == 512){
                i = 0;
                bufferFull = true;
                fillBuffer = false;
                currState = MIC_IDLE;
            }
            break;
            
        default:
            currState = MIC_IDLE;
            break;
    }
    TMR0_WriteTimer(0x10000 - (350 - TMR0_ReadTimer()));
    INTCONbits.TMR0IF = 0;
} // end myTMR0ISR