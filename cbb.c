/* 
 * File:   cbb.h
 * Author: Akshay
 *
 * Created on 18 August, 2026, 9:10 PM
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

unsigned char clock_reg[3];
unsigned char return_time;
unsigned char sec;
unsigned char time[7];
unsigned char log[11];
unsigned char pos = 0;
unsigned char start_pos =0;
unsigned char log_count = 0;

unsigned char saved_password[4] = {'1', '1', '1', '1'};

unsigned char *menu[] = {"View Log", "Clear Log", "Download Log", "Set time", "Change Passwrd"};

void get_time(unsigned char *clock_reg)
{
    clock_reg[0] = read_ds1307(HOUR_ADDR); // HH -> BCD 
    clock_reg[1] = read_ds1307(MIN_ADDR); // MM -> BCD 
    clock_reg[2] = read_ds1307(SEC_ADDR); // SS -> BCD 
    //BCD Format 
    //clock_reg[0] = HH
    //clock_reg[1] = MM
    //clock_reg[2] = SS
   
    /* To store the time in HH:MM:SS format */
    // HH -> 
    time[0] = ((clock_reg[0] >> 4) & 0x03) + '0';
    time[1] = (clock_reg[0] & 0x0F) + '0';;
    // MM 
    time[2] = ((clock_reg[1] >> 4) & 0x07) + '0';
    time[3] = (clock_reg[1] & 0x0F) + '0';    
    // SS
    time[4] = ((clock_reg[2] >> 4) & 0x07) + '0';
    time[5] = (clock_reg[2] & 0x0F) + '0';
    time[6] = '\0';
}

void display_time(unsigned char *time)
{
    clcd_putch(time[0], LINE2(0));
    clcd_putch(time[1], LINE2(1));
    clcd_putch(':', LINE2(2));
    clcd_putch(time[2], LINE2(3));
    clcd_putch(time[3], LINE2(4));
    clcd_putch(':', LINE2(5));
    clcd_putch(time[4], LINE2(6));
    clcd_putch(time[5], LINE2(7));
}

void display_dashboard(unsigned char event[], unsigned char speed)
{
    clcd_print("TIME      EV  SP", LINE1(0));
    get_time(clock_reg);
    display_time(time);
    clcd_print(event, LINE2(10));
    clcd_putch(speed / 10 + '0', LINE2(14));
    clcd_putch(speed % 10 + '0', LINE2(15));
}

void log_event(unsigned char event[], unsigned char speed)
{
    unsigned char addr;

    get_time(clock_reg);

    for(int i = 0; i < 6; i++)
    {
        log[i] = time[i];
    }

    log[6] = event[0];
    log[7] = event[1];

    log[8] = speed / 10 + '0';
    log[9] = speed % 10 + '0';
    log[10] = '\0';

    addr = pos * 10;

    ext_eeprom_24C02_str_write(addr, log);

    pos++;

    if(log_count < 10)
    {
        log_count++;
    }
    else
    {
        start_pos++;

        if(start_pos >= 10)
            start_pos = 0;
    }

    if(pos >= 10)
        pos = 0;
}
    

