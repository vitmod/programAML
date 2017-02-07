/**************************************************************************
* ��Ȩ���� (C)2001, ����������ͨѶ�ɷ����޹�˾��
*
* �ļ����ƣ�AmlogicFrame.c
* �ļ���ʶ��
* ����ժҪ��ʵ��Amlogic��MicroWindows֡��������
* ����˵����
* ��ǰ�汾��
* ��    �ߣ�������
* ������ڣ�2009.07.23
*
* �޸ļ�¼1	��
* 	�޸����ڣ�
* 	�� �� �ţ�
* 	�� �� �ˣ�
* 	�޸����ݣ�
***************************************************************************/
/**************************************************************************
*                         ͷ�ļ�����                                     *
**************************************************************************/
//#include <zmem.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/fb.h>
#include <sys/mman.h>
#include <sys/ipc.h>
#include <semaphore.h>
#include <time.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <sys/stat.h>
#include "Amlogic.h"
#include "MwFb.h"
#include "defines.h"
#include "stgfb.h"
#include "MwFbDriver.h"

/**************************************************************************
*                           	�궨��                                  *
**************************************************************************/
#define FBDEVNAME	"/dev/fb0"	//framebuffer�豸�Ľڵ�����
#define LOGODEVNAME	"/dev/fb1"	//framebuffer�豸�Ľڵ�����
#define LOGOFILENAME "/proc/logo_end"
#define USE_OFFSET_FUNC 

/**************************************************************************
*                           	�ṹ�嶨��                                  *
**************************************************************************/
typedef enum
{	
	SD_EPG = 1,//����
	HD_EPG, //����
	XD_EPG //����
}PAGE_TYPE_t;

typedef struct
{
	UCHAR *	m_pucFrameBuffer;//Framebuffer����ַ(���ڻ���MW ����)
	UCHAR *	m_pucFrameBufferMMAP;//Framebuffer���ִ�ӳ�䵽�û��ռ�Ļ���ַ
	CHAR			m_PixelType;//��������
	INT32			m_FBDevWidth;//Framebuffer�豸�Ŀ��
	INT32			m_FBDevHeight;//Framebuffer�豸�ĸ߶�
	INT32			m_FBDevColorDepth;//Framebuffer�豸����ɫ���
	INT32			m_iFBBufSize;//Framebuffer�Ļ�������С
	INT32			m_MWWidth;//microwindows�Ĵ��ڿ��
	INT32			m_MWHeight;//microwindows�Ĵ��ڸ߶�
	INT32			m_iMWBufSize;//microwindows�Ļ�������С
	INT32			m_iLeftOffset;//Framebuffer�豸�����ƫ��
	INT32			m_iTopOffset;//Framebuffer�豸�Ķ���ƫ��
	INT32			m_iFbDevFD;//Framebuffer�豸���
	INT32			m_iGe2dFD;//ge2d�豸���
	INT32			m_iLineLength;//Framebuffer map��ƫ����
	INT32			m_iExtraLeftOffset;//framebuffer�������ƫ�������������ļ�����
	INT32			m_iExtraTopOffset;//framebuffer�������ƫ�������������ļ�����
}T_FBAdaptor;

/**************************************************************************
*                          ȫ�ֱ���                                      *
**************************************************************************/
static 	T_FBAdaptor	g_AmlFBAdaptor = {NULL,NULL,FB_COLORMODE_565,720,576,2,0,640,526,0,0,0,-1,-1,0,0,0};
//static 	int 	 		g_fbstatus		= SD_EPG;	/* reference to struct PAGE_TYPE_t*/
static	int			g_bBlend		= 1; /* �Ƿ�����͸������ */

int                 g_nFbDevNum = 1;
T_MwDriverState     m_tMwDrvState;
int 	m_iSleep = 0;

#define  FBIOPUT_OSD_ORDER_SET  		0x46f8

//if the top offset value is 21, it should be converted to 10
#define ADJUST_TOP_NUMERATOR		  10
#define ADJUST_TOP_DENOMINATOR	  21
//if the left offset value is 38, it should be converted to 33
#define ADJUST_LEFT_NUMERATOR		35
#define ADJUST_LEFT_DENOMINATOR	38

#define LAYER_GUI		0
#define LAYER_TVMS		1

/**************************************************************************
*                        �ֲ�����ԭ��                                    *
**************************************************************************/
INT32		MwKbSleep(unsigned char bOption);
static INT32 	FB_SetLayer(INT32 layer, INT32 ctrl);
static INT32 	FB_SetAlpha(INT32 layer, INT32 alpha);
static INT32 	FB_SetColorKey(INT32 layer, INT32 colorKey,INT32 keymode);
static INT32 	Region_Check(INT32 nLeft, INT32 nTop, INT32 nWidth, INT32 nHeight, INT32 *pnX, INT32 *pnY, INT32 *pnW, INT32 *pnH);
static INT32	FB_EraseRect(INT32 layer, INT32 nType, INT32 nLeft, INT32 nTop, INT32 nWidth, INT32 nHeight);
static INT32	FB_RefreshRect(unsigned char bLayer, INT32 nLeft, INT32 nTop, INT32 nWidth, INT32 nHeight);
static INT32  FB_SetColorKeyEx(INT32 layer, INT32  enable,INT32 colorKey,INT32  keymode);
//void MwFbSetScreenOffset(int x,int y);
void MwFbRefreshBuf(MWRECT* pRect,void *pBuf);
void SetScreanInfo(int iOsdType, char* val);
void FillFBWithBlack();

#include        <sys/time.h>
int             BrcmFbTiming(void)
{
	int     iResult;
	struct timeval  tTime;

	gettimeofday(&tTime, NULL);
	iResult = tTime.tv_sec * 1000 + tTime.tv_usec / 1000;

	return  iResult;
}
/**************************************************************************************/
/*add by wjf,fb0 and fb1 sync mechamism */
union semun {
	int val;
	struct semid_ds *buf;
	unsigned short *array;
};
static const char* g_osd_semid="osd_sem";
static sem_t  * g_osd_sem;


static void  osd_set_screen_offset(T_FBAdaptor * pFB) 
{
#define	SCREEN_AXIS_FILE "/sys/class/display/axis"
	char  cmd[30];
	int fd=-1;

	memset(cmd,0,30);
	sprintf(cmd,"%d,%d,%d,%d ",pFB->m_iExtraLeftOffset,pFB->m_iExtraTopOffset,pFB->m_FBDevWidth,pFB->m_FBDevHeight);

	fd=open(SCREEN_AXIS_FILE, O_RDWR);
	if(fd<0)
	{
		printf("can't open file %s\n",SCREEN_AXIS_FILE);
		return ;
	}

	write(fd,cmd,strlen(cmd));

	close(fd);
}
static sem_t  * osd_sem_creat(const char* sem_id)
{
	static sem_t*  sem=NULL;
        int  ret;
	/*
	if(0>shm_unlink(sem_id))
	{
	    printf("%s\n",strerror(errno));
	}*/
       
	return &sem;
}
static  void osd_del_sem(sem_t  *sem)
{
	sem_destroy(sem);
}
static INT32 osd_sem_lock(sem_t  *sem)
{
       struct timespec ts;
       int sem_value;
       return 0;
       clock_gettime(CLOCK_REALTIME, &ts);
       ts.tv_nsec +=10000;//wait 10us
	if((-1==sem_timedwait(&sem,&ts))&&(errno == ETIMEDOUT))
        {
            printf("timeout unable to lock osd sem \n");
            do{
                 sem_post(sem);
                 if(0> sem_getvalue(sem,&sem_value))
                 {
                    printf("error occured when read sem value\n");
                    break;
                 }
                 printf("release  %d \n",sem_value+1);
            }while(sem_value<0) ;//exclude sem_value==0,because we will lock it before we exit this funciton.
            return -1;
        }
        return 0;
}
static INT32 osd_sem_unlock(sem_t  *sem)
{
    int ret;
    return 0;
    printf("sem addr %p\n",sem);
    ret=sem_post(sem);
    printf("ret value:%d\n",ret);
    if(0>ret)
    printf("%s\n",strerror(errno));
    return ret;
}


/**************************************************************************************/

/**************************************************************************
*                        �ֲ�����ʵ��                                    *
**************************************************************************/
#define   FBIOPUT_GE2D_BLEND			 0x4700

/**blend op relative macro*/
#define OPERATION_ADD           0    //Cd = Cs*Fs+Cd*Fd
#define OPERATION_SUB           1    //Cd = Cs*Fs-Cd*Fd
#define OPERATION_REVERSE_SUB   2    //Cd = Cd*Fd-Cs*Fs
#define OPERATION_MIN           3    //Cd = Min(Cd*Fd,Cs*Fs)
#define OPERATION_MAX           4    //Cd = Max(Cd*Fd,Cs*Fs)
#define OPERATION_LOGIC         5

