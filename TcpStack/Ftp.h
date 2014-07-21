// Ftp.h : statement of the TCP module
//

#if !defined(AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A56__INCLUDED_)
#define AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A56__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000



/////////////////////////////////////////
//自定义变量
//////////////////////////////////////////

#include "packet.h"
#include "Mac.h"
#include "Tcp.h"

//设备句柄以及是否继续运行标志
typedef struct _LISTENER_INFO
{
	HANDLE DebugPrintDriver;
	HANDLE ReceiveEvent;
	HANDLE DecodeEvent;
	HANDLE DecodeWaiter;
	bool   ReceiveKeepGoing;
	bool   DecodeKeepGoing;
} LISTENER_INFO, *PLISTENER_INFO;


//
//定义全局变量
//
typedef struct _GLOBAL
{
	LISTENER_INFO*   ListenerInfo;         //全局变量
    CRITICAL_SECTION cs_pq;                //用于保护堆缓冲区的临界区声明
    CRITICAL_SECTION cs_ConnList;          //用于保护连接的临界区声明
    CRITICAL_SECTION cs_TcbList;           //用于保护Tcb的临界区声明

    PACKETQUEUE*     PacketQueue;          //全局变量,接收包队列

	unsigned char SrcIpAddr[4];            //源IP地址

	BOOL     HasStarted;                   //用于指示设备是否已启动成功
	BOOL     HasReceived;                  //用于指示是否已经读取数据

	Mac*     mac;                          //MAC模块类指针

	CONNECTIONLIST * ConnList;             //连接个数列表，这个是应用程序与DLL共享的

	UINT GlobalTcbNum;                     //全局TCB号
    
} GLOBAL, *PGLOBAL;  

//全局变量
extern GLOBAL      Global;


//
//自定义函数
//

BOOL InstallTheDriver();                                   //安装驱动
BOOL CreateWorkDevice();                                   //创建设备句柄      
VOID StartReceive();                                       //开始接收数据 


//FTP控制中心
void FtpControlCenter(int Type, char* pszAppendix, USHORT SzLen, char* SelfPath, char* RemotePath, UINT FileSize, PUCHAR IP, PUCHAR SubIP, USHORT SrcPort, USHORT DstPort, UINT RemoteTcbNum, int CurStatus);                   
                   

//处理各种操作的线程
/*UINT*/VOID
ControlThreadFunction(void*);

//
//共用变量及函数
//

# define     InstallExeFile      "IGN SIGN.exe"
# define     DRIVERNAME          "\\\\.\\Passthru"

//出错处理及显示
VOID ErrMsg (HRESULT hr,
             LPCWSTR  lpFmt,
             ...);


VOID ErrPtf (char* str, UINT ISN, UINT ACKNUM, UINT CURISN, USHORT WINDOW, UINT TIME);


//安装

#include <stdio.h>
#include <stdlib.h>
#include <windows.h>
#include <wchar.h>
#include "netcfgx.h"
#include "netcfgn.h"
#include <setupapi.h>
#include <devguid.h>
#include <objbase.h>

#include <windowsx.h>
#include <commctrl.h>        // For common controls, e.g. Tree


#define LOCK_TIME_OUT     5000
#define APP_NAME          L"BindView"

VOID ReleaseRef (IN IUnknown* punk);

HRESULT GetKeyValue (HINF hInf,
                     LPCWSTR lpszSection,
                     LPCWSTR lpszKey,
                     DWORD  dwIndex,
                     LPWSTR *lppszValue);

HRESULT GetPnpID (LPWSTR lpszInfFile,
                  LPWSTR *lppszPnpID);

HRESULT HrGetINetCfg (IN BOOL fGetWriteLock,
                      IN LPCWSTR lpszAppName,
                      OUT INetCfg** ppnc,
                      OUT LPWSTR *lpszLockedBy);

HRESULT HrReleaseINetCfg (IN INetCfg* pnc,
                          IN BOOL fHasWriteLock);

HRESULT HrInstallComponent(IN INetCfg* pnc,
                           IN LPCWSTR szComponentId,
                           IN const GUID* pguidClass);

HRESULT HrInstallNetComponent (IN INetCfg *pnc,
                               IN LPCWSTR lpszComponentId,
                               IN const GUID    *pguidClass,
                               IN LPCWSTR lpszInfFullPath);

HRESULT InstallSpecifiedComponent (LPWSTR lpszInfFile,
                                   LPWSTR lpszPnpID,
                                   const GUID *pguidClass);




#endif // !defined(AFX_NNN_H__9226F720_F847_4D0A_B812_6866434E5A55__INCLUDED_)

