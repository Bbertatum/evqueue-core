/*
 * This file is part of evQueue
 * 
 * evQueue is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * evQueue is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with evQueue. If not, see <http://www.gnu.org/licenses/>.
 * 
 * Author: Thibault Kummer <bob@coldsource.net>
 */

#ifndef _GLOBAL_H_
#define _GLOBAL_H_

#define WORKFLOW_NAME_MAXLEN            64
#define QUEUE_NAME_MAX_LEN              64
#define TASK_NAME_MAX_LEN               64
#define TASK_BINARY_MAX_LEN            128
#define PARAMETERS_MAX_LEN     (1024*1024)
#define PARAMETER_NAME_MAX_LEN          64
#define ERROR_MAX_LEN                  256

#define PROCESS_MANAGER_MSGQID   0xB16B00B5

extern int listen_socket; // Global because we must close it in children

#include <unistd.h>

struct st_msgbuf
{
	long type;
	
	struct {
		pid_t pid;
		pid_t tid;
		char retcode;
	} mtext;
};

#endif
