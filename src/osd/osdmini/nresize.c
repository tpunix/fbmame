
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#define INTER_RESIZE_COEF_BITS  11
#define INTER_RESIZE_COEF_SCALE (1 << 11)
#define NE10_MAX_ESIZE          16

#define NE10_MIN(a,b) ((a)>(b)?(b):(a))
#define NE10_MAX(a,b) ((a)<(b)?(b):(a))

static inline uint32_t ne10_align_size (int32_t sz, int32_t n)
{
    return (sz + n - 1) & -n;
}

static inline int32_t ne10_floor (float a)
{
    return ( ( (a) >= 0) ? ( (int32_t) a) : ( (int32_t) a - 1));
}

static inline int32_t ne10_clip (int32_t x, int32_t a, int32_t b)
{
    return (x >= a ? (x < b ? x : b - 1) : a);
}

static inline uint8_t ne10_cast_op (int32_t val)
{
    int32_t bits = INTER_RESIZE_COEF_BITS * 2;
    int32_t SHIFT = bits;
    int32_t DELTA = 1 << (bits - 1) ;
    int32_t temp = NE10_MIN (255, NE10_MAX (0, (val + DELTA) >> SHIFT));
    return (uint8_t) (temp);
};

static void ne10_img_hresize_linear_c (const uint8_t** src,
                                       int32_t** dst,
                                       int32_t count,
                                       const int32_t* xofs,
                                       const int16_t* alpha,
                                       int32_t swidth,
                                       int32_t dwidth,
                                       int32_t cn,
                                       int32_t xmin,
                                       int32_t xmax)
{
    int32_t dx, k;

    int32_t dx0 = 0;

    //for (k = 0; k <= count - 2; k++)
    if (count == 2)
    {
        k = 0;
        const uint8_t *S0 = src[k], *S1 = src[k + 1];
        int32_t *D0 = dst[k], *D1 = dst[k + 1];
        for (dx = dx0; dx < xmax; dx++)
        {
            int32_t sx = xofs[dx];
            int32_t a0 = alpha[dx * 2], a1 = alpha[dx * 2 + 1];
            int32_t t0 = S0[sx] * a0 + S0[sx + cn] * a1;
            int32_t t1 = S1[sx] * a0 + S1[sx + cn] * a1;
            D0[dx] = t0;
            D1[dx] = t1;
        }

        for (; dx < dwidth; dx++)
        {
            int32_t sx = xofs[dx];
            D0[dx] = (int32_t) S0[sx] * INTER_RESIZE_COEF_SCALE;
            D1[dx] = (int32_t) S1[sx] * INTER_RESIZE_COEF_SCALE;
        }
    }

    //for (; k < count; k++)
    if (count == 1)
    {
        k = 0;
        const uint8_t *S = src[k];
        int32_t *D = dst[k];
        for (dx = 0; dx < xmax; dx++)
        {
            int32_t sx = xofs[dx];
            D[dx] = S[sx] * alpha[dx * 2] + S[sx + cn] * alpha[dx * 2 + 1];
        }

        for (; dx < dwidth; dx++)
            D[dx] = (int32_t) S[xofs[dx]] * INTER_RESIZE_COEF_SCALE;
    }
}


static void ne10_img_vresize_linear_c (const int32_t** src, uint8_t* dst, const int16_t* beta, int32_t width)
{
    int32_t b0 = beta[0], b1 = beta[1];
    const int32_t *S0 = src[0], *S1 = src[1];

    int32_t x = 0;
    for (; x <= width - 4; x += 4)
    {
        int32_t t0, t1;
        t0 = S0[x] * b0 + S1[x] * b1;
        t1 = S0[x + 1] * b0 + S1[x + 1] * b1;
        dst[x] = ne10_cast_op (t0);
        dst[x + 1] = ne10_cast_op (t1);
        t0 = S0[x + 2] * b0 + S1[x + 2] * b1;
        t1 = S0[x + 3] * b0 + S1[x + 3] * b1;
        dst[x + 2] = ne10_cast_op (t0);
        dst[x + 3] = ne10_cast_op (t1);
    }

    for (; x < width; x++)
        dst[x] = ne10_cast_op (S0[x] * b0 + S1[x] * b1);
}


static uint8_t resize_buf2[0x10000];