unsigned char check_password(unsigned char key,unsigned char reset_flag)
{
    static unsigned char user_password[4];
    static unsigned char i, attempt;
    
    if(reset_flag == RESET_PASSWORD)
    {
        attempt = 3;
        i = 0;
        
        user_password[0] = '\0';
        user_password[1] = '\0';
        user_password[2] = '\0';
        user_password[3] = '\0';
        
        return_time = 5;
        key = 0;
    }
    
    if(return_time == 0)
    {
        return TIMEOUT;
    }
    
    if(key == SW4 && i < 4)
    {
        clcd_putch('*', LINE2(4+i));
        user_password[i++] = '1';
        return_time = 5;
    }
    else if(key == SW5 && i < 4)
    {
        clcd_putch('*', LINE2(4+i));
        user_password[i++] = '0';
        return_time = 5;
    }
    
    if(i == 4)
    {
        if(strncmp(user_password, saved_password, 4) == 0)
        {
            clear_screen();
            clcd_print("PASSWORD SUCCESS", LINE1(0));
            sec = 1;
            while(sec != 0)
            {
            }
            clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
            __delay_us(100);
            
            return SUCCESS;
        }
        else
        {
            attempt--;
            
            if(attempt == 0)
            {
                clear_screen();
                clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                __delay_us(100);
                clcd_print("You are blocked", LINE1(0));
                clcd_print("Wait for", LINE2(0));
                clcd_print("s", LINE2(11));
                
                sec = 60;
                while(sec != 0)
                {
                    clcd_putch(sec / 10 + '0', LINE2(9));
                    clcd_putch(sec % 10 + '0', LINE2(10));
                }
                
                attempt = 3;
            }
            else
            {
                clear_screen();
                clcd_write(DISP_ON_AND_CURSOR_OFF, INST_MODE);
                __delay_us(100);
                clcd_print(" WRONG PASSWORD ", LINE1(0));
                clcd_print("Attempt left  ", LINE2(0));
                clcd_putch(attempt + '0', LINE2(14));
                sec = 1;
                while(sec != 0)
                {
                }
            }
            
            clear_screen();
            clcd_print("ENTER PASSWORD", LINE1(0));
            clcd_write(DISP_ON_AND_CURSOR_ON, INST_MODE);
            __delay_us(100);
            
            i = 0;
            return_time = 5;
        }
    }
    
    return RESET_NULL;
}

unsigned char menu_screen(unsigned char key, unsigned char reset_flag)
{
    static unsigned char menu_pos, prev_pos;   
    
    if(reset_flag == RESET_MENU)
    {
        menu_pos = 0;
    }

    if(key == SW4 && menu_pos < 4)
    {
        menu_pos++;
    }
    else if(key == SW5 && menu_pos > 0)
    {
        menu_pos--;
    }
    
    if(menu_pos != prev_pos)
    {
        prev_pos = menu_pos;
        clear_screen();
    }

    if(menu_pos == 0)
    {
        clcd_putch('*', LINE1(0));
        clcd_print(menu[0], LINE1(2));
        clcd_print(menu[1], LINE2(2));
    }
    else if(menu_pos == 4)
    {
        clcd_print(menu[3], LINE1(2));
        clcd_putch('*', LINE2(0));
        clcd_print(menu[4], LINE2(2));
    }
    else
    {
        clcd_print(menu[menu_pos - 1], LINE1(2));
        clcd_putch('*', LINE2(0));
        clcd_print(menu[menu_pos], LINE2(2));
    }

    return menu_pos;
}

void display_log(unsigned char key, unsigned char reset_flag)
{
    static char log_pos;
    unsigned char log_data[11];
    unsigned char addr;
    unsigned char actual_pos;
    unsigned char i;

    if(reset_flag == RESET_LOG)
    {
        log_pos = 0;

        clear_screen();

        clcd_print("# TIME     E  SP", LINE1(0));
    }

    if(key == SW4 && log_count > 0)
    {
        log_pos++;

        if(log_pos >= log_count)
            log_pos = 0;
    }
    else if(key == SW5 && log_count > 0)
    {
        if(log_pos == 0)
            log_pos = log_count - 1;
        else
            log_pos--;
    }

    actual_pos = start_pos + log_pos;

    if(actual_pos >= 10)
        actual_pos = actual_pos - 10;

    addr = actual_pos * 10;

    for(i = 0; i < 10; i++)
    {
        log_data[i] = ext_eeprom_24C02_read(addr + i);
    }
    
    clcd_putch((log_pos) + '0', LINE2(0));

    clcd_putch(log_data[0], LINE2(2));
    clcd_putch(log_data[1], LINE2(3));
    clcd_putch(':', LINE2(4));

    clcd_putch(log_data[2], LINE2(5));
    clcd_putch(log_data[3], LINE2(6));
    clcd_putch(':', LINE2(7));

    clcd_putch(log_data[4], LINE2(8));
    clcd_putch(log_data[5], LINE2(9));

    clcd_putch(log_data[6], LINE2(11));
    clcd_putch(log_data[7], LINE2(12));

    clcd_putch(log_data[8], LINE2(14));
    clcd_putch(log_data[9], LINE2(15));
}

