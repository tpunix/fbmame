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
#include <termios.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <signal.h>
#include <linux/vt.h>
#include <linux/kd.h>


/******************************************************************************/

#define USE_THREAD_RENDER

//#define USE_CURRENT_VT

static int vtfd, vtkmode, new_vt, current_vt;
static struct termios new_termios, cur_termios;
extern UINT8 vt_keystate[128];

static void vt_init(void)
{
	int fd;
	struct vt_stat vtstate;
	char vpath[16];

	memset(vt_keystate, 0, sizeof(vt_keystate));

	fd = open("/dev/tty0", O_RDWR);
	ioctl(fd, VT_GETSTATE, &vtstate);
	current_vt = vtstate.v_active;
#ifdef USE_CURRENT_VT
	new_vt = current_vt;
#else
	ioctl(fd, VT_OPENQRY, &new_vt);
#endif
	close(fd);


	// fixup current vt
	sprintf(vpath, "/dev/tty%d", current_vt);
	fd = open(vpath, O_RDWR|O_NONBLOCK);
	if(fd>0){
		ioctl(fd, KDGKBMODE, &vtkmode);
		if(vtkmode==K_MEDIUMRAW){
			ioctl(fd, KDSKBMODE, K_XLATE);
			ioctl(fd, KDSETMODE, KD_TEXT);
		}
		close(fd);
	}


	sprintf(vpath, "/dev/tty%d", new_vt);
	vtfd = open(vpath, O_RDWR|O_NONBLOCK);
	if(vtfd>0){
#ifdef USE_CURRENT_VT
		printk("Use current VT %d\n", new_vt);
#else
		printk("Switch VT from %d to %d\n", current_vt, new_vt);
		ioctl(vtfd, VT_ACTIVATE, new_vt);
		ioctl(vtfd, VT_WAITACTIVE, new_vt);
#endif

		tcgetattr(vtfd, &cur_termios);
		new_termios = cur_termios;
		cfmakeraw(&new_termios);
		tcsetattr(vtfd, TCSAFLUSH, &new_termios);

		ioctl(vtfd, KDSETMODE, KD_GRAPHICS);

		ioctl(vtfd, KDGKBMODE, &vtkmode);
		ioctl(vtfd, KDSKBMODE, K_MEDIUMRAW);
	}else{
		printk("Open %s failed!\n", vpath);
	}

}

static void vt_exit(void)
{
	if(vtfd>0){
		ioctl(vtfd, KDSKBMODE, vtkmode);

		ioctl(vtfd, KDSETMODE, KD_TEXT);

		tcsetattr(vtfd, TCSAFLUSH, &cur_termios);
#ifndef USE_CURRENT_VT
		ioctl(vtfd, VT_ACTIVATE, current_vt);
		ioctl(vtfd, VT_WAITACTIVE, current_vt);
#endif

		close(vtfd);
		vtfd = -1;
	}
}

static int vt_update_key(void)
{
	int retv;
	UINT8 key;

	while(1){
		retv = read(vtfd, &key, 1);
		if(retv<=0)
			break;

		if(key&0x80){
			vt_keystate[key&0x7f] = 0;
		}else{
			vt_keystate[key&0x7f] = key;
		}
	}

	return key;
}

static void sigint_handle(int signum)
{
	printk("\nsigint_handle!\n");
	vt_exit();
	exit(1);
}

/******************************************************************************/


static int fb_fd, fb_xres, fb_yres, fb_bpp, fb_pitch;
static UINT8 *fb_base_addr;
static UINT8 *fb_win_addr;

static fb_var_screeninfo g_vinfo;


#define FBO_IDLE    0
#define FBO_WRITE   1
#define FBO_READY   2
#define FBO_DISPLAY 3

typedef struct _buffer_obj
{
	int state;
	int age;
	UINT8 *addr;
	int y_offset;
}FBO;

static FBO fbo_list[3];
static int fbo_ages = 1;


#ifdef USE_THREAD_RENDER
static osd_event *render_event;
static osd_thread *render_thread;
static void *video_fbcon_render_thread(void *param);
#endif


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

	if(vinfo.yres_virtual < fb_yres*2){
		vinfo.yres_virtual = fb_yres*2;
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

	for(i=0; i<3; i++){
		fbo_list[i].state = FBO_IDLE;
		fbo_list[i].age = 0;
		fbo_list[i].y_offset = i*fb_yres;
		fbo_list[i].addr = fb_base_addr + i*fb_yres*fb_pitch;
	}

	fb_win_addr = fb_base_addr;

#ifdef USE_THREAD_RENDER
	render_event = osd_event_alloc(0, 0);
	render_thread = osd_thread_create(video_fbcon_render_thread, NULL);
#endif

	return 0;
}


