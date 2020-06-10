// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic
/***************************************************************************

    alsa_sound.c

    Alsa sound interface.

*******************************************************************c********/

#include "sound_module.h"
#include "modules/osdmodule.h"

#define printk osd_printf_info

#if defined(OSD_MINI)





class sound_alsa : public osd_module, public sound_module
{
public:
	sound_alsa() : osd_module(OSD_SOUND_PROVIDER, "alsa"), sound_module()
	{
	}
	virtual ~sound_alsa() { }

	virtual int init();
	virtual void exit();

	// sound_module
	virtual void update_audio_stream(bool is_throttled, const INT16 *buffer, int samples_this_frame) { }
	virtual void set_mastervolume(int attenuation) { }

private:
	INT8 *stream_buffer;
	INT32 stream_buffer_size;

};


/******************************************************************************/

int sound_alsa::init()
{
	printk("Sound_ALSA init!\n");
	return 0;
}

void sound_alsa::exit()
{
	printk("Sound_ALSA exit!\n");
}


/******************************************************************************/



/******************************************************************************/



#else
MODULE_NOT_SUPPORTED(sound_alsa, OSD_SOUND_PROVIDER, "alsa")
#endif

MODULE_DEFINITION(SOUND_ALSA, sound_alsa)

