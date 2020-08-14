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
#include "rendersw.inc"


#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <errno.h>


/******************************************************************************/


//============================================================
//  tripple buffer
//============================================================

static int qobj_ages = 1;

SIMPLE_QUEUE *simple_queue_create(int size)
{
	SIMPLE_QUEUE *sq;

	sq = (SIMPLE_QUEUE*)malloc(sizeof(SIMPLE_QUEUE));

	sq->size = size;
	sq->list = (QOBJ*)malloc(size*sizeof(QOBJ));

	memset(sq->list, 0, size*sizeof(QOBJ));
	return sq;
}

void simple_queue_free(SIMPLE_QUEUE *sq)
{
	free(sq->list);
	free(sq);
}


QOBJ *get_idle_qobj(SIMPLE_QUEUE *sq)
{
	int i;

	for(i=0; i<sq->size; i++){
		if(sq->list[i].state==QOBJ_IDLE){
			sq->list[i].state = QOBJ_WRITE;
			return &(sq->list[i]);
		}
	}

	//printk("No IDLE QOBJ!\n");
	return NULL;
}

QOBJ *get_ready_qobj(SIMPLE_QUEUE *sq)
{
	int i, r, age;

	r = -1;
	age = 0x7fffffff;

	for(i=0; i<sq->size; i++){
		if(sq->list[i].state==QOBJ_READY){
			if(sq->list[i].age<age){
				r = i;
				age = sq->list[i].age;
			}
		}
	}

	if(r==-1){
		//printk("No READY QOBJ!\n");
		return NULL;
	}

	sq->list[r].state = QOBJ_USING;
	return &sq->list[r];
}


void qobj_set_ready(QOBJ *qobj)
{
	qobj->age = qobj_ages++;
	qobj->state = QOBJ_READY;
}

void qobj_set_idle(QOBJ *qobj)
{
	qobj->age = 0;
	qobj->state = QOBJ_IDLE;
}

void qobj_init(QOBJ *qobj, void *data1, int data2)
{
	qobj->state = QOBJ_IDLE;
	qobj->age = 0;
	qobj->data1 = data1;
	qobj->data2 = data2;
}


/******************************************************************************/


int fb_xres, fb_yres, fb_bpp, fb_pitch;
UINT8 *fb_base_addr;

SIMPLE_QUEUE *fbo_queue;
mqd_t render_mq;

static osd_thread *render_thread;
static void *video_render_thread(void *param);
static int video_running;


void (*osd_video_init_backend)(void) = NULL;
void (*osd_video_exit_backend)(void) = NULL;


void osd_video_init(void)
{
	int i;
	struct mq_attr mattr;

	osd_video_init_backend();

	mattr.mq_flags = 0;
	mattr.mq_maxmsg = 4;
	mattr.mq_msgsize = sizeof(void*);
	mattr.mq_curmsgs = 0;
	render_mq = mq_open("/mq_render", O_RDWR|O_CREAT|O_EXCL, 0666, &mattr);
	if(render_mq<0 && errno==EEXIST){
		mq_unlink("/mq_render");
		render_mq = mq_open("/mq_render", O_RDWR|O_CREAT|O_EXCL, 0666, &mattr);
	}
	if(render_mq<0){
		perror("mq_create(mq_render)");
		exit(-1);
	}

	video_running = 1;
	render_thread = osd_thread_create(video_render_thread, NULL);

}



//============================================================
//  show fps
//============================================================

static INT64 vtm_old=0, vtm_new, vtm_sec=0;
static int fps=0;

void video_show_fps(void)
{
	fps += 1;
	vtm_new = osd_ticks();
	if(vtm_new - vtm_sec >= 1000000){
		printk("  fps: %d\n", fps);
		fps = 0;
		vtm_sec = vtm_new;
	}
	//printk("video: %12d %06d\n", (int)vtm_new, (int)(vtm_new-vtm_old));
	vtm_old = vtm_new;
}


