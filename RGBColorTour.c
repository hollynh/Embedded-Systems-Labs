//--------------------------------------------------------------------
// Name:            Holly Hammons
// Date:            Fall 2020
// Purp:            Make RGB Color Tour
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
#include "mcc_generated_files/mcc.h"
#include <inttypes.h> 

#pragma warning disable 520     // warning: (520) function "xyz" is never called  
#pragma warning disable 1498    // fputc.c:16:: warning: (1498) pointer (unknown)

#define DUTY_INC    0x10
#define NUM_COLOR 6
uint8_t colorTour = false;
uint16_t timerInterval = 65436;
void TMR0_ISR(void);

void main(void) {
    uint16_t duty1 = 127;
    uint16_t duty2 = 127;
    uint16_t duty3 = 127;
    uint8_t i;
    char cmd;
    
    SYSTEM_Initialize();
    TMR0_SetInterruptHandler(TMR0_ISR); // Set ISR for TMR0 interrupt
    INTERRUPT_GlobalInterruptEnable(); // Enables interrupt system
    INTERRUPT_PeripheralInterruptEnable(); // Enables minor interrupts
    
    EPWM1_LoadDutyValue(0xFF);
    EPWM2_LoadDutyValue(0xFF);
    EPWM3_LoadDutyValue(0xFF);
    
    printf("Lab05\r\n");
    
    for (;;) {

        if (EUSART1_DataReady) { // wait for incoming data on USART
            cmd = EUSART1_Read();
            switch (cmd) { // and do what it tells you to do
                case '?':
                    printf("------------------------------\r\n");
                    printf("     Red:   0x3f\r\n");
                    printf("     Green: 0x3f\r\n");
                    printf("     Blue:  0x3f\r\n");
                    printf("?: Help menu\r\n");    
                    printf("s: status\r\n");
                    printf("Z: Reset processor\r\n");
                    printf("z: Clear the terminal\r\n");
                    printf("R/r: increase/decrease Red intensity\r\n");
                    printf("G/g: increase/decrease Green intensity\r\n");
                    printf("B/b: increase/decrease Blue intensity\r\n");
                    printf("C/c: start/stop color cycle\r\n");
                    printf("a: All LEDs off\r\n");
                    printf("+/-: increase/decrease the color tour speed.\r\n");
                    printf("------------------------------\r\n");
                    break;
                
                // status of duty cycle
                case 's':
                    printf("The value of duty cycle 1 is: \r\n");  
                    printf("The value of duty cycle 2 is: \r\n");  
                    printf("The value of duty cycle 3 is: \r\n");   
                    break;
                // reset processor    
                case 'Z':
                    for (i = 0; i < 40; i++) printf("\n");
                    RESET();
                    break;
                    
                // clear terminal
                case 'z':
                    for (i = 0; i < 40; i++) printf("\n");
                    break;    
                    
                // increase red intensity    
                case 'R':
                    if (duty1 <= 0xFF - DUTY_INC) duty1 += DUTY_INC;
                    EPWM1_LoadDutyValue(duty1);
                    break;
                    
                // decrease red intensity    
                case 'r':
                    if (duty1 >= DUTY_INC) duty1 -= DUTY_INC;
                    EPWM1_LoadDutyValue(duty1);
                    break;
                
                // increase green intensity    
                case 'G':
                    if (duty2 <= 0xFF - DUTY_INC) duty2 += DUTY_INC;
                    EPWM2_LoadDutyValue(duty2);
                    break;
                    
                // decrease green intensity    
                case 'g':
                    if (duty2 >= DUTY_INC) duty2 -= DUTY_INC;
                    EPWM2_LoadDutyValue(duty2);
                    break;
                    
                // increase blue intensity    
                case 'B':
                    if (duty3 <= 0xFF - DUTY_INC) duty3 += DUTY_INC;
                    EPWM3_LoadDutyValue(duty3);
                    break;
                    
                // decrease blue intensity    
                case 'b':
                    if (duty3 >= DUTY_INC) duty3 -= DUTY_INC;
                    EPWM3_LoadDutyValue(duty3);
                    break;
                
                    
                // start/stop color cycle
                case 'C':
                    colorTour = true;
                    //while(colorTour == true);
                    break;
                
                case 'c':
                    colorTour = false;
                    break;
                    
                // all LEDs off                    
                case 'a':
                    EPWM1_LoadDutyValue(0xFF);
                    EPWM2_LoadDutyValue(0xFF);
                    EPWM3_LoadDutyValue(0xFF);
                    break;
                    
                // increase/decrease the color tour speed    
                case '+':
                    timerInterval = timerInterval + 200;
                    break;
                    
                case '-':
                    if(timerInterval >= 400){
                        timerInterval = timerInterval - 200;
                    }
                    break;

                // unknown key
                default:
                    printf("Unknown key %c\r\n", cmd);
                    break;
            } // end switch
        } // end if
    } // end while 
} // end main
void myTMR0ISR (void){
    static uint8_t j = 1;
    
    uint16_t initRed[NUM_COLOR] = {0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF};
    uint16_t initGre[NUM_COLOR] = {0x00, 0xFF, 0xFF, 0xFF, 0x00, 0x00};
    uint16_t initBlu[NUM_COLOR] = {0xFF, 0xFF, 0x00, 0x00, 0x00, 0xFF};

    static uint8_t currRed = 0x00;
    static uint8_t currGre = 0x00;
    static uint8_t currBlu = 0xFF;
    
    EPWM1_LoadDutyValue(currRed);
    EPWM2_LoadDutyValue(currGre);
    EPWM3_LoadDutyValue(currBlu);
    
    uint16_t deltaRed[NUM_COLOR] = {0x0000, 0x0000, 0x0001, 0x0000, 0x0000, 0xFFFF}; 
    uint16_t deltaGre[NUM_COLOR] = {0x0001, 0x0000, 0x0000, 0xFFFF, 0x0000, 0x0000};
    uint16_t deltaBlu[NUM_COLOR] = {0x0000, 0xFFFF, 0x0000, 0x0000, 0x0001, 0x0000};
    if(colorTour == true){
        if(j < 6){
            if((currRed != initRed[j]) && (currGre != initGre[j]) && (currBlu != initBlu[j])){
                currRed = currRed + deltaRed[j];
                currGre = currGre + deltaGre[j];
                currBlu = currBlu + deltaBlu[j];
                INTCONbits.TMR0IF = 0;
                TMR0_WriteTimer(timerInterval);
            }
            else{
                j++;
                INTCONbits.TMR0IF = 0;
                TMR0_WriteTimer(timerInterval);
            }
        }
        else{
            j = 0;
            INTCONbits.TMR0IF = 0;
            TMR0_WriteTimer(timerInterval);
        } 
    }
}