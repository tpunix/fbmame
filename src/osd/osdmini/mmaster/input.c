
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <poll.h>
#include <mqueue.h>
#include <pthread.h>
#include <linux/input.h>
#include <sys/ioctl.h>

#include "mmaster.h"


/******************************************************************************/

static int input_running;

static mqd_t mq_input;
static pthread_t pth_poll;

static struct pollfd pfds[16];
static int pfds_nr;

static u64 kbit_mask[2] = {0xfffffffffffffffe, 0xe080ffdf018fffff};

/******************************************************************************/


static int input_match(int fd, int evtype, u8 *mask, int len)
{
	int i, retv;
	u8 keybits[128];

	retv = ioctl(fd, EVIOCGBIT(evtype, 128), keybits);
	if(retv<=0)
		return 0;

	if(retv<len)
		len = retv;

	for(i=0; i<len; i++){
		if(keybits[i]&mask[i])
			return 1;
	}

	return 0;
}


static int k_ctrl=0, k_c=0;

static int check_ctrl_c(u32 evkey)
{
#if 0
	int key = evkey>>16;
	int val = evkey&0xffff;

	if(key==KEY_LEFTCTRL)
		k_ctrl = val;
	if(key==KEY_C)
		k_c = val;

	if(k_ctrl && k_c)
		return 1;
#endif
	return 0;
}

static void *input_poll_thread(void *arg)
{
	int i, retv;
	struct input_event evt;
	u32 evkey;

	while(input_running){
		retv = poll(pfds, pfds_nr, 100);
		if(retv==0){
			continue;
		}else if(retv<0){
			if(errno==EINTR)
				continue;
			break;
		}

		for(i=0; i<pfds_nr; i++){
			if(!(pfds[i].revents & POLLIN)){
				continue;
			}

			while(1){
				retv = read(pfds[i].fd, &evt, sizeof(evt));
				if(retv<=0)
					break;
				if(evt.type==EV_KEY){
					evkey = (evt.code<<16) | (evt.value & 0xffff);
				}else if(evt.type==EV_SYN){
					if(check_ctrl_c(evkey))
						input_running = 0;
					//printf("EV_KEY: %08x\n", evkey);
					mq_send(mq_input, (char*)&evkey, sizeof(evkey), 0);
					evkey = 0;
				}
			}

		}
	}

	input_running = -1;

	pthread_exit((void*)-1);
}


void input_event_init(void)
{
	char devname[256];
	int i, fd, retv;

	struct mq_attr mattr;

	mattr.mq_flags = 0;
	mattr.mq_maxmsg = 16;
	mattr.mq_msgsize = sizeof(int);
	mattr.mq_curmsgs = 0;
	mq_input = mq_open("/mq_input", O_RDWR|O_NONBLOCK|O_CREAT|O_EXCL, 0666, &mattr);
	if(mq_input<0 && errno==EEXIST){
		retv = mq_unlink("/mq_input");
		printf("/mq_input exist! unlink it ... %d\n", retv);
		mq_input = mq_open("/mq_input", O_RDWR|O_NONBLOCK|O_CREAT|O_EXCL, 0666, &mattr);
	}
	if(mq_input<0){
		printf("mq_open(/mq_input) failed! %d\n", errno);
		exit(-1);
	}


	pfds_nr = 0;
	for(i=0; i<16; i++){
		sprintf(devname, "/dev/input/event%d", i);
		fd = open(devname, O_RDWR|O_NONBLOCK);
		if(fd<0)
			break;

		if(input_match(fd, EV_KEY, (u8*)kbit_mask, sizeof(kbit_mask))==0){
			close(fd);
			continue;
		}

		printf("\nOpen %s ... fd=%d\n", devname, fd);
		ioctl(fd, EVIOCGNAME(256), devname);
		printf("     name: %s\n", devname);

		ioctl(fd, EVIOCGRAB, 1);

		pfds[pfds_nr].fd = fd;
		pfds[pfds_nr].events = POLLIN;
		pfds_nr += 1;
	}


	input_running = 1;

	pthread_create(&pth_poll, NULL, input_poll_thread, NULL);

}

void input_event_exit(void)
{
	input_running = 0;
	while(input_running!=-1){
		usleep(1000);
	}

	mq_close(mq_input);
	mq_unlink("/mq_input");

}


/******************************************************************************/

