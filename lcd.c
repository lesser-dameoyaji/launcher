#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <unistd.h>
#include "types.h"
#include "lcd.h"
#include <i2c/smbus.h>

// 
static int lcd_fd;
static __u8 lcd_tvram[0x80];
static int lcd_curx;
static int lcd_cury;
static int lcd_blink;
static bool lcd_update;

int lcd_init(void)
{
	char *i2cdevName = "/dev/i2c-1";
	int ret, i;

	// initialize variables
	for(i=0; i<sizeof(lcd_tvram); i++)
	{
		lcd_tvram[i] = 0x20;
	}

	if((lcd_fd = open(i2cdevName,O_RDWR)) < 0){
		fprintf(stderr,"Can not open i2c port\n");
		return -1;
	}
	lcd_curx = 0;
	lcd_cury = 0;
	lcd_blink = -1;
	lcd_update = false;


	// I2C address
	ioctl(lcd_fd, I2C_SLAVE, LCD_I2C_ADDR);

	usleep(40000);
	// Function Set: 8bitBus/2Line/Single height font/IS=0(InstructionTableSelect)
	ret = i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x38);
	usleep(30);

	// Function Set: 8bitBus/2Line/Single height font/IS=1(InstructionTableSelect)
	ret = i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x39);
	usleep(30);

	// Internal OSC frequency: BS=1(1/4bias)/F2:0=0
	ret = i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x14);
	usleep(30);

	// Contrast set: 
	ret = i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x73);
	usleep(30);

	// Power/ICON/Contrast control:
	ret = i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x56);
	usleep(30);

	// Follower control:
	ret = i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x6c);
	usleep(200000);

	// Function Set: 
	ret = i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x38);
	usleep(30);

	// Clear Display: 
	ret = i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x01);
	usleep(30);

	// Display ON/OFF control: 
	ret = i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x0c);
	usleep(30);

	if(ret < 0)
		fprintf(stderr, "error %s\n", strerror(ret));

	return ret;
}

int lcd_close(void)
{
	return close(lcd_fd);
}

//
// RAW Level API
//
int lcd_write_chr(__u8 chr)
{
	return i2c_smbus_write_byte_data(lcd_fd, LCD_DATA, chr);
}

int lcd_write_blk(__u8 cmd, __u8* blk, __u8 len)
{
	union i2c_smbus_data data;
	__u8 i;
	if (len > 32)
		len = 32;
	// first byte = length
	data.block[0] = len;
	// following byte = data
	for (i = 0; i < len; i++)
		data.block[i+1] = blk[i];
	return i2c_smbus_access(lcd_fd, I2C_SMBUS_WRITE, cmd, len, &data);
}

int lcd_goto_home(void)
{
	return i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x02);
}

int lcd_goto_xy(int x, int y)
{
	__u8 addr;

	addr = (y==0)? 0:0x40 + (__u8)(x & 0x7f);

	// send Set DDRAM address Instruction
	return i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x80 + addr);
}

int lcd_clear(void)
{
	int i;
	for(i=0; i<sizeof(lcd_tvram);i++)
		lcd_tvram[i] = ' ';
	lcd_goto_xy(0, 0);
	lcd_curx = 0;
	lcd_cury = 0;
	// send Clear Display Instruction
	return i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x01);
}

int lcd_cursor(int x, int y, bool onoff)
{
#if 1
	lcd_curx = (x >= 0)? (x % 0x40) : lcd_curx;
	lcd_cury = (y >= 0)? ((y < 2)? y : 2) : lcd_cury;
	if((lcd_blink == -1) && (onoff==true))
		lcd_blink = 0;
	else if(onoff==false)
		lcd_blink = -1;
#else
	__u8 cmd;
	if(onoff == true)
		cmd = 0x0d;
	else
		cmd = 0x0c;
	i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x39);
	usleep(30);
	i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, cmd);
	usleep(30);
	i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x38);
#endif
	return 0;
}

//
//
// bufferd API
//
//
static void lcd_increment_cursor(int n)
{
	lcd_curx += n;
	if(lcd_curx > 16)
	{
		lcd_curx = 0;
		lcd_cury = 1;
	}
	if(lcd_blink >= 0)
		lcd_update = true;
}

int lcd_flush(void)
{
	__u8 swap_c;
	int ret = -1;

	if(lcd_blink >= 0)
	{
		lcd_blink++;
		lcd_update = true;
	}
	if(lcd_update == false)
		return -1;
	lcd_update = false;

	// blink
	if(lcd_blink > 15)
	{
		lcd_blink = 0;
	}
	else if(lcd_blink > 10)
	{
		swap_c = lcd_tvram[lcd_curx + 0x40 * lcd_cury];
		lcd_tvram[lcd_curx + 0x40 * lcd_cury] = '_';
	}
	while(1)
	{
		if(lcd_goto_xy(0, 0) < 0)
		{
			fprintf(stderr, "lcd_goto_xy1 fail in lcd_flush\n");
			break;
		}
		if(lcd_write_blk(LCD_DATA, &lcd_tvram[0x00], 8) < 0)
		{
			fprintf(stderr, "lcd_write_blk1 fail in lcd_flush\n");
			break;
		}
		if(lcd_write_blk(LCD_DATA, &lcd_tvram[0x08], 8) < 0)
		{
			fprintf(stderr, "lcd_write_blk1 fail in lcd_flush\n");
			break;
		}
		if(lcd_goto_xy(0, 1) < 0)
		{
			fprintf(stderr, "lcd_goto_xy2 fail in lcd_flush\n");
			break;
		}
		if(lcd_write_blk(LCD_DATA, &lcd_tvram[0x40], 8) < 0)
		{
			fprintf(stderr, "lcd_write_blk2 fail in lcd_flush\n");
			break;
		}
		if(lcd_write_blk(LCD_DATA, &lcd_tvram[0x48], 8) < 0)
		{
			fprintf(stderr, "lcd_write_blk2 fail in lcd_flush\n");
			break;
		}
		ret = 0;
		break;
	}
	if(lcd_blink > 10)
	{
		lcd_tvram[lcd_curx + 0x40 * lcd_cury] = swap_c;
	}
	return ret;
}

int lcd_printf(char* format, ...)
{
	va_list args;
	int max_len, len;

	va_start(args, format);
	max_len = 0x40 - lcd_curx;

	len = vsnprintf(&lcd_tvram[lcd_curx + 0x40 * lcd_cury], max_len, format, args);

	lcd_increment_cursor(len);
	lcd_update = true;
	va_end(args);

	return 0;
}

char lcd_getc(void)
{
	char c = (char)lcd_tvram[lcd_curx + 0x40 * lcd_cury];

	lcd_increment_cursor(1);
	return c;
}

int lcd_putc(char c)
{
	lcd_tvram[lcd_curx + 0x40 * lcd_cury] = (__u8)c;
	
	lcd_increment_cursor(1);
	lcd_update = true;
	return 1;
}
