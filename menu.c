#include <stdio.h>
#include <wiringPi.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "lcd.h"
#include "config.h"

//
// menu
//
typedef struct {
	char	title[15];
	char	exec[256];
} menu_item_t;

menu_item_t menu_item[8] = {
	{"can_repeater", "/home/pi/work/can_repeater/a.out"},
	{"Halt", "sudo halt -p"},
	{"Exit", ""}
};
int menu_item_max = 3;

//
// GPIO pin
//
typedef struct {
	int pin;
	unsigned char crcnt;
	unsigned char prev_detect;
} pin_descriptor_t;

pin_descriptor_t pin_desc[4];

int pin_init(void);
unsigned char pin_read(int index);

#define BTN_L 13
#define BTN_R 21
#define BTN_U 19
#define BTN_D 26

unsigned char update = 1;

int menu_title(unsigned char btn)
{
	static int count = 0;
	static int fd = -1;
	static char addr_prev[32] = "0.0.0.0";
	static char addr_now[32] = "";
	struct ifreq ifr;

	// initialize
	if(fd < 0)
	{
		printf("enter menu_title\n");

		fd = socket(AF_INET, SOCK_DGRAM, 0);

		ifr.ifr_addr.sa_family = AF_INET;
		strncpy(ifr.ifr_name, "wlan0", IFNAMSIZ-1);

		sprintf(&lcd_tvram[0x00], "wlan0");
		sprintf(&lcd_tvram[0x40], "%s", addr_prev);
	}
	if(count < 20)
		count++;

	if((count >= 20) && (btn > 0))
	{
		close(fd);
		update = 1;
		lcd_clear();
		return 1;
	}
	// display IP address wlan0
	ioctl(fd, SIOCGIFADDR, &ifr);

	sprintf(addr_now, "%s", inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));

	if(strcmp(addr_now, addr_prev) != 0)
	{
		// different address
		strcpy(&lcd_tvram[0x40], addr_now);
		strcpy(addr_prev, addr_now);
		update++;
	}

	if(update > 0)
	{
		update = 0;
		lcd_flush();
	}

	return 0;
}
int menu_launch(unsigned char btn)
{
	static int cursor = 0;
	static int index = 0;
	int len, i;

	if(btn==BTN_U)
	{
		update = 1;
		if(cursor == 0)
		{
			index--;
		}
		else
		{
			cursor = 0;
		}
		if(index < 0)
		{
			index = 0;
			cursor = 0;
			return -1;
		}
	}
	if(btn == BTN_D)
	{
		update = 1;
		if(cursor == 0)
		{
			cursor = 1;
		}
		else
		{
			index++;
		}
		if(index >= (menu_item_max-1))
		{
			index = menu_item_max - 1;
			cursor = 0;
		}
	}
	if(btn == BTN_R)
	{
		lcd_clear();
		len = strlen(menu_item[index+cursor].exec);
		if(len > 0)
		{
			system(menu_item[index+cursor].exec);
			lcd_clear();
			update = 1;
		}
		else
		{
			return 100;
		}
	}
	// display
	if(update > 0)
	{
		update = 0;
		lcd_clear();

		if(cursor == 0)
		{
			lcd_tvram[0x00] = 0x7e;
			lcd_tvram[0x40] = ' ';
		}
		else
		{
			lcd_tvram[0x00] = ' ';
			lcd_tvram[0x40] = 0x7e;
		}
		for(i=0; (i < 2) && ((index+i) < menu_item_max); i++)
		{
			sprintf(&lcd_tvram[0x40 * i + 1], "%s", menu_item[index + i].title);
		}
		lcd_flush();
	}
	return 0;
}

int menu_init(void)
{
	char* item[8];
	int num, i;
	
	// LCD initialize
	lcd_init();
	lcd_goto_home();

	// gpio pin initialize
	pin_init();
	
	i = 0;
	while((num = config_read(item)) > 0)
	{
		printf("%s:", item[0]);
		if(strncmp("item", item[0], 4) == 0)
		{
			strcpy(menu_item[i].title, item[1]);
			strcpy(menu_item[i].exec, item[2]);
			
			printf("%s:%s\n", menu_item[i].title, menu_item[i].exec);
			i++;
		}
		printf("\n");
	}
	if(i > 0)
		menu_item_max = i;
}

int menu(void)
{
	int i;
	unsigned char btn;
	static unsigned char mn = 0;

	for(i=0; i<4; i++)
	{
		if((pin_read(i) & 0x03) == 0x03)
		{
			// button priority L<R<U<D
			btn = pin_desc[i].pin;
		}
	}
	switch(mn)
	{
		case 0:
			mn += menu_title(btn);
			break;
		case 1:
			mn += menu_launch(btn);
			break;
		default:
			return -1;
	}
	return 0;
}

//
// GPIO pin function
//
int pin_init(void)
{
	int i;

	for(i=0; i<4; i++)
	{
		pin_desc[i].crcnt = 0;
		pin_desc[i].prev_detect = 0;
	}

	pin_desc[0].pin = BTN_L;
	pin_desc[1].pin = BTN_R;
	pin_desc[2].pin = BTN_U;
	pin_desc[3].pin = BTN_D;

	if(wiringPiSetupGpio() < 0)
	{
		printf("GPIO initialize fail\n");
		return -1;
	}
	pinMode(BTN_L, INPUT);
	pinMode(BTN_R, INPUT);
	pinMode(BTN_U, INPUT);
	pinMode(BTN_D, INPUT);

	pullUpDnControl(BTN_L, PUD_UP);
	pullUpDnControl(BTN_R, PUD_UP);
	pullUpDnControl(BTN_U, PUD_UP);
	pullUpDnControl(BTN_D, PUD_UP);

	return 0;
}

unsigned char pin_read(int index)
{
	unsigned char now_detect;
	unsigned char detect;

	if(digitalRead(pin_desc[index].pin) == 0)
	{
		now_detect = 1;
	}
	else
	{
		now_detect = 0;
	}
	if(now_detect == pin_desc[index].prev_detect)
	{
		pin_desc[index].crcnt = 0;
		detect = pin_desc[index].prev_detect;
	}
	else
	{
		pin_desc[index].crcnt++;
		if(pin_desc[index].crcnt < 2)
		{
			detect = pin_desc[index].prev_detect;
		}
		else
		{
			pin_desc[index].prev_detect = now_detect;
			detect = now_detect | 0x02;	// edge flag on
		}
	}
	return detect;
}

