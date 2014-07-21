// Tcp.h : statement of the TCP module
//

#if !defined(AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A59__INCLUDED_)
#define AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A59__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000


#include "mmTimers.h"
# include <sys/stat.h>
/////////////////////////////////////////
//自定义变量
//////////////////////////////////////////


# define    TIME_RCV_DELAY       1000       //用于定时处理函数，处理接收延迟发送确认
# define    RETRAN_COUNT         3          //重传次数
# define    INIT_WIN_SIZE        1          //初始发送时窗口大小
# define    INIT_RETRAN_TIME     9000       //初始发送时重传时间
# define    ALPHA                0.8        //用于计算重传时间的参数
# define    PerPacketSize        800        //每个包的大小

 
class MyTimers;                    //声明MyTimers类

class CONNECTION;                  //声明CONNECTION类

class TCBLIST;                     //声明TCB 链表，每个TCB表示一个连接

//TCB,TCP控制包,将其封装成类
class TCB
{
	friend class TCBLIST;          //声明友元类
public:

	//基本类成员函数
	TCB();
	TCB(UINT TcbNum);
	
	~TCB();
	TCB* NextTCB() {return link;}     //当前节点的下一个节点
	void InsertAfterSelf(TCB*);       //在当前节点后插入一个新节点
	TCB* RemoveAfterSelf();           //删除当前节点的下一个节点
	
	//与状态图转换有关的函数
    void Dispatch(int InputType, PUCHAR packet);           //状态转换的主控函数
	
	BOOL DealWithInput(int InputType, PUCHAR packet);      //输入处理函数
	BOOL DealWithOutput(int OutputType);                   //输出处理函数

	void Quit();                                           //退出
	
//private:
public:
	
	//输入控制结构
	typedef struct _INPUT_STRUCT
	{
        int   InputType;                     //输入类型
		int   LastInputType;                 //上次的输入类型，用于命令发送超时时的重发
		FILE  *TransFile;                    //要传送的文件指针
		char  SelfDirect[MAX_PATH];          //本方当前路径
		char  RemoteDirect[MAX_PATH];        //对方当前路径
		char  ParaInfo[MAX_PATH];            //参数信息，如文件名
		USHORT ParaLen;                      //参数信息长度
		UINT  m_MaxSendSize;                 //要传送的文件长度
		UCHAR InputBuffer[PerPacketSize];    //输入缓冲区
		USHORT BufferSize;                   //输入缓冲区大小
		CRITICAL_SECTION Cs_IBuffer;         //用于保护输入缓冲区的临界区声明
		HANDLE InputEvent;                   //等待事件，用于发送数据时等待确认
	} INPUT_STRUCT, *P_INPUT_STRUCT;

	//输出控制结构
	typedef struct _OUTPUT_STRUCT
	{
        int    OutputType;             //输出类型
		FILE   *RcvFile;               //接收文件
		//PUCHAR OutputBuffer;         //输出缓冲区
		//ULONG  BufferSize;           //输出缓冲区大小
		//CRITICAL_SECTION Cs_OBuffer; //用于保护输出缓冲区的临界区声明
	} OUTPUT_STRUCT, *P_OUTPUT_STRUCT;
	
	//接收相关控制变量
	typedef struct _RECEIVE_CONTROLER
	{
		UINT LastAckNum;              //接收当前确认号,这些都是文件偏移量
		UINT ISN;                     //接收序列号 
		UINT WindowSize;              //接收窗口大小
		UINT TempWinSize;             //用于计算已接收到的顺序来的包的个数
		UINT DelayTime;               //延迟发送确认时间
	}RECEIVE_CONTROLER, *P_RECEIVE_CONTROLER;

	//发送相关控制变量
    typedef struct _SEND_CONTROLER
	{
		UINT ISN;                     //发送当前序列号,这些都是文件偏移量
		UINT CurAckNum;               //发送当前确认号
		UINT WindowSize;              //发送窗口大小
		UINT ReTranTime;              //发送重传时间
	}SEND_CONTROLER, *P_SEND_CONTROLER;

    INPUT_STRUCT InputStru;           //输入控制
    OUTPUT_STRUCT OutputStru;         //输出控制
    RECEIVE_CONTROLER RecCtrler;      //接收控制
    SEND_CONTROLER SendCtrler;        //发送控制

	CONNECTION*  m_Conn;              //回指指针，指向本操作属于的连结

	BOOL    m_bBlockAck;              //标识是否阻止ACK命令的到来
	BOOL    m_bBlockData;             //标识是否阻止DATA数据的到来

	BOOL    m_bQuit;                  //退出标志
	BOOL    m_bConnected;             //连接是否成功标识
	BOOL    m_bContinueSend;          //是否继续发送标识

