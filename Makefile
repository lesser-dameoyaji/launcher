all:
	gcc -lpthread -li2c -lwiringPi -o launcher main.c lcd.c menu.c config.c


