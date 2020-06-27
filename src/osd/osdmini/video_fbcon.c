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

#define USE_CURRENT_VT

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


#ifdef USE_THREAD_RENDER
static osd_event *render_event;
static osd_thread *render_thread;
static void *video_fbcon_render_thread(void *param);
#endif

void fb_wait_vsync(void);

static int fb_init(void)
{
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

	fb_win_addr = fb_base_addr;

#ifdef USE_THREAD_RENDER
	render_event = osd_event_alloc(0, 0);
	render_thread = osd_thread_create(video_fbcon_render_thread, NULL);
#endif

	return 0;
}

void fb_wait_vsync(void)
{
	int crtc = 0;
	int retv = ioctl(fb_fd, FBIO_WAITFORVSYNC, &crtc);
	if(retv<0){
		printk("  FBIO_WAITFORVSYNC failed! %d\n", retv);
	}
}

UINT8 *video_swapbuf_fbcon(UINT8 *current)
{
	UINT8 *next_buffer;
	int retv;

	if(current == fb_base_addr){
		next_buffer = fb_base_addr + fb_yres*fb_pitch;
		g_vinfo.yoffset = 0;
	}else{
		next_buffer = fb_base_addr;
		g_vinfo.yoffset = fb_yres;
	}

	retv = ioctl(fb_fd, FBIOPAN_DISPLAY, &g_vinfo);
	if(retv<0){
		printk("  FBIOPAN_DISPLAY failed! %d\n", retv);
	}
	usleep(1000);

	return next_buffer;
}

//============================================================
//  osd_update
//============================================================


static int old_width=0, old_height=0;
static int nw=0, nh=0;
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
	render_target *target;
	render_primitive_list *primlist;

	UINT8 *fb_draw_ptr;
	int nw;
	int nh;
};

static struct render_param rparam;

static void do_render(struct render_param *pr)
{
	video_show_fps();

	// do the drawing here
	if(fb_bpp==32){
		software_renderer<UINT32, 0,0,0, 16,8,0>::draw_primitives(*pr->primlist, pr->fb_draw_ptr, pr->nw, pr->nh, fb_pitch/4);
	}else if(fb_bpp==16){
		software_renderer<UINT16, 3,2,3, 11,5,0>::draw_primitives(*pr->primlist, pr->fb_draw_ptr, pr->nw, pr->nh, fb_pitch/2);
	}

	pr->primlist->release_lock();

	fb_win_addr = video_swapbuf_fbcon(fb_win_addr);

}

void video_update_fbcon(render_target *our_target, bool skip_draw)
{
	//video_show_fps();
	if(skip_draw){
		vt_update_key();
		return;
	}

	// get the minimum width/height for the current layout
	int minwidth, minheight;
	our_target->compute_minimum_size(minwidth, minheight);

	if(old_width!=minwidth || old_height!=minheight){
		printk("Change res to %dx%d\n", minwidth, minheight);
		old_width = minwidth;
		old_height = minheight;

		nw = (minwidth * fb_yres) / minheight;
		if(nw<fb_xres){
			nh = fb_yres;
			fb_draw_offset = ((fb_xres-nw)/2)*(fb_bpp/8);
		}else{
			nh = (minheight * fb_xres) / minwidth;
			nw = fb_xres;
			fb_draw_offset = ((fb_yres-nh)/2)*fb_pitch;
		}
		printk("Scale: %dx%d\n", nw, nh);

	}

	UINT8 *fb_draw_ptr = fb_win_addr+fb_draw_offset;

	// make that the size of our target
	our_target->set_bounds(nw, nh);

	// get the list of primitives for the target at the current size
	render_primitive_list &primlist = our_target->get_primitives();

	// lock them, and then render them
	primlist.acquire_lock();


	rparam.target = our_target;
	rparam.primlist = &primlist;
	rparam.fb_draw_ptr = fb_draw_ptr;
	rparam.nw = nw;
	rparam.nh = nh;

#ifdef USE_THREAD_RENDER
	osd_event_set(render_event);
#else
	do_render(&rparam);
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

		do_render(&rparam);
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