void clear_log(void)
{
    unsigned char event[3] = "CL";

    pos = 0;
    start_pos = 0;
    log_count = 0;

    log_event(event, 0);

    log_count = 1;
}
unsigned char change_password(unsigned char key, unsigned char reset_flag)
{
    static unsigned char new_password[4];
    static unsigned char confirm_password[4];
    static unsigned char i;
    static unsigned char stage;

    if(reset_flag == RESET_CHANGE_PASS)
    {
        i = 0;
        stage = 0;

        new_password[0] = '\0';
        new_password[1] = '\0';
        new_password[2] = '\0';
        new_password[3] = '\0';

        confirm_password[0] = '\0';
        confirm_password[1] = '\0';
        confirm_password[2] = '\0';
        confirm_password[3] = '\0';

        clear_screen();
        clcd_print(" NEW  PASSWORD ", LINE1(0));
        clcd_write(DISP_ON_AND_CURSOR_ON, INST_MODE);
        __delay_us(100);
    }

    if(stage == 0)
    {
        if(key == SW4 && i < 4)
        {
            new_password[i++] = '1';
            clcd_putch('*', LINE2(4 + i - 1));
        }
        else if(key == SW5 && i < 4)
        {
            new_password[i++] = '0';
            clcd_putch('*', LINE2(4 + i - 1));
        }

        if(i == 4)
        {
            i = 0;
            stage = 1;

            clear_screen();
            clcd_print("CONFIRM PASSWORD", LINE1(0));
            clcd_write(DISP_ON_AND_CURSOR_ON, INST_MODE);
            __delay_us(100);
        }
    }
    else
    {
        if(key == SW4 && i < 4)
        {
            confirm_password[i++] = '1';
            clcd_putch('*', LINE2(4 + i - 1));
        }
        else if(key == SW5 && i < 4)
        {
            confirm_password[i++] = '0';
            clcd_putch('*', LINE2(4 + i - 1));
        }

        if(i == 4)
        {
            TMR2ON = 1;
            
            if(strncmp(new_password, confirm_password, 4) == 0)
            {
                for(i = 0; i < 4; i++)
                {
                    saved_password[i] = new_password[i];
                }

                clear_screen();
                clcd_print("    PASSWORD    ", LINE1(0));
                clcd_print("    CHANGED     ", LINE2(0));                   

                sec = 2;
                while(sec != 0)
                {
                }

                return SUCCESS;
            }
            else
            {
                clear_screen();
                clcd_print("    PASSWORD    ", LINE1(0));
                clcd_print("    MISMATCH    ", LINE2(0));

                sec = 2;
                while(sec != 0)
                {
                }

                return TIMEOUT;
            }         
        }
    }
    
    return RESET_NULL;
}

void download_log(void)
{
    unsigned char log_data[10];
    unsigned char actual_pos;
    unsigned char addr;
    unsigned char i;
    unsigned char j;

    puts("\r\n");
    puts("----------LOG DATA----------\r\n");
    puts("# TIME     E  SP\r\n");

    for(i = 0; i < log_count; i++)
    {
        actual_pos = start_pos + i;

        if(actual_pos >= 10)
            actual_pos = actual_pos - 10;

        addr = actual_pos * 10;

        for(j = 0; j < 10; j++)
        {
            log_data[j] = ext_eeprom_24C02_read(addr + j);
        }

        putchar(i + '1');
        putchar(' ');

        putchar(log_data[0]);
        putchar(log_data[1]);
        putchar(':');
        putchar(log_data[2]);
        putchar(log_data[3]);
        putchar(':');
        putchar(log_data[4]);
        putchar(log_data[5]);

        putchar(' ');

        putchar(log_data[6]);
        putchar(log_data[7]);

        putchar(' ');

        putchar(log_data[8]);
        putchar(log_data[9]);

        puts("\r\n");
    }

    puts("----------------------------\r\n");
}

