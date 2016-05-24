#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>

int main(void)
{
	int i;
	int buttons_fd;
	char key_value[4];
	char first = 1;

	/*打开键盘设备文件*/
	buttons_fd = open("/dev/IRQ-Test", 0);
	if (buttons_fd < 0)
	{
		perror("open device buttons");
		exit(1);
	}

	for (;;)
	{
		char key_value_temp[4];
		int ret;

		/*开始读取键盘驱动发出的数据，注意key_value和键盘驱动中定义为一致的类型*/
		ret = read(buttons_fd, key_value_temp, sizeof(key_value_temp));
		if (ret != sizeof(key_value_temp))
		{
			perror("read buttons:");
			exit(1);
		}

		/*打印键值*/
		for (i = 0; i < sizeof(key_value); i ++)
		{
			if(key_value[i] != key_value_temp[i])
			{
				key_value[i] = key_value_temp[i];
				printf("K%d is %s%s", i+1, key_value[i] == '0' ? "up" : "down", first ? ", " : "");
			}
		}
		first = 0;
		if(first == 0)
			printf("\n");
	}

	/*关闭设备文件句柄*/
	close(buttons_fd);
	return 0;
}