//============================================================
//  tripple buffer
//============================================================

static FBO *get_idle_fbo(void)
{
	int i;

	for(i=0; i<3; i++){
		if(fbo_list[i].state==FBO_IDLE){
			fbo_list[i].state = FBO_WRITE;
			return &fbo_list[i];
		}
	}

	printk("No IDLE FBO!\n");
	return NULL;
}

static FBO *get_ready_fbo(void)
{
	FBO *fbo;
	int i, r, age;

	r = -1;
	age = 0x7fffffff;

	for(i=0; i<3; i++){
		fbo = &fbo_list[i];
		if(fbo->state==FBO_READY){
			if(fbo->age<age){
				r = i;
				age = fbo->age;
			}


		}
	}

	if(r==-1){
		printk("No READY FBO!\n");
		return NULL;
	}

	fbo_list[r].state = FBO_DISPLAY;
	return &fbo_list[r];
}


static void mark_fbo(FBO *fbo, int state, int age)
{
	fbo->age = age;
	fbo->state = state;
}


static void video_show_fbo(FBO *fbo)
{
	int retv;

	if(fbo){
		g_vinfo.yoffset = fbo->y_offset;
		// PAN_DISPLAY已经有等待VSYNC的动作.
		retv = ioctl(fb_fd, FBIOPAN_DISPLAY, &g_vinfo);
		if(retv<0){
			printk("  FBIOPAN_DISPLAY failed! %d\n", retv);
		}
	}else{
		int crtc = 0;
		retv = ioctl(fb_fd, FBIO_WAITFORVSYNC, &crtc);
		if(retv<0){
			printk("  FBIO_WAITFORVSYNC failed! %d\n", retv);
		}
	}

}


//============================================================
//  osd_update
//============================================================


static int game_width=0, game_height=0;
static int fb_draw_w=0, fb_draw_h=0;
static int fb_draw_offset = 0;

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

struct render_param
{
	render_primitive_list *primlist;
};

static struct render_param rparam;

static FBO *display_fbo = NULL;


#define TEST_THREAD

static void do_render(void)
{
	render_primitive_list *primlist;
	FBO *draw_fbo;
	UINT8 *fb_draw_ptr;

	video_show_fps();

#ifdef TEST_THREAD
	render_primitive_list &_primlist = our_target->get_primitives();
	_primlist.acquire_lock();
	primlist = &_primlist;
#else
	primlist = rparam.primlist;
#endif

	draw_fbo = get_idle_fbo();
	if(draw_fbo){
		fb_draw_ptr = draw_fbo->addr + fb_draw_offset;

		// do the drawing here
		software_renderer<UINT32, 0,0,0, 16,8,0>::draw_primitives(*primlist, fb_draw_ptr, fb_draw_w, fb_draw_h, fb_pitch/4);

		mark_fbo(draw_fbo, FBO_READY, fbo_ages++);

	}
	primlist->release_lock();


	if(display_fbo){
		mark_fbo(display_fbo, FBO_IDLE, 0);
		display_fbo = NULL;
	}

	display_fbo = get_ready_fbo();
	video_show_fbo(display_fbo);

}

void video_update_fbcon(bool skip_draw)
{
	//video_show_fps();
	if(skip_draw){
		vt_update_key();
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


#ifndef TEST_THREAD
	// get the list of primitives for the target at the current size
	render_primitive_list &_primlist = our_target->get_primitives();
	_primlist.acquire_lock();
	rparam.primlist = &_primlist;
#endif


#ifdef USE_THREAD_RENDER
	osd_event_set(render_event);
#else
	do_render();
#endif

	vt_update_key();

}

#ifdef USE_THREAD_RENDER

static void *video_fbcon_render_thread(void *param)
{

	while(1){
		osd_event_wait(render_event, OSD_EVENT_WAIT_INFINITE);
		if(osdmini_run==0)
			break;

		do_render();
	}

	printk("video_fbcon_render_thread stop!\n");
	return NULL;
}

#endif


/******************************************************************************/


void video_init_fbcon(void)
{
	signal(SIGINT, sigint_handle);

	vt_init();

	fb_init();
}

void video_exit_fbcon(void)
{
	printk("video_exit_fbcon!\n");
	vt_exit();
}


/******************************************************************************/


