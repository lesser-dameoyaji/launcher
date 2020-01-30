// 
// config file 書式
//  [name] = [param], [param]......
//
// fileから1行読み出して、name, paramに分解
// 

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>

#include <linux/can.h>
#include <linux/can/raw.h>

#include "config.h"

// proto type
static int read_line(int fh, char* buf, int max_len);
static int parse(char* str, char** item_addr);
int tranc_spc(char* str, int len);
void read_can_param(char* str_canframe, canid_t* can_id, char* can_data);

static int fd = -1;
static char line_buf[256];


int config_save(bool isclose, char* format, ...)
{
	int ret = 0, len;
	va_list args;
	va_start(args, format);
	
	if(fd < 0)
	{
		fd = open(CONFIG_FILE_NAME, O_WRONLY);
		if(fd < 0)
		{
			perror("write open config");
			return -1;
		}
	}
	
	vsprintf(line_buf, format, args);
	len = sprintf(line_buf, "%s\n", line_buf);
	if(len > 0)
		write(fd, line_buf, len);
	else
		ret = -2;
	
	va_end(args);
	
	if((isclose == false) && (fd >= 0))
	{
		close(fd);
		fd = -1;
	}
	return ret;
}

int config_read(char** item)
{
	int llen, idx;
	int ret;

	if(fd < 0)
	{
		fd = open(CONFIG_FILE_NAME, O_RDONLY);
		if(fd < 0)
		{
			perror("read open config");
			return -1;
		}
	}
	
	// 一行ずつ読みだして、解析
	while (1)
	{
		llen = read_line(fd, line_buf, sizeof(line_buf));
		if(llen < 0)
		{
			// エラー or 終端まできたのでclose
			close(fd);
			fd = -1;
			ret =  RESULT_EOF;
			break;
		}
		
		if(strchr(line_buf, (int)'=') != NULL)
		{
			// セパレータでparse
			ret = parse(line_buf, item);
			break;
		}
	}
	
	return ret;
}

static int read_line(int fh, char* buf, int max_len)
{
	int idx;
	char c;
	size_t l;
	int ret = 0;
	
	for(idx=0; idx < max_len; idx++)
	{
		l = read(fh, &c, 1);
		/* 末尾だったら終了 */
		if (l == 0) {
			/* 何も読み出せなかったら、戻り値でEOFを通知 */
			if(ret == 0) {
				ret = RESULT_EOF;
			}
			break;
		}
		/* エラーだったら終了 */
		if (l < 0) {
			ret = RESULT_FAIL;
			break;
		}
		if (c == '\r') {
			break;
		}
		if (c == '\n') {
			break;
		}
		buf[idx] = c;
		ret++;
	}
	buf[idx] = '\0';
	return ret;
}


int parse_num(char* str, char sep)
{
	char* cur;
	int ret = 1;
	
	cur = str;
	
	if(*str == '\0')
		return 0;
	
	while((cur=strchr(cur, (int)sep)) != NULL) {
		cur++;
		ret++;
	}
	return ret;
}

static int parse(char* str, char** item_addr)
{
	int idx = 0;
	int sep = (int)'=';
	int ret = 0;
	char* cur = str;
	
	if (*str == '\0')
	{
		return 0;
	}
	
	item_addr[idx++] = str;
	sep = (int)'=';					// 初回セパレータは = 
	
	while((cur = strchr(cur, (int)sep)) != NULL)
	{
		sep = (int)',';				// 2回目以降のセパレータは ,
		
		// セパレータをterminatorに変換
		*cur = '\0';
		cur++;
		
		// セパレータ後の空白はスキップ
		while(*cur == ' ')
		{
			cur++;
		}
		
		// 終端なら終了
		if(*cur=='\0')
		{
			break;
		}
		
		item_addr[idx] = cur;
		idx++;
	}
	return idx;
}

int tranc_spc(char* str, int len)
{
	int idx_src, idx_dst;
	len++;
	for(idx_src = 0, idx_dst = 0; idx_src < len; idx_src++)
	{
		if((str[idx_src] == ' ') || (str[idx_src] == '\t'))
		{
		}
		else
		{
			if(idx_src != idx_dst)
				str[idx_dst] = str[idx_src];
			idx_dst++;
		}
		
		if(str[idx_src] == '\0')
			break;
	}
	
	return idx_dst;
}

#if 0
void read_can_param(char* str_canframe, canid_t* can_id, char* can_data)
{
	int nparse, idx;
	char* parse_candata[2];
	
	nparse = parse(str_canframe, '#', parse_candata, 2);
	if(nparse == 2)
	{
		unsigned char c[2];
		int byte_data_len;
		
		// CAN ID 変換
		*can_id = (canid_t)strtol(parse_candata[0], NULL, 16);
		printf("can_id = %X\n", can_id);
		
		// CAN frame 変換
		byte_data_len = strlen(parse_candata[1]);
		for(idx=0; idx < 16; idx++)
		{
			if(byte_data_len > (idx * 2))
			{
				c[0] = parse_candata[1][idx * 2];
				c[1] = '\0';
				can_data[idx] = (unsigned char)strtol(c, NULL, 16) << 4;
				if(byte_data_len > (idx * 2 + 1))
				{
					c[0] = parse_candata[1][idx * 2 + 1];
					c[1] = '\0';
					can_data[idx] |= (unsigned char)strtol(c, NULL, 16);
				}
			}
			else
			{
				break;
			}
			printf(" %X,", can_data[idx]);
		}
		printf("\n");
	}
}
#endif
