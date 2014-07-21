// packet.h : statement of the TCP module
//

#if !defined(AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A58__INCLUDED_)
#define AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A58__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



/////////////////////////////////////////
//自定义变量
//////////////////////////////////////////



///
//接收包模块中相关的变量
///

# include <winioctl.h>
# include "winsvc.h"
# include "commen.h"
# include <process.h>
//
//接收包相关的变量
//
     

//整个包的信息，包括数据部分
typedef struct _PACKET          
{
	unsigned char* pPacketContent;
	UINT   PacketLenth;
} PACKET, *P_PACKET;


//用C++的类思想构造一个队列，盛放收上来的包
class PACKETQUEUE
{
public:
	PACKETQUEUE(long=1024);
	~PACKETQUEUE(){delete []Packets;}
	BOOL EnQueue(const PACKET & Packet);
	P_PACKET DeQueue();
	void MakeEmpty(){front=rear=0;}
	int IsEmpty() const {return front==rear;}
	int IsFull() const {return (rear+1)%maxSize == front;}
	int Length() const {return (rear-front+maxSize)%maxSize;}
private:
	int rear,front;
    P_PACKET Packets;
	int maxSize;
};	

#define MY_MTU                   (1024)      //包的最大长度
#define MAXQUEUELEN              (1024*8)    //存放包的队列的最大长度


//IP包头封装格式
typedef struct _IP_HEADER
{
	unsigned char Ver_IHL;				//暂时用一个字节存储下面的4版本号和4头长度
	//unsigned char Version;			//4版本号
	//unsigned char IpHeaderLen;		//4头长度
	unsigned char TOS;					//8服务类型
	unsigned char IpLen[2];				//16总长度
	unsigned char Id[2];				//16标记域
	unsigned char Flag_Frag[2];			//暂时用2个字节存储下面的3分片标志和13分片偏移
	//unsigned char IpFlag;				//3分片标志
	//unsigned char IpFragment[2];		//13分片偏移
	unsigned char TTL;					//8生存时间
	unsigned char IpType;				//8协议类型
	unsigned char CheckSum[2];			//16校验和
	unsigned char SrcIpAddr[4];			//32源IP地址
	unsigned char DestIpAddr[4];		//32目的IP地址
}IP_HEADER, * P_IP_HEADER;

//TCP包头封装格式
typedef struct _TCP_HEADER
{
	unsigned char SrcPort[2];			//16位远端口号
	unsigned char DestPort[2];			//16位目的端口号
	unsigned char ISN[4];		        //32位序号
	unsigned char ACK[4];			    //32位确认序号
    unsigned char TcpLen;               //4位首部长度+4位保留
	unsigned char MARK;                 //2位保留+6位标志
	unsigned char Window[2];            //16位窗口大小
	unsigned char CheckSum[2];          //16位校验和
	unsigned char UrgenPointer[2];      //16位紧急指针
}TCP_HEADER, * P_TCP_HEADER;


# define  MYSELF_SIG1      128
# define  MYSELF_SIG2      151
# define  MYSELF_SIG3      23
# define  MYSELF_SIG4      79
	

//自定义TCP数据部分，用于标识
typedef struct _TCP_DATA_IDENTIFIER
{
	unsigned char Myself[4];            //32位用于标识自己包
	unsigned char TcbNum[4];            //32位用于标识TCB号
	unsigned char RemoteTcbNum[4];      //32位用于标识对方TCB号
	unsigned char Comm[2];              //16位用于标识命令
	unsigned char CurStatus[2];         //16位用于标识当前TCB状态
}TCP_DATA_IDENTIFIER, *P_TCP_DATA_IDENTIFIER;

//自定义TCP数据部分的附加字段，用于传递信息
typedef struct _TCP_DATA_APPENDIX
{
	unsigned char SubIpAddress[4];      //32位用于标识子网内部IP地址
	unsigned char DataLen[2];           //16位用于指示数据长度
	unsigned char FileSize[4];          //32位用于指示传递的文件的大小
}TCP_DATA_APPENDIX, *P_TCP_DATA_APPENDIX;

