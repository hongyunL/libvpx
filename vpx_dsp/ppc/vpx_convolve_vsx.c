/*
 *  Copyright (c) 2017 The WebM project authors. All Rights Reserved.
 *
 *  Use of this source code is governed by a BSD-style license
 *  that can be found in the LICENSE file in the root of the source
 *  tree. An additional intellectual property rights grant can be found
 *  in the file PATENTS.  All contributing project authors may
 *  be found in the AUTHORS file in the root of the source tree.
 */
#include <string.h>
#include "./vpx_dsp_rtcd.h"
#include "vpx_dsp/vpx_filter.h"
#include "vpx_dsp/ppc/types_vsx.h"

// TODO(lu_zero): unroll
static inline void copy_w16(const uint8_t *src, ptrdiff_t src_stride,
                            uint8_t *dst, ptrdiff_t dst_stride, int32_t h) {
  int i;

  for (i = h; i--;) {
    vec_vsx_st(vec_vsx_ld(0, src), 0, dst);
    src += src_stride;
    dst += dst_stride;
  }
}

static inline void copy_w32(const uint8_t *src, ptrdiff_t src_stride,
                            uint8_t *dst, ptrdiff_t dst_stride, int32_t h) {
  int i;

  for (i = h; i--;) {
    vec_vsx_st(vec_vsx_ld(0, src), 0, dst);
    vec_vsx_st(vec_vsx_ld(16, src), 16, dst);
    src += src_stride;
    dst += dst_stride;
  }
}

static inline void copy_w64(const uint8_t *src, ptrdiff_t src_stride,
                            uint8_t *dst, ptrdiff_t dst_stride, int32_t h) {
  int i;

  for (i = h; i--;) {
    vec_vsx_st(vec_vsx_ld(0, src), 0, dst);
    vec_vsx_st(vec_vsx_ld(16, src), 16, dst);
    vec_vsx_st(vec_vsx_ld(32, src), 32, dst);
    vec_vsx_st(vec_vsx_ld(48, src), 48, dst);
    src += src_stride;
    dst += dst_stride;
  }
}

void vpx_convolve_copy_vsx(const uint8_t *src, ptrdiff_t src_stride,
                           uint8_t *dst, ptrdiff_t dst_stride,
                           const int16_t *filter_x, int32_t filter_x_stride,
                           const int16_t *filter_y, int32_t filter_y_stride,
                           int32_t w, int32_t h) {
  (void)filter_x;
  (void)filter_y;
  (void)filter_x_stride;
  (void)filter_y_stride;

  switch (w) {
    case 16: {
      copy_w16(src, src_stride, dst, dst_stride, h);
      break;
    }
    case 32: {
      copy_w32(src, src_stride, dst, dst_stride, h);
      break;
    }
    case 64: {
      copy_w64(src, src_stride, dst, dst_stride, h);
      break;
    }
    default: {
      int i;
      for (i = h; i--;) {
        memcpy(dst, src, w);
        src += src_stride;
        dst += dst_stride;
      }
      break;
    }
  }
}

static inline void avg_w16(const uint8_t *src, ptrdiff_t src_stride,
                           uint8_t *dst, ptrdiff_t dst_stride, int32_t h) {
  int i;

  for (i = h; i--;) {
    const uint8x16_t v = vec_avg(vec_vsx_ld(0, src), vec_vsx_ld(0, dst));
    vec_vsx_st(v, 0, dst);
    src += src_stride;
    dst += dst_stride;
  }
}

static inline void avg_w32(const uint8_t *src, ptrdiff_t src_stride,
                           uint8_t *dst, ptrdiff_t dst_stride, int32_t h) {
  int i;

  for (i = h; i--;) {
    const uint8x16_t v0 = vec_avg(vec_vsx_ld(0, src), vec_vsx_ld(0, dst));
    const uint8x16_t v1 = vec_avg(vec_vsx_ld(16, src), vec_vsx_ld(16, dst));
    vec_vsx_st(v0, 0, dst);
    vec_vsx_st(v1, 16, dst);
    src += src_stride;
    dst += dst_stride;
  }
}

static inline void avg_w64(const uint8_t *src, ptrdiff_t src_stride,
                           uint8_t *dst, ptrdiff_t dst_stride, int32_t h) {
  int i;

  for (i = h; i--;) {
    const uint8x16_t v0 = vec_avg(vec_vsx_ld(0, src), vec_vsx_ld(0, dst));
    const uint8x16_t v1 = vec_avg(vec_vsx_ld(16, src), vec_vsx_ld(16, dst));
    const uint8x16_t v2 = vec_avg(vec_vsx_ld(32, src), vec_vsx_ld(32, dst));
    const uint8x16_t v3 = vec_avg(vec_vsx_ld(48, src), vec_vsx_ld(48, dst));
    vec_vsx_st(v0, 0, dst);
    vec_vsx_st(v1, 16, dst);
    vec_vsx_st(v2, 32, dst);
    vec_vsx_st(v3, 48, dst);
    src += src_stride;
    dst += dst_stride;
  }
}

