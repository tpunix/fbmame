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
	mini_osd_options();

	bool slave() const { return bool_value("slave"); }

private:
	static const options_entry s_option_entries[];

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

	mini_osd_options &options() { return m_options; }

private:

	mini_osd_options &m_options;
};


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


//============================================================
//  GLOBAL VARIABLES
//============================================================

extern render_target *our_target;

extern input_device *keyboard_device;
extern UINT8 vt_keystate[];

extern int fb_xres, fb_yres, fb_bpp, fb_pitch;
extern UINT8 *fb_base_addr;


//============================================================
//  FUNCTION PROTOTYPES
//============================================================

SIMPLE_QUEUE *simple_queue_create(int size);
void simple_queue_free(SIMPLE_QUEUE *sq);
QOBJ *get_idle_qobj(SIMPLE_QUEUE *sq);
QOBJ *get_ready_qobj(SIMPLE_QUEUE *sq);
void qobj_set_ready(QOBJ *qobj);
void qobj_set_idle(QOBJ *qobj);
void qobj_init(QOBJ *qobj, void *data1, int data2);


void input_keyboard_init(void);


extern void (*osd_input_init)(void);
extern void (*osd_input_update)(void);
extern void (*osd_input_exit)(void);


extern void (*osd_video_init_backend)(void);
extern void (*osd_video_exit_backend)(void);

void osd_video_init(void);
void osd_video_update(bool skip_draw);
void osd_video_exit(void);


void input_register_vt(void);
void input_register_remote(void);
void video_register_fbcon(void);
void video_register_remote(void);