static void ne10_img_resize_generic_linear_c (uint8_t* src,
        uint8_t* dst,
        const int32_t* xofs,
        const int16_t* _alpha,
        const int32_t* yofs,
        const int16_t* _beta,
        int32_t xmin,
        int32_t xmax,
        int32_t ksize,
        int32_t srcw,
        int32_t srch,
        int32_t srcstep,
        int32_t dstw,
        int32_t dsth,
        int32_t dststep,
        int32_t channels)
{

    const int16_t* alpha = _alpha;
    const int16_t* beta = _beta;
    int32_t cn = channels;
    srcw *= cn;
    dstw *= cn;

    int32_t bufstep = (int32_t) ne10_align_size (dstw, 16);


    int32_t *buffer_ = (int32_t*) resize_buf2;

    const uint8_t* srows[NE10_MAX_ESIZE];
    int32_t* rows[NE10_MAX_ESIZE];
    int32_t prev_sy[NE10_MAX_ESIZE];
    int32_t k, dy;
    xmin *= cn;
    xmax *= cn;

    for (k = 0; k < ksize; k++)
    {
        prev_sy[k] = -1;
        rows[k] = (int32_t*) buffer_ + bufstep * k;
    }

    // image resize is a separable operation. In case of not too strong
    for (dy = 0; dy < dsth; dy++, beta += ksize) {
        int32_t sy0 = yofs[dy], k, k0 = ksize, k1 = 0;

        for (k = 0; k < ksize; k++) {
            int32_t sy = ne10_clip (sy0 + k, 0, srch);
            for (k1 = NE10_MAX (k1, k); k1 < ksize; k1++) {
                if (sy == prev_sy[k1])  // if the sy-th row has been computed already, reuse it.
                {
                    if (k1 > k)
                        memcpy (rows[k], rows[k1], bufstep * sizeof (rows[0][0]));
                    break;
                }
            }
            if (k1 == ksize)
                k0 = NE10_MIN (k0, k); // remember the first row that needs to be computed
            srows[k] = (const uint8_t*) (src + srcstep * sy);
            prev_sy[k] = sy;
        }

        if (k0 < ksize)
            ne10_img_hresize_linear_c (srows + k0, rows + k0, ksize - k0, xofs, alpha,
                                       srcw, dstw, cn, xmin, xmax);

        ne10_img_vresize_linear_c ( (const int32_t**) rows, (uint8_t*) (dst + dststep * dy), beta, dstw);
    }

}

static void ne10_img_resize_cal_offset_linear (int32_t* xofs,
        int16_t* ialpha,
        int32_t* yofs,
        int16_t* ibeta,
        int32_t *xmin,
        int32_t *xmax,
        int32_t ksize,
        int32_t ksize2,
        int32_t srcw,
        int32_t srch,
        int32_t dstw,
        int32_t dsth,
        int32_t channels)
{
    float inv_scale_x = (float) dstw / srcw;
    float inv_scale_y = (float) dsth / srch;

    int32_t cn = channels;
    float scale_x = 1. / inv_scale_x;
    float scale_y = 1. / inv_scale_y;
    int32_t k, sx, sy, dx, dy;


    float fx, fy;

    float cbuf[NE10_MAX_ESIZE];

    for (dx = 0; dx < dstw; dx++)
    {
        fx = (float) ( (dx + 0.5) * scale_x - 0.5);
        sx = ne10_floor (fx);
        fx -= sx;

        if (sx < ksize2 - 1)
        {
            *xmin = dx + 1;
            if (sx < 0)
                fx = 0, sx = 0;
        }

        if (sx + ksize2 >= srcw)
        {
            *xmax = NE10_MIN (*xmax, dx);
            if (sx >= srcw - 1)
                fx = 0, sx = srcw - 1;
        }

        for (k = 0, sx *= cn; k < cn; k++)
            xofs[dx * cn + k] = sx + k;

        cbuf[0] = 1.f - fx;
        cbuf[1] = fx;

        for (k = 0; k < ksize; k++)
            ialpha[dx * cn * ksize + k] = (int16_t) (cbuf[k] * INTER_RESIZE_COEF_SCALE);
        for (; k < cn * ksize; k++)
            ialpha[dx * cn * ksize + k] = ialpha[dx * cn * ksize + k - ksize];
    }

    for (dy = 0; dy < dsth; dy++)
    {
        fy = (float) ( (dy + 0.5) * scale_y - 0.5);
        sy = ne10_floor (fy);
        fy -= sy;

        yofs[dy] = sy;

        cbuf[0] = 1.f - fy;
        cbuf[1] = fy;

        for (k = 0; k < ksize; k++)
            ibeta[dy * ksize + k] = (int16_t) (cbuf[k] * INTER_RESIZE_COEF_SCALE);

    }

}


static uint8_t resize_buf1[0x20000];
void ne10_img_resize_bilinear_rgba_c (uint8_t* dst,
                                      uint32_t dst_width,
                                      uint32_t dst_height,
                                      uint32_t dst_stride,
                                      uint8_t* src,
                                      uint32_t src_width,
                                      uint32_t src_height,
                                      uint32_t src_stride)
{
    int32_t dstw = dst_width;
    int32_t dsth = dst_height;
    int32_t srcw = src_width;
    int32_t srch = src_height;

    int32_t cn = 4;

    int32_t xmin = 0;
    int32_t xmax = dstw;
    int32_t width = dstw * cn;

    int32_t ksize = 0, ksize2;
    ksize = 2;
    ksize2 = ksize / 2;

    int32_t* xofs = (int32_t*) resize_buf1;
    int32_t* yofs = xofs + width;
    int16_t* ialpha = (int16_t*) (yofs + dsth);
    int16_t* ibeta = ialpha + width * ksize;

    ne10_img_resize_cal_offset_linear (xofs, ialpha, yofs, ibeta, &xmin, &xmax, ksize, ksize2, srcw, srch, dstw, dsth, cn);

    ne10_img_resize_generic_linear_c (src, dst, xofs, ialpha, yofs, ibeta, xmin, xmax, ksize, srcw, srch, src_stride, dstw, dsth, dst_stride, cn);
}


