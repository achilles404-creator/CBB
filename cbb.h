/* 
 * File:   cbb.h
 * Author: Akshay
 *
 * Created on 18 August, 2026, 9:10 PM
 */

#ifndef CBB_H
#define	CBB_H

#define DASH_BOARD            0x01
#define PASS_SCREEN           0x02
#define MENU_SCREEN           0x03
#define LOG_SCREEN            0x04
#define CHANGE_PASS_SCREEN    0x05
#define SET_TIME_SCREEN       0x06

#define RESET_PASSWORD        0xF1
#define RESET_MENU            0xF2
#define RESET_LOG             0xF3
#define RESET_CHANGE_PASS     0xF4
#define RESET_SET_TIME        0xF5
#define RESET_NULL            0xFF

#define TIMEOUT               0x11
#define SUCCESS               0x22

void display_dashboard(unsigned char event[], unsigned char speed);
unsigned char menu_screen(unsigned char key,unsigned char reset_flag);
void log_event(unsigned char event[], unsigned char speed);
void display_log(unsigned char key, unsigned char reset_flag);
void download_log(void);
void clear_log(void);
unsigned char set_time(unsigned char key, unsigned char reset_flag);
unsigned char check_password(unsigned char key,unsigned char reset_flag);
unsigned char change_password(unsigned char key, unsigned char reset_flag);

#endif	/* CBB_H */

