//--------------------------------------------------------------------
// Name:            Holly Hammons
// Date:            Fall 2020
// Purp:            Lab 6
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

// for transmit function
char IRtransmitBuffer[MAX_BUFFER_SIZE];
void transmitCharacterOverIR(char letter, uint16_t baudRate);
uint8_t mask;
uint16_t bitPeriod[5] = {53328, 13333, 6672, 1664, 832};

uint8_t checksum = 0;

// for isr
typedef enum{IDLE, RECIEVE_MSG, LAST_BYTE} myState_t;

void main(void) {
    uint8_t i;
    uint8_t baudRateSelected = 1;
    char cmd;
    char letter = '0';
    
    SYSTEM_Initialize();
    
    EUSART2_SetRxInterruptHandler(myEUSART2ISR);
    INTERRUPT_GlobalInterruptEnable();
    INTERRUPT_PeripheralInterruptEnable();
    
    EPWM2_LoadDutyValue(LED_OFF);

    // Delay so the Baud rate generator is stable and prints the splash screen correctly
    TMR1_WriteTimer(0);
    PIR1bits.TMR1IF = 0;
    while (TMR1_HasOverflowOccured() == false);

    // Since EUSART2 is double buffered, clear out any garbage with two reads from those buffers
    if (EUSART2_DataReady) (void) EUSART2_Read();
    if (EUSART2_DataReady) (void) EUSART2_Read();

    printf("inLab 6\r\n");
    printf("Receive and decode an IR packet\r\n");
    printf("Dev'21 Board wiring\r\n");
    printf("Install a jumper wire from RC0 to RB7 ONLY AFTER unplugging PICKit3\r\n");
    printf("Install a jumper over IR_TX header pins\r\n");
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
                    if(receiveNewMessage == true){
                        printf("receiveNewMessage = true\r\n");
                    }
                    else{
                        printf("receiveNewMessage = false\r\n");
                    }
                    
                    printf("-------------------------------------------------\r\n");
                    printf("?: help menu\r\n");
                    printf("o: k\r\n");
                    printf("Z: Reset processor\r\n");
                    printf("z: Clear the terminal\r\n");
                    printf("b: set the Baud rate of the sent characters\r\n");
                    printf("r: reset EUSART2\r\n");
                    printf("m: Enter in transmit Message\r\n");
                    printf("S: Send message in foreground\r\n");
                    printf("R: Receive message using EUSART2 ISR via IR decoder\r\n");
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
                            break;
                        case 1: SPBRGH2 = 0x34;
                            SPBRG2 = 0x14;
                            break;
                        case 2: SPBRGH2 = 0x1A;
                            SPBRG2 = 0x0B;
                            break;
                        case 3: SPBRGH2 = 0x06;
                            SPBRG2 = 0x83;
                            break;
                        case 4: SPBRGH2 = 0x03;
                            SPBRG2 = 0x41;
                            break;
                        default: SPBRGH2 = 0x34;
                            SPBRG2 = 0x14;
                            break;
                    }
                    printf("Baud rate assigned %02x:%02x\r\n", SPBRGH2, SPBRG2);
                    break;
                    
                //--------------------------------------------
                // reset EUSART2 in case it needs doing
                //--------------------------------------------                
                case 'r':
                    RCSTA2bits.CREN = 0; // Try restarting EUSART2
                    RCSTA2bits.CREN = 1;
                    printf("Just reset EUSART2\r\n");
                    break;
                    
                // read in a message
                case 'm':
                    printf("Enter a message, hit return when done.\r\n");
                    checksum = 0;
                    while(!EUSART1_DataReady);
                    char mes;
                    //uint8_t mesNum = mes;
                    if (EUSART1_DataReady){
                        uint8_t j = 0;
                        while(j < (MAX_BUFFER_SIZE - 2)){
                            mes = EUSART1_Read();
                            if(mes != '\r'){
                                IRtransmitBuffer[j] = mes;
                                checksum = checksum + (uint8_t)mes;
                                printf("%c", mes);
                                j++;
                            }
                            else{
                                IRtransmitBuffer[j] = '\0';
                                IRtransmitBuffer[j+1] = checksum;
                                printf("\r\n");
                                break;
                            }   
                        } 
                    }
                    break;
            
                //--------------------------------------------
                // Transmit the bits of a ASCII character
                //--------------------------------------------  
                
                case 'S':
                    i = 0;
                    while(IRtransmitBuffer[i] != '\0'){
                        transmitCharacterOverIR(IRtransmitBuffer[i], baudRateSelected);
                        i++;
                    }
                    transmitCharacterOverIR('\0', baudRateSelected);
                    transmitCharacterOverIR(checksum, baudRateSelected);
                    printf("Transmitted \r\n");
                    i = 0;
                    printf("Message: ");
                    while(IRtransmitBuffer[i] != '\0'){
                        printf("%c", IRtransmitBuffer[i]);
                        i++;
                    }
                    printf("\r\n");
                    printf("Checksum: %d\r\n", checksum);
                    break;    

                //--------------------------------------------
                // Read the a character from EUSART2 FIFO
                // It's safe to use EUSART2_Read because RC2IF = 1
                //--------------------------------------------
                case 'R':
                    if(receiveNewMessage == true){
                        uint8_t k = 0;
                        printf("Message: ");
                        while(IRrecieveBuffer[k] != '\0'){
                            printf("%c", IRrecieveBuffer[k]);
                            k++;
                        }
                        printf("\r\n");
                        printf("Checksum: %d\r\n", checksum);
                        receiveNewMessage = false;
                    }
                    else{
                        printf("No message, receiveNewMessage = false\r\n");
                    }  
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

void transmitCharacterOverIR(char letter, uint16_t baudRate){
    // Preface character with a '0' bit
    EPWM2_LoadDutyValue(LED_ON);
    TMR1_WriteTimer(0x10000 - bitPeriod[baudRate]);
    PIR1bits.TMR1IF = 0;
    while (TMR1_HasOverflowOccured() == false);

    // LSB first
    mask = 0b00000001;
    while (mask != 0) {
        if ((letter & mask) != 0) EPWM2_LoadDutyValue(LED_OFF);
        else EPWM2_LoadDutyValue(LED_ON);
        mask = mask << 1;
        TMR1_WriteTimer(0x10000 - bitPeriod[baudRate]);
        PIR1bits.TMR1IF = 0;
        while (TMR1_HasOverflowOccured() == false);
    }

    // Need a stop bit to break up successive bytes
    EPWM2_LoadDutyValue(LED_OFF);
    TMR1_WriteTimer(0x10000 - bitPeriod[baudRate]);
    PIR1bits.TMR1IF = 0;
    while (TMR1_HasOverflowOccured() == false);

    //printf("just sent %c    %x\r\n", letter, letter);
    letter += 1;
}