unsigned char set_time(unsigned char key, unsigned char reset_flag)
{
    static unsigned char hour;
    static unsigned char min;
    static unsigned char sec_val;
    static unsigned char field;
    static unsigned char blink;
    static unsigned int delay = 0;

    if(reset_flag == RESET_SET_TIME)
    {
        get_time(clock_reg);

        hour = ((clock_reg[0] >> 4) * 10) + (clock_reg[0] & 0x0F);
        min = ((clock_reg[1] >> 4) * 10) + (clock_reg[1] & 0x0F);
        sec_val = ((clock_reg[2] >> 4) * 10) + (clock_reg[2] & 0x0F);

        field = 0;
        blink = 0;

        clear_screen();

        clcd_print("    SET TIME    ", LINE1(0));

        clcd_putch(hour / 10 + '0', LINE2(4));
        clcd_putch(hour % 10 + '0', LINE2(5));
        clcd_putch(':',              LINE2(6));

        clcd_putch(min / 10 + '0', LINE2(7));
        clcd_putch(min % 10 + '0', LINE2(8));
        clcd_putch(':',              LINE2(9));

        clcd_putch(sec_val / 10 + '0', LINE2(10));
        clcd_putch(sec_val % 10 + '0', LINE2(11));

        sec = 1;
    }

    delay++;
    
    if(delay == 25)
    {
        delay = 0;
        blink = !blink;
    }

    if(blink)
    {
        if(field == 0)
        {
            clcd_putch(' ', LINE2(10));
            clcd_putch(' ', LINE2(11));
        }
        else if(field == 1)
        {
            clcd_putch(' ', LINE2(7));
            clcd_putch(' ', LINE2(8));
        }
        else
        {
            clcd_putch(' ', LINE2(4));
            clcd_putch(' ', LINE2(5));
        }
    }
    else
    {
        if(field == 0)
        {
            clcd_putch(sec_val / 10 + '0', LINE2(10));
            clcd_putch(sec_val % 10 + '0', LINE2(11));
        }
        else if(field == 1)
        {
            clcd_putch(min / 10 + '0', LINE2(7));
            clcd_putch(min % 10 + '0', LINE2(8));
        }
        else
        {
            clcd_putch(hour / 10 + '0', LINE2(4));
            clcd_putch(hour % 10 + '0', LINE2(5));
        }
    }

    if(key == SW4)
    {
        if(field == 0)
        {
            sec_val++;

            if(sec_val >= 60)
                sec_val = 0;
        }
        else if(field == 1)
        {
            min++;

            if(min >= 60)
                min = 0;
        }
        else
        {
            hour++;

            if(hour >= 24)
                hour = 0;
        }

        blink = 0;
    }
    else if(key == SW5)
    {
        field++;

        if(field >= 3)
            field = 0;

        blink = 0;
        
        clcd_putch(sec_val / 10 + '0', LINE2(10));
        clcd_putch(sec_val % 10 + '0', LINE2(11));
        clcd_putch(min / 10 + '0', LINE2(7));
        clcd_putch(min % 10 + '0', LINE2(8));
        clcd_putch(hour / 10 + '0', LINE2(4));
        clcd_putch(hour % 10 + '0', LINE2(5));
        
    }

    if(key == SW6)
    {
        clock_reg[0] = ((hour / 10) << 4) | (hour % 10);
        clock_reg[1] = ((min / 10) << 4) | (min % 10);
        clock_reg[2] = ((sec_val / 10) << 4) | (sec_val % 10);

        write_ds1307(HOUR_ADDR, clock_reg[0]);
        write_ds1307(MIN_ADDR, clock_reg[1]);
        write_ds1307(SEC_ADDR, clock_reg[2]);

        log_event("ST", 0);
        clear_screen();
        clcd_print("    TIME SET    ", LINE1(0));
        clcd_print("  SUCCESSFULLY  ", LINE2(0));

        TMR2ON = 1;
        
        sec = 1;
        while(sec != 0)
        {
        }
        
        TMR2ON = 0;

        return SUCCESS;
    }

    return RESET_NULL;
}