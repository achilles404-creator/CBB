/* 
 * File:   main.c
 * Author: Akshay
 *
 * Created on 18 August, 2026, 9:09 PM
 */

#include <xc.h>
#include <string.h>
#include "cbb.h"
#include "i2c.h"
#include "adc.h"
#include "clcd.h"
#include "uart.h"
#include "timers.h"
#include "ds1307.h"
#include "digital_keypad.h"
#include "external_eeprom.h"

#define _XTAL_FREQ 20000000

#pragma config WDTE = OFF        // Watchdog Timer Enable bit (WDT disabled)

void init_config(void)
{
    init_digital_keypad();
    init_i2c(100000);
    init_ds1307();
    init_clcd();
    init_adc();
    init_timer2();
    init_uart(9600);
    
    PEIE = 1;
    GIE = 1;
}

unsigned char *gr[] = {"GN", "GR", "G1", "G2", "G3", "G4"};

extern unsigned char sec;

void main(void) 
{
    unsigned char screen_flag = DASH_BOARD;
    unsigned char reset_flag = RESET_NULL;
    
    unsigned char key;
    unsigned char event[3] = "ON";
    unsigned char gear = 0;
    unsigned char speed = 0;
    
    unsigned char menu_pos;
    
    init_config();
    
    log_event(event, 0);

    while (1) 
    {
        speed = (unsigned char)(read_adc(CHANNEL0) / 10);       
        if(speed > 99)
            speed = 99;
        
        key = read_digital_keypad(STATE);
             
        if(key ==  SW1)
        {
            strcpy(event, "C ");
            log_event(event, speed);
        }
        else if(key == SW2)
        {
            if(gear < 5)
            gear++;
            strcpy(event, gr[gear]);
            log_event(event, speed);
        }
        else if(key == SW3)
        {
            if(gear > 0)
            gear--;
            strcpy(event, gr[gear]);
            log_event(event, speed);
        }
        if((key == SW4 || key == SW5) && screen_flag == DASH_BOARD)
        {
            screen_flag = PASS_SCREEN;
            reset_flag = RESET_PASSWORD;
            clear_screen();
            clcd_print("ENTER PASSWORD", LINE1(0));
            clcd_write(DISP_ON_AND_CURSOR_ON, INST_MODE);
            __delay_us(100);
            
            TMR2ON = 1;
        }
        
        if (screen_flag == MENU_SCREEN && key == L_SW4)
        {
            switch(menu_pos)
            {
                case 0:
                    screen_flag = LOG_SCREEN;
                    reset_flag = RESET_LOG;
                    clear_screen();
                    break;

                case 1:
                    clear_log();

                    clear_screen();
                    clcd_print("  LOG  CLEARED  ", LINE1(0));
                    clcd_print("  SUCCESSFULLY  ", LINE2(0));
                    
                    TMR2ON = 1;
                    
                    sec = 1;
                    while(sec != 0)
                    {
                    }

                    TMR2ON = 0;

                    screen_flag = MENU_SCREEN;
                    reset_flag = RESET_MENU;
                    clear_screen();
                    break;

                case 2:
                    download_log();

                    clear_screen();
                    clcd_print("  LOG DOWNLOAD  ", LINE1(0));
                    clcd_print("   SUCCESSFUL   ", LINE2(0));

                    TMR2ON = 1;

                    sec = 1;
                    while(sec != 0)
                    {
                    }

                    TMR2ON = 0;

                    screen_flag = MENU_SCREEN;
                    reset_flag = RESET_MENU;
                    clear_screen();
                    break;

                case 3:
                    screen_flag = SET_TIME_SCREEN;
                    reset_flag = RESET_SET_TIME;
                    clear_screen();
                    break;

                case 4:
                    screen_flag = CHANGE_PASS_SCREEN;
                    reset_flag = RESET_CHANGE_PASS;
                    clear_screen();
                    break;
            }
        }
        
        if(key == SW6)
        {
            if(screen_flag == MENU_SCREEN)
            {
                screen_flag = DASH_BOARD;
                reset_flag = RESET_NULL;
                clear_screen();
                clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                __delay_us(100);
            }
            else if(screen_flag != DASH_BOARD && screen_flag != PASS_SCREEN && screen_flag != SET_TIME_SCREEN)
            {
                screen_flag = MENU_SCREEN;
                reset_flag = RESET_MENU;
                clear_screen();
                clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                __delay_us(100);
            }
        }
        
        switch(screen_flag)
        {
            case DASH_BOARD:
                display_dashboard(event, speed);
                break;
            case PASS_SCREEN:
                switch (check_password(key, reset_flag))
                {
                    case TIMEOUT:
                        screen_flag = DASH_BOARD;
                        clear_screen();
                        clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                        __delay_us(100);

                        TMR2ON = 0;
                        break;
                        
                    case SUCCESS:
                        screen_flag = MENU_SCREEN;
                        reset_flag = RESET_MENU;
                        clear_screen();
                        clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                        __delay_us(100);

                        TMR2ON = 0;
                        break;
                break;
                }
                break;
            case MENU_SCREEN: 
                menu_pos = menu_screen(key, reset_flag);
                break;
            case LOG_SCREEN:
                display_log(key, reset_flag);
                break;
            case CHANGE_PASS_SCREEN:
                switch(change_password(key, reset_flag))
                {
                    case SUCCESS:
                        TMR2ON = 0;
                        
                        log_event("CP",speed);
                        
                        screen_flag = MENU_SCREEN;
                        reset_flag = RESET_MENU;
                        clear_screen();
                        clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                        __delay_us(100);
                        break;
                        
                    case TIMEOUT:                        
                        TMR2ON = 0;

                        screen_flag = MENU_SCREEN;
                        reset_flag = RESET_MENU;
                        clear_screen();
                        clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                        __delay_us(100);
                        break;
                break;
                }
                break;
            case SET_TIME_SCREEN:
                if(set_time(key, reset_flag) ==  SUCCESS)
                {
                    screen_flag = MENU_SCREEN;
                    reset_flag = RESET_MENU;
                    clear_screen();
                }
                break;
        }
        reset_flag = RESET_NULL;
    }
    return;
}