	MyTimers *m_pMyStimer;            //用于发送超时的定时器
	MyTimers *m_pMyRtimer;            //用于接收延迟的定时器

    HANDLE  m_CommEvent;              //等待事件，用于和FTP的命令操作过程通信
	HANDLE  m_Comm_Data_Event;        //等待事件，用于命令与数据操作的同步

	UCHAR  m_IpAddr[4];               //IP地址
	UCHAR  m_SubIpAddr[4];            //内部IP地址
	USHORT m_SrcPort;                 //本地端口号
	USHORT m_DstPort;                 //远程端口号

	int   m_ReTranCount;              //重传次数
	DWORD dwStart;                    //计时开始
	
    int  CurStatus;                    //当前状态

	BOOL m_bCallTime;                 //计时标志
	BOOL m_bRcvDelay;                 //接收延迟标志

	UINT m_CurrISN;                   //当前发送的包的ISN

	UINT m_TcbNum;                    //TCB号
	UINT m_RemoteTcbNum;              //对方的TCB号
	TCB* link;                        //下一个节点指针
};

//TCB链表类
class TCBLIST
{
public:

	//基本类成员函数
	TCBLIST(){Last = First = new TCB();}
	~TCBLIST();
	void MakeEmpty();                        //将链表置为空表
	int Length() const;                      //计算链表的长度
	TCB* Find(UINT TcbNum);                  //根据TCB号寻找指定的TCB节点
	void AppendList(TCB *add);               //将新节点接到链表上
	void RemoveNode(TCB *tcb);               //将指定节点删除
	
private:
	TCB *First, *Last;
};


//对CMMTimers类的继承
class MyTimers : public CMMTimers
{
public:
	MyTimers(UINT x, TCB* tcb, BOOL IsSend);
	void timerProc();
protected:
	TCB *m_tcb;
	BOOL m_IsSend;
};


class CONNECTIONLIST;  //声明CONNECTION链表，每个CONNECTION表示一个建立起来的链接，每个链接对应一个IP

class CONNECTION
{
	friend class CONNECTIONLIST;                //声明友元类
	
public:

	//基本类成员函数
	CONNECTION();
	CONNECTION(UINT nIP, UINT nSubIP, USHORT SrcPort, USHORT DstPort);
	CONNECTION(PUCHAR puIP, PUCHAR puSubIP, USHORT SrcPort, USHORT DstPort);
	CONNECTION* NextConn() {return link;}      //当前节点的下一个节点
	void InsertAfterSelf(CONNECTION*);         //在当前节点后插入一个新节点
	CONNECTION* RemoveAfterSelf();             //删除当前节点的下一个节点

//private:
public:
	
	CONNECTION *link;

	BOOL   m_Active;                            //标识本连结是否处于活动状态

	//连结属性，可以用来标识一个连接

	UCHAR  m_IP[4];                             //本连结的IP,对方IP                     
    UCHAR  m_SubIP[4];                          //本连结的子网IP,对方的子网IP
	USHORT m_SrcPort;                           //源端口号
	USHORT m_DstPort;                           //目的端口号

	UINT   m_ConnRetranTime;                    //本连接的往返时间初始值
	    
	TCBLIST* TcbList;                           //TCB列表

	//连结参数，可以用来表示本次连结的状态以及参数信息

	char   m_SelfCurDirect[MAX_PATH];           //本方当前路径
	char   m_RemoteCurDirect[MAX_PATH];         //对方当前路径
	int  m_CurType;                             //本连结的当前状态
	char m_pszAppendix[MAX_PATH];               //操作附加信息
	USHORT m_SzLen;                             //附加信息长度
	UINT m_FileSize;                            //本次操作文件大小
	UINT m_TcbNum;                              //本地TCB号
	UINT m_RemoteTcbNum;                        //远程TCB号
	int  m_CurStatus;                           //当前TCB状态  
	
	CRITICAL_SECTION Cs_ConPara1;               //用于保护连结参数的临界区声明
	CRITICAL_SECTION Cs_ConPara2;               //用于保护连结参数的临界区声明
};

//CONNECTION链表类
class CONNECTIONLIST
{
public:

	//基本类成员函数
	CONNECTIONLIST(){Last = First = new CONNECTION();}
	~CONNECTIONLIST();
	void MakeEmpty();                                                 //将链表置为空表
	int Length() const;                                               //计算链表的长度
	CONNECTION* FindByIPAndSubIP(PUCHAR puIP, PUCHAR puSubIP);        //根据IP和SubIP寻找指定节点
	void AppendList(CONNECTION *add);                                 //将新节点接到链表上
	void RemoveNode(CONNECTION *del);                                 //将指定节点删除
 
private:
	CONNECTION *First, *Last;
};


#endif // !defined(AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A55__INCLUDED_)

