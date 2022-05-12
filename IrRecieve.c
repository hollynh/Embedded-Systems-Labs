//--------------------------------------------------------------------
// Name:            Holly Hammons
// Date:            Fall 2021
// Purp:            Lab 7
//
// Assisted:        no one
// Assisted by:     TAs 
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

#define LED_ON      25 
#define LED_OFF     0
#define MAX_BUFFER_SIZE 32 //change this

//----------------------------------------------
// Main "function"
//----------------------------------------------
void myEUSART2ISR(void);
char IRrecieveBuffer[MAX_BUFFER_SIZE];
uint8_t receiveBusy = false;
uint8_t receiveNewMessage = false;

uint16_t bitPeriod[5] = {53328, 13333, 6672, 1664, 832};

uint8_t checksum = 0;

// for receive ISR
typedef enum{IDLE, RECIEVE_MSG, LAST_BYTE} myState_t;

// for transmit ISR
void myTMR1ISR(void);
char transmitIRBuffer[MAX_BUFFER_SIZE];
uint8_t transmitStart = false;
uint8_t transmitBusy = false;
uint16_t bitPeriodInTMR1Counts = 13333;        // 1200 Baud default
uint8_t userEnter8bit(void);
uint8_t sourceAd = 0;
uint8_t destAd = 0;
uint8_t newCharacterToSend = false;

void decodeIntoASCII(char msg[]);

typedef enum{TX_IDLE, TX_START_BIT, TX_DATA_BITS, TX_STOP_BIT} myTXstates_t;

