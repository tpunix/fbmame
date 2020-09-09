
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <pthread.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#include "mmaster.h"



/******************************************************************************/


#define SHM_KEY 0x76057810
#define SHM_SIZE 0x02000000


static int shmid;
static u8 *shm_addr;
static u8 *shm_ctrl;


void shm_init(void)
{

	shmid = shmget(SHM_KEY, SHM_SIZE, 0666|IPC_CREAT);
	if(shmid<0){
		perror("shmget");
		exit(-1);
	}

	shm_addr = (u8*)shmat(shmid, NULL, 0);
	if(shm_addr==(void*)-1){
		perror("shmat");
		exit(-1);
	}

	printf("shm id:%08x addr:%p\n", shmid, shm_addr);

	shm_ctrl = shm_addr+SHM_SIZE-4096;

}


void shm_exit(void)
{
	shmdt(shm_addr);
	shmctl(shmid, IPC_RMID, 0);
}


/******************************************************************************/


static int qobj_ages = 1;

SIMPLE_QUEUE *simple_queue_create(int size)
{
	SIMPLE_QUEUE *sq;

	sq = (SIMPLE_QUEUE*)malloc(sizeof(SIMPLE_QUEUE));

	sq->size = size;
	sq->list = (QOBJ*)malloc(size*sizeof(QOBJ));

	memset(sq->list, 0, size*sizeof(QOBJ));
	return sq;
}

void simple_queue_free(SIMPLE_QUEUE *sq)
{
	free(sq->list);
	free(sq);
}


QOBJ *get_idle_qobj(SIMPLE_QUEUE *sq)
{
	int i;

	for(i=0; i<sq->size; i++){
		if(sq->list[i].state==QOBJ_IDLE){
			sq->list[i].state = QOBJ_WRITE;
			return &(sq->list[i]);
		}
	}

	//printk("No IDLE QOBJ!\n");
	return NULL;
}

QOBJ *get_ready_qobj(SIMPLE_QUEUE *sq)
{
	int i, r, age;

	r = -1;
	age = 0x7fffffff;

	for(i=0; i<sq->size; i++){
		if(sq->list[i].state==QOBJ_READY){
			if(sq->list[i].age<age){
				r = i;
				age = sq->list[i].age;
			}
		}
	}

	if(r==-1){
		//printk("No READY QOBJ!\n");
		return NULL;
	}

	sq->list[r].state = QOBJ_USING;
	return &sq->list[r];
}


void qobj_set_ready(QOBJ *qobj)
{
	qobj->age = qobj_ages++;
	qobj->state = QOBJ_READY;
}

void qobj_set_idle(QOBJ *qobj)
{
	qobj->age = 0;
	qobj->state = QOBJ_IDLE;
}

void qobj_init(QOBJ *qobj, void *data1, int data2)
{
	qobj->state = QOBJ_IDLE;
	qobj->age = 0;
	qobj->data1 = data1;
	qobj->data2 = data2;
}


/******************************************************************************/


static u64 vtm_old=0, vtm_new, vtm_sec=0;
static int fps=0;

void video_show_fps(void)
{
	fps += 1;
	vtm_new = osd_ticks();
	if(vtm_new - vtm_sec >= 1000000){
		printf("  fps: %d\n", fps);
		fps = 0;
		vtm_sec = vtm_new;
	}
	vtm_old = vtm_new;
}


/******************************************************************************/


struct shm_video
{
	int magic;

	int fbx;
	int fby;
	int fbpp;
	int fbpitch;

	int vobj_cnt;
	QOBJ vobj[4];
};

SIMPLE_QUEUE vbo_queue;


/******************************************************************************/


#define FBDEV  "/dev/fb0"

static int fbfd;
static u8 *fbptr;

static int fbx;
static int fby;
static int fbpp;
static int fbpitch;

static struct fb_var_screeninfo g_vinfo;

static pthread_t pth_video;

static int video_running;


/******************************************************************************/


void video_queue_init(void)
{
	int i;
	struct shm_video *sv;

	sv = (struct shm_video*)shm_ctrl;
	sv->magic = 0x76057810;
	sv->fbx = fbx;
	sv->fby = fby;
	sv->fbpp = fbpp;
	sv->fbpitch = fbpitch;

	sv->vobj_cnt = 4;
	for(i=0; i<4; i++){
		qobj_init(&sv->vobj[i], 0, i*fby*fbpitch);
	}

	vbo_queue.size = 4;
	vbo_queue.list = sv->vobj;
}


static void *video_update_thread(void *arg)
{
	QOBJ *new_obj;

	while(video_running){
		new_obj = get_ready_qobj(&vbo_queue);
		if(new_obj){
			memcpy(fbptr, shm_addr+new_obj->data2, fby*fbpitch);
			qobj_set_idle(new_obj);
			usleep(13000);
		}else{
			usleep(3000);
		}
	}

	video_running = -1;
	return NULL;
}


void fb_init(void)
{
    struct fb_var_screeninfo vinfo;
    struct fb_fix_screeninfo finfo;

    fbfd = open(FBDEV, O_RDWR);
    if(fbfd<0){
        printf("Open %s failed!\n", FBDEV);
        exit(-1);
    }

    ioctl(fbfd, FBIOGET_VSCREENINFO, &vinfo);
    ioctl(fbfd, FBIOGET_FSCREENINFO, &finfo);

	fbx = vinfo.xres;
    fby = vinfo.yres;
    fbpp = vinfo.bits_per_pixel;
	fbpitch = finfo.line_length;

	if(vinfo.yres_virtual < fby*3){
		vinfo.yres_virtual = fby*3;
		int retv = ioctl(fbfd, FBIOPUT_VSCREENINFO, &vinfo);
		if(retv<0){
			printf("  FBIOPUT_VSCREENINFO failed! %d\n", retv);
		}
	}
	g_vinfo = vinfo;
	printf(FBDEV": %dx%d-%d\n", fbx, fby, fbpp);

    fbptr = (u8*)mmap(0, finfo.smem_len, PROT_READ|PROT_WRITE, MAP_SHARED, fbfd, 0);
    if(fbptr<0){
        printf("Map %s failed!\n", FBDEV);
        exit(-1);
    }

	g_vinfo = vinfo;


	shm_init();

	video_queue_init();

	video_running = 1;

	pthread_create(&pth_video, NULL, video_update_thread, NULL);

}


void fb_exit(void)
{
	video_running = 0;
	while(video_running!=-1){
		usleep(1000);
	}

	shm_exit();
}


/******************************************************************************/

