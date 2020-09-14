
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>



#ifdef __aarch64__
# define USE_NEON
# ifdef USE_NEON
#  include <arm_neon.h>
# endif
#else
# define USE_SSE4
# ifdef USE_SSE4
#  include <smmintrin.h>
# endif
#endif


#define COEF_BITS 12
#define COEF_MASK ((1<<COEF_BITS)-1)


// 两个行缓存. 最大值: 1920x4x4 = 30720
static uint8_t row_buf0[0x8000];
static uint8_t row_buf1[0x8000];

// 两个src缓存. 最大值: 1920x4 = 7680
// 仅用于palette格式的src
//static uint8_t src_buf0[0x2000];
//static uint8_t src_buf1[0x2000];

#if defined(USE_SSE4)
static uint8_t shufb_b2h[16] =
{
	0x00, 0x80, 0x04, 0x80, 0x01, 0x80, 0x05, 0x80,
	0x02, 0x80, 0x06, 0x80, 0x03, 0x80, 0x07, 0x80,
};
#endif

#if defined(USE_NEON)
static uint8_t shufb_b2h0[8] =
{
	0x00, 0x80, 0x01, 0x80, 0x02, 0x80, 0x03, 0x80,
};
static uint8_t shufb_b2h1[8] =
{
	0x04, 0x80, 0x05, 0x80, 0x06, 0x80, 0x07, 0x80,
};

#endif

static void m_resize_h(int count, uint32_t **rows, int dw, uint8_t **srows, int sw, int step_x)
{
	int x, fx, sx, cf0, cf1;
	uint8_t *src0, *src1, *s0, *s1;
	uint32_t *dst0, *dst1;

	fx = 0;
	if(sw*2==dw)
		fx = 1<<(COEF_BITS-1);
	
#if defined(USE_SSE4)
	__m128i mshufb = _mm_load_si128((__m128i*)shufb_b2h);
#endif
#if defined(USE_NEON)
	uint8x8_t mindex0 = vld1_u8(shufb_b2h0);
	uint8x8_t mindex1 = vld1_u8(shufb_b2h1);
#endif

	if(count==1){
		src1 = srows[1];
		dst1 = rows[1];

		for(x=0; x<dw; x++){
			cf1 = fx&COEF_MASK;
			cf0 = COEF_MASK-cf1;
			sx = fx>>COEF_BITS;
			s1 = src1+sx*4;

#if defined(USE_SSE4)
			__m128i ms1 = _mm_loadl_epi64((__m128i*)s1);
			__m128i mcf = _mm_set1_epi32((cf1<<16) | cf0);
			__m128i ms3 = _mm_shuffle_epi8(ms1, mshufb);
			*(__m128i*)dst1 = _mm_madd_epi16(ms3, mcf);
#elif defined(USE_NEON)
			uint16x4_t mcf0 = vld1_dup_u16((const uint16_t*)&cf0);
			uint16x4_t mcf1 = vld1_dup_u16((const uint16_t*)&cf1);

			uint64x2_t ms1 = vld1q_lane_u64((const uint64_t *)s1, ms1, 0);
			uint8x16_t ms1b = vreinterpretq_u8_u64(ms1);
			uint16x4_t ms3l = vreinterpret_u16_u8(vqtbl1_u8(ms1b, mindex0));
			uint16x4_t ms3h = vreinterpret_u16_u8(vqtbl1_u8(ms1b, mindex1));
			uint32x4_t m5 = vmull_u16(ms3l, mcf0);
			uint32x4_t m7 = vmlal_u16(m5, ms3h, mcf1);
			vst1q_u32(dst1, m7);
#else
			dst1[0] = s1[0]*cf0 + s1[4]*cf1;
			dst1[1] = s1[1]*cf0 + s1[5]*cf1;
			dst1[2] = s1[2]*cf0 + s1[6]*cf1;
			dst1[3] = s1[3]*cf0 + s1[7]*cf1;
#endif

			dst1 += 4;
			fx += step_x;
		}

	}else{
		src0 = srows[0];
		src1 = srows[1];
		dst0 = rows[0];
		dst1 = rows[1];

		for(x=0; x<dw; x++){
			cf1 = fx&COEF_MASK;
			cf0 = COEF_MASK-cf1;
			sx = fx>>COEF_BITS;
			s0 = src0+sx*4;
			s1 = src1+sx*4;

#if defined(USE_SSE4)
			__m128i mcf = _mm_set1_epi32((cf1<<16) | cf0);
			__m128i ms0 = _mm_loadl_epi64((__m128i*)s0);
			__m128i ms1 = _mm_loadl_epi64((__m128i*)s1);
			__m128i ms2 = _mm_shuffle_epi8(ms0, mshufb);
			__m128i ms3 = _mm_shuffle_epi8(ms1, mshufb);
			*(__m128i*)dst0 = _mm_madd_epi16(ms2, mcf);
			*(__m128i*)dst1 = _mm_madd_epi16(ms3, mcf);
#elif defined(USE_NEON)
			uint16x4_t mcf0 = vld1_dup_u16((const uint16_t*)&cf0);
			uint16x4_t mcf1 = vld1_dup_u16((const uint16_t*)&cf1);

			uint64x2_t ms0 = vld1q_lane_u64((const uint64_t *)s0, ms0, 0);
			uint8x16_t ms0b = vreinterpretq_u8_u64(ms0);
			uint16x4_t ms2l = vreinterpret_u16_u8(vqtbl1_u8(ms0b, mindex0));
			uint16x4_t ms2h = vreinterpret_u16_u8(vqtbl1_u8(ms0b, mindex1));
			uint32x4_t m4 = vmull_u16(ms2l, mcf0);
			uint32x4_t m6 = vmlal_u16(m4, ms2h, mcf1);
			vst1q_u32(dst1, m6);

			uint64x2_t ms1 = vld1q_lane_u64((const uint64_t *)s1, ms1, 0);
			uint8x16_t ms1b = vreinterpretq_u8_u64(ms1);
			uint16x4_t ms3l = vreinterpret_u16_u8(vqtbl1_u8(ms1b, mindex0));
			uint16x4_t ms3h = vreinterpret_u16_u8(vqtbl1_u8(ms1b, mindex1));
			uint32x4_t m5 = vmull_u16(ms3l, mcf0);
			uint32x4_t m7 = vmlal_u16(m5, ms3h, mcf1);
			vst1q_u32(dst1, m7);
#else
			dst0[0] = s0[0]*cf0 + s0[4]*cf1;
			dst0[1] = s0[1]*cf0 + s0[5]*cf1;
			dst0[2] = s0[2]*cf0 + s0[6]*cf1;
			dst0[3] = s0[3]*cf0 + s0[7]*cf1;

			dst1[0] = s1[0]*cf0 + s1[4]*cf1;
			dst1[1] = s1[1]*cf0 + s1[5]*cf1;
			dst1[2] = s1[2]*cf0 + s1[6]*cf1;
			dst1[3] = s1[3]*cf0 + s1[7]*cf1;
#endif
			dst0 += 4;
			dst1 += 4;
			fx += step_x;
		}

	}

}


