/******************************************************************************

                  ��Ȩ���� (C), 2001-2016, ������Ѷ�������޹�˾

 ******************************************************************************
  �� �� ��   : SHDiBiao.h
  �� �� ��   : ����
  ��    ��   : zb
  ��������   : 2016��3��14�� ����һ
  ����޸�   :
  ��������   : �Ϻ��ر���ض���
  �����б�   :
  �޸���ʷ   :
  1.��    ��   : 2016��3��14�� ����һ
    ��    ��   : zb
    �޸�����   : �����ļ�

******************************************************************************/

/*****************************Agent Message Type******************************/

#define EV9000_SHDB_ALARMLINK_PIC                     1                       //��������ͼ��
#define EV9000_SHDB_NURTURING_PIC                     2                       //�����ϴ�ͼ��
#define EV9000_SHDB_DAILY_PIC                         3		     			  //�ճ��ϴ�ͼ��
#define EV9000_SHDB_TEST_PIC						  4						  //�����ϴ�ͼ��
#define EV9000_SHDB_OTHER_SYSTEM_ERROR				  11					  //����ϵͳ����	
#define EV9000_SHDB_PERIMETER_ALARM_ERROR             12                      //�ܽ籨������
#define EV9000_SHDB_NETWORK_ALARM_ERROR               13                      //������������
#define EV9000_SHDB_LOCAL_ALARM_ERROR                 14                      //���ر�������
#define EV9000_SHDB_VIDEO_ERROR                       15                      //��Ƶ��ع���
#define EV9000_SHDB_BUILDING_INTERCOM_ERROR           16                      //¥��Խ�����
#define EV9000_SHDB_ACCESS_CONTROL_ERROR              17                      //������ƹ���
#define EV9000_SHDB_ELE_PATROL                        18                      //����Ѳ������
#define EV9000_SHDB_DVR_SYSTEM_START                  19                      //DVRϵͳ����
#define EV9000_SHDB_DVR_SYSTEM_STOP                   20                      //DVRϵͳ�˳�
#define EV9000_SHDB_DVR_ABNORMAL_STOP                 21                      //DVR�쳣�˳�
#define EV9000_SHDB_DVR_PARAM_CONFIG                  22                      //DVR��������
#define EV9000_SHDB_DVR_PARAM_COMMIT                  23                      //DVR��������
#define EV9000_SHDB_DVR_VIDEO_LOSS                    24                      //DVR��Ƶ��ʧ
#define EV9000_SHDB_DVR_MOTION_DETECTION              25                      //DVR�ƶ����
#define EV9000_SHDB_DVR_EXTERN_TRIGGER                26                      //DVR�ⲿ����
#define EV9000_SHDB_DVR_SYSTEM_ALARM_RESET            27                      //ϵͳ�������                
#define EV9000_SHDB_DVR_ILLEGAL_STOP                  28                      //DVR�Ƿ��˳�                
#define EV9000_SHDB_SYSTEM_SERVICE_CHECK              29                      //ϵͳά��ǩ��
#define EV9000_SHDB_SYSTEM_MAINTENANCE_CHECK          30                      //ϵͳά��ǩ��
#define EV9000_SHDB_DVR_LOCAL_PLAY_BACK               31                      //DVR���ػط�
#define EV9000_SHDB_DVR_REMOTE_PLAY_BACK              32                      //DVRԶ�̻ط�
#define EV9000_SHDB_ACCEPTANCE_PIC                    33                      //�����ϴ�ͼ��
#define EV9000_SHDB_ALARM_PIC                         36                      //�����ϴ�ͼ��(δ¼����ƶ����)
#define EV9000_SHDB_NURTURING_PIC_EX                  37                      //�����ϴ�ͼ��(δ¼����ƶ����)
#define EV9000_SHDB_DAILY_PIC_EX                      38                      //�ճ��ϴ�ͼ��(����δ¼��)                                          
#define EV9000_SHDB_TEST_PIC_EX                       39                      //�����ϴ�ͼ��(δ¼����ƶ����)                     
#define EV9000_SHDB_ACCEPTANCE_PIC_EX                 40                      //�����ϴ�ͼ��(δ¼����ƶ����)
#define EV9000_SHDB_DVR_DISK_ERROR                    41                      //DVR���̴���
#define EV9000_SHDB_SYSTEM_KEEPLIVE_TIMEOUT           42                      //ϵͳ������ʱ
#define EV9000_SHDB_SYSTEM_KEEPLIVE_RECOVER           43                      //ϵͳ�����ָ�
#define EV9000_SHDB_OTHER_VIDEO_EVENT                 44                      //ϵͳ�����¼�

/**********************����ͼ���ϴ�**************************/
/* 
CPacket pack;
pack.SetRootTag("Notify");
pack.CreateElement("CmdType", "SHDB");
pack.CreateElement("SubType", "33");
pack.CreateElement("DeviceID", "320XXXXXXXXXXX");
pack.CreateElement("Format", "CIF");
pack.CreateElement("PIC", "");
pack.CreateElement("Port", "");
pack.CreateElement("UserID", "");
*/

/**********************ά������**************************/
/* 

  1��ά��ǩ��
CPacket pack;
pack.SetRootTag("Notify");
pack.CreateElement("CmdType", "SHDB");
pack.CreateElement("SubType", "29");

  2������ǩ��
 CPacket pack;
 pack.SetRootTag("Notify");
 pack.CreateElement("CmdType", "SHDB");
 pack.CreateElement("SubType", "30");
 pack.CreateElement("Info", ".....");

*/

/*********************���ϱ���**************************/
/*

//��Ƶ���
CPacket pack;
pack.SetRootTag("Notify");
pack.CreateElement("CmdType", "SHDB");
pack.CreateElement("SubType", "15");
pack.CreateElement("Info", ".....")

 //
 CPacket pack;
 pack.SetRootTag("Notify");
 pack.CreateElement("CmdType", "SHDB");
 pack.CreateElement("SubType", "16");
 pack.CreateElement("Info", ".....");

 */

