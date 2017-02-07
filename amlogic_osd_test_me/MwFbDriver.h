/**************************************************************************
* ��Ȩ���� (C)2001, ����������ͨѶ�ɷ����޹�˾��
*
* �ļ����ƣ�MwFbDriver.h
* �ļ���ʶ��
* ����ժҪ�����ļ�����������MicroWindows֡�������������ݺͺ�������
* ����˵����
* ��ǰ�汾��
* ��    �ߣ�
* ������ڣ�
*
* �޸ļ�¼1	��
* 	�޸����ڣ�
* 	�� �� �ţ�
* 	�� �� �ˣ�
* 	�޸����ݣ�
***************************************************************************/

#ifndef		__MWFBDRIVER_H__
#define		__MWFBDRIVER_H__

#ifdef		__cplusplus
extern "C" {
#endif

/**************************************************************************
 *                         ͷ�ļ�����                                     *
 **************************************************************************/
#include	"nano-X.h"

/**************************************************************************
 *                         ��      ��                                     *
 **************************************************************************/
/**************************************************************************
 *                         ��  ��  ��                                     *
 **************************************************************************/
/**************************************************************************
 *                         �� �� �� ��                                    *
 **************************************************************************/
typedef		struct
{
	/* �����ֶ���ͼ�����ģ���޸Ĳ�ά��������ֻ�ɶ�ȡ */
	unsigned char	bVideoState;		/* ��ǰ��Ƶ״̬��0-�رա�1-�� */
	unsigned char	bSleepState;		/* ��ǰ����״̬��0-�رա�1-���� */
	/* �����ֶ��������޸Ĳ�ά����ͼ�����ģ��ֻ�ɶ�ȡ */
	unsigned char	bAlphaBuffer;		/* �Ƿ���͸���Ȼ�������0-��1-�� */
	unsigned char	abAlpha[2];			/* ����ͼ���Ĭ��͸���� */
	unsigned long	auColorKey[2];		/* ����ͼ���ɫ�� */
}
T_MwDriverState;

/**************************************************************************
 *                        ȫ�ֱ�������                                    *
 **************************************************************************/
extern		int		g_nFbDevNum;
/**************************************************************************
 *                        ȫ�ֺ���ԭ��                                    *
 **************************************************************************/
int		MwFbInit(int nScrW, int nScrH, unsigned char bColorMode, void **pMwFb);
void	MwFbUninit(void);

void	MwFbGetScrInfo(int *pnScrW, int *pnScrH, int *pnBpp, void **pMwFb);
void	MwFbGetLineLen(int *pnLineLen);
void	MwFbGetPixType(int *pbColorMode);
void	MwFbGetBufSize(int *pnSize);

int		MwFbEarasePixel(unsigned char bDevice, unsigned char bLayer, MWRECT *ptRect, int nNum);
int		MwFbRefreshPixel(unsigned char bDevice, unsigned char bLayer, MWRECT *ptRect, int nNum);
int		MwFbSetTransparent(unsigned char bDevice, unsigned char bLayer, MWRECT *ptRect, MWTRANSPARENCE *ptTrans);
int		MwFbLayerControl(unsigned char bDevice, unsigned char bLayer,
			unsigned char bAction, unsigned char bArgB, unsigned int uArgU);

int		MwFbDriverControl(unsigned char bAction, unsigned char bOption);
T_MwDriverState		*MwFbGetDeviceConfig(unsigned char bDevice);

#ifdef		__cplusplus
}
#endif

#endif  	/* __MWFBDRIVER_H__ */
