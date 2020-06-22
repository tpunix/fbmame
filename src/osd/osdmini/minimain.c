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


//============================================================
//  CONSTANTS
//============================================================


struct kt_table {
	input_item_id   mame_key;
	int             scan_key;
	const char  *   mame_key_name;
	char        *   ui_name;
};

#define KTT_ENTRY0(MAME, SDL, UI) { ITEM_ID_ ## MAME, SDL, "ITEM_ID_" #MAME, (char *) UI }
#define KTT_ENTRY1(MAME, SDL) KTT_ENTRY0(MAME, SDL, #MAME)

static kt_table key_trans_table[] =
{
	//         MAME key         scancode
	KTT_ENTRY1(  ESC,           0x01 ),
	KTT_ENTRY1(  1,             0x02 ),
	KTT_ENTRY1(  2,             0x03 ),
	KTT_ENTRY1(  3,             0x04 ),
	KTT_ENTRY1(  4,             0x05 ),
	KTT_ENTRY1(  5,             0x06 ),
	KTT_ENTRY1(  6,             0x07 ),
	KTT_ENTRY1(  7,             0x08 ),
	KTT_ENTRY1(  8,             0x09 ),
	KTT_ENTRY1(  9,             0x0a ),
	KTT_ENTRY1(  0,             0x0b ),
	KTT_ENTRY1(  MINUS,         0x0c ), // -
	KTT_ENTRY1(  EQUALS,        0x0d ), // =
	KTT_ENTRY1(  BACKSPACE,     0x0e ),
	KTT_ENTRY1(  TAB,           0x0f ),
	KTT_ENTRY1(  Q,             0x10 ),
	KTT_ENTRY1(  W,             0x11 ),
	KTT_ENTRY1(  E,             0x12 ),
	KTT_ENTRY1(  R,             0x13 ),
	KTT_ENTRY1(  T,             0x14 ),
	KTT_ENTRY1(  Y,             0x15 ),
	KTT_ENTRY1(  U,             0x16 ),
	KTT_ENTRY1(  I,             0x17 ),
	KTT_ENTRY1(  O,             0x18 ),
	KTT_ENTRY1(  P,             0x19 ),
	KTT_ENTRY1(  OPENBRACE,     0x1a ), // [
	KTT_ENTRY1(  CLOSEBRACE,    0x1b ), // ]
	KTT_ENTRY1(  ENTER,         0x1c ),
	KTT_ENTRY1(  LCONTROL,      0x1d ),
	KTT_ENTRY1(  A,             0x1e ),
	KTT_ENTRY1(  S,             0x1f ),
	KTT_ENTRY1(  D,             0x20 ),
	KTT_ENTRY1(  F,             0x21 ),
	KTT_ENTRY1(  G,             0x22 ),
	KTT_ENTRY1(  H,             0x23 ),
	KTT_ENTRY1(  J,             0x24 ),
	KTT_ENTRY1(  K,             0x25 ),
	KTT_ENTRY1(  L,             0x26 ),
	KTT_ENTRY1(  COLON,         0x27 ), // ;:
	KTT_ENTRY1(  QUOTE,         0x28 ), // '"
	KTT_ENTRY1(  TILDE,         0x29 ), // `~
	KTT_ENTRY1(  LSHIFT,        0x2a ),
	KTT_ENTRY1(  BACKSLASH,     0x2b ), // "\"
	KTT_ENTRY1(  Z,             0x2c ),
	KTT_ENTRY1(  X,             0x2d ),
	KTT_ENTRY1(  C,             0x2e ),
	KTT_ENTRY1(  V,             0x2f ),
	KTT_ENTRY1(  B,             0x30 ),
	KTT_ENTRY1(  N,             0x31 ),
	KTT_ENTRY1(  M,             0x32 ),
	KTT_ENTRY1(  COMMA,         0x33 ), // ,
	KTT_ENTRY1(  STOP,          0x34 ), // .
	KTT_ENTRY1(  SLASH,         0x35 ), // / 
	KTT_ENTRY1(  RSHIFT,        0x36 ),
	KTT_ENTRY1(  ASTERISK,      0x37 ), // KP_*
	KTT_ENTRY1(  LALT,          0x38 ),
	KTT_ENTRY1(  SPACE,         0x39 ),
	KTT_ENTRY1(  CAPSLOCK,      0x3a ),
	KTT_ENTRY1(  F1,            0x3b ),
	KTT_ENTRY1(  F2,            0x3c ),
	KTT_ENTRY1(  F3,            0x3d ),
	KTT_ENTRY1(  F4,            0x3e ),
	KTT_ENTRY1(  F5,            0x3f ),
	KTT_ENTRY1(  F6,            0x40 ),
	KTT_ENTRY1(  F7,            0x41 ),
	KTT_ENTRY1(  F8,            0x42 ),
	KTT_ENTRY1(  F9,            0x43 ),
	KTT_ENTRY1(  F10,           0x44 ),
	KTT_ENTRY1(  NUMLOCK,       0x45 ),
	KTT_ENTRY1(  SCRLOCK,       0x46 ),
	KTT_ENTRY1(  7_PAD,         0x47 ),
	KTT_ENTRY1(  8_PAD,         0x48 ),
	KTT_ENTRY1(  9_PAD,         0x49 ),
	KTT_ENTRY1(  MINUS_PAD,     0x4a ),
	KTT_ENTRY1(  4_PAD,         0x4b ),
	KTT_ENTRY1(  5_PAD,         0x4c ),
	KTT_ENTRY1(  6_PAD,         0x4d ),
	KTT_ENTRY1(  PLUS_PAD,      0x4e ),
	KTT_ENTRY1(  1_PAD,         0x4f ),
	KTT_ENTRY1(  2_PAD,         0x50 ),
	KTT_ENTRY1(  3_PAD,         0x51 ),
	KTT_ENTRY1(  0_PAD,         0x52 ),
	KTT_ENTRY1(  DEL_PAD,       0x53 ),
	// 54-56
	KTT_ENTRY1(  F11,           0x57 ),
	KTT_ENTRY1(  F12,           0x58 ),
	// 59-5f
	KTT_ENTRY1(  ENTER_PAD,     0x60 ),
	KTT_ENTRY1(  RCONTROL,      0x61 ),
	KTT_ENTRY1(  SLASH_PAD,     0x62 ),
	KTT_ENTRY1(  PRTSCR,        0x63 ),
	KTT_ENTRY1(  RALT,          0x64 ),
	// 65-66
	KTT_ENTRY1(  HOME,          0x66 ),
	KTT_ENTRY1(  UP,            0x67 ),
	KTT_ENTRY1(  PGUP,          0x68 ),
	KTT_ENTRY1(  LEFT,          0x69 ),
	KTT_ENTRY1(  RIGHT,         0x6a ),
	KTT_ENTRY1(  END,           0x6b ),
	KTT_ENTRY1(  DOWN,          0x6c ),
	KTT_ENTRY1(  PGDN,          0x6d ),
	KTT_ENTRY1(  INSERT,        0x6e ),
	KTT_ENTRY1(  DEL,           0x6f ),
	// 70-76
	KTT_ENTRY1(  PAUSE,         0x77 ),
	// 78-7c
	KTT_ENTRY1(  LWIN,          0x7d ),
	KTT_ENTRY1(  RWIN,          0x7e ),
	KTT_ENTRY1(  MENU,          0x7f ),

	{ ITEM_ID_INVALID }
};


