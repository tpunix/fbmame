// license:BSD-3-Clause
// copyright-holders:Aaron Giles
//============================================================
//
//  osdmini.h - Core header
//
//============================================================

#include "options.h"
#include "osdepend.h"
#include "modules/lib/osdobj_common.h"


#define printk osd_printf_info

class mini_osd_options : public osd_options
{
public:
	// construction/destruction
	mini_osd_options();

};

//============================================================
//  TYPE DEFINITIONS
//============================================================

class mini_osd_interface : public osd_common_t
{
public:
	// construction/destruction
	mini_osd_interface(mini_osd_options &options);
	virtual ~mini_osd_interface();

	// general overridables
	virtual void init(running_machine &machine);
	virtual void update(bool skip_redraw);
	virtual void osd_exit();

private:
};



//============================================================
//  GLOBAL VARIABLES
//============================================================

extern const options_entry mame_win_options[];

// defined in winwork.c
extern int osd_num_processors;

extern render_target *our_target;

extern input_device *keyboard_device;

extern int osdmini_run;



//============================================================
//  FUNCTION PROTOTYPES
//============================================================


extern void (*osd_input_init)(void);
extern void (*osd_input_update)(void);
extern void (*osd_input_exit)(void);

extern void (*osd_video_init)(void);
extern void (*osd_video_update)(bool skip_draw);
extern void (*osd_video_exit)(void);


void input_register_vt(void);
void video_register_fbcon(void);

