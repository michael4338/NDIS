// Mac.h: interface for the Mac class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_MAC_H__1B8C02DE_BA6F_4D14_8CB0_14EA67714C2F__INCLUDED_)
#define AFX_MAC_H__1B8C02DE_BA6F_4D14_8CB0_14EA67714C2F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000





#include <winioctl.h>
#include "winsvc.h"

//定义与发送相关的量
#define IO_SEND_PACKET_MAC			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x926, METHOD_BUFFERED, FILE_ANY_ACCESS) 
#define IO_GET_SRCMAC_MAC           CTL_CODE(FILE_DEVICE_UNKNOWN, 0x927, METHOD_BUFFERED, FILE_ANY_ACCESS)

# define MAC_ADDR_LEN               6            //查询的MAC地址的长度 
# define OID_802_3_CURRENT_ADDRESS  128          //代表查询 以太网/802_3 的源MAC地址


# define MAX_SEND_ARPPKTLEN         60           //发送ARP封包时的最大长度，包括包头部分
# define  _IP_MAC_LEN               0xFF         //IP地址与MAC地址的对应表的长度

class Mac  
{
public:
	Mac();
	virtual ~Mac();

    /////////////////////////////////////////
    //自定义变量
    //////////////////////////////////////////
private:
    //以太网封装格式
    typedef struct _ETHERNET_HEADER
	{
		unsigned char DestMac[6];
	    unsigned char SrcMac[6];
	    unsigned char EthType[2];
	}ETH_HEADER, * P_ETH_HEADER;

    //ARP包头格式
    typedef struct _ARP_HEADER
	{
	    unsigned char HardWareType[2];
	    unsigned char ProtocolType[2];
	    unsigned char HardWareAddrLen;
	    unsigned char ProtocolAddrLen;
	    unsigned char Operation[2];
	    unsigned char SrcMacAddr[6];
	    unsigned char SrcIpAddr[4];
	    unsigned char DestMacAddr[6];
	    unsigned char DestIpAddr[4];
	}ARP_HEADER,* P_ARP_HEADER;
    
    typedef struct _IP_MAC
	{
	    unsigned char IP[4];
	    unsigned char MAC[6];
	}IP_MAC;
    IP_MAC IP_MAC_STABLE[_IP_MAC_LEN];
    int CUR_IP_MAC_LEN;

    //当前目的IP地址，根据它来发ARP包
    unsigned char DestIpAddr[4];

    //源IP地址，保存它用来判断是否为同一网段
    unsigned char SrcIpAddr[4];

    //源MAC地址
    unsigned char SrcMacAddr[6];

    //当前目的MAC地址
    unsigned char DestMacAddr[6];

    //网关MAC地址
    unsigned char GateWayMacAddr[6];

    //子网掩码
    unsigned char SubNetMask[4];

    //用于指示是否已成功利用ARP请求得到目的MAC
    HANDLE   ArpMacEvent;   

    //查询信息结构
    typedef struct _SECLAB_QUERY_OID
	{
    //NDIS_OID        Oid;
	ULONG             Oid;
    UCHAR             Data[sizeof(ULONG)];
	} SECLAB_QUERY_OID, *PSECLAB_QUERY_OID;

public:

    //解析以太网包头，对外接口1
    BOOL  DecodeMac(unsigned char* &packet);

    //为包加上以太网包头，对外接口2
    BOOL  SendWithMac(unsigned char* &packet, int PacketLen, HANDLE DriverHandle);

    //初始化MAC模块
    void  InitMac();

    //从Arp应答包中找出目标地址
    BOOL  GetMacFromArpPacket(unsigned char *packet);

    //得到子网掩码
    int   GetSubNetMask();

    //得到本地MAC地址
    int   GetSrcMac(HANDLE DriverHandle);

    //从数据包中查找本地和远程IP地址
    void  GetSrcAndDestIpAddr(unsigned char *packet);

    //判断当前通信双方是否在同一网段内
    BOOL  BothInLocalNetwork();

    //判断变量是否已经被初始化
    BOOL IsEmpty(unsigned char *s, int len);

    //判断两个地址信息是否相同
    BOOL IsSame(unsigned char *p, unsigned char *q, int len);

    //检查对应表中是否有当前通信双方的对应信息
    BOOL CheckRecord();

    //往对应表里加入当前通信双方信息
    void AddToRecord();

    //发ARP包以得到MAC地址
    BOOL SendArpToGetMac(HANDLE DriverHandle);

};




#endif // !defined(AFX_MAC_H__1B8C02DE_BA6F_4D14_8CB0_14EA67714C2F__INCLUDED_)
