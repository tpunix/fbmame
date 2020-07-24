// license:BSD-3-Clause
// copyright-holders:Aaron Giles
//============================================================
//
//  minimain.c - Main function for mini OSD
//
//============================================================

#include "emu.h"
#include "osdepend.h"
#include "osdcore.h"
#include "osdmini.h"


#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <termios.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <mqueue.h>
#include <errno.h>


/******************************************************************************/

mqd_t input_mq;

void input_init_remote(void)
{

	input_mq = mq_open("/mq_input", O_RDWR|O_NONBLOCK);
	if(input_mq<0){
		perror("mq_open(/mq_input)");
		exit(-1);
	}

	input_keyboard_init();
}


void input_update_remote(void)
{
	int retv;
	UINT32 key, value;

	while(1){
		retv = mq_receive(input_mq, (char*)&key, 4, NULL);
		if(retv<=0)
			break;
		value = key&0xffff;
		key >>= 16;
		if(key>127)
			continue;

		if(value){
			vt_keystate[key] = 1;
		}else{
			vt_keystate[key] = 0;
		}
	}

}

void input_exit_remote(void)
{
	mq_close(input_mq);
}


/******************************************************************************/


void input_register_remote(void)
{
	osd_input_init = input_init_remote;
	osd_input_exit = input_exit_remote;
	osd_input_update = input_update_remote;
}

/******************************************************************************/