void main(void) {
    uint8_t i;
    uint8_t baudRateSelected = 1;
    char cmd;
    char letter = '0';
    
    SYSTEM_Initialize();
    
    // PUT THIS HERE- THIS WAS CAUSING A HANG IN PUTTY!
    PIR1bits.TMR1IF = 0;
    while (PIR1bits.TMR1IF == 0);
    
    
    EUSART2_SetRxInterruptHandler(myEUSART2ISR);
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();
    
    EPWM2_LoadDutyValue(LED_OFF);

    // Delay so the Baud rate generator is stable and prints the splash screen correctly
    TMR1_WriteTimer(0);
    TMR1_SetInterruptHandler(myTMR1ISR);
    

    // Since EUSART2 is double buffered, clear out any garbage with two reads from those buffers
    if (EUSART2_DataReady) (void) EUSART2_Read();
    if (EUSART2_DataReady) (void) EUSART2_Read();

    for (;;) {

        if (EUSART1_DataReady) { // wait for incoming data on USART
            cmd = EUSART1_Read();
            switch (cmd) { // and do what it tells you to do

                //--------------------------------------------
                // Reply with help menu
                //--------------------------------------------
                case '?':
                    printf("-------------------------------------------------\r\n");
                    printf("TX msg: ");
                    uint8_t i = 2;
                    while(transmitIRBuffer[i] != '\0'){
                        printf("%c", transmitIRBuffer[i]);
                        i++;
                    }
                    printf("\r\n");
                    
                    printf("My identity: %d \r\n", sourceAd);
                    printf("TX address: %d \r\n", destAd);
                    
                    if(receiveNewMessage == true){
                        printf("receiveNewMessage = true\r\n");
                    }
                    else{
                        printf("receiveNewMessage = false\r\n");
                    }
                    
                    switch (baudRateSelected) {
                        case 0: printf("300 Baud\r\n");
                            break;
                        case 1: printf("1200 Baud\r\n");
                            break;
                        case 2: printf("2400 Baud\r\n");
                            break;
                        case 3: printf("9600 Baud\r\n");
                            break;
                        case 4: printf("19200 Baud\r\n");
                            break;
                        default: printf("1200 Baud\r\n");
                            break;
                    }
                    
                    printf("-------------------------------------------------\r\n");
                    printf("?:   help menu\r\n");
                    printf("o:   k\r\n");
                    printf("Z:   Reset processor\r\n");
                    printf("z:   Clear the terminal\r\n");
                    printf("b:   set Baud rate\r\n");
                    printf("m:   create a NULL terminated message with SRC and DEST prefix.\r\n");
                    printf("s:   set Source transmit identity \r\n");
                    printf("d:   set Destination transmit target \r\n");
                    printf("S:   Send message using TMR1 ISR\r\n");
                    printf("R:   Receive message using EUSART2 ISR via IR decoder\r\n");
                    printf("M:   Monitor all IR traffic. \r\n");
                    printf("x/X: decode tx/RX message. \r\n");
                    printf("-------------------------------------------------\r\n");
                    break;

                //--------------------------------------------
                // Reply with "ok", used for PC to PIC test
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
                // Set the Baud rate - use MCC EUSART2 configuration register tab            
                //--------------------------------------------                    
                case 'b':
                    printf("Choose the index of the target baud rate\r\n");
                    printf("0: 300 baud\r\n");
                    printf("1: 1200 baud\r\n");
                    printf("2: 2400 baud\r\n");
                    printf("3: 9600 baud\r\n");
                    printf("4: 19200 baud\r\n");
                    while (EUSART1_DataReady);
                    baudRateSelected = EUSART1_Read() - '0';
                    switch (baudRateSelected) {
                        case 0: SPBRGH2 = 0xD0;
                            SPBRG2 = 0x55;
                            bitPeriodInTMR1Counts = 53328;
                            break;
                        case 1: SPBRGH2 = 0x34;
                            SPBRG2 = 0x14;
                            bitPeriodInTMR1Counts = 13333;
                            break;
                        case 2: SPBRGH2 = 0x1A;
                            SPBRG2 = 0x0B;
                            bitPeriodInTMR1Counts = 6672;
                            break;
                        case 3: SPBRGH2 = 0x06;
                            SPBRG2 = 0x83;
                            bitPeriodInTMR1Counts = 1664;
                            break;
                        case 4: SPBRGH2 = 0x03;
                            SPBRG2 = 0x41;
                            bitPeriodInTMR1Counts = 832;
                            break;
                        default: SPBRGH2 = 0x34;
                            SPBRG2 = 0x14;
                            bitPeriodInTMR1Counts = 13333;
                            break;
                    }
                    printf("Baud rate assigned %02x:%02x\r\n", SPBRGH2, SPBRG2);
                    break;
                   
                case 'm':
                    printf("Enter a message, hit return when done.\r\n");
                    checksum = 0;
                    while(!EUSART1_DataReady);
                    char mes;
                    transmitIRBuffer[0] = sourceAd;
                    transmitIRBuffer[1] = destAd;
                    if (EUSART1_DataReady){
                        uint8_t j = 2;
                        while(j < (MAX_BUFFER_SIZE - 2)){
                            mes = EUSART1_Read();
                            if(mes != '\r'){
                                transmitIRBuffer[j] = mes;
                                checksum = checksum + (uint8_t)mes;
                                printf("%c", mes);
                                j++;
                            }
                            else{
                                transmitIRBuffer[j] = '\0';
                                transmitIRBuffer[j+1] = checksum;
                                printf("\r\n");
                                break;
                            }   
                        } 
                    }
                    break;
                
                // for storing source address
                case 's':
                    sourceAd = userEnter8bit();
                    while(EUSART1_DataReady);
                    printf("\r\n");
                    break;
                 
                // for storing destination address
                case 'd':
                    destAd = userEnter8bit();
                    while(EUSART1_DataReady);
                    printf("\r\n");
                    break;
                //--------------------------------------------
                // Transmit the bits of a ASCII character
                //--------------------------------------------  
                    
                case 'S':
                    newCharacterToSend = true;
                    transmitStart = true;
                    while(transmitStart == true);
                    printf("Transmitted \r\n");
                    printf("Message:            ");
                    uint8_t d = 2;
                    while(transmitIRBuffer[d] != '\0'){
                        printf("%c", transmitIRBuffer[d]);
                        d++;
                    }
                    printf("\r\n");
                    printf("Checksum computed:  %d \r\n", checksum);
                    printf("Sender address:     %d \r\n", transmitIRBuffer[0]);
                    printf("Target address:     %d \r\n", transmitIRBuffer[1]);
                    break;    

                //--------------------------------------------
                // Read the a character from EUSART2 FIFO
                // It's safe to use EUSART2_Read because RC2IF = 1
                //--------------------------------------------
                case 'R':
                    if(receiveNewMessage == true){
                        uint8_t i = 1;
                        printf("Received \r\n");
                        printf("Message:            ");
                        while(IRrecieveBuffer[i] != '\0'){
                            printf("%c", IRrecieveBuffer[i]);
                            i++;
                        }
                        printf("\r\n");
                        printf("Checksum computed:  %d\r\n", checksum);
                        printf("Checksum received:  %d\r\n", IRrecieveBuffer[i + 1]);
                        printf("Sender address:     %d \r\n", IRrecieveBuffer[0]);
                        printf("Target address:     %d \r\n", IRrecieveBuffer[1]);
                        receiveNewMessage = false;
                    }
                    else{
                        printf("No message, receiveNewMessage = false\r\n");
                    }  
                    break;
                    
                case 'M':
                    ;
                    uint8_t k = 0;
                    printf("Hit any key to exit \r\n");
                    if(receiveNewMessage == true){
                        printf("SRC DEST    CHECK   MESSAGE \r\n");
                        // MAY NEED TO ADJUST FORMATTING
                        printf("  %d  %d      %d   ",transmitIRBuffer[0], transmitIRBuffer[1], checksum);
                        while(IRrecieveBuffer[k] != '\0'){
                            printf("%c", IRrecieveBuffer[k]);
                            k++;
                        }
                        while(!EUSART1_DataReady);
                        uint8_t temp = EUSART1_Read();
                        printf("Exited case 'M' \r\n");
                        break;
                    }
                    else{
                        printf("receiveNewMessage = false, no data to display \r\n");
                        while(!EUSART1_DataReady);
                        uint8_t temp = EUSART1_Read();
                        printf("Exited case 'M' \r\n");
                        break;
                    }
                    break;
                    //--------------------------------------------
                    // If something unknown is hit, tell user
                    //--------------------------------------------
                case 'x':
                    printf("TX buffer contents \r\n");
                    decodeIntoASCII(transmitIRBuffer);
                    break;
                    
                case 'X':
                    printf("RX buffer contents \r\n");
                    decodeIntoASCII(IRrecieveBuffer);
                    break;
                    
                default:
                    printf("Unknown key %c\r\n", cmd);
                    break;
            } // end switch            
        } // end if
    } // end while 
} // end main

 void myEUSART2ISR(void){
    static uint8_t i = 0;
    static myState_t currState = IDLE;
    char msgRec;
    
    switch(currState){
        case IDLE:
            msgRec = RCREG2;
            IRrecieveBuffer[i] = msgRec;
            i++;
            receiveBusy = true;
            currState = RECIEVE_MSG;
            break;
        case RECIEVE_MSG:
            msgRec = RCREG2;
            if(msgRec != '\0'){
                IRrecieveBuffer[i] = msgRec;
                i++; 
            }
            else{
                IRrecieveBuffer[i] = msgRec;
                i++;
                currState = LAST_BYTE;
            }
            break;
        case LAST_BYTE:
            IRrecieveBuffer[i] = RCREG2;
            receiveNewMessage = true;
            receiveBusy = false;
            currState = IDLE;
            i = 0;
            break;
    }
    PIR3bits.RC2IF = 0;
}

