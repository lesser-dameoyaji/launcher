#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include "lcd.h"
#include <i2c/smbus.h>
// 
int lcd_fd;
__u8 lcd_tvram[0x80];

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
	// send Clear Display Instruction
	return i2c_smbus_write_byte_data(lcd_fd, LCD_INSTRUCTION, 0x01);
}


//
//
// bufferd API
//
//

int lcd_flush(void)
{
	if(lcd_goto_xy(0, 0) < 0)
	{
		fprintf(stderr, "lcd_goto_xy1 fail in lcd_flush\n");
		return -1;
	}
	if(lcd_write_blk(LCD_DATA, &lcd_tvram[0x00], 8) < 0)
	{
		fprintf(stderr, "lcd_write_blk1 fail in lcd_flush\n");
		return -1;
	}
	if(lcd_write_blk(LCD_DATA, &lcd_tvram[0x08], 8) < 0)
	{
		fprintf(stderr, "lcd_write_blk1 fail in lcd_flush\n");
		return -1;
	}
	if(lcd_goto_xy(0, 1) < 0)
	{
		fprintf(stderr, "lcd_goto_xy2 fail in lcd_flush\n");
		return -1;
	}
	if(lcd_write_blk(LCD_DATA, &lcd_tvram[0x40], 8) < 0)
	{
		fprintf(stderr, "lcd_write_blk2 fail in lcd_flush\n");
		return -1;
	}
	if(lcd_write_blk(LCD_DATA, &lcd_tvram[0x48], 8) < 0)
	{
		fprintf(stderr, "lcd_write_blk2 fail in lcd_flush\n");
		return -1;
	}

	return 0;
}

