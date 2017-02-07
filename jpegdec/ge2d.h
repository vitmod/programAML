/****************************************************************************
**
** Copyright (C) 2010 AMLOGIC, INC.
** All rights reserved.
****************************************************************************/

#ifndef GE2D_H
#define GE2D_H

#define   GE2D_STRETCHBLIT_NOALPHA   0x4702
#define   GE2D_BLIT_NOALPHA			0x4701
#define   GE2D_BLEND					0x4700
#define   GE2D_BLIT					0x46ff
#define   GE2D_STRETCHBLIT   		0x46fe
#define   GE2D_FILLRECTANGLE 		0x46fd
#define   GE2D_SRCCOLORKEY   		0x46fc
#define   OSD_SRCCOLORKEY			0x46fb
#define   OSD_SRCKEY_ENABLE			0x46fa
#define   GE2D_CONFIG				0x46f9

typedef enum  	
{
	OSD0_OSD0 = 0,
	OSD0_OSD1,	 
	OSD1_OSD1,
	OSD1_OSD0,
	ALLOC_OSD0,
	ALLOC_OSD1,
	ALLOC_ALLOC,
	TYPE_INVALID,
} ge2d_src_dst_t;



typedef struct {
	unsigned long  addr;
	unsigned int	w;
	unsigned int	h;
}config_planes_t;

typedef    struct {
	int  src_dst_type;
	int  alu_const_color;
	unsigned int src_format;
	unsigned int dst_format ; //add for src&dst all in user space.

	config_planes_t src_planes[4];
	config_planes_t dst_planes[4];
}config_para_t;

typedef struct {
	int	x;   /* X coordinate of its top-left point */
	int	y;   /* Y coordinate of its top-left point */
	int	w;   /* width of it */
	int	h;   /* height of it */
} rectangle_t;

typedef  struct {
	unsigned int color ;
	rectangle_t src1_rect;
	rectangle_t src2_rect;
	rectangle_t	dst_rect;
	int op;
} ge2d_op_para_t ;

#define GE2D_ENDIAN_SHIFT		24
#define GE2D_ENDIAN_MASK		(0x1 << GE2D_ENDIAN_SHIFT)
#define GE2D_BIG_ENDIAN			(0 << GE2D_ENDIAN_SHIFT)
#define GE2D_LITTLE_ENDIAN		(1 << GE2D_ENDIAN_SHIFT)

#define GE2D_COLOR_MAP_SHIFT	20
#define GE2D_COLOR_MAP_MASK		(0xf << GE2D_COLOR_MAP_SHIFT)
/* 16 bit */
#define GE2D_COLOR_MAP_YUV422	(0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGB655	(1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV655	(1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGB844	(2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV844	(2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA6442	(3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA6442	(3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA4444	(4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA4444	(4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGB565	(5 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV565	(5 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB4444	(6 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV4444	(6 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB1555	(7 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV1555	(7 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA4642	(8 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA4642	(8 << GE2D_COLOR_MAP_SHIFT)
/* 24 bit */
#define GE2D_COLOR_MAP_RGB888	(0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUV444	(0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA5658	(1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA5658	(1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB8565	(2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV8565	(2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_RGBA6666	(3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA6666	(3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB6666	(4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV6666	(4 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_BGR888	(5 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_VUY888	(5 << GE2D_COLOR_MAP_SHIFT)
/* 32 bit */
#define GE2D_COLOR_MAP_RGBA8888	(0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_YUVA8888	(0 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ARGB8888	(1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AYUV8888	(1 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_ABGR8888	(2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_AVUY8888	(2 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_BGRA8888	(3 << GE2D_COLOR_MAP_SHIFT)
#define GE2D_COLOR_MAP_VUYA8888	(3 << GE2D_COLOR_MAP_SHIFT)

