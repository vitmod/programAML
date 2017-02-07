/**********************************************************
* ��Ȩ���� (C)2005, ����������ͨѶ�ɷ����޹�˾
*
* �ļ����ƣ� v120.h
* �ļ���ʶ��
* ����ժҪ��

* ����˵����
* ��ǰ�汾��
* ��    �ߣ� Ǯ��
* ������ڣ�
*
* �޸ļ�¼1��
**********************************************************/
#ifndef __B120_H__
#define __B120_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "nano-X.h"
//#include "serv.h"

//extern	int (*NUL_Read)(MWKEY *kbuf, MWKEYMOD *modifiers, MWSCANCODE *scancode);
//#define EM10_SUCCESS 					0
#define  GE2D_BLIT_WITHOUTKEY		0x46f7
#define  GE2D_BLIT_NOALPHA		 0x4701
#define  GE2D_BLIT                       0x46ff
#define  GE2D_STRETCHBLIT       0x46fe
#define  GE2D_FILLRECTANGLE     0x46fd
#define  GE2D_SRCCOLORKEY       0x46fc
#define  FBIOPUT_OSD_SRCCOLORKEY                0x46fb
#define  FBIOPUT_OSD_SRCKEY_ENABLE      0x46fa
#define  GE2D_CONFIG			0x46f9
#define  FBIOPUT_OSD_SET_ALPHA          0x46f6

//ɫ��ģʽ���壺
#define FB_COLORMODE_565				0x01
#define FB_COLORMODE_24				0x02
#define FB_COLORMODE_32				0x03

#define MASKNONE						0xff0000ff	/* ������ɫ����-�������κ���ɫ */
#define MASKBLACK						0x00000000	/* ������ɫ����-���˺�ɫ */
#define MASKMAUVE						0x00F800F8	/* ������ɫ����-�����Ϻ�ɫ */

#define  aml_swap(x,y,t)     {(t)=(x);(x)=(y);(y)=(t);}

typedef enum
{
                OSD0_OSD0 =0,
                OSD0_OSD1,
                OSD1_OSD1,
                OSD1_OSD0,
                TYPE_INVALID,
}ge2d_src_dst_t;

typedef  struct{
        int     key_enable;
        int     key_color;
        int     key_mask;
	int  	key_mode;
}src_key_ctrl_t;

typedef    struct {
        int  op_type;
        int  src_dst_type;
        int  alu_const_color;
        src_key_ctrl_t  src_key;
}config_para_t;

typedef  struct {
        config_para_t  config;
        void  *pt_osd ;
}ge2d_op_ctl_t;

typedef struct {
     int            x;   /* X coordinate of its top-left point */
     int            y;   /* Y coordinate of its top-left point */
     int            w;   /* width of it */
     int            h;   /* height of it */
}rectangle_t;
typedef  struct {
        unsigned int   color ;
        rectangle_t src1_rect;
        rectangle_t src2_rect;
        rectangle_t     dst_rect;
        int op;
}ge2d_op_para_t ;

/* �������� */
int		MwFbInit(int nScrW, int nScrH, unsigned char bColorMode, void **pMwFb);
void	MwFbUninit(void);
int		MwFbRefreshRegn(MWRECT *ptRect, int nNum);
void	MwFbGetScrInfo(int *pnScrW, int *pnScrH, int *pnBpp, void **pMwFb);
void	MwFbGetLineLen(int *pnLineLen);
void	MwFbGetPixType(int *pbColorMode);
void	MwFbGetBufSize(int *pnSize);
int		MwFbSetTransparent(unsigned char bDevice, unsigned char bLayer, MWRECT *ptRect, MWTRANSPARENCE *ptTrans);

#ifdef __cplusplus
}
#endif

#endif



