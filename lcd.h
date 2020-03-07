#ifndef __lcd_h
#define __lcd_h

#include <linux/i2c-dev.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <string.h>

#define LCD_I2C_ADDR 0x3e
#define LCD_INSTRUCTION 0x00
#define LCD_DATA 0x40

// 
int lcd_init(void);
int lcd_close(void);

// RAW level API
int lcd_write_chr(__u8 chr);
int lcd_write_blk(__u8 cmd, __u8* blk, __u8 len);
int lcd_goto_home(void);
int lcd_goto_xy(int x, int y);
int lcd_clear(void);
int lcd_cursor(int x, int y, bool onoff);

// bufferd API
int lcd_flush(void);
int lcd_printf(char* format, ...);
char lcd_getc(void);
int lcd_putc(char c);

#endif
