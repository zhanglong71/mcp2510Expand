#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>

int main(void)
{
	int i;
	int ret;
	int buttons_fd;
	int child_stat;
	char key_value[4];
	char first = 1;
	char key_value_temp[4];
    struct timeval tv;
    pid_t pid, child_pid;
    fd_set rd;

	/*打开键盘设备文件*/
	buttons_fd = open("/dev/IRQ-Test", 0);
	if (buttons_fd < 0) {
		perror("open device buttons");
		exit(1);
	}

	for (;;) {
		/*开始读取键盘驱动发出的数据，注意key_value和键盘驱动中定义为一致的类型*/
		ret = read(buttons_fd, key_value_temp, sizeof(key_value_temp));
		if (ret != sizeof(key_value_temp)) {
			perror("read buttons:");
			exit(1);
		}

		/** 查找键值 **/
		for (i = 0; i < sizeof(key_value); i ++) {
			if(key_value[i] != key_value_temp[i]) {
				key_value[i] = key_value_temp[i];
				printf("K%d is %s%s", i+1, key_value[i] == '0' ? "up" : "down", first ? ", " : "");

                if((i == 0) && (key_value[i] != '0')) { /** 如果检测到K1按下，等待 **/
                    tv.tv_sec = 10;
                    tv.tv_usec = 10;

                    FD_ZERO(&rd);
                    FD_SET(buttons_fd, &rd);
                    printf("\n");
                    ret = select(buttons_fd + 1, &rd, NULL, NULL, &tv);
                    if(ret < 0) {
                        /** error **/
                        sleep(1);
                        continue;
                    } else if(ret == 0) {
                      #if 1
                        //system("recover.sh");
                        system("cp -rv net.conf.dfl /etc/net.conf");
                        printf("recover done!\n");
                      #else
                        /** expired **/
                        pid = fork();
                        if(pid < 0) {
                            /** error **/
                        } else if(pid > 0) {
                            /** parent **/
                            child_pid = wait(&child_stat);
                        } else /* pid = 0 */{
                            /** child **/
                            printf("prepare recover...!\n");
                            //char *arglist[] = {"./recover.sh", key_value, NULL};
                            //char *arglist[] = {"./app_raw_write", "/dev/mcp2510", NULL};
                            ret = execl("./recover.sh", key_value, NULL);
                            perror("execl");
                            printf("process never go to here! ret = %d\n", ret);
                            _exit(0);
                        }
                      #endif
                    } else {
                        /** nothing. 没到时间，按键就松开了, 啥也不用干 **/
                    }
                }
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

