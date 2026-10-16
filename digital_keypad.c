#include <xc.h>
#include "digital_keypad.h"

void init_digital_keypad(void)
{
    /* Set Keypad Port as input */
    KEYPAD_PORT_DDR = KEYPAD_PORT_DDR | INPUT_LINES;
}

unsigned char read_digital_keypad(unsigned char mode)
{
    static unsigned char key = ALL_RELEASED;
    static unsigned char prev_key = ALL_RELEASED;
    static unsigned int press_count = 0;

    if (mode == LEVEL_DETECTION)
    {
        return KEYPAD_PORT & INPUT_LINES;
    }
    else
    {
        if ((KEYPAD_PORT & INPUT_LINES) != ALL_RELEASED)
        {
            if (key == (KEYPAD_PORT & INPUT_LINES))
            {
                ++press_count;
            }
            else
            {
                key = KEYPAD_PORT & INPUT_LINES;
                press_count = 0;
            }
        }
        else
        {
            if (press_count > 0 && press_count < 25)
            {
                if (key != prev_key)
                {
                    prev_key = key;
                    press_count = 0;
                    key = ALL_RELEASED;

                    return prev_key;
                }
            }

            press_count = 0;
            key = ALL_RELEASED;
            prev_key = ALL_RELEASED;
        }

        if (press_count > 25)
        {
            if (key != prev_key)
            {
                prev_key = key;
                press_count = 0;

                return key | 0x80;
            }
            press_count = 0;
        }
    }

    return ALL_RELEASED;
}
