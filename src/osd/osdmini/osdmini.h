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



//============================================================
//  FUNCTION PROTOTYPES
//============================================================

INT64 get_time(void);


void video_init_fbcon(void);
void video_update_fbcon(render_target *our_target);
void video_exit_fbcon(void);


// use this to ping the watchdog
void winmain_watchdog_ping(void);
void winmain_dump_stack();