#if defined(USE_SSE4) || defined(USE_NEON)
static uint8_t shufb_d2b[16] =
{
	0x03, 0x07, 0x0b, 0x0f, 0x80, 0x80, 0x80, 0x80,
	0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80, 0x80,
};
#endif

static void m_resize_v(uint8_t *dst, int dw, uint32_t **rows, int cf)
{
	uint32_t *s0 = rows[0];
	uint32_t *s1 = rows[1];
	int cf0 = COEF_MASK-cf;
	int cf1 = cf;
	int x;

#if defined(USE_SSE4)
	__m128i mcf0 = _mm_set1_epi32(cf0);
	__m128i mcf1 = _mm_set1_epi32(cf1);
	__m128i mshufb = _mm_load_si128((__m128i*)shufb_d2b);

	for(x=0; x<dw; x+=2){
		__m128i ms0 = _mm_load_si128((__m128i*)s0);
		__m128i ms1 = _mm_load_si128((__m128i*)s1);
		__m128i md00 = _mm_mullo_epi32(ms0, mcf0);
		__m128i md01 = _mm_mullo_epi32(ms1, mcf1);
		__m128i md0s = md00 + md01;
		__m128i md0r = _mm_shuffle_epi8(md0s, mshufb);
		*(uint32_t*)dst = _mm_cvtsi128_si32(md0r);
		s0 += 4;
		s1 += 4;
		dst += 4;

		__m128i ms2 = _mm_load_si128((__m128i*)s0);
		__m128i ms3 = _mm_load_si128((__m128i*)s1);
		__m128i md20 = _mm_mullo_epi32(ms2, mcf0);
		__m128i md21 = _mm_mullo_epi32(ms3, mcf1);
		__m128i md2s = md20 + md21;
		__m128i md2r = _mm_shuffle_epi8(md2s, mshufb);
		*(uint32_t*)dst = _mm_cvtsi128_si32(md2r);

		s0 += 4;
		s1 += 4;
		dst += 4;
	}
#elif defined(USE_NEON)
	uint32x4_t mcf0 = vld1q_dup_u32((const uint32_t*)&cf0);
	uint32x4_t mcf1 = vld1q_dup_u32((const uint32_t*)&cf1);
	uint8x16_t mindex = vld1q_u8(shufb_d2b);

	for(x=0; x<dw; x+=2){
		uint32x4_t ms0 = vld1q_u32(s0+0);
		uint32x4_t ms1 = vld1q_u32(s1+0);
		uint32x4_t ms2 = vld1q_u32(s0+4);
		uint32x4_t ms3 = vld1q_u32(s1+4);

		uint32x4_t md0s = vmulq_u32(ms0, mcf0) + vmulq_u32(ms1, mcf1);
		uint8x16_t md0b = vqtbl1q_u8(vreinterpretq_u8_u32(md0s), mindex);
		*(uint32_t*)(dst+0) = *(uint32_t*)&md0b;

		uint32x4_t md2s = vmulq_u32(ms2, mcf0) + vmulq_u32(ms3, mcf1);
		uint8x16_t md2b = vqtbl1q_u8(vreinterpretq_u8_u32(md2s), mindex);
		*(uint32_t*)(dst+4) = *(uint32_t*)&md2b;

		s0 += 8;
		s1 += 8;
		dst += 8;
	}
#else
	for(x=0; x<dw; x++){
		uint32_t d0, d1, d2, d3;
		d0 = s0[0]*cf0 + s1[0]*cf1;
		d1 = s0[1]*cf0 + s1[1]*cf1;
		d2 = s0[2]*cf0 + s1[2]*cf1;
		d3 = s0[3]*cf0 + s1[3]*cf1;

		dst[0] = d0>>(COEF_BITS*2);
		dst[1] = d1>>(COEF_BITS*2);
		dst[2] = d2>>(COEF_BITS*2);
		dst[3] = d3>>(COEF_BITS*2);

		s0 += 4;
		s1 += 4;
		dst += 4;
	}
#endif

}


