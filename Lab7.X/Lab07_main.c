// **** Include libraries here ****
// Standard libraries
#include <stdio.h>
#include <string.h>

//CSE13E Support Library
#include "BOARD.h"

// Microchip libraries
#include <xc.h>
#include <sys/attribs.h>
#include "Buttons.h"
#include "Adc.h"
#include "Oled.h"
#include "Ascii.h"
#include "Leds.h"


// **** Set any macros or preprocessor directives here ****
// Set a macro for resetting the timer, makes the code a little clearer.
#define TIMER_2HZ_RESET() (TMR1 = 0)
#define SIXTY 60
#define EIGHT 8
#define FIVE 5
#define LONG_PRESS 5

// **** Set any local typedefs here ****

typedef enum {
    SETUP, SELECTOR_CHANGE_PENDING, COOKING, RESET_PENDING
} OvenState;

typedef enum {
    BAKE, TOAST, BROIL
} CookMode;

typedef struct {
    OvenState state;
    uint16_t cookTimeLeft;
    uint16_t cookTimeStart;
    uint16_t temperature;
    uint16_t buttonPressTime;
    CookMode mode;
    //add more members to this struct
} OvenData;

// **** Declare any datatypes here ****

// **** Define any module-level, global, or external variables here ****
static char l1[100], l2[100], l3[100], l4BOFF[100], l4BON[100];
static uint8_t Button = BUTTON_EVENT_NONE;
static uint16_t clock = 0;
static uint8_t SETTING_SELECTOR = FALSE;
static uint16_t adcRead;
static uint16_t elapsedTime;
static uint16_t startingTemp = 300;
static uint16_t inMins;
static uint16_t inSecs;
static OvenData oven = {SETUP, 1, 1, 300, 0, BAKE};
static int count = 0;
static char LED_Adj;
static uint16_t adcReadTime;
static uint16_t TIMER_TICK = FALSE;
static uint8_t adcChange = FALSE;
static uint16_t ONE_EIGTH = 1 / 8;
static uint16_t REMAINDER;
static uint16_t LED_COUNT;
static uint16_t ticking;
static char S = 0xFF;
static char start = 0xFF;

// **** Put any helper functions here ****

void updateLED(void)
{
    LED_Adj = LEDS_GET();
    LED_Adj = LED_Adj << 1;
    LEDS_SET(LED_Adj);
}

void updateTemp(void)                         // helper function for updating temp
{
    oven.temperature = adcRead;
}

void updateTime(void)                         // helper function for updating time
{
    inMins = oven.cookTimeLeft / SIXTY;
    inSecs = oven.cookTimeLeft % SIXTY;
}

