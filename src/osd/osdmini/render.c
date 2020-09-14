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






void blend_line_sample(UINT32 *dst, UINT32 *src, int width)
{
	int x;

	for(x=0; x<width; x++){
		UINT32 sp = *src++;
		UINT32 sa = (sp>>24)&0xff;
		UINT32 sr = (sp>>16)&0xff;
		UINT32 sg = (sp>> 8)&0xff;
		UINT32 sb = (sp>> 0)&0xff;
		if(sa){
			UINT32 dp = *dst;
			UINT32 dr = (dp>>16)&0xff;
			UINT32 dg = (dp>> 8)&0xff;
			UINT32 db = (dp>> 0)&0xff;
			UINT32 da = 0x100-sa;

			dr = (sr*sa + dr*da)>>8;
			dg = (sg*sa + dg*da)>>8;
			db = (sb*sa + db*da)>>8;

			*dst = 0xff000000 | (dr<<16) | (dg<<8) | db;
		}
		dst++;
	}
}

/******************************************************************************/

void ne10_img_resize_bilinear_rgba_c(UINT8* dst, UINT32 dst_width, UINT32 dst_height, UINT32 dst_stride,
                                     UINT8* src, UINT32 src_width, UINT32 src_height, UINT32 src_stride);
void m_resize_rgba_c(UINT8* dst, int dst_width, int dst_height, int dst_stride,
					 UINT8* src, int src_width, int src_height, int src_stride);


static void draw_quad_rgb32(const render_primitive *prim, UINT8 *dst_addr, int width, int height, int pitch)
{
	int x0 = prim->bounds.x0;
	int y0 = prim->bounds.y0;
	int x1 = prim->bounds.x1;
	int y1 = prim->bounds.y1;
	int tw = prim->texture.width;
	int th = prim->texture.height;
	int w = x1-x0;
	int h = y1-y0;
	int x, y;



	UINT32 *src_addr = (UINT32*)prim->texture.base;
	dst_addr += y0*pitch + x0*4;

	int format = PRIMFLAG_GET_TEXFORMAT(prim->flags);
	int mode = PRIMFLAG_GET_BLENDMODE(prim->flags);


	if(format==TEXFORMAT_RGB32 || (format==TEXFORMAT_ARGB32 && mode==BLENDMODE_NONE)){
#if 1
		m_resize_rgba_c(
		//ne10_img_resize_bilinear_rgba_c(
			dst_addr, w, h, pitch,
			(UINT8*)prim->texture.base, tw, th, prim->texture.rowpixels*4);
#else
		for(y=0; y<h; y++){
			memcpy(dst_addr, src_addr, w*4);
			dst_addr += pitch;
			src_addr += prim->texture.rowpixels;
		}
#endif
	}else if(format==TEXFORMAT_ARGB32 && mode==BLENDMODE_ALPHA){
		UINT32 pr = 256.0f * prim->color.r;
		UINT32 pg = 256.0f * prim->color.g;
		UINT32 pb = 256.0f * prim->color.b;
		UINT32 pa = 256.0f * prim->color.a;

		if(w>tw)
			w = tw;
		if(th>1){
			if(h>th)
				h = th;
		}

		if(pr>=256 && pg>=256 && pb>=256 && pa>=256){
			// simple mode, no color
			for(y=0; y<h; y++){
				blend_line_sample((UINT32*)dst_addr, (UINT32*)src_addr, w);
				dst_addr += pitch;
				if(th>1)
					src_addr += prim->texture.rowpixels;
			}

		}else{

			for(y=0; y<h; y++){
				UINT32 *dst = (UINT32*)dst_addr;
				UINT32 *src = (UINT32*)src_addr;
				for(x=0; x<w; x++){
					UINT32 sp = *src++;
					UINT32 sa = (sp>>24)&0xff;
					UINT32 sr = (sp>>16)&0xff;
					UINT32 sg = (sp>> 8)&0xff;
					UINT32 sb = (sp>> 0)&0xff;
					if(sa){
						sa *= pa;
						UINT32 dp = *dst;
						UINT32 dr = (dp>>16)&0xff;
						UINT32 dg = (dp>> 8)&0xff;
						UINT32 db = (dp>> 0)&0xff;
						UINT32 da = 0x10000-sa;

						dr = (sr*pr*sa + dr*da)>>24;
						dg = (sg*pg*sa + dg*da)>>24;
						db = (sb*pb*sa + db*da)>>24;

						*dst++ = 0xff000000 | (dr<<16) | (dg<<8) | db;
					}else{
						dst++;
					}
				}
				dst_addr += pitch;
				if(th>1)
					src_addr += prim->texture.rowpixels;
			}
		}
	}

}


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

	UINT32 r = 255.0f * prim->color.r;
	UINT32 g = 255.0f * prim->color.g;
	UINT32 b = 255.0f * prim->color.b;
	UINT32 a = 256.0f * prim->color.a;

	UINT32 *dst;
	dst_addr += y0*pitch + x0*4;

	if( (PRIMFLAG_GET_BLENDMODE(prim->flags) == BLENDMODE_NONE) || a>=256){
		UINT32 color = 0xff000000 | (r<<16) | (g<<8) | b;

		for(y=y0; y<y1; y++){
			dst = (UINT32*)dst_addr;
			for(x=x0; x<x1; x++){
				*dst++ = color;
			}
			dst_addr += pitch;
		}

	}else if(a>0){
		r = (r*a)<<16;
		g = (g*a)<<8;
		b = (b*a)<<0;
		UINT32 inva = 256-a;

		for(y=y0; y<y1; y++){
			dst = (UINT32*)dst_addr;
			for(x=x0; x<x1; x++){
				UINT32 dp = *dst;
				UINT32 dr = (r + (dp&0x00ff0000)*inva)>>24;
				UINT32 dg = (g + (dp&0x0000ff00)*inva)>>16;
				UINT32 db = (b + (dp&0x000000ff)*inva)>>8;
				*dst++ = 0xff000000 | (dr<<16) | (dg<<8) | 0;
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
			draw_quad_rgb32(prim, dst_addr, width, height, pitch);
		}

		prim = prim->next();
	}

}


/******************************************************************************/