#define COLOR_FACTOR_ZERO                     0
#define COLOR_FACTOR_ONE                      1
#define COLOR_FACTOR_SRC_COLOR                2
#define COLOR_FACTOR_ONE_MINUS_SRC_COLOR      3
#define COLOR_FACTOR_DST_COLOR                4
#define COLOR_FACTOR_ONE_MINUS_DST_COLOR      5
#define COLOR_FACTOR_SRC_ALPHA                6
#define COLOR_FACTOR_ONE_MINUS_SRC_ALPHA      7
#define COLOR_FACTOR_DST_ALPHA                8
#define COLOR_FACTOR_ONE_MINUS_DST_ALPHA      9
#define COLOR_FACTOR_CONST_COLOR              10
#define COLOR_FACTOR_ONE_MINUS_CONST_COLOR    11
#define COLOR_FACTOR_CONST_ALPHA              12
#define COLOR_FACTOR_ONE_MINUS_CONST_ALPHA    13
#define COLOR_FACTOR_SRC_ALPHA_SATURATE       14

#define ALPHA_FACTOR_ZERO                     0
#define ALPHA_FACTOR_ONE                      1
#define ALPHA_FACTOR_SRC_ALPHA                2
#define ALPHA_FACTOR_ONE_MINUS_SRC_ALPHA      3
#define ALPHA_FACTOR_DST_ALPHA                4
#define ALPHA_FACTOR_ONE_MINUS_DST_ALPHA      5
#define ALPHA_FACTOR_CONST_ALPHA              6
#define ALPHA_FACTOR_ONE_MINUS_CONST_ALPHA    7

#define LOGIC_OPERATION_CLEAR       0
#define LOGIC_OPERATION_COPY        1
#define LOGIC_OPERATION_NOOP        2
#define LOGIC_OPERATION_SET         3
#define LOGIC_OPERATION_COPY_INVERT 4
#define LOGIC_OPERATION_INVERT      5
#define LOGIC_OPERATION_AND_REVERSE 6
#define LOGIC_OPERATION_OR_REVERSE  7
#define LOGIC_OPERATION_AND         8
#define LOGIC_OPERATION_OR          9
#define LOGIC_OPERATION_NAND        10
#define LOGIC_OPERATION_NOR         11
#define LOGIC_OPERATION_XOR         12
#define LOGIC_OPERATION_EQUIV       13
#define LOGIC_OPERATION_AND_INVERT  14
#define LOGIC_OPERATION_OR_INVERT   15 

#define BLENDOP_ADD           0    //Cd = Cs*Fs+Cd*Fd
#define BLENDOP_SUB           1    //Cd = Cs*Fs-Cd*Fd
#define BLENDOP_REVERSE_SUB   2    //Cd = Cd*Fd-Cs*Fs
#define BLENDOP_MIN           3    //Cd = Min(Cd*Fd,Cs*Fs)
#define BLENDOP_MAX           4    //Cd = Max(Cd*Fd,Cs*Fs)
#define BLENDOP_LOGIC         5    //Cd = Cs op Cd
#define BLENDOP_LOGIC_CLEAR       (BLENDOP_LOGIC+0 )
#define BLENDOP_LOGIC_COPY        (BLENDOP_LOGIC+1 )
#define BLENDOP_LOGIC_NOOP        (BLENDOP_LOGIC+2 )
#define BLENDOP_LOGIC_SET         (BLENDOP_LOGIC+3 )
#define BLENDOP_LOGIC_COPY_INVERT (BLENDOP_LOGIC+4 )
#define BLENDOP_LOGIC_INVERT      (BLENDOP_LOGIC+5 )
#define BLENDOP_LOGIC_AND_REVERSE (BLENDOP_LOGIC+6 )
#define BLENDOP_LOGIC_OR_REVERSE  (BLENDOP_LOGIC+7 )
#define BLENDOP_LOGIC_AND         (BLENDOP_LOGIC+8 )
#define BLENDOP_LOGIC_OR          (BLENDOP_LOGIC+9 )
#define BLENDOP_LOGIC_NAND        (BLENDOP_LOGIC+10)
#define BLENDOP_LOGIC_NOR         (BLENDOP_LOGIC+11)
#define BLENDOP_LOGIC_XOR         (BLENDOP_LOGIC+12)
#define BLENDOP_LOGIC_EQUIV       (BLENDOP_LOGIC+13)
#define BLENDOP_LOGIC_AND_INVERT  (BLENDOP_LOGIC+14)
#define BLENDOP_LOGIC_OR_INVERT   (BLENDOP_LOGIC+15) 


static inline unsigned blendop(unsigned color_blending_mode,
                               unsigned color_blending_src_factor,
                               unsigned color_blending_dst_factor,
                               unsigned alpha_blending_mode,
                               unsigned alpha_blending_src_factor,
                               unsigned alpha_blending_dst_factor)
{
    return (color_blending_mode << 24) |
           (color_blending_src_factor << 20) |
           (color_blending_dst_factor << 16) |
           (alpha_blending_mode << 8) |
           (alpha_blending_src_factor << 4) |
           (alpha_blending_dst_factor << 0);
}

static INT32 osdBlend(U16 x0,U16 y0,U16 x1,U16 y1)
{
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;	
	config_para_t ge2d_config;	
	ge2d_op_para_t  op_para;


     	ge2d_config.op_type=OSD0_OSD0;
	ge2d_config.alu_const_color= 0x800000F0;//0x80000080; // BGRA  ALPHA= 50%
	ioctl(pFB->m_iGe2dFD, FBIOPUT_GE2D_CONFIG, &ge2d_config) ;
	op_para.src1_rect.x= x0;
	op_para.src1_rect.y= y0 + pFB->m_FBDevHeight * 2;
	op_para.src1_rect.w= x1 - x0 + 1;
	op_para.src1_rect.h= y1 - y0 + 1;
	
	op_para.src2_rect.x= x0;
	op_para.src2_rect.y= y0 + pFB->m_FBDevHeight * 3;
	op_para.src2_rect.w= x1 - x0 + 1;
	op_para.src2_rect.h= y1 - y0 + 1;
	
	op_para.dst_rect.x= x0;
	op_para.dst_rect.y= y0 + pFB->m_FBDevHeight;
	op_para.dst_rect.w= x1 - x0 + 1;
	op_para.dst_rect.h= y1 - y0 + 1;
	
	//op_para.op = blendop( 
	//		OPERATION_ADD,
			/* color blending src factor */
	//		COLOR_FACTOR_ONE_MINUS_SRC_ALPHA,
			/* color blending dst factor */
	//		COLOR_FACTOR_SRC_ALPHA,
			/* alpha blending_mode */
	//		OPERATION_ADD,
			/* alpha blending src factor */
			//ALPHA_FACTOR_SRC_ALPHA ,
	//		ALPHA_FACTOR_ZERO,
			/* color blending dst factor */
	//		ALPHA_FACTOR_ONE/*ALPHA_FACTOR_ONE_MINUS_SRC_ALPHA*/);
	op_para.op = blendop( 
			OPERATION_ADD,
			/* color blending src factor */
			COLOR_FACTOR_ONE_MINUS_DST_ALPHA,
			/* color blending dst factor */
			COLOR_FACTOR_DST_ALPHA,
			/* alpha blending_mode */
			OPERATION_ADD,
			/* alpha blending src factor */
			//ALPHA_FACTOR_SRC_ALPHA ,
			ALPHA_FACTOR_ZERO,
			/* color blending dst factor */
			ALPHA_FACTOR_ONE/*ALPHA_FACTOR_ONE_MINUS_SRC_ALPHA*/);//src2_key
			
	ioctl(pFB->m_iGe2dFD, FBIOPUT_GE2D_BLEND, &op_para) ;
     	return 0;
}
static INT32  osdUpdateRegion(U16 x0,U16 y0,U16 x1,U16 y1)//anti_flick
{
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;
	config_para_t ge2d_config;
	ge2d_op_para_t  op_para;
	U16 tmp;
//printf("$$$osdUpdateRegion:%d,%d,%d,%d\n", x0, y0, x1, y1);
	if(x0>x1) 
		aml_swap(x0,x1,tmp);
	if(y0>y1)
		aml_swap(y0,y1,tmp);	
	if(x1-x0 <=0)
	{
		return 0;
	}
	if(y1-y0 <= 0)
	{
		return 0;
	}

	ge2d_config.op_type=OSD0_OSD0;	

	ge2d_config.alu_const_color=0xff0000ff;
	
	osd_sem_lock(g_osd_sem);
	
	ioctl(pFB->m_iGe2dFD, FBIOPUT_GE2D_CONFIG, &ge2d_config) ;

	//add by wjf  start 
	if (4 == pFB->m_FBDevColorDepth){		
		FB_SetColorKey(0, m_tMwDrvState.auColorKey[0],1);	
		op_para.src1_rect.x=x0;
		op_para.src1_rect.y=y0 + pFB->m_FBDevHeight * 2;
		op_para.src1_rect.w=x1-x0;
		op_para.src1_rect.h=y1-y0;

		op_para.dst_rect.x=op_para.src1_rect.x;
		op_para.dst_rect.y=op_para.src1_rect.y;
		op_para.dst_rect.w=op_para.src1_rect.w;
		op_para.dst_rect.h=op_para.src1_rect.h;

		ioctl(pFB->m_iGe2dFD, 0x46f7, &op_para) ;
		FB_SetColorKey(0, m_tMwDrvState.auColorKey[0],0);	
	}
	//add by wjf  end

	op_para.src1_rect.x=x0;
	op_para.src1_rect.y=y0 + pFB->m_FBDevHeight * 2;
	op_para.src1_rect.w=x1-x0;
	op_para.src1_rect.h=y1-y0;

	op_para.dst_rect.x=x0;
	op_para.dst_rect.y=y0;
	op_para.dst_rect.w=op_para.src1_rect.w;
	op_para.dst_rect.h=op_para.src1_rect.h;

	ioctl(pFB->m_iGe2dFD, FBIOPUT_GE2D_STRETCHBLIT, &op_para) ;

	osd_sem_unlock(g_osd_sem);
	return 0;
}