void myTMR1ISR(void) {
    static myTXstates_t tmr1ISRstate = TX_IDLE;
    static uint8_t mask = 0b00000001;
    static uint8_t n = 0;
    static char letter;
    static uint8_t checkSumSent = false;
    
    switch (tmr1ISRstate) {
    case TX_IDLE:
        if(transmitStart == true){
            n = 0;
            tmr1ISRstate = TX_START_BIT;
            transmitBusy = true;
        }
        break;
    case TX_START_BIT:
        mask = 0b00000001;
        EPWM2_LoadDutyValue(LED_ON);
        letter = transmitIRBuffer[n];
        tmr1ISRstate = TX_DATA_BITS;
        break;
    case TX_DATA_BITS:
        if (mask == 0) {
            tmr1ISRstate = TX_STOP_BIT;
            EPWM2_LoadDutyValue(LED_OFF);
        } else {
            if ((letter & mask) != 0) EPWM2_LoadDutyValue(LED_OFF);
            else EPWM2_LoadDutyValue(LED_ON);
        }
        mask = mask << 1;
        break;  
    case TX_STOP_BIT:
        EPWM2_LoadDutyValue(LED_OFF);
        if(letter == '\0'){
            checkSumSent = true;
            tmr1ISRstate = TX_START_BIT;
        }
        else if(checkSumSent == true){
           transmitStart = false;
           transmitBusy = false;
           checkSumSent = false;
           tmr1ISRstate = TX_IDLE; 
        }
        else{
           tmr1ISRstate = TX_START_BIT;
        }
        n++;
        break;
    default:
        tmr1ISRstate = TX_IDLE;
        break;
    }
    
    
    TMR1_WriteTimer(0x10000 - 13333); // 6666 = 2400 Baud
    PIR1bits.TMR1IF = 0;
}

uint8_t userEnter8bit(void){
    printf("Enter the address, hit return when done.\r\n");
    uint8_t currSum = 0;
    while(!EUSART1_DataReady);
    char val;
    val = EUSART1_Read();
    while(val != '\r'){
        if(((val - 48) >= 0) && ((val - 48) <= 9)){
        currSum = (currSum * 10) + (val - 48);
        printf("%c", val);
        }
        val = EUSART1_Read();
    }
    return currSum;
}

void decodeIntoASCII(char msg[]){
    uint8_t i = 0;
    while(msg[i] != '\0'){
        printf("    %2d:     %02x    %c\r\n",    i,   msg[i],  msg[i]);
        i++;
    } 
    printf("    %2d:     %02x    %c\r\n",    i,   msg[i],  msg[i]);
    i++;
    printf("    %2d:     %02x    %c\r\n",    i,   msg[i],  msg[i]);
}