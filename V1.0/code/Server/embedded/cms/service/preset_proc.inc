#ifndef __PRESET_PROC_H__
#define __PRESET_PROC_H__


/*----------------------------------------------*
 * 包含头文件                                   *
 *----------------------------------------------*/
#include <sys/types.h>
#ifdef WIN32
#include <winsock2.h>
#else
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#endif

#include "libsip.h"
#include "libxml/Packet.h"

#include "common/gbldef.inc"
#include "common/DBOper.h"

#include <vector>
#include <string>
using namespace std;

#ifdef __cplusplus
#if __cplusplus
extern "C" {
#endif
#endif /* __cplusplus */

    /*----------------------------------------------*
     * 外部变量说明                                 *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 外部函数原型说明                             *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 内部函数原型说明                             *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 全局变量                                     *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 模块级变量                                   *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 常量定义                                     *
     *----------------------------------------------*/

    /*----------------------------------------------*
     * 宏定义                                       *
     *----------------------------------------------*/

    /* 预置位自动归位结构 */
    typedef struct  _preset_auto_back_t
    {
        unsigned int uDeviceIndex;                        /* 设备索引*/
        unsigned int uPresetID;                           /* 预置位ID */
        int iDurationTime;                                /* 自动归位时间 */
        int iDurationTimeCount;                           /* 自动归位时间计数 */
        int iStatus;                                      /* 是否需要自动归位:0:否，1:是 */
    } preset_auto_back_t;

    /* 预置位自动归位队列*/
    typedef struct _preset_auto_back_list_t
    {
        osip_list_t* preset_auto_back_list;

        /* the lock */
#ifdef MULTI_THR
        osip_mutex_t*  lock;
#endif

    } preset_auto_back_list_t;

    /* 待处理预置位自动归位队列 */
    typedef std::deque <preset_auto_back_t*> needtoproc_preset_auto_back_queue;


    /* 点位自动解锁结构 */
    typedef struct  _device_auto_unlock_t
    {
        unsigned int uDeviceIndex;                        /* 设备索引*/
        int iDurationTimeCount;                           /* 自动解锁时间计数 */
        int iStatus;                                      /* 是否需要自动解锁:0:否，1:是 */
    } device_auto_unlock_t;

    /* 点位自动解锁队列*/
    typedef struct _device_auto_unlock_list_t
    {
        osip_list_t* device_auto_unlock_list;

        /* the lock */
#ifdef MULTI_THR
        osip_mutex_t*  lock;
#endif

    } device_auto_unlock_list_t;

    /* 待处理点位自动解锁队列 */
    typedef std::deque <device_auto_unlock_t*> needtoproc_device_auto_unlock_queue;

    extern device_auto_unlock_t* device_auto_unlock_find(unsigned int uDeviceIndex);
    extern device_auto_unlock_t* device_auto_unlock_find2(unsigned int uDeviceIndex);
    extern void device_auto_unlock_free(device_auto_unlock_t* node);
    extern int device_auto_unlock_init(device_auto_unlock_t** node);
    extern int device_auto_unlock_list_clean();
    extern void device_auto_unlock_list_free();
    extern int device_auto_unlock_list_init();
    extern int device_auto_unlock_proc(unsigned int uDeviceIndex);
    extern int device_auto_unlock_remove(unsigned int uDeviceIndex);
    extern int device_auto_unlock_update(unsigned int uDeviceIndex);
    extern int device_auto_unlock_use(unsigned int uDeviceIndex);
    extern preset_auto_back_t* preset_auto_back_find(unsigned int uDeviceIndex);
    extern preset_auto_back_t* preset_auto_back_find2(unsigned int uDeviceIndex);
    extern void preset_auto_back_free(preset_auto_back_t* node);
    extern int preset_auto_back_init(preset_auto_back_t** node);
    extern int preset_auto_back_list_clean();
    extern void preset_auto_back_list_free();
    extern int preset_auto_back_list_init();
    extern int preset_auto_back_proc(unsigned int uDeviceIndex, unsigned int uPresetID);
    extern int preset_auto_back_remove(unsigned int uDeviceIndex);
    extern int preset_auto_back_update(unsigned int uDeviceIndex);
    extern int preset_auto_back_use(unsigned int uDeviceIndex, unsigned int uPresetID, int iDurationTime);
    extern void scan_device_auto_unlock_list();
    extern void scan_preset_auto_back_list();
    extern void ShowDeviceAutoUnLockTaskInfo(int sock);
    extern void ShowPresetAutoBackTaskInfo(int sock);

#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* __cplusplus */


#endif /* __UDP_TL_H__ */
