// license:BSD-3-Clause
// copyright-holders:Miodrag Milanovic
/***************************************************************************

    alsa_sound.c

    Alsa sound interface.

*******************************************************************c********/

#include <time.h>
#include <sys/time.h>

#include "sound_module.h"
#include "modules/osdmodule.h"

#include <alsa/asoundlib.h>



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
	virtual void update_audio_stream(bool is_throttled, const INT16 *buffer, int samples_this_frame);
	virtual void set_mastervolume(int attenuation);

private:
	INT8 *stream_buffer;
	INT32 stream_buffer_size;

};


/******************************************************************************/

static snd_pcm_t *snd_pcm;
static snd_pcm_uframes_t period_size = 96; // in frames
static snd_pcm_uframes_t buffer_size;       // in frames

static INT16 *snd_buf = NULL;

/******************************************************************************/


int sound_alsa::init()
{
	snd_pcm_hw_params_t *hw_params;
	snd_pcm_sw_params_t* sw_params;
	int retv;

	printk("Sound_ALSA init!\n");

	retv = snd_pcm_open(&snd_pcm, "hw:0,0", SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
	//retv = snd_pcm_open(&snd_pcm, "hw:0,0", SND_PCM_STREAM_PLAYBACK, 0);
	if(retv<0){
		printk("Can't open audio device!\n");
		return retv;
	}
	//snd_pcm_nonblock(snd_pcm, 1);

	snd_pcm_hw_params_alloca(&hw_params);
	snd_pcm_hw_params_any(snd_pcm, hw_params);

    snd_pcm_hw_params_set_access(snd_pcm, hw_params, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(snd_pcm, hw_params, SND_PCM_FORMAT_S16_LE);
    snd_pcm_hw_params_set_channels(snd_pcm, hw_params, 2);

	UINT32 set_rate = sample_rate();
	snd_pcm_hw_params_set_rate_near(snd_pcm, hw_params, &set_rate, 0);
	printk("  sample_rate: %d set_rate: %d\n", sample_rate(), set_rate);

	retv = snd_pcm_hw_params_set_period_size_near(snd_pcm, hw_params, &period_size, 0);
	if(retv<0){
		printk("Can't set sample rate!\n");
		return retv;
	}
	printk("  period_size: %d\n", (int)period_size);


	buffer_size = sample_rate();
	retv = snd_pcm_hw_params_set_buffer_size_near(snd_pcm, hw_params, &buffer_size);
	if(retv<0){
		printk("Can't set buffer size!\n");
		return retv;
	}
	printk("  buffer_size: %d\n", (int)buffer_size);

	retv = snd_pcm_hw_params(snd_pcm, hw_params);
	if(retv<0){
		printk("Can't set hw audio params!\n");
		return retv;
	}

#if 0
	snd_pcm_sw_params_alloca(&sw_params);
	snd_pcm_sw_params_current(snd_pcm, sw_params);
	snd_pcm_sw_params_set_start_threshold(snd_pcm, sw_params, sample_rate()/10);
	snd_pcm_sw_params(snd_pcm, sw_params);
#endif

	retv = snd_pcm_prepare(snd_pcm);
	if(retv<0){
		printk("Can't prepare audio!\n");
		return retv;
	}

	snd_buf = (INT16*)malloc(sample_rate()*4);

	return 0;
}


void sound_alsa::exit()
{
	printk("Sound_ALSA exit!\n");
	free(snd_buf);
	snd_pcm_drop(snd_pcm);
	snd_pcm_close(snd_pcm);
}


/******************************************************************************/


void sound_alsa::set_mastervolume(int attenuation)
{
}


int xrun_recovery(snd_pcm_t *handle, int err)
{
	if (err == -EPIPE) { /* under-run */
		err = snd_pcm_prepare(handle);
	 	if (err < 0)
			printk("Can't recovery from underrun, prepare failed: %s\n",snd_strerror(err));
	} else if (err == -ESTRPIPE) {
		while ((err = snd_pcm_resume(handle)) == -EAGAIN)
			sleep(1);       /* wait until the suspend flag is released */
		if (err < 0) {
			err = snd_pcm_prepare(handle);
			if (err < 0)
				printk("Can't recovery from suspend, prepare failed: %s\n", snd_strerror(err));
		}
	}

	return err;  
}



void sound_alsa::update_audio_stream(bool is_throttled, const INT16 *buffer, int samples_this_frame)
{
	int retv;

	snd_pcm_sframes_t avial = snd_pcm_avail(snd_pcm);
	//printk("snd: avial = %d\n", (int)avial);
	if(avial>=(buffer_size - samples_this_frame*2)){
		int i;
		for(i=0; i<samples_this_frame; i++){
			*(UINT32*)(snd_buf+i*8+0) = *(UINT32*)(buffer+i*4);
			*(UINT32*)(snd_buf+i*8+4) = *(UINT32*)(buffer+i*4);
		}
		samples_this_frame *= 2;
		buffer = snd_buf;
		printk("snd_pcm_writei: resample ...\n");
	}

	while(samples_this_frame){
		retv = snd_pcm_writei(snd_pcm, buffer, samples_this_frame);
		if(retv<0){
			printk("snd_pcm_writei: %s\n", snd_strerror(retv));
			if(xrun_recovery(snd_pcm, retv)<0){
				break;
			}
			continue;
		}

		samples_this_frame -= retv;
		buffer += retv*2;
	}

}


/******************************************************************************/



#else
MODULE_NOT_SUPPORTED(sound_alsa, OSD_SOUND_PROVIDER, "alsa")
#endif

MODULE_DEFINITION(SOUND_ALSA, sound_alsa)

