/*
 * File:   CPing.cpp
 * Author: jaylong35
 *
 * Created on 2011年1月26日, 下午3:12
 */
#include "CPing.h"
#include "string.h"
#include<stdio.h>
#include<stdlib.h>

CPing::CPing(const char* ip, int timeout)
{
    m_strIp = ip;
    m_nTimeOut = timeout;
    m_nSend = 0;
    m_nRecv = 0;
    m_nSocketfd = 0;
}
CPing::CPing(const CPing& orig)
{
}
CPing::~CPing()
{
}
bool CPing::ping(int times)
{
    struct hostent* host;
    struct protoent* protocol;
    unsigned int inaddr = 0l;
    int size = 50 * 1024;

    if ((protocol = getprotobyname("icmp")) == NULL)
    {
        perror("getprotobyname");
        return 0;
    }

    /*生成使用ICMP的原始套接字,这种套接字只有root才能生成*/
    if ((m_nSocketfd = socket(AF_INET, SOCK_RAW, protocol->p_proto)) < 0)
    {
        perror("socket error");
        return 0;
    }

    /* 回收root权限,设置当前用户权限*/
    setuid(getuid());
    /*扩大套接字接收缓冲区到50K这样做主要为了减小接收缓冲区溢出的
      的可能性,若无意中ping一个广播地址或多播地址,将会引来大量应答*/
    setsockopt(m_nSocketfd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));



    struct timeval iTimeout;
    iTimeout.tv_sec = 5;
    iTimeout.tv_usec = 0;
    setsockopt(m_nSocketfd,SOL_SOCKET,SO_RCVTIMEO,(char *)&iTimeout,sizeof(struct timeval));



    
    memset(&m_dest_addr, 0, sizeof(m_dest_addr));
    m_dest_addr.sin_family = AF_INET;

    /*判断是主机名还是ip地址*/
    if ((inaddr = inet_addr(m_strIp.c_str())) == INADDR_NONE)
    {
        if ((host = gethostbyname(m_strIp.c_str())) == NULL) /*是主机名*/
        {
            perror("gethostbyname error");
            return 0;
        }

        memcpy((char*) &m_dest_addr.sin_addr, host->h_addr, host->h_length);
    }
    else /*是ip地址*/
    {
        memcpy((char*) &m_dest_addr.sin_addr, (char*) &inaddr, sizeof(unsigned int));
    }

    /*获取main的进程id,用于设置ICMP的标志符*/
    m_Pid = getpid();
    printf("\nPING %s(%s): %d bytes data in ICMP packets.\n", m_strIp.c_str(),
           inet_ntoa(m_dest_addr.sin_addr), SEND_DATA_LEN);
    int i = 0;


    while (i < times)
    {
        i++;
        send_packet(1); /*发送所有ICMP报文*/
        recv_packet(); /*接收所有ICMP报文*/
    }


    statistics(); /*进行统计*/

    
    return 1;
}
unsigned short CPing::cal_chksum(unsigned short* addr, int len)
{
    int nleft = len;
    int sum = 0;
    unsigned short* w = addr;
    unsigned short answer = 0;

    while (nleft > 1)
    {
        sum += *w++;
        nleft -= 2;
    }

    if (nleft == 1)
    {
        *(unsigned char*)(&answer) = *(unsigned char*)w;
        sum += answer;
    }

    sum = (sum >> 16) + (sum & 0xffff);
    sum += (sum >> 16);
    answer = ~sum;
    return answer;
}
void CPing::tv_sub(struct timeval* out, struct timeval* in)
{
    if ((out->tv_usec -= in->tv_usec) < 0)
    {
        --out->tv_sec;
        out->tv_usec += 1000000;
    }

    out->tv_sec -= in->tv_sec;
}
void CPing::statistics()
{
    printf("\n--------------------PING statistics-------------------\n");
    printf("\n%d packets transmitted, %d received , %%%d lost\n", m_nSend, m_nRecv,
           (m_nSend - m_nRecv) / m_nSend * 100);
    close(m_nSocketfd);
    m_nTimeOut = m_nSend - m_nRecv;
    m_dAvgTime = m_dTotalResponseTimes / m_nRecv;
    return;
}
/*设置ICMP报头*/
int CPing::pack(int pack_no)
{
    int packsize;
    struct icmp* icmp;
    struct timeval* tval;
    icmp = (struct icmp*) m_sendpacket;
    icmp->icmp_type = ICMP_ECHO;
    icmp->icmp_code = 0;
    icmp->icmp_cksum = 0;
    icmp->icmp_seq = pack_no;
    icmp->icmp_id = m_Pid;
    packsize = 8 + SEND_DATA_LEN;
    tval = (struct timeval*) icmp->icmp_data;
    gettimeofday(tval, NULL); /*记录发送时间*/
    icmp->icmp_cksum = cal_chksum((unsigned short*) icmp, packsize);  /*校验算法*/
    return packsize;
}
/*发送三个ICMP报文*/
void CPing::send_packet(int num)
{
    if (num > MAX_NO_PACKETS)
    {
        num = MAX_NO_PACKETS;
    }

    int packetsize;
    int i = 0;

    while (i < num)
    {
        i++;
        m_nSend++;
        packetsize = pack(m_nSend); /*设置ICMP报头*/

        if (sendto(m_nSocketfd, m_sendpacket, packetsize, 0,
                   (struct sockaddr*) &m_dest_addr, sizeof(m_dest_addr)) < 0)
        {
            perror("sendto error");
            continue;
        }

        usleep(10); /*每隔一秒发送一个ICMP报文*/
    }
}
/*接收所有ICMP报文*/
void CPing::recv_packet()
{
    int n, fromlen;
    struct timeval intime, outtime;

    //signal(SIGALRM, statistics);

    fromlen = sizeof(m_from);

    gettimeofday(&intime, NULL);


    while (m_nRecv < m_nSend)
    {
        //alarm(MAX_WAIT_TIME);
        gettimeofday(&outtime, NULL);
        tv_sub(&outtime, &intime);

        if ((outtime.tv_sec * 1000  +  outtime.tv_usec / 1000) > 1000)
        {
            break;
        }


        if ((n = recvfrom(m_nSocketfd, m_recvpacket, sizeof(m_recvpacket), 0,
                          (struct sockaddr*) &m_from, (socklen_t*)&fromlen)) < 0)
        {
            if (errno == EINTR)
            {
                continue;
            }

            perror("recvfrom error");
            continue;
        }

        gettimeofday(&m_tvrecv, NULL); /*记录接收时间*/

        if (unpack(m_recvpacket, n) == -1)
        {
            continue;
        }

        m_nRecv++;
    }
}
/*剥去ICMP报头*/
int CPing::unpack(char* buf, int len)
{
    int iphdrlen;
    struct ip* ip;
    struct icmp* icmp;
    struct timeval* tvsend;
    double rtt;
    ip = (struct ip*) buf;
    iphdrlen = ip->ip_hl << 2; /*求ip报头长度,即ip报头的长度标志乘4*/
    icmp = (struct icmp*)(buf + iphdrlen);   /*越过ip报头,指向ICMP报头*/
    len -= iphdrlen; /*ICMP报头及ICMP数据报的总长度*/

    if (len < 8) /*小于ICMP报头长度则不合理*/
    {
        printf("ICMP packets/'s length is less than 8/n");
        return -1;
    }

    /*确保所接收的是我所发的的ICMP的回应*/
    if ((icmp->icmp_type == ICMP_ECHOREPLY) && (icmp->icmp_id == m_Pid))
    {
        tvsend = (struct timeval*) icmp->icmp_data;
        tv_sub(&m_tvrecv, tvsend); /*接收和发送的时间差*/
        rtt = m_tvrecv.tv_sec * 1000 + m_tvrecv.tv_usec / 1000; /*以毫秒为单位计算rtt*/
        m_dTotalResponseTimes += rtt;

        if (m_dFasterResponseTime == -1)
        {
            m_dFasterResponseTime = rtt;
        }
        else if (m_dFasterResponseTime > rtt)
        {
            m_dFasterResponseTime = rtt;
        }

        if (m_dLowerResponseTime == -1)
        {
            m_dLowerResponseTime = rtt;
        }
        else if (m_dLowerResponseTime < rtt)
        {
            m_dLowerResponseTime = rtt;
        }

        /*显示相关信息*/
        printf("\n%d/tbyte from %s/t: icmp_seq=%u/tttl=%d/trtt=%.3f/tms\n",
               len,
               inet_ntoa(m_from.sin_addr),
               icmp->icmp_seq,
               ip->ip_ttl,
               rtt);
    }
    else
    {
        return -1;
    }

	return 0;
}