/******************************************************************************/


//============================================================
//  GLOBALS
//============================================================

// a single rendering target
static render_target *our_target;

// a single input device
static input_device *keyboard_device;

UINT8 vt_keystate[128];

int osdmini_run = 0;


//============================================================
//  FUNCTION PROTOTYPES
//============================================================


static INT32 keyboard_get_state(void *device_internal, void *item_internal);


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


//============================================================
//  constructor
//============================================================

mini_osd_interface::mini_osd_interface(mini_osd_options &options)
: osd_common_t(options)
{
}


//============================================================
//  destructor
//============================================================

mini_osd_interface::~mini_osd_interface()
{
}


/******************************************************************************/


void mini_osd_interface::init(running_machine &machine)
{

	video_init_fbcon();

	// call our parent
	osd_common_t::init(machine);
	osd_common_t::init_subsystems();

	// initialize the video system by allocating a rendering target
	our_target = machine.render().target_alloc();

	// nothing yet to do to initialize sound, since we don't have any
	// sound updates are handled by update_audio_stream() below

	// initialize the input system by adding devices
	// let's pretend like we have a keyboard device
	keyboard_device = machine.input().device_class(DEVICE_CLASS_KEYBOARD).add_device("Keyboard");
	if (keyboard_device == NULL)
		fatalerror("Error creating keyboard device\n");

	int i = 0;
	while(1){
		if(key_trans_table[i].mame_key == ITEM_ID_INVALID)
			break;

		int sc = key_trans_table[i].scan_key;
		keyboard_device->add_item(
				key_trans_table[i].ui_name, key_trans_table[i].mame_key, keyboard_get_state, &vt_keystate[sc]);
		i += 1;
	}

	osdmini_run = 1;
}


//============================================================
//  osd_update
//============================================================

void mini_osd_interface::update(bool skip_redraw)
{
	video_update_fbcon(our_target);
}

void mini_osd_interface::osd_exit(void)
{
	osdmini_run = 0;

	osd_common_t::osd_exit();

	printk("\nosd_exit!\n");

	video_exit_fbcon();
}

//============================================================
//  keyboard_get_state
//============================================================

static INT32 keyboard_get_state(void *device_internal, void *item_internal)
{
	UINT8 *keystate = (UINT8 *)item_internal;
	return *keystate;
}



/******************************************************************************/


char *osd_get_clipboard_text(void)
{
	// can't support clipboards generically
	return NULL;
}


/******************************************************************************/