static INT32  osdUpdateRegionEx(U16 x0,U16 y0,U16 x1,U16 y1)//anti_flick
{
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;
	config_para_t ge2d_config;
	ge2d_op_para_t  op_para;
	U16 tmp;
	
	//printf("@@@osdUpdateRegionEx:%d,%d,%d,%d\n", x0, y0, x1, y1);

	if(x0>x1) 
		aml_swap(x0,x1,tmp);
	if(y0>y1)
		aml_swap(y0,y1,tmp);	
	if(x1-x0 <=0)
	{
		return 0;
	}
	if(y1-y0 <= 0)
	{
		return 0;
	}
	osd_sem_lock(g_osd_sem);
	
	ge2d_config.op_type=OSD0_OSD0;	

	ge2d_config.alu_const_color=0xff0000ff;
	
	ioctl(pFB->m_iGe2dFD, FBIOPUT_GE2D_CONFIG, &ge2d_config) ;
	
	//add by wjf  start 
	if (4 == pFB->m_FBDevColorDepth){
		FB_SetColorKey(0, m_tMwDrvState.auColorKey[0],1);	
		op_para.src1_rect.x=x0;
		op_para.src1_rect.y=y0 + pFB->m_FBDevHeight * 3;
		op_para.src1_rect.w=x1-x0;
		op_para.src1_rect.h=y1-y0;

		op_para.dst_rect.x=op_para.src1_rect.x;
		op_para.dst_rect.y=op_para.src1_rect.y;
		op_para.dst_rect.w=op_para.src1_rect.w;
		op_para.dst_rect.h=op_para.src1_rect.h;

		ioctl(pFB->m_iGe2dFD, 0x46f7, &op_para) ;
              	FB_SetColorKey(0, m_tMwDrvState.auSrc2ColorKey[0],1);	
		op_para.src1_rect.x=x0;
		op_para.src1_rect.y=y0 + pFB->m_FBDevHeight * 3;
		op_para.src1_rect.w=x1-x0;
		op_para.src1_rect.h=y1-y0;

		op_para.dst_rect.x=op_para.src1_rect.x;
		op_para.dst_rect.y=op_para.src1_rect.y;
		op_para.dst_rect.w=op_para.src1_rect.w;
		op_para.dst_rect.h=op_para.src1_rect.h;

		ioctl(pFB->m_iGe2dFD, 0x46f7, &op_para) ;
		FB_SetColorKey(0, m_tMwDrvState.auColorKey[0],0);	
	}
	//add by wjf  end
	
	osdBlend(x0, y0, x1, y1);
	
	op_para.src1_rect.x=x0;
	op_para.src1_rect.y=y0 + pFB->m_FBDevHeight;
	op_para.src1_rect.w=x1-x0;
	op_para.src1_rect.h=y1-y0;

	op_para.dst_rect.x=x0;
	op_para.dst_rect.y=y0;
	op_para.dst_rect.w=op_para.src1_rect.w;
	op_para.dst_rect.h=op_para.src1_rect.h;

	ioctl(pFB->m_iGe2dFD, FBIOPUT_GE2D_STRETCHBLIT, &op_para) ;
	osd_sem_unlock(g_osd_sem);
	return 0;
}

static INT32 FB_SetLayer(INT32 layer, INT32 ctrl)
{
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;
	BOOL RetErr = 0;

	if(pFB->m_iFbDevFD > 0)
	{
		RetErr = ioctl(pFB->m_iFbDevFD, FBIOBLANK, &ctrl );
		if( RetErr == -1 )
		{
			printf ("GFB_SetLayer ERROR: STGFB_IO_SET_VIEWPORT_HANDLE failed.");
		}
	}
	else
	{
		printf ("ERROR: STGFB_IO_SET_VIEWPORT_HANDLE failed.");
		RetErr = -1;
	}
	return RetErr;
}

static INT32 FB_SetAlpha(INT32 layer, INT32 alpha)
{
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;
	BOOL              RetErr = 0;

	if(pFB->m_iFbDevFD > 0)
	{			
		RetErr = ioctl(pFB->m_iFbDevFD, FBIOPUT_OSD_SET_ALPHA, alpha);
		if( RetErr == -1 )
		{
			printf ("GFB_SetAlpha ERROR: FBIOPUT_OSD_SET_ALPHA failed.");
		}
	}
	else
	{
		printf ("ERROR: FBIOPUT_OSD_SET_ALPHA failed");
		RetErr = -1;		
	}

	return RetErr;
}
//add by wjf start 
static INT32 FB_Enable32bitColorKey(T_FBAdaptor* pFB,INT32 layer, INT32 iColorKey,INT32 keymode)
{
	config_para_t  config;

	if(layer==0 )
	{
		config.op_type=OSD0_OSD0;
	//}
	//else{
	//	config.op_type=OSD1_OSD1;
	}

	config.src_key.key_color=iColorKey;
	config.src_key.key_enable=1;
	config.src_key.key_mask=0x0; 
	config.src_key.key_mode=keymode; //set keymode 

	ioctl( pFB->m_iGe2dFD,FBIOPUT_GE2D_SRCCOLORKEY, &config) ;	
	return 0;
}
//SetColorKeyEx only use for ge2d blend op to set ge2d src2 key.
static INT32  FB_SetColorKeyEx(INT32 layer, INT32  enable,INT32 colorKey,INT32  keymode) //src2_key
{
#define  GE2D_SRC2COLORKEY   0x46f6
       T_FBAdaptor 		*pFB = &g_AmlFBAdaptor;
	INT32  			u32Enable=1;
	BOOL              	RetErr = 0;
	INT32 			i32ColorKey	= -1;
	U8 a,r,g,b;
       config_para_t  config;


        r = (colorKey >> 16) & 0xff ;  
	g = (colorKey >> 8)  & 0xff ;
	b = colorKey & 0xff ;
	a = colorKey >> 24 & 0xff;
       i32ColorKey = ((r << 24) | (g << 16) | (b << 8) | a); 
       config.src2_key.key_color=i32ColorKey;
	config.src2_key.key_enable=enable;
	config.src2_key.key_mask=0x0; 
	config.src2_key.key_mode=keymode; //set keymode 

	ioctl( pFB->m_iGe2dFD, GE2D_SRC2COLORKEY, &config);
	return 0; 
}
//add by wjf end

static INT32 FB_SetColorKey(INT32 layer, INT32 colorKey,INT32  keymode)
{
	T_FBAdaptor 		*pFB = &g_AmlFBAdaptor;
	INT32  			u32Enable=1;
	BOOL              	RetErr = 0;
	INT32 			i32ColorKey	= -1;
	U8 a,r,g,b;

	if((pFB->m_iFbDevFD > 0) && (4 != pFB->m_FBDevColorDepth))
	{
		if (ioctl(pFB->m_iFbDevFD, FBIOPUT_OSD_SRCKEY_ENABLE, &u32Enable)) 
		{
			printf("FBIOPUT_OSD_SRCKEY_ENABLE: failed");
			return -1;
		}
	}

	if(4 == pFB->m_FBDevColorDepth)
	{
		/*nano-x will invoke this interface, and the colorkey may be set with a invalid value(not 0xff000000), 
		so we must reset as 0xff000000*/
		//colorKey = MASKNONE;

		r = (colorKey >> 16) & 0xff ;  
		g = (colorKey >> 8)  & 0xff ;
		b = colorKey & 0xff ;
		a = colorKey >> 24 & 0xff;

		i32ColorKey = ((r << 24) | (g << 16) | (b << 8) | a); //

		return FB_Enable32bitColorKey(pFB,layer,i32ColorKey,keymode);
		//add by wjf end
	}	
	else
	{		
		r = (colorKey >> 16) & 0xff ;  
		g = (colorKey >> 8)  & 0xff ;
		b = colorKey & 0xff ;
		a = colorKey >> 24 & 0xff;
		if(3 == pFB->m_FBDevColorDepth){
			i32ColorKey = ((b << 16) | (g << 8) | r);
		}
		else if(2 == pFB->m_FBDevColorDepth){
			i32ColorKey = (((r >> 2) & 0x3f) << 10 | (g >> (3 & 0x1f)) << 5 | (b >> (3 & 0x1f))) ;
		}	
		else
		{
			printf("FB_SetColorKey: 0x%x failed",colorKey);
			return -1;
		}
		i32ColorKey = i32ColorKey <<8 | a; 
	} 	

	RetErr = ioctl(pFB->m_iFbDevFD, FBIOPUT_OSD_SRCCOLORKEY, &i32ColorKey);
	if( RetErr == -1 )
	{
		printf ("FB_SetColorKey ERROR: STGFB_IO_SET_OVERLAY_ALPHA failed");
	}

	return RetErr;
}

