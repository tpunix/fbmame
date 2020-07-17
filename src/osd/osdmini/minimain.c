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

// a single rendering target
render_target *our_target;

// a single input device
input_device *keyboard_device;

int osdmini_run = 0;


//============================================================
//  mini_osd_options
//============================================================

mini_osd_options::mini_osd_options()
: osd_options()
{
	//add_entries(s_option_entries);
}

//============================================================
//  main
//============================================================

int main(int argc, char *argv[])
{
	// cli_frontend does the heavy lifting; if we have osd-specific options, we
	// create a derivative of cli_options and add our own
	mini_osd_options options;
	mini_osd_interface osd(options);
	osd.register_options();
	cli_frontend frontend(options, osd);
	return frontend.execute(argc, argv);
}


/******************************************************************************/

mini_osd_interface::mini_osd_interface(mini_osd_options &options) : osd_common_t(options)
{
}


mini_osd_interface::~mini_osd_interface()
{
}


/******************************************************************************/


void mini_osd_interface::init(running_machine &machine)
{
	osdmini_run = 1;

	// call our parent
	osd_common_t::init(machine);
	osd_common_t::init_subsystems();

	// initialize the video system by allocating a rendering target
	our_target = machine.render().target_alloc();

	// initialize the input system by adding devices
	keyboard_device = machine.input().device_class(DEVICE_CLASS_KEYBOARD).add_device("Keyboard");


	input_init_vt();
	video_init_fbcon();

}


void mini_osd_interface::update(bool skip_redraw)
{
	video_update_fbcon(skip_redraw);
	input_update_vt();
}


void mini_osd_interface::osd_exit(void)
{
	printk("\nosd_exit!\n");
	osdmini_run = 0;

	osd_common_t::osd_exit();

	video_exit_fbcon();
	input_exit_vt();
}


/******************************************************************************/


char *osd_get_clipboard_text(void)
{
	// can't support clipboards generically
	return NULL;
}


/******************************************************************************/