/*This function will update your OLED to reflect the state .*/
void updateOvenOLED(OvenData ovenData)
{
    //OledClear(OLED_COLOR_BLACK);
    //update OLED here
    updateTime();
    sprintf(l1, "|%s%s%s%s%s| Mode:  BAKE", OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF);
    sprintf(l2, "%s\n|     |  TIME: %u:%02u ", l1, inMins, inSecs);
    sprintf(l3, "%s\n|-----|  TEMP: %u%sF", l2, oven.temperature, DEGREE_SYMBOL);
    sprintf(l4BON, "%s\n|%s%s%s%s%s|", l3, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF);

    if (oven.mode == BAKE) {
        if (oven.state == COOKING || oven.state == RESET_PENDING) {

            sprintf(l1, "|%s%s%s%s%s| Mode:  BAKE", OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON);
            sprintf(l2, "%s\n|     |  TIME: %u:%02u ", l1, inMins, inSecs);
            sprintf(l3, "%s\n|-----|  TEMP: %u%sF", l2, oven.temperature, DEGREE_SYMBOL);
            sprintf(l4BON, "%s\n|%s%s%s%s%s|", l3, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON);
            OledDrawString(l4BON);
        }
        if (oven.state == SETUP || oven.state == SELECTOR_CHANGE_PENDING) {
            if (SETTING_SELECTOR == FALSE) {
                sprintf(l1, "|%s%s%s%s%s| Mode:  BAKE", OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF);
                sprintf(l2, "%s\n|     | > TIME: %u:%02u ", l1, inMins, inSecs);
                sprintf(l3, "%s\n|-----|  TEMP: %u%sF", l2, oven.temperature, DEGREE_SYMBOL);
                sprintf(l4BON, "%s\n|%s%s%s%s%s|", l3, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF);
                OledDrawString(l4BON);
            } else {
                sprintf(l1, "|%s%s%s%s%s| Mode:  BAKE", OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF);
                sprintf(l2, "%s\n|     |  TIME: %u:%02u ", l1, inMins, inSecs);
                sprintf(l3, "%s\n|-----| > TEMP: %u%sF", l2, oven.temperature, DEGREE_SYMBOL);
                sprintf(l4BON, "%s\n|%s%s%s%s%s|", l3, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF);
                OledDrawString(l4BON);
            }
        }
    }
    if (oven.mode == TOAST) {
        if (oven.state == COOKING || oven.state == RESET_PENDING) {
            sprintf(l1, "|%s%s%s%s%s| Mode:  TOAST", OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF);
            sprintf(l2, "%s\n|     |  TIME: %u:%02u ", l1, inMins, inSecs);
            sprintf(l3, "%s\n|-----|", l2);
            sprintf(l4BON, "%s\n|%s%s%s%s%s|", l3, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF);
            OledDrawString(l4BON);
        }
            if (oven.state == SETUP || oven.state == RESET_PENDING) {
                sprintf(l1, "|%s%s%s%s%s| Mode:  TOAST", OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF);
                sprintf(l2, "%s\n|     |  TIME: %u:%02u ", l1, inMins, inSecs);
                sprintf(l3, "%s\n|-----|", l2);
                sprintf(l4BON, "%s\n|%s%s%s%s%s|", l3, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON, OVEN_BOTTOM_ON);
                OledDrawString(l4BON);
           
        }
    }

    if (oven.mode == BROIL) {
        if (oven.state == COOKING || oven.state == RESET_PENDING) {
            sprintf(l1, "|%s%s%s%s%s| Mode:  BROIL", OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF, OVEN_TOP_OFF);
            sprintf(l2, "%s\n|     |  TIME: %u:%02u ", l1, inMins, inSecs);
            sprintf(l3, "%s\n|-----|  TEMP: 500%sF", l2, DEGREE_SYMBOL);
            sprintf(l4BON, "%s\n|%s%s%s%s%s|", l3, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF);
            OledDrawString(l4BON);
        }
        if (oven.state == SETUP || oven.state == RESET_PENDING) {
            sprintf(l1, "|%s%s%s%s%s| Mode:  BROIL", OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON, OVEN_TOP_ON);
            sprintf(l2, "%s\n|     | > TIME: %u:%02u ", l1, inMins, inSecs);
            sprintf(l3, "%s\n|-----|  TEMP: 500%sF", l2, DEGREE_SYMBOL);
            sprintf(l4BON, "%s\n|%s%s%s%s%s|", l3, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF, OVEN_BOTTOM_OFF);
            OledDrawString(l4BON);
        }
        
    }
    OledUpdate();
}

/*This function will execute your state machine.  
 * It should ONLY run if an event flag has been set.*/
void runOvenSM(void)
{
    //write your SM logic here.
    switch (oven.state) {
    case SETUP:
        if (Button & BUTTON_EVENT_4DOWN) {                 // check if button 4 is pressed
            oven.buttonPressTime = clock;
            LEDS_SET(0xFF);                                // turn on all LEDS
            oven.state = COOKING;
            ONE_EIGTH = (oven.cookTimeStart * FIVE) / EIGHT;
            REMAINDER = (oven.cookTimeStart * FIVE) % EIGHT;
            updateOvenOLED(oven);
            ticking = 0;                                    
        }
        if (Button & BUTTON_EVENT_3DOWN) {                 // check if button 3 is pressed
            oven.buttonPressTime = clock;                  // set press time equal to free running timer
            oven.state = SELECTOR_CHANGE_PENDING;          // set new state
        }
        if (adcChange) {                                   // if ADC is moved
            if (oven.mode == BAKE && SETTING_SELECTOR) {    
                adcRead = AdcRead();
                adcRead = (adcRead >> 2) + startingTemp;
                updateTemp();
                updateOvenOLED(oven);                      // math for updating temperature via adc
            } else {
                adcReadTime = AdcRead();                   // math for updating time via adc
                adcReadTime = (adcReadTime >> 2) + 1;
                oven.cookTimeStart = adcReadTime;
                oven.cookTimeLeft = adcReadTime;
                oven.state = SETUP;
                updateOvenOLED(oven);

            }
        }
        break;
    case SELECTOR_CHANGE_PENDING:                         
        if (Button & BUTTON_EVENT_3UP) {                   // if button 3 is not pressed
            elapsedTime = clock - oven.buttonPressTime;    // find elapsed time
            if (elapsedTime < LONG_PRESS) {               
                if (oven.mode == BROIL) {
                    oven.mode = BAKE;
                    oven.state = SETUP;
                    updateOvenOLED(oven);
                } else {
                    oven.mode++;                           // increment mode to get toast/broil
                    updateOvenOLED(oven);
                    oven.state = SETUP;
                }
            } else {
                if (SETTING_SELECTOR == 0) {               // condition to edit only time
                    SETTING_SELECTOR = 1;
                    oven.state = SETUP;                   
                    updateOvenOLED(oven);
                } else if (SETTING_SELECTOR == 1) {        // condition to edit only temperature
                    SETTING_SELECTOR = 0;
                    oven.state = SETUP;
                    updateOvenOLED(oven);
                }
            }
        }
        break;

    case COOKING:
        if (TIMER_TICK) {
            ticking++;
            if (REMAINDER > 0 && ticking == ONE_EIGTH + 1) {    
                updateLED();                               // check for timer remainder
                ticking = 0;
                REMAINDER--;
            }
            if (REMAINDER == 0 && ticking == ONE_EIGTH) {
                updateLED();                            
                ticking = 0;
                REMAINDER--;
            }
            if ((clock - oven.buttonPressTime) % FIVE == 0) {
                oven.cookTimeLeft--;                       // check if button press is divis by 5 --> means its pressed
                updateOvenOLED(oven);
            }
            if (oven.cookTimeLeft == 0) {                  // if 0 press 1 second has passed by
                oven.cookTimeLeft = oven.cookTimeStart;
                oven.state = SETUP;
                updateOvenOLED(oven);
                LEDS_SET(0x00);
            }

        }
        if (Button & BUTTON_EVENT_4DOWN) {                 
            oven.buttonPressTime = clock;
            oven.state = RESET_PENDING;
        }
        break;
    case RESET_PENDING:
        if (TIMER_TICK) {
            elapsedTime = clock - oven.cookTimeLeft;  
            if (elapsedTime >= LONG_PRESS) {
                oven.cookTimeLeft = oven.cookTimeStart;
                LEDS_SET(0x00);                    
                oven.state = SETUP;
                updateOvenOLED(oven);
            }
        } else {
            updateLED();
            updateOvenOLED(oven);
            oven.state = RESET_PENDING;
        }
        if (Button == BUTTON_EVENT_4UP) {
            oven.state = COOKING;
        }
        break;
    }
}