static INT32 Region_Check(INT32 nLeft, INT32 nTop, INT32 nWidth, INT32 nHeight, INT32 *pnX, INT32 *pnY, INT32 *pnW, INT32 *pnH)
{
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;
	INT32 nVirW = pFB->m_FBDevWidth; //pFB->m_MWWidth;
	INT32 nVirH = pFB->m_FBDevHeight;//m_MWHeight;

	if (nLeft >= nVirW || nTop >= nVirH)
	{
		printf("Region_Check:nLeft=%d nTop=%d nWidth=%d nHeight=%d m_tMwDrvState.auColorKey[0]=%lu", nLeft, nTop, nWidth, nHeight, m_tMwDrvState.auColorKey[0]);
		printf("GUI Region_Check Err: range Failed");
		return -1;
	}

	if (nLeft >= 0)
	{
		*pnX	= nLeft + pFB->m_iLeftOffset ;
		*pnW	= nWidth;
	}
	else
	{
		*pnX	= pFB->m_iLeftOffset;
		*pnW	= nWidth + nLeft;
	}
	nLeft + nWidth > nVirW ? (*pnW = nVirW - nLeft) : 0;

	if (nTop >= 0)
	{
		*pnY	= nTop+ pFB->m_iTopOffset ;
		*pnH	= nHeight;
	}
	else
	{
		*pnY	= pFB->m_iTopOffset ;
		*pnH	= nHeight + nTop;
	}
	nTop + nHeight> nVirH ? (*pnH = nVirH - nTop) : 0;
	if (*pnW <= 0 || *pnH <= 0)
	{
		printf("Region_Check:nLeft=%d nTop=%d nWidth=%d nHeight=%d m_tMwDrvState.auColorKey[0]=%lu", nLeft, nTop, nWidth, nHeight, m_tMwDrvState.auColorKey[0]);
		printf("GUI Region_Check Err: range Failed");	
		return -1;
	}

	return 0;
}

/**************************************************************************
* �������ƣ�FB_EraseRect
* �������������ϲ��ͼ��buffer����Linux��Framebuffer���е����ص����
* ���������
*			bLayer				ͼ��ID
*			nType				���Ǹ�ͼ��Buffer
*			nLeft				ˢ���������ϽǺ�����
*			nTop				ˢ���������Ͻ�������
*			nWidth				ˢ��������
*			nHeight				ˢ������߶�
* �����������
* �� �� ֵ���ɹ�����ʧ��
* ����˵����
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2009.08.20  V1.0	     ������	     ��ʼ�汾
***************************************************************************/
INT32	FB_EraseRect(INT32 layer, INT32 nType, INT32 nLeft, INT32 nTop, INT32 nWidth, INT32 nHeight)
{
	INT32   nResult = 0;

	T_FBAdaptor *pFB = &g_AmlFBAdaptor;
	config_para_t ge2d_config;
	ge2d_op_para_t  op_para;

	//printf("FB_EraseRect: %d %d %d %d %d\r\n", nLeft, nTop, nWidth, nHeight, m_tMwDrvState.auColorKey[0]);
	//we will erase the rect directly.
	ge2d_config.op_type=OSD0_OSD0; 
	ge2d_config.alu_const_color=0xff0000ff;
       printf("%s : %d\n",__FUNCTION__,__LINE__);
	osd_sem_lock(g_osd_sem);
       printf("%s : %d\n",__FUNCTION__,__LINE__);
	ioctl( pFB->m_iGe2dFD, FBIOPUT_GE2D_CONFIG, &ge2d_config) ;
	op_para.src1_rect.x=nLeft;
	op_para.src1_rect.y=nTop;
	op_para.src1_rect.w=nWidth;
	op_para.src1_rect.h=nHeight;
	if(m_tMwDrvState.auColorKey[0] == MASKNONE)
	{
		//in 32bits mode, colorkey is only valid while using ge2d, ge2d can convert all color(same as colorkey)
		//to 0x0000000, so those color will be filted. following operations take framebuffer directly, so we must 
		//set all color with 0x000000	
		if (pFB->m_FBDevColorDepth == 4)
		{//add by wjf
			op_para.color= 0;  //no transparent  
		}
		else
		{
			op_para.color=0x00;   
		}		

	}
	else
	{
		op_para.color= 0;		
	} 
	nResult = ioctl(pFB->m_iGe2dFD, FBIOPUT_GE2D_FILLRECTANGLE, &op_para) ;
	//clear ralative memory in lower buffer. 
	op_para.src1_rect.y+= pFB->m_FBDevHeight * 2;
	nResult = ioctl(pFB->m_iGe2dFD, FBIOPUT_GE2D_FILLRECTANGLE, &op_para) ;
       printf("%s : %d\n",__FUNCTION__,__LINE__);
	osd_sem_unlock(g_osd_sem);
       printf("%s : %d\n",__FUNCTION__,__LINE__);
	return	nResult;
}

/**************************************************************************
* �������ƣ�B120RefreshRect
* ������������һ�����������ˢ�����ݷ�ӳ����ʾ�豸��
* ���������
*			bLayer				ͼ��ID
*			nLeft				ˢ���������ϽǺ�����
*			nTop				ˢ���������Ͻ�������
*			nWidth				ˢ��������
*			nHeight				ˢ������߶�
* �����������
* �� �� ֵ�����͸���ʾ�豸���ֽ��������岻��
* ����˵����
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2006.02.14  V1.0	     ������	     ��ʼ�汾
***************************************************************************/
INT32 FB_RefreshRect(unsigned char bLayer, INT32 nLeft, INT32 nTop, INT32 nWidth, INT32 nHeight)
{	
	INT32	nResult = -1;
	INT32   nX = nLeft;
	INT32   nY = nTop;
	INT32   nW = nWidth;
	INT32   nH = nHeight;

	nResult = Region_Check(nLeft, nTop, nWidth, nHeight, &nX, &nY, &nW, &nH);
	if(nResult == -1)
	{
		printf("Error: range failed");
		return -1;
	}

	osdUpdateRegion(nX, nY, nX + nW, nY + nH);

	return	nResult;
}

/**************************************************************************
* �������ƣ�SetScreanInfo
* ����������fill screan with black
* ���������*			
* �����������
* �� �� ֵ��
* ����˵����
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2006.02.14  V1.0	     ������	     ��ʼ�汾
***************************************************************************/
void SetScreanInfo(int iOsdType, char* val)
{
	FILE* iOsdDevFD;
	char strBlank[8];
	UINT32 uiWriteLen;
	if(iOsdType == LAYER_GUI){
		iOsdDevFD = fopen("/sys/class/graphics/fb0/blank", "w");    
	}
	else if(iOsdType == LAYER_TVMS)
	{
		iOsdDevFD = fopen("/sys/class/graphics/fb1/blank", "w");    
	}

        if (iOsdDevFD == 0)
        {
                printf("Error in opening /sys/class/graphics/fb1/blank\n");
                return;
        }
                     
        sprintf(strBlank, "%s", val);

        uiWriteLen = strlen(strBlank);
        if (fwrite(strBlank, sizeof(char), uiWriteLen, iOsdDevFD) != uiWriteLen)
        {
                printf("Error set screen as black mode\n");  
                fclose(iOsdDevFD);
                return;
        }
        fclose(iOsdDevFD);
}

void FillFBWithBlack()
{
	const char     acName[] = "/dev/fb1";	//��AMLƽ̨ʹ��
	struct fb_var_screeninfo vinfo;
	int iFrameBufLen = 0;	
	unsigned char *pucOsdFrameBuffer;
	INT32 iOSDFbFD = -1;	
	INT32  i32Enable	=	1;
	INT32	iColorKey = 0x00000000;
	iOSDFbFD = open(acName, O_RDWR);	
	if (iOSDFbFD <= 0)
	{
		printf("osdOpen err: open Failed %d[%s]!\n", iOSDFbFD, strerror(errno));
		return;
	}	
	
	vinfo.xres = 720;
	vinfo.yres = 576;

	vinfo.xres_virtual = 720;
	vinfo.yres_virtual = 576 * 2;
	
	//��Frame buffer����bpp
	vinfo.bits_per_pixel 	= 2 * 8;
	vinfo.activate		= FB_ACTIVATE_FORCE;
	
	if (ioctl(iOSDFbFD, FBIOPUT_VSCREENINFO, &vinfo) < 0)
	{
		printf("osdOpen err: ioctl FBIOPUT_VSCREENINFO Failed %d[%s]!\n",iOSDFbFD, strerror(errno));
		goto osdopenfailed;
	}
		
	iFrameBufLen = vinfo.xres_virtual * vinfo.yres_virtual * 2;
	pucOsdFrameBuffer = (UCHAR *)mmap(NULL, iFrameBufLen, PROT_READ | PROT_WRITE,MAP_SHARED,iOSDFbFD,0);
	if(pucOsdFrameBuffer == (void *)-1)
	{
		printf("osdOpen err: mmap Failed %d[%s]!\n",iOSDFbFD, strerror(errno));
		goto osdopenfailed;	
	}
	memset(pucOsdFrameBuffer, 0, iFrameBufLen);
	ioctl(iOSDFbFD, FBIOPUT_OSD_SRCKEY_ENABLE, &i32Enable);
	ioctl(iOSDFbFD,FBIOPUT_OSD_SRCCOLORKEY, &iColorKey);
osdopenfailed:
	munmap(pucOsdFrameBuffer, iFrameBufLen);
	close(iOSDFbFD);
	
}
int  MwEnableAntiflicker(void)
{
    T_FBAdaptor *pFB = &g_AmlFBAdaptor;

    ioctl(pFB->m_iGe2dFD, FBIOPUT_GE2D_SET_COEF, 0x40004) ;
    return 0;
}
int  MwDisableAntiflicker(void)
{
    T_FBAdaptor *pFB = &g_AmlFBAdaptor;

    ioctl(pFB->m_iGe2dFD, FBIOPUT_GE2D_SET_COEF, 0x10001) ;
    return 0;
}

