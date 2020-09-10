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
#include "modules/sync/osdsync.h"
#include "osdmini.h"


#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <mqueue.h>
#include <errno.h>


/******************************************************************************/








/******************************************************************************/

/******************************************************************************/


static void draw_line_rgb32(const render_primitive *prim, UINT8 *dst_addr, int width, int height, int pitch)
{
	int x0 = prim->bounds.x0;
	int y0 = prim->bounds.y0;
	int x1 = prim->bounds.x1;
	int y1 = prim->bounds.y1;
	int x, y;

	int r = 255.0f * prim->color.r * prim->color.a;
	int g = 255.0f * prim->color.g * prim->color.a;
	int b = 255.0f * prim->color.b * prim->color.a;
	int color = 0xff000000 | (r<<16) | (g<<8) | b;

	if(x0==x1){
		if(y0>y1){ y=y0; y0=y1; y1=y;}
		dst_addr += y0*pitch + x0*4;
		for(y=y0; y<=y1; y++){
			*(UINT32*)(dst_addr) = color;
			dst_addr += pitch;
		}
	}else if(y0==y1){
		if(x0>x1){ x=x0; x0=x1; x1=x;}
		dst_addr += y0*pitch + x0*4;
		for(x=x0; x<=x1; x++){
			*(UINT32*)(dst_addr) = color;
			dst_addr += 4;
		}
	}else{
		printk("LINE: (%d,%d)-(%d,%d) %08x\n", x0, y0, x1, y1, color);
	}

}


static void draw_rect_rgb32(const render_primitive *prim, UINT8 *dst_addr, int width, int height, int pitch)
{
	int x0 = prim->bounds.x0;
	int y0 = prim->bounds.y0;
	int x1 = prim->bounds.x1;
	int y1 = prim->bounds.y1;
	int x, y;

	UINT32 r, g, b;
	UINT32 a = 255.0f * prim->color.a;

	UINT32 *dst;
	dst_addr += y0*pitch + x0*4;

	if( (PRIMFLAG_GET_BLENDMODE(prim->flags) == BLENDMODE_NONE) || a>=0xff){
		r = 255.0f * prim->color.r;
		g = 255.0f * prim->color.g;
		b = 255.0f * prim->color.b;
		UINT32 color = 0xff000000 | (r<<16) | (g<<8) | b;

		for(y=y0; y<y1; y++){
			dst = (UINT32*)dst_addr;
			for(x=x0; x<x1; x++){
				*dst++ = color;
			}
			dst_addr += pitch;
		}

	}else if(a>0){
		r = (255.0f * prim->color.r * prim->color.a);
		g = (255.0f * prim->color.g * prim->color.a);
		b = (255.0f * prim->color.b * prim->color.a);
		UINT32 inva = 255-a;

		for(y=y0; y<y1; y++){
			dst = (UINT32*)dst_addr;
			for(x=x0; x<x1; x++){
				UINT32 dp = *dst;
				r += ((dp&0x00ff0000)*inva)>>24;
				g += ((dp&0x0000ff00)*inva)>>16;
				b += ((dp&0x000000ff)*inva)>>8;
				*dst++ = 0xff000000 | (r<<16) | (g<<8) | b;
			}
			dst_addr += pitch;
		}
	}

}

/******************************************************************************/

void draw_primlist(render_primitive_list *primlist, UINT8 *dst_addr, int width, int height, int pitch)
{
	const render_primitive *prim = primlist->first();

	while(prim){
		if(prim->type==render_primitive::LINE){
			draw_line_rgb32(prim, dst_addr, width, height, pitch);
		}else if(prim->texture.base==NULL){
			draw_rect_rgb32(prim, dst_addr, width, height, pitch);
		}else{
		}

		prim = prim->next();
	}

}


/******************************************************************************/