void m_resize_rgba_c(uint8_t* dst, int dw, int dh, int dstride, uint8_t* src, int sw, int sh, int sstride)
{
	int y, fy, cf, step_y, step_x;
	int ky, k0, k1;
	uint32_t *rows[2];
	uint8_t *srows[2];

	step_y = ((sh-1)<<COEF_BITS)/(dh-1);
	step_x = ((sw-1)<<COEF_BITS)/(dw-1);

	fy = 0;
	if(sh*2==dh)
		fy = 1<<(COEF_BITS-1);

	k0 = -1; // src缓存行0
	k1 = -1; // src缓存行1
	// dst行在k0与k1之间插值产生
	rows[0] = (uint32_t*)row_buf0;
	rows[1] = (uint32_t*)row_buf1;


	for(y=0; y<dh; y++){
		cf = fy&COEF_MASK;
		ky = fy>>COEF_BITS; // 当前行对应的src行

		if(ky>k1){
			// 需要新的两个src缓存行
			k0 = ky;
			k1 = ky+1;
			srows[0] = src + k0*sstride;
			srows[1] = srows[0] + sstride;
			m_resize_h(2, rows, dw, srows, sw, step_x);
		}else if(ky==k1){
			// 需要一个新的缓存行: (k1, new) -> (k0, k1)
			uint32_t *tmp = rows[1];
			rows[1] = rows[0];
			rows[0] = tmp;
			k0 = k1;
			k1 = ky+1;
			srows[1] = src + k1*sstride;
			m_resize_h(1, rows, dw, srows, sw, step_x);
		}else{
			// 不需要更新缓存行
		}

		//printf("Line %3d: src=%3d  k0=%3d k1=%3d\n", y, ky, k0, k1);
		m_resize_v(dst, dw, rows, cf);

		dst += dstride;
		fy += step_y;  // 下一行
	}

}