//自定义TCP数据部分，用于尾部标记自己包
typedef struct _TCP_DATA_TAIL_IDENTIFIER
{
	unsigned char Myself[3];            //24位用于标记自己包
}TCP_DATA_TAIL_IDENTIFIER, *P_TCP_DATA_TAIL_IDENTIFIER;

//TCP伪头部的封装格式
typedef struct _TEMP_TCP_HEADER
{
	unsigned char SrcIpAddr[4];
	unsigned char DestIpAddr[4];
	unsigned char Reserved;
	unsigned char nProtocolType;
	unsigned char TcpLen[2];
}TEMP_TCP_HEADER, * P_TEMP_TCP_HEADER;

//控制IO参数
#define IO_RECEIVE_PACKET		CTL_CODE(FILE_DEVICE_UNKNOWN, 0x925, METHOD_BUFFERED, FILE_ANY_ACCESS)
#define IO_SEND_PACKET			CTL_CODE(FILE_DEVICE_UNKNOWN, 0x926, METHOD_BUFFERED, FILE_ANY_ACCESS) 

///
//发送包模块中相关的变量
///

# define MAX_SEND_IPPKTLEN          1024         //发送IP封包时的包的最大长度，不包括数据链路部分
# define PACKETNUM                  2048         //发送封包个数         


//
//接收模块函数
//

//接收线程,入队列
/*UINT*/VOID
ReceiveThreadFunction(void*);
//解析包线程，出队列
/*UINT*/VOID
DecodeThreadFunction(void*);

//包解析的入口
void DecodePacket(UCHAR* pPacketContent,UINT len);


//
//发送模块函数
//

//得到本机IP地址,并将其保存到全局变量中
int GetSrcIpAddr();

//转换短整型字节顺序
USHORT Lhtons(USHORT hostshort);

//计算IP校验和
USHORT CheckSumFunc(USHORT *pAddr,int len);

//计算TCP校验和
USHORT UdpOrTcpCheckSumFunc(PUCHAR pIPBuf, int size);

//发送TCP包
BOOL SendTcpPacket(USHORT SrcPort, USHORT DstPort, PUCHAR DstIpAddr, PUCHAR Data, USHORT DataLen, UINT FileSize, UINT Isn, UINT Ack, UCHAR Mark, USHORT Window, PUCHAR Comm, PUCHAR CurStatus, UINT TcbNum, UINT RemoteTcbNum);


//从包中得到SRC IP信息
PUCHAR GetSrcIpFromPacket(PUCHAR packet);

//从包中得到SubIP信息
PUCHAR GetSubIPFromPacket(PUCHAR packet);

//从包中得到远程端口信息
USHORT GetDstPortFromPacket(PUCHAR packet);

//从包中得到本地端口信息
USHORT GetSrcPortFromPacket(PUCHAR packet);

//从包中得到ACK NUM信息
UINT GetAckNumFromPacket(PUCHAR packet);

//从包中得到窗口大小信息
USHORT GetWindowSizeFromPacket(PUCHAR packet);

//从包中得到ISN信息
UINT GetIsnFromPacket(PUCHAR packet);

//从包中得到数据长度信息
USHORT GetDataLenFromPacket(PUCHAR packet);

//从包中得到数据信息
PUCHAR GetDataFromPacket(PUCHAR packet);

//从包中得到文件大小信息
UINT GetFileSizeFromPacket(PUCHAR packet);

//从包中得到数据标识信息
P_TCP_DATA_IDENTIFIER GetIdentifierFromPacket(PUCHAR packet);

//从包中得到数据标识信息
P_TCP_DATA_TAIL_IDENTIFIER GetTailIdentifierFromPacket(PUCHAR packet);

//从包中得到数据附加信息
P_TCP_DATA_APPENDIX GetAppendixFromPacket(PUCHAR packet);

#endif // !defined(AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A55__INCLUDED_)