//============================================================
//  osd_update
//============================================================


static float game_aspect=0;
static int game_width=0, game_height=0;
static int fb_draw_w=0, fb_draw_h=0;
static int fb_draw_offset = 0;


static void do_render(render_primitive_list *primlist)
{
	QOBJ *draw_obj;
	UINT8 *fb_draw_ptr;

	//video_show_fps();

	draw_obj = get_idle_qobj(fbo_queue);
	if(draw_obj){
		fb_draw_ptr = (UINT8*)(draw_obj->data1) + fb_draw_offset;
		// do the drawing here
		software_renderer<UINT32, 0,0,0, 16,8,0>::draw_primitives(*primlist, fb_draw_ptr, fb_draw_w, fb_draw_h, fb_pitch/4);
		qobj_set_ready(draw_obj);
	}

	primlist->release_lock();
	primlist->free_use();

}


static void *video_render_thread(void *param)
{
	int retv=0;
	render_primitive_list *primlist;

	while(video_running){
		retv = mq_receive(render_mq, (char*)&primlist, sizeof(void*), NULL);
		if(retv<0){
			if(errno==EINTR || errno==EAGAIN)
				continue;
			break;
		}

		if(video_running==0)
			break;
		do_render(primlist);
	}
	if(retv<0)
		perror("mq_receive");

	video_running = -1;
	printk("video_render_thread stop!\n");
	return NULL;
}


/******************************************************************************/


void osd_video_update(bool skip_draw)
{
	if(skip_draw){
		return;
	}

	// get the minimum width/height for the current layout
	int minwidth, minheight;
	our_target->compute_minimum_size(minwidth, minheight);

	float new_aspect = our_target->effective_aspect();

	if(game_width!=minwidth || game_height!=minheight || game_aspect != new_aspect){
		printk("Change res to %dx%d  aspect %f\n", minwidth, minheight, new_aspect);
		game_width = minwidth;
		game_height = minheight;
		game_aspect = new_aspect;

#if 0
		fb_draw_w = (minwidth * fb_yres) / minheight;
		if(fb_draw_w<fb_xres){
			fb_draw_h = fb_yres;
			fb_draw_offset = ((fb_xres-fb_draw_w)/2)*(fb_bpp/8);
		}else{
			fb_draw_h = (minheight * fb_xres) / minwidth;
			fb_draw_w = fb_xres;
			fb_draw_offset = ((fb_yres-fb_draw_h)/2)*fb_pitch;
		}
#else
		float fb_aspect = (float)fb_xres/(float)fb_yres;
		if(new_aspect>fb_aspect){
			fb_draw_w = fb_xres;
			fb_draw_h = (float)fb_xres/new_aspect;
			fb_draw_offset = ((fb_yres-fb_draw_h)/2)*fb_pitch;
		}else{
			fb_draw_w = (float)fb_yres*new_aspect;
			fb_draw_h = fb_yres;
			fb_draw_offset = ((fb_xres-fb_draw_w)/2)*(fb_bpp/8);
		}
#endif
		printk("Scale: %dx%d\n", fb_draw_w, fb_draw_h);

	}
	our_target->set_bounds(fb_draw_w, fb_draw_h, 1);


	// get the list of primitives for the target at the current size
	render_primitive_list &primlist = our_target->get_primitives();
	primlist.acquire_lock();

	render_primitive_list *plist = &primlist;
	int retv = mq_send(render_mq, (char*)&plist, sizeof(void*), 0);
}


void osd_video_exit(void)
{
	printk("osd_video_exit!\n");
	char *empty = NULL;

	video_running = 0;

	osd_sleep(100000);
	mq_send(render_mq, (char*)&empty, sizeof(char*), 0);

	while(video_running!=-1){
		osd_sleep(1000);
	}

	mq_close(render_mq);
	mq_unlink("/mq_render");

	osd_video_exit_backend();
}

/******************************************************************************/