void vpx_convolve_avg_vsx(const uint8_t *src, ptrdiff_t src_stride,
                          uint8_t *dst, ptrdiff_t dst_stride,
                          const int16_t *filter_x, int32_t filter_x_stride,
                          const int16_t *filter_y, int32_t filter_y_stride,
                          int32_t w, int32_t h) {
  (void)filter_x;
  (void)filter_y;
  (void)filter_x_stride;
  (void)filter_y_stride;

  switch (w) {
    case 16: {
      avg_w16(src, src_stride, dst, dst_stride, h);
      break;
    }
    case 32: {
      avg_w32(src, src_stride, dst, dst_stride, h);
      break;
    }
    case 64: {
      avg_w64(src, src_stride, dst, dst_stride, h);
      break;
    }
    default: {
      vpx_convolve_avg_c(src, src_stride, dst, dst_stride, filter_x,
                         filter_x_stride, filter_y, filter_y_stride, w, h);
      break;
    }
  }
}

static inline void convolve_line(uint8_t *dst, const uint8_t *const src_x,
                                 const int16_t *const x_filter) {
  const int16x8_t s = unpack_to_s16_h(vec_vsx_ld(0, src_x));
  const int16x8_t f = vec_vsx_ld(0, x_filter);
  const int32x4_t sum = vec_msum(s, f, vec_splat_s32(0));
  const int32x4_t bias =
      vec_sl(vec_splat_s32(1), vec_splat_u32(FILTER_BITS - 1));
  const int32x4_t avg = vec_sr(vec_sums(sum, bias), vec_splat_u32(FILTER_BITS));
  const uint8x16_t v = vec_splat(
      vec_packsu(vec_pack(avg, vec_splat_s32(0)), vec_splat_s16(0)), 3);
  vec_ste(v, 0, dst);
}

// TODO(lu_zero): Implement 8x8 and bigger block special cases
static inline void convolve_horiz(const uint8_t *src, ptrdiff_t src_stride,
                                  uint8_t *dst, ptrdiff_t dst_stride,
                                  const InterpKernel *x_filters, int x0_q4,
                                  int x_step_q4, int w, int h) {
  int x, y;
  src -= SUBPEL_TAPS / 2 - 1;

  for (y = 0; y < h; ++y) {
    int x_q4 = x0_q4;
    for (x = 0; x < w; ++x) {
      convolve_line(dst + x, &src[x_q4 >> SUBPEL_BITS],
                    x_filters[x_q4 & SUBPEL_MASK]);
      x_q4 += x_step_q4;
    }
    src += src_stride;
    dst += dst_stride;
  }
}

static inline void convolve_avg_horiz(const uint8_t *src, ptrdiff_t src_stride,
                                      uint8_t *dst, ptrdiff_t dst_stride,
                                      const InterpKernel *x_filters, int x0_q4,
                                      int x_step_q4, int w, int h) {
  int x, y;
  src -= SUBPEL_TAPS / 2 - 1;

  for (y = 0; y < h; ++y) {
    int x_q4 = x0_q4;
    for (x = 0; x < w; ++x) {
      uint8_t v;
      convolve_line(&v, &src[x_q4 >> SUBPEL_BITS],
                    x_filters[x_q4 & SUBPEL_MASK]);
      dst[x] = ROUND_POWER_OF_TWO(dst[x] + v, 1);
      x_q4 += x_step_q4;
    }
    src += src_stride;
    dst += dst_stride;
  }
}

void vpx_convolve8_horiz_vsx(const uint8_t *src, ptrdiff_t src_stride,
                             uint8_t *dst, ptrdiff_t dst_stride,
                             const int16_t *filter_x, int x_step_q4,
                             const int16_t *filter_y, int y_step_q4, int w,
                             int h) {
  const InterpKernel *const filters_x = get_filter_base(filter_x);
  const int x0_q4 = get_filter_offset(filter_x, filters_x);

  (void)filter_y;
  (void)y_step_q4;

  convolve_horiz(src, src_stride, dst, dst_stride, filters_x, x0_q4, x_step_q4,
                 w, h);
}

void vpx_convolve8_avg_horiz_vsx(const uint8_t *src, ptrdiff_t src_stride,
                                 uint8_t *dst, ptrdiff_t dst_stride,
                                 const int16_t *filter_x, int x_step_q4,
                                 const int16_t *filter_y, int y_step_q4, int w,
                                 int h) {
  const InterpKernel *const filters_x = get_filter_base(filter_x);
  const int x0_q4 = get_filter_offset(filter_x, filters_x);

  (void)filter_y;
  (void)y_step_q4;

  convolve_avg_horiz(src, src_stride, dst, dst_stride, filters_x, x0_q4,
                     x_step_q4, w, h);
}
