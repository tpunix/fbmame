
#ifndef _MMASTER_H_
#define _MMASTER_H_


typedef unsigned long long u64;
typedef unsigned int   u32;
typedef unsigned short u16;
typedef unsigned char  u8;


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



u64 osd_ticks(void);


void input_event_init(void);
void input_event_exit(void);


void fb_init(void);
void fb_exit(void);


#endif


