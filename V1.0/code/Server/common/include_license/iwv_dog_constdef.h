#if !defined(__IWVDOG_CONST_DEF__)
#define __IWVDOG_CONST_DEF__

/************************************************************************/
/* wpsha   加密狗常量及结构体定义文件                                   */
/************************************************************************/
#define S4_PROGRAMME_DISABLE		0x00000000    //方案--禁用	

//加密狗类型
enum DOG_TYPE
{
	DOG_TYPE_UNCHK		 = 0,			//未检测
	DOG_TYPE_UNSET,						//原厂未初始化
	DOG_TYPE_WISVISION,					//金智视讯系统
	DOG_TYPE_UNKNOW						//未知类型
};

//错误码
#define S4_INVALID_DEVICE								0xFF000000                      /* 无效的设备*/  
#define S4_INVALID_FILE									0xFF000001                      /* 无效的文件*/  
#define S4_INVALID_DEVTYPE								0xFF000002						/* 不支持的加密狗类型*/
#define S4_ERROR_ALGOCHK								0xFF000003						/* 算法验证返回错误数据*/
#define S4_ERROR_READCONF								0xFF000004						/* 读取配置数据失败*/

//目录分配
#define	WV_EXE_FILE_NAME								"CodeTran.hex"					/* 程序文件名称(PC端文件名称)*/
#define	WV_EXE_FILE_ID									"0001"							/* 程序文件ID(加密狗内部文件ID)*/
#define EV_DATA_FILE_LEN								96								/* 数据文件最大长度*/


//配置数据文件ID定义0x0000,0x0015,0x0016,0x0018,0x001e,0x3f00,0x3f01,0x3f02,0x3f03,0x3f04 系统保留不可用
#define DATA_FILEID_CMS									"0101"

//加密狗接口参数相关定义
#define S4_MAX_DATA_LEN		98								//输入的有效数据最大长度（不包含输入数据类型及长度）
#define S4_CONF_DATA_OFFSET	0								//配置信息数据存储偏移量
#define S4_CONF_DATA_LEN	2								//配置信息数据最大长度
typedef struct
{
	unsigned char func_type;								//输入类型 0~验证加密狗合法性；1~读取配置数据
	unsigned char param_len;								//数据长度
	unsigned char param_data[S4_MAX_DATA_LEN];				//数据
}VM_PARAM;


/************************************************************************/
/* 配置数据																*/
/************************************************************************/
//应用类型，用于多个加密狗时，启用最大性能的加密狗
typedef enum 
{
	WV_APP_TYPE_COMMON = 0,									//通用   只验证应用是否合法
	WV_APP_TYPE_CMS,										//CMS
	WV_APP_TYPE_TSU,										//TSU
	WV_APP_TYPE_MGW											//MGW
}WV_APP_TYPE;


//CMS配置数据
typedef struct
{
	unsigned long	dwProgramme;														//方案编号
	unsigned long	dwVMCount;															//管理的前端点位路数
	unsigned long	dwCCount;															//可支持的客户端数量
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