/**************************************************************************
*                        ȫ�ֺ���ʵ��                                    *
**************************************************************************/
/**************************************************************************
* �������ƣ�MwFbInit
* ������������ʼ����ʾ�豸����
* ���������nScrW				��������֡������
*			nScrH				��������֡����߶�
*			bColorMode			��������֡����ɫ��ģʽ
*			pMwFb				���ڽ���֡��������ַ�ı���ָ��
* ���������pMwFb				֡��������ַ��д������
* �� �� ֵ��0					��ʼ���ɹ�
*			-1					��ʼ��ʧ��
* ����˵����
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2007.01.23  V1.0	     ���ɸ�	     ��ʼ�汾
***************************************************************************/
int	MwFbInit(int nScrW, int nScrH, unsigned char bColorMode, void **pMwFb)
{
	INT32	iFBOffset = 0;
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;
	struct fb_fix_screeninfo fb_fix;
	struct fb_var_screeninfo fb_var,logo_fb_var;
	int fd,fb_xres,fb_yres;	
	struct stat s;
	int retry_count = 2*240;
	int retry_times = 0;
	int logo_fbd = -1 ;
	
	//wait logo end
	/*while (retry_times < retry_count)
	{
		retry_times++;
		fd = open(LOGOFILENAME, O_RDONLY);
		if (fd < 0)
		{
			usleep(500);
			continue;
		}
		if (fstat(fd, &s) < 0) {
			usleep(500);
			close(fd);
			continue;
		}
		printf("+++++++++logo has finished");	
		close(fd);
		break;
	}	*/
	
	/*inorder to show vod, set osd1 with blank*/	
	SetScreanInfo(LAYER_TVMS, "0");
	SetScreanInfo(LAYER_GUI, "0");
	
	memset(&fb_fix,0,sizeof(struct fb_fix_screeninfo));
	memset(&fb_var,0,sizeof(struct fb_var_screeninfo));
	
	if(pFB->m_iFbDevFD < 0)
	{
		pFB->m_iFbDevFD	=	open(FBDEVNAME, O_RDWR);
		if (pFB->m_iFbDevFD < 0)
		{
			printf("Nano-X sever: Can't open fb device\r\n");
			goto MwFbInitFailed;;
		}
	}
	if (pFB->m_iGe2dFD < 0)
	{
		pFB->m_iGe2dFD	= open("/dev/ge2d", O_RDWR);
		if (pFB->m_iGe2dFD < 0)
		{
			printf("Nano-X sever: Can't open ge2d\r\n");
			goto MwFbInitFailed;;
		}
	}	
	//make gui behind osd
	//osd_order = 1;	
	//ioctl(pFB->m_iFbDevFD, FBIOPUT_OSD_ORDER_SET,&osd_order); 

	if(ioctl(pFB->m_iFbDevFD, FBIOGET_FSCREENINFO, &fb_fix) == -1 ||
		ioctl(pFB->m_iFbDevFD, FBIOGET_VSCREENINFO, &fb_var) == -1)
	{
		printf("Error reading screen info");
		goto MwFbInitFailed;;
	}
	fb_xres=fb_var.xres ;
	fb_yres=fb_var.yres ;
	pFB->m_PixelType	= bColorMode;	
	if (pFB->m_PixelType == FB_COLORMODE_565)
	{
		pFB->m_FBDevColorDepth	= 2;
	}
	else if (pFB->m_PixelType == FB_COLORMODE_24)
	{
		pFB->m_FBDevColorDepth = 3;
	}
	else if (pFB->m_PixelType == FB_COLORMODE_32)
	{
		pFB->m_FBDevColorDepth = 4;
	}
	else
	{
		goto MwFbInitFailed;
	}

	//wait logo complete
	//usleep(2000000);
	//logo fb resolution match tv mode. so we use it as UI resoluiton.
	logo_fbd= open(LOGODEVNAME, O_RDWR);
	if(logo_fbd<0)
	{
		printf("Nano-X server:can not get logo device,use default resolution\n");
	}
	else
	{
		if(ioctl(logo_fbd, FBIOGET_VSCREENINFO, &logo_fb_var)==-1)
		{
			printf("Error reading logo screen info\n");
		}
		else
		{
			fb_xres=logo_fb_var.xres;
			fb_yres=logo_fb_var.yres;
		}
	} 	
       fb_var.xres = fb_xres;
	fb_var.xres_virtual = fb_xres;
       fb_var.yres=fb_yres;
	fb_var.yres_virtual = fb_yres * 4;       

	fb_var.activate = FB_ACTIVATE_FORCE ;
	fb_var.bits_per_pixel = pFB->m_FBDevColorDepth * 8;	 
	if (ioctl(pFB->m_iFbDevFD, FBIOPUT_VSCREENINFO, &fb_var) < 0) {   
		return -1;
	}		

	printf("bColorMode=%d\n",bColorMode);
	printf("smem_len=%d, line_length=%d, xres=%d, yres=%d, bits_per_pixel=%d",
		fb_fix.smem_len, fb_fix.line_length, fb_var.xres, fb_var.yres, fb_var.bits_per_pixel);

	printf("smem_len=%d, line_length=%d, xres=%d, yres=%d, bits_per_pixel=%d nScrW = %d nScrH = %d",
		fb_fix.smem_len, fb_fix.line_length, fb_var.xres, fb_var.yres, fb_var.bits_per_pixel, nScrW, nScrH);

	pFB->m_FBDevWidth	= fb_var.xres;
	pFB->m_FBDevHeight	= fb_var.yres;

	printf("\n\n\n\n!!!@@####nano-X now in SD mode!!\n\n\n\n");
	pFB->m_MWWidth		= nScrW;
	pFB->m_MWHeight		= nScrH;

	if (pFB->m_MWWidth > pFB->m_FBDevWidth)
	{
		pFB->m_MWWidth = pFB->m_FBDevWidth;
	}

	if (pFB->m_MWHeight	 > pFB->m_FBDevHeight)
	{
		pFB->m_MWHeight = pFB->m_FBDevHeight;
	}    

	pFB->m_iLineLength	= fb_var.xres_virtual * pFB->m_FBDevColorDepth;	

	pFB->m_iLeftOffset	= 0; // (pFB->m_FBDevWidth -pFB->m_MWWidth) / 2;
	pFB->m_iTopOffset	= 0; //(pFB->m_FBDevHeight - pFB->m_MWHeight) / 2;    

	//yincs20100712
#ifdef USE_OFFSET_FUNC
	//�������ļ��ж�ȡƫ����
	int trytime=0;
	FILE *fdmwparams = NULL;
	INT32 iWidth = 0;
	INT32 iHeight = 0;
	INT32 iLeftPos = 0;
	INT32 iTopPos = 0;
	char ac_buf[128] = {0};
	int iFileExist = 0;
	do 
	{
		fdmwparams = fopen("/etc/mwparams", "rw");
		if (!fdmwparams)
		{
			printf("/etc/mwparams seems doesn't exist!!!");
			usleep(200000);
			continue;
		}

		if (!fgets(ac_buf, 128, fdmwparams))
		{
			fclose(fdmwparams);
			usleep(200000);
			continue;
		}

		sscanf(ac_buf, "Left:%d,Top:%d,Width:%d,Height:%d\n", &iLeftPos, &iTopPos,&iWidth,&iHeight);
		//if (iLeftPos >= 0 && iTopPos >= 0 )
		{
			pFB->m_iExtraLeftOffset= (iLeftPos * ADJUST_LEFT_NUMERATOR) / ADJUST_LEFT_DENOMINATOR;
			pFB->m_iExtraTopOffset = (iTopPos * ADJUST_TOP_NUMERATOR) / ADJUST_TOP_DENOMINATOR;
			osd_set_screen_offset(pFB);

			printf("set mwparams successfule! Left:%d,Top:%d\n", iLeftPos, iTopPos);
		}

		//no use for SD mode --begin
		if (iWidth> 0 && iHeight> 0 )
		{
			printf("set mwparams successfule! Width:%d,Height:%d\n", iWidth, iHeight);
		}
		else
		{
			iWidth = 0;
			iHeight = 0;
		}
		//--end

		fclose(fdmwparams);
		fdmwparams = NULL;
		iFileExist = 1;
		break;
	}
	while(trytime++<5);

	if (iFileExist == 1) //yincs 0716: tobe,  not centered  if offset exisit
	{
		if (2*pFB->m_iLeftOffset + pFB->m_iExtraLeftOffset + pFB->m_MWWidth > pFB->m_FBDevWidth)
		{
			pFB->m_iExtraLeftOffset =  (pFB->m_FBDevWidth - pFB->m_MWWidth) - pFB->m_iLeftOffset*2;

		}

		if (2*pFB->m_iTopOffset  +pFB->m_iExtraTopOffset + pFB->m_MWHeight> pFB ->m_FBDevHeight)
		{
			pFB->m_iExtraTopOffset   = (pFB ->m_FBDevHeight - pFB->m_MWHeight) - pFB->m_iTopOffset*2;

		}

		printf("pFB->m_iExtraLeftOffset  is %d,pFB->m_iExtraTopOffset is %d",pFB->m_iExtraLeftOffset, pFB->m_iExtraTopOffset);
	}
#endif

	pFB->m_iMWBufSize	= (pFB->m_MWWidth + 2*pFB->m_iLeftOffset) * (pFB->m_MWHeight + 2*pFB->m_iTopOffset) * pFB->m_FBDevColorDepth;
	pFB->m_iFBBufSize	= pFB->m_iLineLength * pFB->m_FBDevHeight * 4;

	if(NULL == pFB->m_pucFrameBufferMMAP)
	{	
		pFB->m_pucFrameBufferMMAP	=	mmap(NULL, pFB->m_iFBBufSize,  PROT_READ|PROT_WRITE, MAP_SHARED/*MAP_PRIVATE*/, pFB->m_iFbDevFD, 0);	
		memset(pFB->m_pucFrameBufferMMAP, 0, pFB->m_iFBBufSize);
		pFB->m_pucFrameBufferMMAP += pFB->m_iLineLength * pFB->m_FBDevHeight * 2;
	}

	if(NULL == pFB->m_pucFrameBuffer)
	{
		pFB->m_pucFrameBuffer	=	pFB->m_pucFrameBufferMMAP; /* ����֡������ */
	}
	if ((NULL == pFB->m_pucFrameBuffer) || (NULL == pFB->m_pucFrameBufferMMAP))
	{
		printf("Nano-X sever: Can't map framebuf\r\n");
		goto MwFbInitFailed;
	}
	iFBOffset = ( pFB->m_iTopOffset) *pFB->m_iLineLength +( pFB->m_iLeftOffset) * pFB->m_FBDevColorDepth;

	*pMwFb = pFB->m_pucFrameBuffer + iFBOffset;
	printf("Nano-X sever initialize over!fb address:%p, %p\n", pFB->m_pucFrameBuffer, *pMwFb);
	printf("\n\nnano-X param:g_iScreenWidth =%d g_iScreenHeight =%d g_iVirScrW =%d g_iVirScrH =%d \n\n",
		pFB->m_FBDevWidth, pFB->m_FBDevHeight,pFB->m_MWWidth ,pFB->m_MWHeight);

	m_tMwDrvState.bAlphaBuffer	= 0;
	m_tMwDrvState.abAlpha[0]	= 0xFF;
	if (pFB->m_FBDevColorDepth != 4)
	{
		m_tMwDrvState.auColorKey[0]	= 0x00000000;
	}
	else{
		m_tMwDrvState.auColorKey[0]	= MASKNONE;
              m_tMwDrvState.auSrc2ColorKey[0]=MASKBLACK; //src2_key
              
		printf("osd0 set colorkey:0x%x\n",m_tMwDrvState.auColorKey[0]);
	}
	g_osd_sem=osd_sem_creat(g_osd_semid);	
	FB_SetColorKey(LAYER_GUI, m_tMwDrvState.auColorKey[0],0);
       printf("\n++++erase rect--------\n");       
	//FB_EraseRect(0, 2, 0, 0, pFB->m_MWWidth + 2*pFB->m_iLeftOffset, pFB->m_MWHeight + 2*pFB->m_iTopOffset);
	FB_EraseRect(LAYER_GUI, 3, 0, 0, pFB->m_FBDevWidth, pFB->m_FBDevHeight);	
	//FB_EraseRect(LAYER_TVMS, 3, 0, 0, pFB->m_FBDevWidth, pFB->m_FBDevHeight);
	printf("+++set screen info---------\n");
	SetScreanInfo(LAYER_TVMS, "1");
//	FillFBWithBlack();
	//SetScreanInfo(LAYER_GUI, "1");
	
	return 0;
MwFbInitFailed:
	printf("faield");
	if(pFB->m_iFbDevFD > 0)
	{
		close(pFB->m_iFbDevFD);
		pFB->m_iFbDevFD = -1;
	}
	if(pFB->m_iGe2dFD > 0)
	{
		close(pFB->m_iGe2dFD);
		pFB->m_iGe2dFD = -1;
	}
	if(NULL != pFB->m_pucFrameBuffer)
	{
		//free(pFB->m_pucFrameBuffer);
		pFB->m_pucFrameBuffer = NULL;
	}

	if(NULL != pFB->m_pucFrameBufferMMAP)
	{
		pFB->m_pucFrameBufferMMAP -= pFB->m_iLineLength * pFB->m_FBDevHeight * 2;
		memset(pFB->m_pucFrameBufferMMAP, 0, pFB->m_iFBBufSize);
		munmap(pFB->m_pucFrameBufferMMAP,pFB->m_iFBBufSize);
		pFB->m_pucFrameBufferMMAP = NULL;
	}	
	return -1;
}

