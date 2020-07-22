// license:BSD-3-Clause
// copyright-holders:Aaron Giles
//============================================================
//
//  minimain.c - Main function for mini OSD
//
//============================================================

#include "emu.h"
#include "osdepend.h"
#include "render.h"
#include "clifront.h"
#include "osdcore.h"
#include "modules/sync/osdsync.h"
#include "osdmini.h"


#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>


/******************************************************************************/


extern SIMPLE_QUEUE *fbo_queue;


void video_init_remote(void)
{
	int i;


	fbo_queue = simple_queue_create(3);
	for(i=0; i<3; i++){
		qobj_init(&fbo_queue->list[i], (void*)(fb_base_addr + i*fb_yres*fb_pitch), i*fb_yres);
	}

}


void video_exit_remote(void)
{
	simple_queue_free(fbo_queue);
}


/******************************************************************************/


void video_register_remote(void)
{
	osd_video_init_backend = video_init_remote;
	osd_video_exit_backend = video_exit_remote;
}


/******************************************************************************/