#define GE2D_FMT_S8_Y			0x00000 /* 00_00_0_00_0_00 */
#define GE2D_FMT_S8_CB			0x00040 /* 00_01_0_00_0_00 */
#define GE2D_FMT_S8_CR			0x00080 /* 00_10_0_00_0_00 */
#define GE2D_FMT_S8_R			0x00000 /* 00_00_0_00_0_00 */
#define GE2D_FMT_S8_G			0x00040 /* 00_01_0_00_0_00 */
#define GE2D_FMT_S8_B			0x00080 /* 00_10_0_00_0_00 */
#define GE2D_FMT_S8_A			0x000c0 /* 00_11_0_00_0_00 */
#define GE2D_FMT_S8_LUT			0x00020 /* 00_00_1_00_0_00 */
#define GE2D_FMT_S16_YUV422		0x20100 /* 01_00_0_00_0_00 */
#define GE2D_FMT_S16_RGB		(GE2D_LITTLE_ENDIAN|0x00100) /* 01_00_0_00_0_00 */
#define GE2D_FMT_S24_YUV444		0x20200 /* 10_00_0_00_0_00 */
#define GE2D_FMT_S24_RGB		(GE2D_LITTLE_ENDIAN|0x00200) /* 10_00_0_00_0_00 */
#define GE2D_FMT_S32_YUVA444	0x20300 /* 11_00_0_00_0_00 */
#define GE2D_FMT_S32_RGBA		(GE2D_LITTLE_ENDIAN|0x00300) /* 11_00_0_00_0_00 */
#define GE2D_FMT_M24_YUV420		0x20007 /* 00_00_0_00_1_11 */
#define GE2D_FMT_M24_YUV422		0x20006 /* 00_00_0_00_1_10 */
#define GE2D_FMT_M24_YUV444		0x20004 /* 00_00_0_00_1_00 */
#define GE2D_FMT_M24_RGB		0x00004 /* 00_00_0_00_1_00 */
#define GE2D_FMT_M24_YUV420T	0x20017 /* 00_00_0_10_1_11 */
#define GE2D_FMT_M24_YUV420B	0x2001f /* 00_00_0_11_1_11 */
#define GE2D_FMT_S16_YUV422T	0x20110 /* 01_00_0_10_0_00 */
#define GE2D_FMT_S16_YUV422B	0x20138 /* 01_00_0_11_0_00 */
#define GE2D_FMT_S24_YUV444T	0x20210 /* 10_00_0_10_0_00 */
#define GE2D_FMT_S24_YUV444B	0x20218 /* 10_00_0_11_0_00 */

