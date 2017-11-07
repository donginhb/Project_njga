/* 
 * File:   CPing.h
 * Author: jaylong35
 *
 * Created on 2011年1月26日, 下午3:12
 */
#ifndef CPING_H
#define	CPING_H
#include <string>
#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>
#include <sys/time.h>
using namespace std;
  
#define PACKET_SIZE     4096
#define SEND_DATA_LEN   56
#define ERROR           -1
#define SUCCESS         1
#define MAX_WAIT_TIME   1
#define MAX_NO_PACKETS  100
class CPing
{
public:
    CPing(const char * ip, int timeout);
    CPing(const CPing& orig);
    virtual ~CPing();
private:
    std::string m_strIp;
    int m_nTimeOut;
    int m_nPkgLen;
    double m_dAvgTime;
    double m_dFasterResponseTime;
    double m_dLowerResponseTime;
    double m_dTotalResponseTimes;
    int m_nSend;
    int m_nRecv;
    int m_nSocketfd;
    pid_t m_Pid;
    struct sockaddr_in m_dest_addr;
    struct sockaddr_in m_from;
    char m_sendpacket[PACKET_SIZE];
    char m_recvpacket[PACKET_SIZE];
    struct timeval m_tvrecv;
public:
    enum
    {
        PING_FAILED,
        PING_SUCCEED
    };
    std::string GetIp() { return m_strIp; }
    int GetTimeOut() { return m_nTimeOut; }
    int GetPkgLen() { return m_nPkgLen; }
    void SetIp(const char * ip) { m_strIp = ip; }
    void SetTimeOut(int timeout) { m_nTimeOut = timeout; }
    void SetPkgLen(int pkglen) { m_nPkgLen = pkglen; }
    double GetAvgResponseTime() { return m_dAvgTime; }
    double GetFasterResponseTime() { return m_dFasterResponseTime; }
    double GetLowerResponseTime() { return m_dLowerResponseTime; }
    unsigned int GetPingStatus();
    static unsigned short cal_chksum(unsigned short *addr, int len);
    void statistics();
    int pack(int pack_no);
    void send_packet(int num);
    void recv_packet(void);
    int unpack(char *buf, int len);
    void tv_sub(struct timeval *out, struct timeval *in);
    bool ping(int times);
};
#endif	/* CPING_H */