/**************************************************************************
* �������ƣ�MwFbUninit
* ����������ж������
* �����������
* �����������
* �� �� ֵ����
* ����˵����
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2007.01.23  V1.0	     ���ɸ�	     ��ʼ�汾
***************************************************************************/
void	MwFbUninit(void)
{	
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;

	osd_del_sem(g_osd_sem);
	if(pFB->m_iFbDevFD > 0)
	{
		close(pFB->m_iFbDevFD);
	}
	pFB->m_iFbDevFD	=	-1;
	if(pFB->m_iGe2dFD > 0)
	{
		close(pFB->m_iGe2dFD);
	}
	pFB->m_iGe2dFD	=	-1;
	if(NULL != pFB->m_pucFrameBuffer)
	{
		//free(pFB->m_pucFrameBuffer);
	}
	pFB->m_pucFrameBuffer = NULL;

	if(NULL != pFB->m_pucFrameBufferMMAP)
	{
		pFB->m_pucFrameBufferMMAP -= pFB->m_iLineLength * pFB->m_FBDevHeight * 2;
		//memset(pFB->m_pucFrameBufferMMAP, 0, pFB->m_iFBBufSize);
		munmap(pFB->m_pucFrameBufferMMAP,pFB->m_iFBBufSize);
	}
	pFB->m_pucFrameBufferMMAP = NULL;
}

/**************************************************************************
* �������ƣ�MwFbGetScrInfo
* ������������ȡ��Ļ��Ϣ
* ���������pnScrW			���ڴ洢��Ļ��ȵı���ָ��
*			pnScrH			���ڴ洢��Ļ�߶ȵı���ָ��
*			pnBpp			���ڴ洢ÿ���ر������ı���ָ��
*			pMwFb			���ڴ洢֡��������ַ�ı���ָ��
* ���������pnScrW			��Ļ��ȱ�д������
*			pnScrH			��Ļ�߶ȱ�д������
*			pnBpp			ÿ�����ֽ�����д������
*			pMwFb			֡��������ַ��д������
* �� �� ֵ����
* ����˵����
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2007.01.23  V1.0	     ���ɸ�	     ��ʼ�汾
***************************************************************************/
void	MwFbGetScrInfo(int *pnScrW, int *pnScrH, int *pnBpp, void **pMwFb)
{
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;
	INT32 iFBOffset = 0;
	INT32 nVirW,nVirH;

	nVirW = pFB->m_MWWidth;
	nVirH = pFB->m_MWHeight;

	if(pnScrW)
	{
		*pnScrW   = nVirW;
	}
	if(pnScrH)
	{
		*pnScrH    = nVirH;
	}
	if(pnBpp)
	{
		*pnBpp  = pFB->m_FBDevColorDepth << 3;
	}
	iFBOffset = (pFB->m_iTopOffset) *pFB->m_iLineLength +(pFB->m_iLeftOffset) * pFB->m_FBDevColorDepth;
	if(pMwFb)
	{
		*pMwFb = pFB->m_pucFrameBuffer + iFBOffset;
	}
}

/**************************************************************************
* �������ƣ�MwFbGetLineLen
* ������������ȡ֡������һ�еĳ���
* ���������pnLineLen			���ڴ洢֡�������г��ȵı���ָ��
* ���������pnLineLen			֡�������г��ȱ�д������
* �� �� ֵ����
* ����˵����
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2007.01.23  V1.0	     ���ɸ�	     ��ʼ�汾
***************************************************************************/
void	MwFbGetLineLen(int *pnLineLen)
{
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;
	*pnLineLen = pFB->m_iLineLength;
}

