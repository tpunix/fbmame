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
#include <sys/shm.h>


/******************************************************************************/


#define SHM_KEY 0x76057810
#define SHM_SIZE 0x01000000


static int shmid;
static UINT8 *shm_addr;
static UINT8 *shm_ctrl;


void shm_init(void)
{

	shmid = shmget(SHM_KEY, SHM_SIZE, 0666);
	if(shmid<0){
		perror("shmget");
		exit(-1);
	}

	shm_addr = (UINT8*)shmat(shmid, NULL, 0);
	if(shm_addr==(void*)-1){
		perror("shmat");
		exit(-1);
	}

	printf("shm id:%08x addr:%p\n", shmid, shm_addr);

	shm_ctrl = shm_addr+SHM_SIZE-4096;

}


void shm_exit(void)
{
	shmdt(shm_addr);
}


/******************************************************************************/


struct shm_video
{
	int magic;

	int fbx;
	int fby;
	int fbpp;
	int fbpitch;

	int vobj_cnt;
	QOBJ vobj[4];

	int cmd_state;
};


extern SIMPLE_QUEUE *fbo_queue;


void video_init_remote(void)
{
	int i;
	struct shm_video *sv;

	shm_init();

	sv = (struct shm_video*)shm_ctrl;
	printf("shm magic: %08x\n", sv->magic);

	fb_xres = sv->fbx;
	fb_yres = sv->fby;
	fb_bpp  = sv->fbpp;
	fb_pitch= sv->fbpitch;

	fbo_queue = (SIMPLE_QUEUE *)malloc(sizeof(SIMPLE_QUEUE));
	fbo_queue->size = sv->vobj_cnt;
	fbo_queue->list = sv->vobj;

	for(i=0; i<fbo_queue->size; i++){
		fbo_queue->list[i].data1 = shm_addr + i*fb_yres*fb_pitch;
		memset(shm_addr + i*fb_yres*fb_pitch, 0, fb_yres*fb_pitch);
	}

}


void video_exit_remote(void)
{
	free(fbo_queue);
	shm_exit();
}


/******************************************************************************/


void video_register_remote(void)
{
	osd_video_init_backend = video_init_remote;
	osd_video_exit_backend = video_exit_remote;
}


/******************************************************************************/


