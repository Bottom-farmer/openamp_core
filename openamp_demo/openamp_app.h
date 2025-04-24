#ifndef __OPENAMP_APP_H__
#define __OPENAMP_APP_H__

#include "operation_interface.h"
#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <time.h>
#include <unistd.h>

void app_echo_init(void);
int  app_echo_send(char *send_buf);

#endif /* __OPENAMP_APP_H__ */