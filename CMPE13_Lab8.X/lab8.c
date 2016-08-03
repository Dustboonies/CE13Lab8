// **** Include libraries here ****
// Standard C libraries


//CMPE13 Support Library
#include "BOARD.h"
#include "Morse.h"
#include "Oled.h"

// Microchip libraries
#include <xc.h>
#include <plib.h>

// User libraries


// **** Set any macros or preprocessor directives here ****
// Specify a bit mask for setting/clearing the pin corresponding to BTN4. Should only be used when
// unit testing the Morse event checker.
#define BUTTON4_STATE_FLAG (1 << 7)

// **** Declare any data types here ****

// **** Define any module-level, global, or external variables here ****
MorseEvent mevent;
int mflag = 0;
char display[100];
char *clet = " ";
char *csymb = " ";
char *templet = " ";
char *tempsymb = " ";
// **** Declare any function prototypes here ****
void updateScreen();
void clrSymb();
void clrLet();
void appendBot();
void appendTop();

int main(void)
{
    BOARD_Init();

    // Configure Timer 2 using PBCLK as input. We configure it using a 1:16 prescalar, so each timer
    // tick is actually at F_PB / 16 Hz, so setting PR2 to F_PB / 16 / 100 yields a .01s timer.
    OpenTimer2(T2_ON | T2_SOURCE_INT | T2_PS_1_16, BOARD_GetPBClock() / 16 / 100);

    // Set up the timer interrupt with a medium priority of 4.
    INTClearFlag(INT_T2);
    INTSetVectorPriority(INT_TIMER_2_VECTOR, INT_PRIORITY_LEVEL_4);
    INTSetVectorSubPriority(INT_TIMER_2_VECTOR, INT_SUB_PRIORITY_LEVEL_0);
    INTEnable(INT_T2, INT_ENABLED);

    /******************************************************************************
     * Your code goes in between this comment and the following one with asterisks.
     *****************************************************************************/
    OledInit();
    MorseInit();
    while (1) {
        if (mflag) {
            if (mevent == MORSE_EVENT_DOT) {
                MorseDecode(MORSE_CHAR_DOT);
            } else if (mevent == MORSE_EVENT_DASH) {
                MorseDecode(MORSE_CHAR_DASH);
            } else if (mevent == MORSE_EVENT_INTER_LETTER) {
                templet[0] = MorseDecode(MORSE_CHAR_END_OF_CHAR);
            } else if (mevent == MORSE_EVENT_INTER_WORD) {
                MorseDecode(MORSE_CHAR_DECODE_RESET);
            }
            updateScreen();
            mevent = 0;
            mflag = 0;
        }
    }


    /******************************************************************************
     * Your code goes in between this comment and the preceding one with asterisks.
     *****************************************************************************/
    while (1);
}

void __ISR(_TIMER_2_VECTOR, IPL4AUTO) TimerInterrupt100Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 8;

    //******** Put your code here *************//
    mevent = MorseCheckEvents();
    if (mevent != MORSE_EVENT_NONE) {
        mflag = 1;
    }
}

void updateScreen()
{
    if (mevent == MORSE_EVENT_DOT) {
        tempsymb = ".";
        appendBot();
    } else if (mevent == MORSE_EVENT_DASH) {
        tempsymb = "-";
        appendBot();
    } else if (mevent == MORSE_EVENT_INTER_LETTER) {
        clrSymb();
        appendTop();
    } else if (mevent == MORSE_EVENT_INTER_WORD) {
        clrSymb();
        appendTop();
    }
    sprintf(display, "%s\n\n\n%s", clet, csymb);
    //    printf("\n%s", display);
    OledDrawString(display);
    OledUpdate();
}

void clrSymb(char* str)
{
    memset(csymb, 0, sizeof (csymb));
}

void clrLet(char* str)
{
    memset(clet, 0, sizeof (clet));
}

void appendBot()
{
    sprintf(csymb, "%s%s", csymb, tempsymb);
}

void appendTop()
{
    sprintf(clet, "%s%s", clet, templet);
}