/**************************************************************************
* �������ƣ�MwFbGetPixType
* ������������ȡ���ظ�ʽ
* ���������pbColorMode			���ڴ洢֡���������ظ�ʽ�ı���ָ��
* ���������pbColorMode			���ظ�ʽ��д������
* �� �� ֵ����
* ����˵����
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2007.01.23  V1.0	     ���ɸ�	     ��ʼ�汾
***************************************************************************/
void	MwFbGetPixType(int *pbColorMode)
{
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;
	if (pFB->m_PixelType == FB_COLORMODE_565)
	{
		*pbColorMode = MWPF_TRUECOLOR565;
	}
	else if (pFB->m_PixelType == FB_COLORMODE_24)
	{
		*pbColorMode = MWPF_TRUECOLOR888;
	}
	else if (pFB->m_PixelType == FB_COLORMODE_32)
	{
		/*return MWPF_TRUECOLOR8888, microwindow can handle pic though fblin32alpha*/
		*pbColorMode = MWPF_TRUECOLOR8888;
	}
}

/**************************************************************************
* �������ƣ�MwFbGetBufSize
* ������������ȡ֡��������С
* ���������pnSize				���ڴ洢֡��������С�ı���ָ��
* ���������pnSize				֡��������С��д������
* �� �� ֵ����
* ����˵����
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2007.01.23  V1.0	     ���ɸ�	     ��ʼ�汾
***************************************************************************/
void	MwFbGetBufSize(int *pnSize)
{
	INT32 nVirW,nVirH;
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;

	nVirW = (pFB->m_MWWidth + 2*pFB->m_iLeftOffset);
	nVirH = pFB->m_MWHeight;//(g_iVirScrH + 2*g_topOffset);

	*pnSize = nVirW * nVirH * pFB->m_FBDevColorDepth;
}

/**************************************************************************
* �������ƣ�MwFbLayerControl
* �����������Ը���ͼ����п���
* ���������bDevice				������ͼ����������ʾ�豸���
*			bLayer				�����Ƶ�ͼ���ţ�0-GUI��1-OSD
*			bAction				���ƶ�����Ŀǰ�Ѷ������¶�����
*				0				ֱ�ӿ���ͼ��Ŀ����͹ر�
*				1				����ͼ���Ĭ��͸���Ⱥ�͸��ɫ
*				2				��ȡ��Ĭ��ͼ���Ӧ���ڴ洰��ID
*				3				��ȡ��ʾʱ�ڵײ�Ӳ���ϵĺ���ƫ��
*				4				��ȡ��ʾʱ�ڵײ�Ӳ���ϵ�����ƫ��
*				0x40			��������Ƶʱ͸���ȵ��ӹ��ܵĿ����͹ر�
* �����������
* �� �� ֵ��0					����õ�ִ��
*			1					�޷�ʶ�������
* ����˵����
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2002/08/01	     V1.0	    XXXX	      XXXX
***************************************************************************/
int MwFbLayerControl(unsigned char bDevice, unsigned char bLayer,
					 unsigned char bAction, unsigned char bArgB, unsigned int uArgU)
{
	INT32	iResult;
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;

	//printf("MwFbLayerControl:%d %d %d %d %d\n", bDevice, bLayer, bAction, bArgB, uArgU);

	iResult	= 0;
	switch(bAction)
	{
	case 0:
		if (bArgB == 0)
		{
			FB_SetLayer(0, 0);
		}
		else
		{
			FB_SetLayer(0, 1);
			if (m_tMwDrvState.bVideoState == 0)
			{
				FB_SetAlpha(0, 0xff);
			}
			else
			{
				if(g_bBlend == 0)
				{
					FB_SetAlpha(0, 0xff);
				}
				else
				{
					FB_SetAlpha(0, bArgB);
					FB_SetColorKey(0, m_tMwDrvState.auColorKey[0],0);
				}
			}
		}
		break;

	case 1:
		/*
		m_tMwDrvState.abAlpha[0]	= bArgB;
		m_tMwDrvState.auColorKey[0]	= uArgU;
		*/
		printf("GFB_SetAlpha:%d, %d, %d\n",g_bBlend, bArgB, uArgU);
		if (bArgB == 0)
		{
			FB_SetLayer(0, 0);
		}
		else
		{
			FB_SetLayer(0, 1);
		}

		if(g_bBlend == 0)
		{
			FB_SetAlpha(0, 0xff);
		}
		else
		{                                             
			FB_SetAlpha(0, bArgB);
			FB_SetColorKey(0, uArgU,0);                                              
		}                        

		break;

	case 3:
		iResult = pFB->m_iLeftOffset;
		break;

	case 4:
		iResult = pFB->m_iTopOffset;
		break;

	case 0x40:
		g_bBlend = bArgB;
		break;

	case	2:						/* ��ȡ��Ĭ��ͼ���Ӧ���ڴ洰��ID */
		/* ��Ҫ֧�ָ���ͼ��������ڴ�������룬�����������ľ�� */
	default:
		iResult	= 1;
		break;
	}

	return	iResult;
}

/**************************************************************************
* �������ƣ�MwFbDriverControl
* �������������������п���
* ���������bAction				������Ϊ��1-�����������йص���Ϊ
*			bOption				����ѡ�ȡֵ��������bAction�����йأ�
*		bActionȡֵ		bOption����
*			1			0-�˳�����״̬��1-��������״̬
* �����������
* �� �� ֵ��Ŀǰʼ�շ�����ֵ��������
* ����˵��������������������ĳ�ֹ��ܵ�ʵ�֣�ֻ�Ǹ����ϲ�ͨ���������ݵ���
*			Ϣ�Եײ���������Ϊ��һЩ�����Ա���Ӧϵͳ״̬�ĸı䡣���豸����
*			ʱ����رպ������ģ������ƹ���
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2007.01.23  V1.0	     ���ɸ�	     ��ʼ�汾
***************************************************************************/
int MwFbDriverControl(unsigned char bAction, unsigned char bOption)
{
	//printf("MwFbDriverControl:%d %d\n", bAction, bOption);

	if (bAction == 1)
	{
		MwKbSleep(bOption);
	}
	return	0;
}

/**************************************************************************
* �������ƣ�MwFbSetTransparent
* ��������������͸����
* ���������bDevice				����������ʾ�豸���
*			bLayer				ͼ�㣬0-GUI��1-OSD
*			ptRect				������͸���ȵ�����ָ��
*			ptTrans				͸������������
* �����������
* �� �� ֵ��0					���óɹ�
*			����ֵ				����ʧ��
* ����˵��������ĳ�������͸���ȣ���͸�����������ݸ���ʾ�豸������ǰ�汾
*			�������ӿڶ��岻ͬ���ǣ����ﲻ��ptRect��ptTransΪ�յ���������
*			�����⴦�������������������Ϊ��
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2007.01.23  V1.0	     ���ɸ�	     ��ʼ�汾
***************************************************************************/
int	MwFbSetTransparent(unsigned char bDevice, unsigned char bLayer, MWRECT *ptRect, MWTRANSPARENCE *ptTrans)
{
	int		iResult;

	if (bLayer > 0)						/* ͼ���ŷǷ��� */
	{
		iResult	= 1;
		goto	MwFbSetTranExit0;
	}

	if (ptTrans == NULL)				/* δָ��͸���ȣ� */
	{
		iResult	= 2;
		goto	MwFbSetTranExit0;
	}

	if (ptRect == NULL)					/* δָ������ */
	{
		iResult	= 3;
		goto	MwFbSetTranExit0;
	}

	iResult	= 0;
	switch (ptTrans->bMode)				/* ����͸����ģʽ�ֱ��� */
	{
	case	3:						/* ͳһ͸���ȣ� */
		{
			m_tMwDrvState.abAlpha[0]	= ptTrans->bOrgTrans;
			m_tMwDrvState.auColorKey[0]	= ptTrans->uColorKey;
			printf("MwFbSetTransparent:%d %d %d %d\n", bDevice, bLayer, ptTrans->bOrgTrans, ptTrans->uColorKey);
			break;
		}
	default:
		{
			iResult	= 4;
			break;
		}
	}

MwFbSetTranExit0:
	return	iResult;
}

T_MwDriverState		*MwFbGetDeviceConfig(unsigned char bDevice)
{
	m_tMwDrvState.bAlphaBuffer	= 0;

	return	&m_tMwDrvState;
}