#define GE2D_FORMAT_S8_Y			(GE2D_FMT_S8_Y)           
#define GE2D_FORMAT_S8_CB			(GE2D_FMT_S8_CB)           
#define GE2D_FORMAT_S8_CR			(GE2D_FMT_S8_CR)           
#define GE2D_FORMAT_S8_R			(GE2D_FMT_S8_R)           
#define GE2D_FORMAT_S8_G			(GE2D_FMT_S8_G)           
#define GE2D_FORMAT_S8_B			(GE2D_FMT_S8_B)           
#define GE2D_FORMAT_S8_A			(GE2D_FMT_S8_A)           
#define GE2D_FORMAT_S8_LUT			(GE2D_FMT_S8_LUT)         
#define GE2D_FORMAT_S16_YUV422		(GE2D_FMT_S16_YUV422	| GE2D_COLOR_MAP_YUV422)
#define GE2D_FORMAT_S16_RGB_655		(GE2D_FMT_S16_RGB		| GE2D_COLOR_MAP_RGB655)  
#define GE2D_FORMAT_S16_RGB_565		(GE2D_FMT_S16_RGB		| GE2D_COLOR_MAP_RGB565) 
#define GE2D_FORMAT_S16_RGB_844		(GE2D_FMT_S16_RGB		| GE2D_COLOR_MAP_RGB844) 
#define GE2D_FORMAT_S16_RGBA_6442	(GE2D_FMT_S16_RGB		| GE2D_COLOR_MAP_RGBA6442)
#define GE2D_FORMAT_S16_RGBA_4444	(GE2D_FMT_S16_RGB		| GE2D_COLOR_MAP_RGBA4444)
#define GE2D_FORMAT_S16_ARGB_4444	(GE2D_FMT_S16_RGB		| GE2D_COLOR_MAP_ARGB4444)
#define GE2D_FORMAT_S16_ARGB_1555	(GE2D_FMT_S16_RGB		| GE2D_COLOR_MAP_ARGB1555)
#define GE2D_FORMAT_S16_RGBA_4642	(GE2D_FMT_S16_RGB		| GE2D_COLOR_MAP_RGBA4642)
#define GE2D_FORMAT_S24_YUV444		(GE2D_FMT_S24_YUV444	| GE2D_COLOR_MAP_YUV444) 
#define GE2D_FORMAT_S24_RGB			(GE2D_FMT_S24_RGB		| GE2D_COLOR_MAP_RGB888)   
#define GE2D_FORMAT_S32_YUVA444		(GE2D_FMT_S32_YUVA444	| GE2D_COLOR_MAP_YUVA4444)   
#define GE2D_FORMAT_S32_RGBA		(GE2D_FMT_S32_RGBA		| GE2D_COLOR_MAP_RGBA8888) 
#define GE2D_FORMAT_S32_ARGB		(GE2D_FMT_S32_RGBA		| GE2D_COLOR_MAP_ARGB8888) 
#define GE2D_FORMAT_S32_ABGR		(GE2D_FMT_S32_RGBA		| GE2D_COLOR_MAP_ABGR8888) 
#define GE2D_FORMAT_S32_BGRA		(GE2D_FMT_S32_RGBA		| GE2D_COLOR_MAP_BGRA8888) 
#define GE2D_FORMAT_S24_RGBA_5658	(GE2D_FMT_S24_RGB		| GE2D_COLOR_MAP_RGBA5658)  
#define GE2D_FORMAT_S24_ARGB_8565	(GE2D_FMT_S24_RGB		| GE2D_COLOR_MAP_ARGB8565) 
#define GE2D_FORMAT_S24_RGBA_6666	(GE2D_FMT_S24_RGB		| GE2D_COLOR_MAP_RGBA6666)
#define GE2D_FORMAT_S24_ARGB_6666	(GE2D_FMT_S24_RGB		| GE2D_COLOR_MAP_ARGB6666)
#define GE2D_FORMAT_S24_BGR			(GE2D_FMT_S24_RGB		| GE2D_COLOR_MAP_BGR888)
#define GE2D_FORMAT_M24_YUV420		(GE2D_FMT_M24_YUV420)   
#define GE2D_FORMAT_M24_YUV422		(GE2D_FMT_M24_YUV422)
#define GE2D_FORMAT_M24_YUV444		(GE2D_FMT_M24_YUV444)
#define GE2D_FORMAT_M24_RGB			(GE2D_FMT_M24_RGB)
#define GE2D_FORMAT_M24_YUV420T		(GE2D_FMT_M24_YUV420T)
#define GE2D_FORMAT_M24_YUV420B		(GE2D_FMT_M24_YUV420B)
#define GE2D_FORMAT_S16_YUV422T		(GE2D_FMT_S16_YUV422T | GE2D_COLOR_MAP_YUV422)
#define GE2D_FORMAT_S16_YUV422B		(GE2D_FMT_S16_YUV422B | GE2D_COLOR_MAP_YUV422)   
#define GE2D_FORMAT_S24_YUV444T		(GE2D_FMT_S24_YUV444T | GE2D_COLOR_MAP_YUV444)   
#define GE2D_FORMAT_S24_YUV444B		(GE2D_FMT_S24_YUV444B | GE2D_COLOR_MAP_YUV444)

#endif /* GE2D_H */
