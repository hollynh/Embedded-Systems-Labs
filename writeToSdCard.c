//--------------------------------------------------------------------
// Name:            Holly Hammons
// Date:            Fall 2021
// Purp:            Lab09
//
// Assisted:        no one
// Assisted by:     TA's
//
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
#include "sdCard.h"
#pragma warning disable 520     // warning: (520) function "xyz" is never called  3
#pragma warning disable 1498    // fputc.c:16:: warning: (1498) pointer (unknown)


void myTMR0ISR(void);

#define BLOCK_SIZE          512
#define MAX_NUM_BLOCKS      128
#define SINE_WAVE_ARRAY_LENGTH 26

// Large arrays need to be defined as global even though you may only need to 
// use them in main.  This quirk will be important in the next two assignments.
uint8_t sdCardBuffer[BLOCK_SIZE];

uint8_t redBuffer[BLOCK_SIZE];
uint8_t blueBuffer[BLOCK_SIZE];

uint8_t fillBuffer = false;

uint8_t redFull = false;
uint8_t blueFull = false;

uint16_t sampleRate = 1600;

typedef enum  {MIC_IDLE, MIC_ACQUIRE} myTMR0states_t;

const uint8_t sin[SINE_WAVE_ARRAY_LENGTH] = {128, 159, 187, 213, 233, 248, 255, 255, 248, 233, 213, 187, 159, 128, 97, 69, 43, 23, 8, 1, 1, 8, 23, 43, 69, 97};

//----------------------------------------------
// Main "function"
//----------------------------------------------