int main()
{
    LEDS_INIT();
    BOARD_Init();
    AdcInit();
    OledInit();
    //initalize timers and timer ISRs:
    // <editor-fold defaultstate="collapsed" desc="TIMER SETUP">

    // Configure Timer 2 using PBCLK as input. We configure it using a 1:16 prescalar, so each timer
    // tick is actually at F_PB / 16 Hz, so setting PR2 to F_PB / 16 / 100 yields a .01s timer.

    T2CON = 0; // everything should be off
    T2CONbits.TCKPS = 0b100; // 1:16 prescaler
    PR2 = BOARD_GetPBClock() / 16 / 100; // interrupt at .5s intervals
    T2CONbits.ON = 1; // turn the timer on

    // Set up the timer interrupt with a priority of 4.
    IFS0bits.T2IF = 0; //clear the interrupt flag before configuring
    IPC2bits.T2IP = 4; // priority of  4
    IPC2bits.T2IS = 0; // subpriority of 0 arbitrarily 
    IEC0bits.T2IE = 1; // turn the interrupt on

    // Configure Timer 3 using PBCLK as input. We configure it using a 1:256 prescaler, so each timer
    // tick is actually at F_PB / 256 Hz, so setting PR3 to F_PB / 256 / 5 yields a .2s timer.

    T3CON = 0; // everything should be off
    T3CONbits.TCKPS = 0b111; // 1:256 prescaler
    PR3 = BOARD_GetPBClock() / 256 / 5; // interrupt at .5s intervals
    T3CONbits.ON = 1; // turn the timer on

    // Set up the timer interrupt with a priority of 4.
    IFS0bits.T3IF = 0; //clear the interrupt flag before configuring
    IPC3bits.T3IP = 4; // priority of  4
    IPC3bits.T3IS = 0; // subpriority of 0 arbitrarily 
    IEC0bits.T3IE = 1; // turn the interrupt on;

    // </editor-fold>

    printf("Welcome to lmichell's Lab07 (Toaster Oven).  Compiled on %s %s.", __TIME__, __DATE__);

    //initialize state machine (and anything else you need to init) here
    //    updateTemp();
    //    updateOvenOLED(oven);
    while (1) {
        //Add main loop code here:
        //check for events
        // on event, run runOvenSM()
        // clear event flags

        if (Button != BUTTON_EVENT_NONE || adcChange || TIMER_TICK) {
            runOvenSM();
            Button = BUTTON_EVENT_NONE;
            adcChange = FALSE;
            TIMER_TICK = FALSE;
        }
    };
}

/*The 5hz timer is used to update the free-running timer and to generate TIMER_TICK events*/
void __ISR(_TIMER_3_VECTOR, ipl4auto) TimerInterrupt5Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 12;
    TIMER_TICK = TRUE;
    clock++;
    //add event-checking code here
}

/*The 100hz timer is used to check for button and ADC events*/
void __ISR(_TIMER_2_VECTOR, ipl4auto) TimerInterrupt100Hz(void)
{
    // Clear the interrupt flag.
    IFS0CLR = 1 << 8;

    //add event-checking code here
    Button = ButtonsCheckEvents();
    adcChange = AdcChanged();
}