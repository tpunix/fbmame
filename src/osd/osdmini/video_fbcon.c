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
#include <linux/fb.h>


/******************************************************************************/


//============================================================
//  tripple buffer
//============================================================

#define QOBJ_IDLE    0
#define QOBJ_WRITE   1
#define QOBJ_READY   2
#define QOBJ_USING   3

typedef struct _queue_obj
{
	int state;
	int age;
	void *data1;
	int data2;
}QOBJ;

typedef struct {
	int size;
	QOBJ *list;
}SIMPLE_QUEUE;

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


static QOBJ *get_idle_qobj(SIMPLE_QUEUE *sq)
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

static QOBJ *get_ready_qobj(SIMPLE_QUEUE *sq)
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


static void qobj_set_ready(QOBJ *qobj)
{
	qobj->age = qobj_ages++;
	qobj->state = QOBJ_READY;
}

static void qobj_set_idle(QOBJ *qobj)
{
	qobj->age = 0;
	qobj->state = QOBJ_IDLE;
}

static void qobj_init(QOBJ *qobj, void *data1, int data2)
{
	qobj->state = QOBJ_IDLE;
	qobj->age = 0;
	qobj->data1 = data1;
	qobj->data2 = data2;
}


/******************************************************************************/


static int fb_fd, fb_xres, fb_yres, fb_bpp, fb_pitch;
static UINT8 *fb_base_addr;

static fb_var_screeninfo g_vinfo;
static int vblank_wait;


static SIMPLE_QUEUE *fbo_queue;
static SIMPLE_QUEUE *render_queue;

static osd_event *render_event;
static osd_thread *render_thread;
static osd_thread *vblank_thread;
static void *video_fbcon_render_thread(void *param);
static void *video_fbcon_vblank_thread(void *param);


static void check_vblank(void)
{
	INT64 tm_start, tm_end;

	ioctl(fb_fd, FBIOPAN_DISPLAY, &g_vinfo);
	tm_start = osd_ticks();
	ioctl(fb_fd, FBIOPAN_DISPLAY, &g_vinfo);
	tm_end = osd_ticks();
	tm_end -= tm_start;

	vblank_wait = (tm_end<15000)? 1: 0;

	printk("check_vblank_time: %d\n", (int)tm_end);
}

static int fb_init(void)
{
	int i;
	struct fb_fix_screeninfo finfo;
	struct fb_var_screeninfo vinfo;

	fb_fd = open("/dev/fb0", O_RDWR);
	if(fb_fd<0){
		fatalerror("Open /dev/fb0 failed!\n");
		return -1;
	}

	ioctl(fb_fd, FBIOGET_FSCREENINFO, &finfo);
	ioctl(fb_fd, FBIOGET_VSCREENINFO, &vinfo);

	fb_xres = vinfo.xres;
	fb_yres = vinfo.yres;
	fb_bpp  = vinfo.bits_per_pixel;
	fb_pitch = finfo.line_length;

	if(vinfo.yres_virtual < fb_yres*3){
		vinfo.yres_virtual = fb_yres*3;
		int retv = ioctl(fb_fd, FBIOPUT_VSCREENINFO, &vinfo);
		if(retv<0){
			printk("  FBIOPUT_VSCREENINFO failed! %d\n", retv);
		}
	}
	g_vinfo = vinfo;

	fb_base_addr = (UINT8*)mmap(0, finfo.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fb_fd, 0);

	printk("/dev/fb0: %dx%d-%d\n", fb_xres, fb_yres, fb_bpp);
	printk("     map: %p  len: %08x\n", fb_base_addr, finfo.smem_len);

	memset(fb_base_addr, 0, finfo.smem_len);

	fbo_queue = simple_queue_create(3);
	for(i=0; i<3; i++){
		qobj_init(&fbo_queue->list[i], (void*)(fb_base_addr + i*fb_yres*fb_pitch), i*fb_yres);
	}

	check_vblank();


	render_queue = simple_queue_create(4);
	render_event = osd_event_alloc(0, 0);
	render_thread = osd_thread_create(video_fbcon_render_thread, NULL);
	vblank_thread = osd_thread_create(video_fbcon_vblank_thread, NULL);

	return 0;
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


static int game_width=0, game_height=0;
static int fb_draw_w=0, fb_draw_h=0;
static int fb_draw_offset = 0;

static void video_swap_buffer(QOBJ *display)
{
	int retv;

	if(display){
		g_vinfo.yoffset = display->data2;
	}

	// PAN_DISPLAY已经有等待VSYNC的动作.
	retv = ioctl(fb_fd, FBIOPAN_DISPLAY, &g_vinfo);
	if(retv<0){
		printk("  FBIOPAN_DISPLAY failed! %d\n", retv);
	}

}


static void do_render(void)
{
	QOBJ *draw_obj;
	QOBJ *render_obj;
	UINT8 *fb_draw_ptr;
	render_primitive_list *primlist;

	video_show_fps();

	render_obj = get_ready_qobj(render_queue);
	primlist = (render_primitive_list*)render_obj->data1;
	qobj_set_idle(render_obj);

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

void video_update_fbcon(bool skip_draw)
{
	if(skip_draw){
		return;
	}

	// get the minimum width/height for the current layout
	int minwidth, minheight;
	our_target->compute_minimum_size(minwidth, minheight);

	if(game_width!=minwidth || game_height!=minheight){
		printk("Change res to %dx%d\n", minwidth, minheight);
		game_width = minwidth;
		game_height = minheight;

		fb_draw_w = (minwidth * fb_yres) / minheight;
		if(fb_draw_w<fb_xres){
			fb_draw_h = fb_yres;
			fb_draw_offset = ((fb_xres-fb_draw_w)/2)*(fb_bpp/8);
		}else{
			fb_draw_h = (minheight * fb_xres) / minwidth;
			fb_draw_w = fb_xres;
			fb_draw_offset = ((fb_yres-fb_draw_h)/2)*fb_pitch;
		}
		printk("Scale: %dx%d\n", fb_draw_w, fb_draw_h);

	}
	our_target->set_bounds(fb_draw_w, fb_draw_h);


	// get the list of primitives for the target at the current size
	render_primitive_list &primlist = our_target->get_primitives();
	primlist.acquire_lock();

	QOBJ *render_obj = get_idle_qobj(render_queue);
	render_obj->data1 = &primlist;
	qobj_set_ready(render_obj);

	osd_event_set(render_event);

}


static void *video_fbcon_render_thread(void *param)
{

	while(osdmini_run){
		osd_event_wait(render_event, OSD_EVENT_WAIT_INFINITE);
		if(osdmini_run==0)
			break;

		do_render();
	}

	printk("video_fbcon_render_thread stop!\n");
	return NULL;
}

static void *video_fbcon_vblank_thread(void *param)
{
	QOBJ *new_obj;
	QOBJ *release_obj = NULL;

	while(osdmini_run){
		new_obj = get_ready_qobj(fbo_queue);
		if(new_obj){
			video_swap_buffer(new_obj);
			if(release_obj){
				qobj_set_idle(release_obj);
			}
			release_obj = new_obj;

			if(vblank_wait){
				osd_sleep(16000);
			}
		}else{
			osd_sleep(3000);
		}
	}

	printk("video_fbcon_vblank_thread stop!\n");
	return NULL;
}


/******************************************************************************/


void video_init_fbcon(void)
{
	fb_init();
}


void video_exit_fbcon(void)
{
	printk("video_exit_fbcon!\n");
}


/******************************************************************************/