void main(void) {

    uint8_t status;
    uint16_t i;
    uint32_t sdCardAddress = 0x00000000;
    char cmd, letter;

    letter = '0';

    SYSTEM_Initialize();
    CS_SetHigh();
    ADC_SelectChannel(MIC_PIN);

    // Provide Baud rate generator time to stabilize before splash screen
    TMR0_WriteTimer(0x0000);
    INTCONbits.TMR0IF = 0;
    while (INTCONbits.TMR0IF == 0);

    TMR0_SetInterruptHandler(myTMR0ISR);

    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();

    printf("inLab 09\r\n");
    printf("SD card testing\r\n");
    printf("Dev'21\r\n");
    printf("No configuration of development board\r\n> "); // print a nice command prompt    
    
    SPI2_Close();
    SPI2_Open(SPI2_DEFAULT);
    
    for (;;) {

        if (EUSART1_DataReady) { // wait for incoming data on USART
            cmd = EUSART1_Read();
            switch (cmd) { // and do what it tells you to do

                    //--------------------------------------------
                    // Reply with help menu
                    //--------------------------------------------
                case '?':
                    //printf("\r\n-------------------------------------------------\r\n");
                    //printf("SD card address:  ");
                    //printf("%04x", sdCardAddress >> 16);
                    //printf(":");
                    //printf("%04x", sdCardAddress & 0X0000FFFF);
                    //printf("\r\n");
                    printf("-------------------------------------------------\r\n");
                    printf("?: help menu\r\n");
                    printf("o: k\r\n");
                    printf("Z: Reset processor\r\n");
                    printf("z: Clear the terminal\r\n");
                    printf("-------------------------------------------------\r\n");
                    printf("i: Initialize SD card\r\n");
                    printf("-------------------------------------------------\r\n");
                    printf("a/A decrease/increase read address\r\n");
                    printf("r: read a block of %d bytes from SD card\r\n", BLOCK_SIZE);
                    printf("1: write a perfect 26 value sine wave to the SD card\r\n", BLOCK_SIZE);
                    printf("-------------------------------------------------\r\n");
                    printf("+/-: Increase/Decrease the sample rate by 10 us\r\n");
                    printf("W: Write microphone => SD card at 1600 us\r\n", BLOCK_SIZE);
                    printf("s: spool memory to a csv file\r\n", BLOCK_SIZE);
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

                    //-------------------------------------------- 
                    // Init SD card to get it read to perform read/write
                    // Will hang in infinite loop on error.
                    //--------------------------------------------    
                case 'i':
                    SPI2_Close();
                    SPI2_Open(SPI2_DEFAULT);    // Reset the SPI channel for SD card communication
                    SDCARD_Initialize(true);
                    break;

                    //--------------------------------------------
                    // Increase or decrease block address
                    //--------------------------------------------                 
                case 'A':
                    
                case 'a':
                    if (cmd == 'a') {
                        sdCardAddress -= BLOCK_SIZE;
                        if (sdCardAddress >= 0x04000000) {
                            printf("Underflowed to high address\r\n");
                            sdCardAddress = 0x04000000 - BLOCK_SIZE;
                        } else {
                            printf("Decreased address\r\n");
                        }
                    } else {
                        sdCardAddress += BLOCK_SIZE;
                        if (sdCardAddress >= 0x04000000) {
                            printf("Overflowed to low address\r\n");
                            sdCardAddress = 0x00000000;
                        } else {
                            printf("Increased address\r\n");
                        }
                    }

                    // 32-bit integers need printed as a pair of 16-bit integers
                    printf("SD card address:  ");
                    printf("%04x", sdCardAddress >> 16);
                    printf(":");
                    printf("%04x", sdCardAddress & 0X0000FFFF);
                    printf("\r\n");
                    break;
                 
                // r: read a block of BLOCK_SIZE bytes from SD card    
                case 'r':
                    READ_TIME_PIN_SetHigh();
                    SDCARD_ReadBlock(sdCardAddress, sdCardBuffer);
                    READ_TIME_PIN_SetLow();
                    printf("Read block: \r\n");
                    printf("    Address:    ");
                    printf("%04x", sdCardAddress >> 16);
                    printf(":");
                    printf("%04x", sdCardAddress & 0X0000FFFF);
                    printf("\r\n");
                    hexDumpBuffer(sdCardBuffer);
                    break;
                    
                case '1':
                    
                    printf("Currently storing sin waves. Press any key to exit. \r\n");
                    uint16_t j = 0;
                    uint8_t k = 0;
                    while(!EUSART1_DataReady){
                        for (i = 0; i < BLOCK_SIZE; i++){                            
                            redBuffer[i] = sin[k];
                            k++;
                            if(k == 26){
                                k = 0;
                            }
                        }
                        for (i = 0; i < BLOCK_SIZE; i++) sdCardBuffer[i] = redBuffer[i];
                        SDCARD_WriteBlock(sdCardAddress, sdCardBuffer);
                        
                        while ((status = SDCARD_PollWriteComplete()) == WRITE_NOT_COMPLETE);
                        
                        //need because it hangs without it
                        INTCONbits.TMR0IF = 0;
                        TMR0_WriteTimer(0);
                        while(INTCONbits.TMR0IF == 0);
                        
                        sdCardAddress += 512;
                        j++; 
                        if(j >= MAX_NUM_BLOCKS){
                            break;
                        }
                    }
                    if(EUSART1_DataReady){
                        uint8_t temp2 = EUSART1_Read();
                    }
                    printf("Number of sin wave blocks stored: %d \r\n", j);
                    break;
                
                // increase sample rate 
                case'+':
                    if((sampleRate + 160) >= 320){
                        sampleRate = sampleRate + 160;
                        printf("New Sample Rate: %d \r\n", sampleRate);
                    }
                    else{
                        printf("Can not increment further, sample rate = %d \r\n", sampleRate);  
                    }
                    break;
                
                // decrease sample rate   
                case '-':
                    if((sampleRate - 320) >= 320){
                        sampleRate = sampleRate - 320;
                        printf("New Sample Rate: %d \r\n", sampleRate);  
                    }
                    else{
                        printf("Can not decrement further, sample rate = %d \r\n", sampleRate);  
                    }
                    break;
                    
                case 'W':
                    printf("Press any key to start recording audio and press any key to stop recording. \r\n");
                    while(!EUSART1_DataReady);
                    uint8_t temp = EUSART1_Read();
                    sdCardAddress = 0;
                    redFull = false;
                    blueFull = false;
                    fillBuffer = true;
                    printf("Recording now. \r\n");
                    while(!EUSART1_DataReady){
                        while(redFull == false);
                        SDCARD_WriteBlock(sdCardAddress, redBuffer);
                        while ((status = SDCARD_PollWriteComplete()) == WRITE_NOT_COMPLETE);
                        sdCardAddress =  sdCardAddress + BLOCK_SIZE;
                        redFull = false;
                
                        while(blueFull == false);
                        SDCARD_WriteBlock(sdCardAddress, blueBuffer);
                        while ((status = SDCARD_PollWriteComplete()) == WRITE_NOT_COMPLETE);
                        sdCardAddress = sdCardAddress + BLOCK_SIZE;
                        blueFull = false;
                    }
                    fillBuffer = false;
                    temp = EUSART1_Read();
                    printf("Done writing to SD card. \r\n");
                    break;
                    
                case 's':
                    printf("You may terminate spooling at anytime with a keypress. \r\n");
                    printf("To spool terminal contents into a file follow these instructions: \r\n");
                    printf("Right mouse click on the upper left of the PuTTY window \r\n");
                    printf("Select:     Change settings... \r\n");
                    printf("Select:     Logging \r\n");
                    printf("Select:     Session logging: All session output \r\n");
                    printf("Log file name:  Browse and provide a .csv extension to your file name \r\n");
                    printf("Select:     Apply \r\n");
                    printf("Press any key to start \r\n");
                    while(!EUSART1_DataReady);
                    uint8_t temp1 = EUSART1_Read();
                    uint16_t index = 0;
                    while(!EUSART1_DataReady){
                         SDCARD_ReadBlock(sdCardAddress, sdCardBuffer);
                         for(uint16_t i = 0; i < BLOCK_SIZE; i++){
                            printf("%d \r\n", sdCardBuffer[i]); 
                        }
                        sdCardAddress += 512;
                        index++;
                        if(index == MAX_NUM_BLOCKS){
                            break;
                        }
                    }
                    if(EUSART1_DataReady){
                        temp1 = EUSART1_Read();
                    }
                    printf("Spooled %d out of the %d blocks. \r\n", index, MAX_NUM_BLOCKS);
                    printf("To close the file follow these instructions: \r\n");
                    printf("Right mouse click on the upper left of the PuTTY window \r\n");
                    printf("Select:     Change settings... \r\n");
                    printf("Select:     Logging \r\n");
                    printf("Select:     Session  logging: None \r\n");
                    printf("Select:     Apply \r\n");
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


//----------------------------------------------
// As configured, we are hoping to get a toggle
// every 100us - this will require some work.
//
// You will be starting an ADC conversion here and
// storing the results (when you reenter) into a global
// variable and setting a flag, alerting main that 
// it can read a new value.
//
// !!!MAKE SURE THAT TMR0 has 0 TIMER PERIOD in MCC!!!!
//----------------------------------------------

void myTMR0ISR(void) {
    static myTMR0states_t currState;
    static char color = 'R';
    uint8_t mic = ADRESH;
    static uint16_t i;
    ADCON0bits.GO_NOT_DONE = 1;
    
    switch(currState){
        case MIC_IDLE:
            i = 0;
            if(fillBuffer == true){
                currState = MIC_ACQUIRE;
            }
            break;
      
        case MIC_ACQUIRE:
            if((color == 'R') && (redFull == false)){
                redBuffer[i] = mic;
                i++;
                if(i == BLOCK_SIZE){
                    redFull = true;
                    color = 'B';
                    i = 0;
                }
            }
            else if((color == 'B') && (blueFull == false)){
                blueBuffer[i] = mic;
                i++;
                if(i == BLOCK_SIZE){
                    blueFull = true;
                    color = 'R';
                    i = 0;
                }
            }
            if(fillBuffer == false){
                currState = MIC_IDLE;
            }
            break;
            
        default:
            currState = MIC_IDLE;
            break;
    }
    TMR0_WriteTimer(0x10000 - (sampleRate - TMR0_ReadTimer()));
    INTCONbits.TMR0IF = 0;
} // end myTMR0ISR