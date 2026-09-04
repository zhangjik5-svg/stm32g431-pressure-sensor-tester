#ifndef TEST_STUB_LCD_H
#define TEST_STUB_LCD_H

#include <stdint.h>

#define Line0 (0U)
#define Line1 (24U)
#define Line2 (48U)
#define Line3 (72U)
#define Line4 (96U)
#define Line5 (120U)
#define Line6 (144U)
#define Line7 (168U)
#define Line8 (192U)
#define Line9 (216U)

void LCD_DisplayStringLine(uint8_t line, uint8_t *text);

#endif
