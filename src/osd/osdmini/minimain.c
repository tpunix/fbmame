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


void (*osd_input_init)(void) = NULL;
void (*osd_input_update)(void) = NULL;
void (*osd_input_exit)(void) = NULL;


//============================================================
//  mini_osd_options
//============================================================


const options_entry mini_osd_options::s_option_entries[] =
{
	{"slave", "0",    OPTION_BOOLEAN, "mame work in slave mode" },
	{ NULL }
};

mini_osd_options::mini_osd_options() : osd_options()
{
	add_entries(s_option_entries);
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

mini_osd_interface::mini_osd_interface(mini_osd_options &options)
	: osd_common_t(options), m_options(options)
{

}


mini_osd_interface::~mini_osd_interface()
{
}


/******************************************************************************/


void mini_osd_interface::init(running_machine &machine)
{
	printk("slave mode: %d\n", options().slave());

	if(options().slave()){
		input_register_remote();
		video_register_remote();
	}else{
		input_register_vt();
		video_register_fbcon();
	}

	// call our parent
	osd_common_t::init(machine);
	osd_common_t::init_subsystems();

	// initialize the video system by allocating a rendering target
	our_target = machine.render().target_alloc();

	// initialize the input system by adding devices
	keyboard_device = machine.input().device_class(DEVICE_CLASS_KEYBOARD).add_device("Keyboard");


	osd_input_init();
	osd_video_init();

}


void mini_osd_interface::update(bool skip_redraw)
{
	osd_video_update(skip_redraw);
	osd_input_update();
}


void mini_osd_interface::osd_exit(void)
{
	printk("\nosd_exit!\n");

	osd_common_t::osd_exit();

	osd_video_exit();
	osd_input_exit();
}


/******************************************************************************/


char *osd_get_clipboard_text(void)
{
	// can't support clipboards generically
	return NULL;
}


/******************************************************************************/

