#if !defined(__IWVDOG_CONST_DEF__)
#define __IWVDOG_CONST_DEF__

/************************************************************************/
/* wpsha   ���ܹ��������ṹ�嶨���ļ�                                   */
/************************************************************************/
#define S4_PROGRAMME_DISABLE		0x00000000    //����--����	

//���ܹ�����
enum DOG_TYPE
{
	DOG_TYPE_UNCHK		 = 0,			//δ���
	DOG_TYPE_UNSET,						//ԭ��δ��ʼ��
	DOG_TYPE_WISVISION,					//������Ѷϵͳ
	DOG_TYPE_UNKNOW						//δ֪����
};

//������
#define S4_INVALID_DEVICE								0xFF000000                      /* ��Ч���豸*/  
#define S4_INVALID_FILE									0xFF000001                      /* ��Ч���ļ�*/  
#define S4_INVALID_DEVTYPE								0xFF000002						/* ��֧�ֵļ��ܹ�����*/
#define S4_ERROR_ALGOCHK								0xFF000003						/* �㷨��֤���ش�������*/
#define S4_ERROR_READCONF								0xFF000004						/* ��ȡ��������ʧ��*/

//Ŀ¼����
#define	WV_EXE_FILE_NAME								"CodeTran.hex"					/* �����ļ�����(PC���ļ�����)*/
#define	WV_EXE_FILE_ID									"0001"							/* �����ļ�ID(���ܹ��ڲ��ļ�ID)*/
#define EV_DATA_FILE_LEN								96								/* �����ļ���󳤶�*/


//���������ļ�ID����0x0000,0x0015,0x0016,0x0018,0x001e,0x3f00,0x3f01,0x3f02,0x3f03,0x3f04 ϵͳ����������
#define DATA_FILEID_CMS									"0101"

//���ܹ��ӿڲ�����ض���
#define S4_MAX_DATA_LEN		98								//�������Ч������󳤶ȣ������������������ͼ����ȣ�
#define S4_CONF_DATA_OFFSET	0								//������Ϣ���ݴ洢ƫ����
#define S4_CONF_DATA_LEN	2								//������Ϣ������󳤶�
typedef struct
{
	unsigned char func_type;								//�������� 0~��֤���ܹ��Ϸ��ԣ�1~��ȡ��������
	unsigned char param_len;								//���ݳ���
	unsigned char param_data[S4_MAX_DATA_LEN];				//����
}VM_PARAM;


/************************************************************************/
/* ��������																*/
/************************************************************************/
//Ӧ�����ͣ����ڶ�����ܹ�ʱ������������ܵļ��ܹ�
typedef enum 
{
	WV_APP_TYPE_COMMON = 0,									//ͨ��   ֻ��֤Ӧ���Ƿ�Ϸ�
	WV_APP_TYPE_CMS,										//CMS
	WV_APP_TYPE_TSU,										//TSU
	WV_APP_TYPE_MGW											//MGW
}WV_APP_TYPE;


//CMS��������
typedef struct
{
	unsigned long	dwProgramme;														//�������
	unsigned long	dwVMCount;															//�����ǰ�˵�λ·��
	unsigned long	dwCCount;															//��֧�ֵĿͻ�������
}CONF_CMS;



typedef union
{
	CONF_CMS	CmsConf;
}CONF_DATA;

typedef	struct  
{
	WV_APP_TYPE	AppType;
	CONF_DATA	AppData;
}APP_CONF;

#endif