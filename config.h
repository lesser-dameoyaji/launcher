#include "types.h"

#define CONFIG_FILE_NAME "/home/pi/work/can_repeater/can_repeater_config.txt"


#define RESULT_EOF		-1
#define RESULT_FAIL		-2


int config_read(char** item);
int config_save(bool isclose, char* format, ...);
