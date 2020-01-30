#include <stdio.h>

#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <net/if.h>
#include <arpa/inet.h>

#include "menu.h"

int main(void)
{
	int loop = 0;
	printf("start launcher\n");

	// menu initialize
	menu_init();

	while(loop==0)
	{

		loop = menu();
		usleep(100000);
	}

	return 0;
}