/**************************************************************************
* �������ƣ�MwFbRefreshPixel
* ������������һϵ�о��������ˢ�����ݷ�ӳ����ʾ�豸��
* ���������bDevice				��ˢ�µ���ʾ�豸���
*			bLayer				��ˢ���������ڵ�ͼ���ţ�0-GUI��1-OSD
*			ptRect				ָ���ˢ������Ľṹָ��
*			nNum				��ˢ�����������
* �����������
* �� �� ֵ�����͸���ʾ�豸���ֽ��������岻��
* ����˵����
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2007.01.23  V1.0	     ���ɸ�	     ��ʼ�汾
***************************************************************************/
int	MwFbRefreshPixel(unsigned char bDevice, unsigned char bLayer, MWRECT *ptRect, int nNum)
{
	INT32	nResult, iLoopA;
	MWRECT	*ptCurRect;

	nResult	= 0;

	if (bLayer > 0)
	{
		goto	MwFbRectExit0;
	}

	ptCurRect	= ptRect;

	if (ptCurRect)
	{		
		for (iLoopA = 0; iLoopA < nNum; iLoopA++)
		{
			nResult	+= FB_RefreshRect(bLayer, ptCurRect[iLoopA].left, ptCurRect[iLoopA].top,
				ptCurRect[iLoopA].right - ptCurRect[iLoopA].left,
				ptCurRect[iLoopA].bottom - ptCurRect[iLoopA].top);
		}
	}


MwFbRectExit0:
	return	nResult;
}

/**************************************************************************
* �������ƣ�MwFbEarasePixel
* ��������������Ļ�ϲ���һϵ�о����������ʾ����
* ���������bDevice				����������ʾ�豸���
*			bLayer				�������������ڵ�ͼ����
*			ptRect				ָ�����������Ľṹָ��
*			nNum				���������������
* �����������
* �� �� ֵ�����͸���ʾ�豸���ֽ��������岻��
* ����˵����
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2007.01.23  V1.0	     ���ɸ�	     ��ʼ�汾
***************************************************************************/
int	MwFbEarasePixel(unsigned char bDevice, unsigned char bLayer, MWRECT *ptRect, int nNum)
{
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;
	INT32	nResult, iLoopA, nX, nY, nW, nH;
	MWRECT	*ptCurRect;
	INT32  nScrW,nScrH;

	nScrW = (pFB->m_MWWidth + 2*pFB->m_iLeftOffset);

	nScrH = (pFB->m_MWHeight + 2*pFB->m_iTopOffset);

	nResult	= 0;

	if (bLayer > 0)
	{
		goto	MwFbClearExit0;
	}

	ptCurRect	= ptRect;
	/* ѭ������ȷд������ˢ�������������Ϣ */
	for (iLoopA = 0; iLoopA < nNum; iLoopA++, ptCurRect++)
	{
		if (ptCurRect->left >= nScrW || ptCurRect->top >= nScrH)
		{
			continue;
		}

		nX	= ptCurRect->left >= 0 ? ptCurRect->left : 0;
		nW	= (ptCurRect->right < nScrW ? ptCurRect->right : nScrW) - nX;

		nY	= ptCurRect->top >= 0 ? ptCurRect->top : 0;
		nH	= (ptCurRect->bottom < nScrH ? ptCurRect->bottom : nScrH) - nY;

		if (nW == 0 || nH == 0)
		{
			continue;
		}

		printf("++++MwFbEarasePixel:%d %d %d %d\n", nX, nY, nW, nH);	
		FB_EraseRect(LAYER_GUI, 1, nX, nY, nW, nH);

	}

MwFbClearExit0:
	return	nResult;
}

/**************************************************************************
* �������ƣ�
* ����������
* ���ʵı�
* �޸ĵı�
* ���������
* ���������
* �� �� ֵ��
* ����˵����
* �޸�����    �汾��     �޸���	     �޸�����
* -----------------------------------------------
* 2006.03.21	V1.0	 ������      ��ʼ�汾
***************************************************************************/
INT32	MwKbSleep(unsigned char bOption)
{
	if (bOption == 1)
	{
		m_iSleep	= 1;
	}
	else
	{
		m_iSleep	= 0;
	}

	return	0;
}

void MwClearNonClientScreen(int x, int y)
{
	//if the target rectangle's area is zero, low-leveled clear will failed  ,to be confirmed
	if(x == 0) x =1;

	if(y== 0) y= 1;

	T_FBAdaptor *pFB = &g_AmlFBAdaptor;	

	//printf("clear top region");
	FB_EraseRect(0, 3, 0, 0, pFB->m_FBDevWidth, y);

	//printf("clear left region");
	FB_EraseRect(0, 3, 0, 0, x, pFB->m_FBDevHeight);

	//printf("clear right region");
	FB_EraseRect(0, 3, pFB->m_FBDevWidth - x , 0, x, pFB->m_FBDevHeight);

	//printf("clear bottom region");
	FB_EraseRect(0, 3, 0, pFB->m_FBDevHeight - y, pFB->m_FBDevWidth, y);

}

int MwFbSetScreenOffset(int x,int y)
{
	//printf("Input parameter x %d,y %d",x,y);
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;	

	x = (x * ADJUST_LEFT_NUMERATOR) / ADJUST_LEFT_DENOMINATOR;
	y = (y * ADJUST_TOP_NUMERATOR	) / ADJUST_TOP_DENOMINATOR;

	if (x < 0 || y < 0  ||  (( ( pFB->m_MWWidth + x + 2*pFB->m_iLeftOffset)> pFB->m_FBDevWidth) && ((  pFB->m_MWHeight + y + 2*pFB->m_iTopOffset)> pFB ->m_FBDevHeight)))
	{
		//printf("invalid x is %d, y is %d",x,y);
		printf("invalid x is %d, y is %d",x,y);
		return -1;
	}

	/*
	printf("pFB->m_MWWidth %d, pFB->m_MWHeight %d, pFB->m_FBDevWidth %d, \
	pFB ->m_FBDevHeight %d, pFB->m_iLeftOffset %d, pFB->m_iTopOffset %d, \
	g_ScrW %d , g_ScrH %d",
	pFB->m_MWWidth, pFB->m_MWHeight, pFB->m_FBDevWidth, pFB ->m_FBDevHeight,
	pFB->m_iLeftOffset, pFB->m_iTopOffset, g_ScrW, g_ScrH);
	*/

	//this will not cause offset change suddenly because above limited
	if( pFB->m_MWWidth + 2*(x + pFB->m_iLeftOffset) > pFB->m_FBDevWidth)
	{
		x = (pFB->m_FBDevWidth - pFB->m_MWWidth - pFB->m_iLeftOffset) / 2;
		printf("fixed x to %d",x);
	}

	if(  pFB->m_MWHeight + 2*(y + pFB->m_iTopOffset) > pFB ->m_FBDevHeight)
	{
		y = (pFB->m_FBDevHeight - pFB->m_MWHeight - pFB->m_iTopOffset) / 2;
		printf("fixed y to %d",y);
	}

	pFB->m_iExtraLeftOffset = x;
	pFB->m_iExtraTopOffset = y;	

	//FB_RefreshRect(0,0,0, pFB->m_MWWidth, pFB ->m_MWHeight);
	osd_set_screen_offset(pFB);

	//MwClearNonClientScreen(x, y);

	return 0;
}

void MwFbRefreshBuf(MWRECT* pRect,void *pBuf)
{	
	INT32	nResult = -1;
	INT32 	nLeft = pRect->left; 
	INT32 	nTop = pRect->top;
	INT32 	nWidth = pRect->right - pRect->left;
	INT32 	nHeight = pRect->bottom - pRect->top;	
	INT32   nX = 0;
	INT32   nY = 0;
	INT32   nW = 0;
	INT32   nH = 0;
	INT32	iLoopA = 0;
	T_FBAdaptor *pFB = &g_AmlFBAdaptor;
	
	nResult = Region_Check(nLeft, nTop, nWidth, nHeight, &nX, &nY, &nW, &nH);
	if(nResult == -1)
	{
		printf("Error: range failed");
		return;
	}

	//fill data to the last framebuffer
	char* pcDst  = pFB->m_pucFrameBuffer + ( pFB->m_iTopOffset) *pFB->m_iLineLength 
		       + ( pFB->m_iLeftOffset) * pFB->m_FBDevColorDepth
		       + pFB->m_iLineLength * pFB->m_FBDevHeight
		       + nX * pFB->m_FBDevColorDepth + nY * (pFB->m_iLineLength);

	for(iLoopA = 0; iLoopA < nH; iLoopA++)
	{   				
		memcpy(pcDst, pBuf, nW * pFB->m_FBDevColorDepth);	
		pBuf +=  nW * pFB->m_FBDevColorDepth;
		pcDst += pFB->m_iLineLength;
	} 

	osdUpdateRegionEx(nX, nY, nX + nW, nY + nH);
	
}
int MwFbClrBuf(MWRECT* pRect, int iType)
{
	return -1;
}

int MwFbBlitBufEx(void *pDstAddr, MWRECT* pDstRect, void *pSrcAddr, MWRECT* pSrcRect,  long op)
{
	return -1;
}

int MwFbRefreshBufEx(MWRECT *pDstRect, void *pSrcAddr, MWRECT *pSrcRect,  void *pSrc1Addr, MWRECT *pSrc1Rect, int iAlpha)
{
	return -1;
}

int MwFbClrBufEx(void *pDstAddr, MWRECT* pDstRect)
{
	return -1;
}

void *MwMallocMem(int iMemSize, int iWidth, int iHeight, int iPriority)
{
	return NULL;
}

void MwFreeMem(void *ptr)
{
		
}

int MwIsDriverMem(void *ptr)
{
	return 0;
}

void MwPrintMemInfo(void)
{
	return;
}

