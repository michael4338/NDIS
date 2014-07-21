// Ftp.cpp : Defines the class behaviors for the TCP Module
//

#include "stdafx.h"
#include "TcpStack.h"

#include "Ftp.h"

#include <process.h>

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif



//全局变量
GLOBAL         Global = {0};


//
//安装
//
BOOL  InstallTheDriver()
{
	char     Directory[MAX_PATH];
    DWORD    DircLen = MAX_PATH;
    
	memset(Directory,0,MAX_PATH);
	
	if(!GetCurrentDirectory(DircLen , Directory))
	{
		//注意这种错误处理方法
		ErrMsg(GetLastError(), L"CAN NOT GET CURRENT DIRECTORY");
		return FALSE;
	}
    
	//strcat(Directory , InstallExeFile);

	if( ShellExecute(NULL,NULL,InstallExeFile,NULL,Directory,SW_HIDE) <= (HINSTANCE)32)
	{
		ErrMsg(GetLastError(), L"CAN NOT GET CURRENT DIRECTORY");
		return FALSE;
	}

	return TRUE;
}

BOOL CreateWorkDevice()
{
	//初始化全局变量
	Global.ListenerInfo = new LISTENER_INFO;
	Global.ListenerInfo->DebugPrintDriver = NULL;
	Global.ListenerInfo->ReceiveKeepGoing = false;
	Global.ListenerInfo->DecodeKeepGoing = false;
	Global.ListenerInfo->ReceiveEvent = CreateEvent(NULL,TRUE,FALSE,NULL); //manual,clear it
	Global.ListenerInfo->DecodeEvent = CreateEvent(NULL,TRUE,FALSE,NULL); //manual,clear it
	Global.ListenerInfo->DecodeWaiter = CreateEvent(NULL,TRUE,FALSE,NULL); //manual,clear it
	Global.PacketQueue = new PACKETQUEUE(MAXQUEUELEN);

    Global.HasStarted = FALSE;
	Global.HasReceived = FALSE;               
	InitializeCriticalSection(&Global.cs_pq);
	InitializeCriticalSection(&Global.cs_ConnList);
	InitializeCriticalSection(&Global.cs_TcbList);

	if(!GetSrcIpAddr())
	{
		ErrMsg(0, L"CAN NOT GET SRCIPADDR , QUIT PLEASE");
		return FALSE;
	}

	Global.mac = new Mac();

	Global.ConnList = new CONNECTIONLIST();

	Global.GlobalTcbNum = 0;
	
	//创建设备
	Global.ListenerInfo->DebugPrintDriver = CreateFile(DRIVERNAME, 
		GENERIC_WRITE | GENERIC_READ, 
        0, 
        NULL, 
        OPEN_EXISTING, 
        0, 
        NULL); 

	if( Global.ListenerInfo->DebugPrintDriver != INVALID_HANDLE_VALUE ) 
	{ 
        //ErrMsg(0, L"SUCCEED TO CREATE THE DEVICE" ); 
		Global.HasStarted = TRUE;
	} 
    else 
	{ 
        ErrMsg(0, L"FAILED TO CREATE THE DEVICE" ); 
		return FALSE;
	}

	return TRUE;
}


VOID StartReceive()
{
	if(! Global.HasStarted)
	{
		ErrMsg(0, L"PLEASE RUN START FIRST");
		return;
	}

	Global.ListenerInfo->ReceiveKeepGoing = true;
	Global.ListenerInfo->DecodeKeepGoing = true;
	_beginthread(ReceiveThreadFunction,0,NULL);
	_beginthread(DecodeThreadFunction,0,NULL);
	Global.HasReceived = TRUE;
}

void FtpControlCenter(int Type, char* pszAppendix, USHORT SzLen, char* SelfPath, char* RemotePath, UINT FileSize, PUCHAR IP, PUCHAR SubIP, USHORT SrcPort, USHORT DstPort, UINT RemoteTcbNum, int CurStatus)
{
	//如果没有启动或接收，退出
	if(!Global.HasStarted || !Global.HasReceived)
	{
		ErrMsg(0, L"PLEASE RUN START AND RECEIVE FIRST");
		return;
	}

	//根据IP和SubIP判断是否存在已有的连接，有则得到此连接对象，无则继续判断是否为连接操作
	//是则继续，不是则退出
	EnterCriticalSection(&Global.cs_ConnList);
	CONNECTION* conn = Global.ConnList->FindByIPAndSubIP(IP, SubIP);
	LeaveCriticalSection(&Global.cs_ConnList);

	if(conn == NULL)
	{

/*      //用于两端均为内网地址的用户使用 
		if(Type != CONNECT_SENDING)
		{
			ErrMsg(0, L"THE CONNECTION DOES NOT EXIST");
			return;
		}
*/
		//用于至少有一端为外部IP地址的用户
		if(Type != CONNECT_SENDING && Type != CONNECT_F_COMMING)
		{
			ErrMsg(0, L"THE CONNECTION DOES NOT EXIST");
			return;
		}

/*      //用于两端均为内网地址的用户使用 
		else
		{
			//创建一个新的连结，并将其链接到CONNECTION链子上
		    conn = new CONNECTION(IP, SubIP, SrcPort, DstPort);

            EnterCriticalSection(&Global.cs_ConnList);
		    Global.ConnList->AppendList(conn);
			LeaveCriticalSection(&Global.cs_ConnList);
		}
*/
				
		//用于至少有一端为外部IP地址的用户
        else
		{
			//创建一个新的连结，并将其链接到CONNECTION链子上
		    conn = new CONNECTION(IP, SubIP, SrcPort, DstPort);

            EnterCriticalSection(&Global.cs_ConnList);
		    Global.ConnList->AppendList(conn);
			LeaveCriticalSection(&Global.cs_ConnList);

			if(Type == CONNECT_F_COMMING)
			{
				char uc2str[5];
				char OutIp[50] = "公网IP地址为：";
				char SubIp[50] = "子网IP地址为：";

				memset(uc2str,0,5);
				for(int i=0;i<4;i++)
				{
					sprintf(uc2str, "%d", IP[i]);
					strcat(strcat(OutIp, uc2str), "  ");
					sprintf(uc2str, "%d", SubIP[i]);
					strcat(strcat(SubIp, uc2str), "  ");
				}

				AfxMessageBox(strcat(strcat(OutIp, "\n"), SubIp));
				return;
			}
		}
	}

	EnterCriticalSection(&conn->Cs_ConPara1);

	if(pszAppendix != NULL && SzLen != 0)
		strcpy(conn->m_pszAppendix, pszAppendix);
	conn->m_SzLen = SzLen;
	conn->m_FileSize = FileSize;
	conn->m_CurType = Type;
	conn->m_RemoteTcbNum = RemoteTcbNum;
	conn->m_CurStatus = CurStatus;

	//暂时为之，以后需要修改
	if(SelfPath == NULL)
    	GetCurrentDirectory(MAX_PATH, conn->m_SelfCurDirect);
	else
		strcpy(conn->m_SelfCurDirect, SelfPath);

	if(RemotePath != NULL)
		strcpy(conn->m_RemoteCurDirect, RemotePath);

    //飞起一个线程，处理本次操作
	_beginthread(ControlThreadFunction, 0, (void*)conn);

	LeaveCriticalSection(&conn->Cs_ConPara1);
}


/*UINT*/VOID
ControlThreadFunction(void* Para)
{
	CONNECTION* pConn = (CONNECTION*) Para;

    EnterCriticalSection(&pConn->Cs_ConPara2);

	//创建一个崭新的TCB，用它来完成本次操作
	TCB * tcb = new TCB(Global.GlobalTcbNum ++);
	
	tcb->m_Conn = pConn;

	tcb->SendCtrler.ReTranTime = pConn->m_ConnRetranTime;

	memcpy(tcb->m_IpAddr, pConn->m_IP, 4);
	memcpy(tcb->m_SubIpAddr, pConn->m_SubIP, 4);
	tcb->m_SrcPort = pConn->m_SrcPort;
	tcb->m_DstPort = pConn->m_DstPort;
    int CurStatusType = pConn->m_CurType;
	tcb->CurStatus = pConn->m_CurStatus;
    memcpy(tcb->InputStru.ParaInfo, pConn->m_pszAppendix, pConn->m_SzLen);
	tcb->InputStru.ParaLen = pConn->m_SzLen;
	tcb->InputStru.m_MaxSendSize = pConn->m_FileSize;
	
	tcb->m_RemoteTcbNum = pConn->m_RemoteTcbNum;

	strcpy(tcb->InputStru.SelfDirect, pConn->m_SelfCurDirect);
	strcpy(tcb->InputStru.RemoteDirect, pConn->m_RemoteCurDirect);

	//将此TCB加入到TCBLIST中去
	EnterCriticalSection(&Global.cs_TcbList);
	pConn->TcbList->AppendList(tcb);
    LeaveCriticalSection(&Global.cs_TcbList);

	LeaveCriticalSection(&pConn->Cs_ConPara2);

    tcb->DealWithInput(CurStatusType, NULL);

	//将此TCB从TCBLIST中删除，并销毁
	//等待2个重传时间以防止有回应或其他操作再来，造成异常
	tcb->CurStatus = STATUS_CLOSED;
	Sleep(tcb->SendCtrler.ReTranTime * 2);
	EnterCriticalSection(&Global.cs_TcbList);
    pConn->TcbList->RemoveNode(tcb);
    LeaveCriticalSection(&Global.cs_TcbList);

	_endthread();
}



//
//错误处理及显示
//
VOID ErrMsg (HRESULT hr,
             LPCWSTR  lpFmt,
             ...)
{

    LPWSTR   lpSysMsg;
    WCHAR    buf[400];
    ULONG    offset;
    va_list  vArgList; 


    if ( hr != 0 ) {
        swprintf( buf,
                  L"Error %#lx: ",
                  hr );
    }
    else {
        buf[0] = 0;
    }

    offset = wcslen( buf );
  
    va_start( vArgList,
              lpFmt );
    vswprintf( buf+offset,
                lpFmt,
                vArgList );

    va_end( vArgList );

    if ( hr != 0 ) {
        FormatMessageW( FORMAT_MESSAGE_ALLOCATE_BUFFER |
                       FORMAT_MESSAGE_FROM_SYSTEM |
                       FORMAT_MESSAGE_IGNORE_INSERTS,
                       NULL,
                       hr,
                       MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                       (LPWSTR)&lpSysMsg,
                       0,
                       NULL );
        if ( lpSysMsg ) {

            offset = wcslen( buf );

            swprintf( buf+offset,
                      L"\n\nPossible cause:\n\n" );

            offset = wcslen( buf );

            wcscat( buf+offset,
                     lpSysMsg );

            LocalFree( (HLOCAL)lpSysMsg );
        }

        MessageBoxW( NULL,
                    buf,
                    L"Error",
                    MB_ICONERROR | MB_OK );
    }
    else {
        MessageBoxW( NULL,
                    buf,
                    L"TcpStack",
                    MB_ICONINFORMATION | MB_OK );
    }

    return;
}

//
//用于调试
//	
FILE* file = NULL;
BOOL FirstTime = TRUE;
BOOL LastTime = FALSE;
VOID ErrPtf (char* str, UINT ISN, UINT ACKNUM, UINT CURISN, USHORT WINDOW, UINT TIME)
{
	if(FirstTime == TRUE)
	{
		file=fopen("C:\\packet.txt","a+");
		char *strstr = "    过程  传送数量  回应数量  当前传送批次  窗口大小 重传时间 \n";
		fwrite(strstr, sizeof(char), strlen(strstr), file);
		FirstTime = FALSE;
    }	

	char Total[100];
	memset(Total, 0, 100);

	char IntToStr[10];

	memset(IntToStr, 0, 10);
	sprintf(IntToStr, "%d", ISN);
	strcat(strcat(strcat(Total, str),"       "), IntToStr);

	memset(IntToStr, 0, 10);
	sprintf(IntToStr, "%d", ACKNUM);
	strcat(strcat(Total, "       "), IntToStr);

	memset(IntToStr, 0, 10);
	sprintf(IntToStr, "%d", CURISN);
	strcat(strcat(Total, "       "), IntToStr);

	memset(IntToStr, 0, 10);
	sprintf(IntToStr, "%d", WINDOW);
	strcat(strcat(Total, "       "), IntToStr);

	memset(IntToStr, 0, 10);
	sprintf(IntToStr, "%d", TIME);
	strcat(strcat(strcat(Total, "       "), IntToStr), "\n");

	fwrite(Total, sizeof(char), strlen(Total), file);

	if(LastTime == TRUE)
	{
		fclose(file);
	}
}




//
//安装
//


VOID ReleaseRef (IN IUnknown* punk)
{
    if ( punk ) {
        punk->Release();
    }

    return;
}

HRESULT GetKeyValue (HINF hInf,
                     LPCWSTR lpszSection,
                     LPCWSTR lpszKey,
                     DWORD  dwIndex,
                     LPWSTR *lppszValue)
{
    INFCONTEXT  infCtx;
    DWORD       dwSizeNeeded;
    HRESULT     hr;

    *lppszValue = NULL;

    if ( SetupFindFirstLineW(hInf,
                             lpszSection,
                             lpszKey,
                             &infCtx) == FALSE )
    {
        return HRESULT_FROM_WIN32(GetLastError());
    }

    SetupGetStringFieldW( &infCtx,
                          dwIndex,
                          NULL,
                          0,
                          &dwSizeNeeded );

    *lppszValue = (LPWSTR)CoTaskMemAlloc( sizeof(WCHAR) * dwSizeNeeded );

    if ( !*lppszValue  )
    {
       return HRESULT_FROM_WIN32(ERROR_NOT_ENOUGH_MEMORY);
    }

    if ( SetupGetStringFieldW(&infCtx,
                              dwIndex,
                              *lppszValue,
                              dwSizeNeeded,
                              NULL) == FALSE )
    {

        hr = HRESULT_FROM_WIN32(GetLastError());

        CoTaskMemFree( *lppszValue );
        *lppszValue = NULL;
    }
    else
    {
        hr = S_OK;
    }

    return hr;
}

HRESULT GetPnpID (LPWSTR lpszInfFile,
                  LPWSTR *lppszPnpID)
{
    HINF    hInf;
    LPWSTR  lpszModelSection;
    HRESULT hr;

    *lppszPnpID = NULL;

    hInf = SetupOpenInfFileW( lpszInfFile,
                              NULL,
                              INF_STYLE_WIN4,
                              NULL );

    if ( hInf == INVALID_HANDLE_VALUE )
    {

        return HRESULT_FROM_WIN32(GetLastError());
    }

    //
    // Read the Model section name from Manufacturer section.
    //

    hr = GetKeyValue( hInf,
                      L"Manufacturer",
                      NULL,
                      1,
                      &lpszModelSection );

    if ( hr == S_OK )
    {

        //
        // Read PnpID from the Model section.
        //

        hr = GetKeyValue( hInf,
                          lpszModelSection,
                          NULL,
                          2,
                          lppszPnpID );

        CoTaskMemFree( lpszModelSection );
    }

    SetupCloseInfFile( hInf );

    return hr;
}

HRESULT HrGetINetCfg (IN BOOL fGetWriteLock,
                      IN LPCWSTR lpszAppName,
                      OUT INetCfg** ppnc,
                      OUT LPWSTR *lpszLockedBy)
{
    INetCfg      *pnc = NULL;
    INetCfgLock  *pncLock = NULL;
    HRESULT      hr = S_OK;

    //
    // Initialize the output parameters.
    //

    *ppnc = NULL;

    if ( lpszLockedBy )
    {
        *lpszLockedBy = NULL;
    }
    //
    // Initialize COM
    //

    hr = CoInitialize( NULL );

    if ( hr == S_OK ) {

        //
        // Create the object implementing INetCfg.
        //

        hr = CoCreateInstance( CLSID_CNetCfg,
                               NULL, 
							   CLSCTX_INPROC_SERVER,
                               IID_INetCfg,
                               (void**)&pnc );
        if ( hr == S_OK ) {

            if ( fGetWriteLock ) {

                //
                // Get the locking reference
                //

                hr = pnc->QueryInterface( IID_INetCfgLock,
                                          (LPVOID *)&pncLock );
                if ( hr == S_OK ) {

                    //
                    // Attempt to lock the INetCfg for read/write
                    //

                    hr = pncLock->AcquireWriteLock( LOCK_TIME_OUT,
                                                    lpszAppName,
                                                    lpszLockedBy);
                    if (hr == S_FALSE ) {
                        hr = NETCFG_E_NO_WRITE_LOCK;
                    }
                }
            }

            if ( hr == S_OK ) {

                //
                // Initialize the INetCfg object.
                //

                hr = pnc->Initialize( NULL );

                if ( hr == S_OK ) {
                    *ppnc = pnc;
                    pnc->AddRef();
                }
                else {

                    //
                    // Initialize failed, if obtained lock, release it
                    //

                    if ( pncLock ) {
                        pncLock->ReleaseWriteLock();
                    }
                }
            }

            ReleaseRef( pncLock );
            ReleaseRef( pnc );
        }

        //
        // In case of error, uninitialize COM.
        //

        if ( hr != S_OK ) {
            CoUninitialize();
        }
    }

    return hr;
}

HRESULT HrReleaseINetCfg (IN INetCfg* pnc,
                          IN BOOL fHasWriteLock)
{
    INetCfgLock    *pncLock = NULL;
    HRESULT        hr = S_OK;

    //
    // Uninitialize INetCfg
    //

    hr = pnc->Uninitialize();

    //
    // If write lock is present, unlock it
    //

    if ( hr == S_OK && fHasWriteLock ) {

        //
        // Get the locking reference
        //

        hr = pnc->QueryInterface( IID_INetCfgLock,
                                  (LPVOID *)&pncLock);
        if ( hr == S_OK ) {
           hr = pncLock->ReleaseWriteLock();
           ReleaseRef( pncLock );
        }
    }

    ReleaseRef( pnc );

    //
    // Uninitialize COM.
    //

    CoUninitialize();

    return hr;
}


HRESULT HrInstallComponent(IN INetCfg* pnc,
                           IN LPCWSTR szComponentId,
                           IN const GUID* pguidClass)
{
    INetCfgClassSetup   *pncClassSetup = NULL;
    INetCfgComponent    *pncc = NULL;
    OBO_TOKEN           OboToken;
    HRESULT             hr = S_OK;

    //
    // OBO_TOKEN specifies on whose behalf this
    // component is being installed.
    // Set it to OBO_USER so that szComponentId will be installed
    // on behalf of the user.
    //

    ZeroMemory( &OboToken,
                sizeof(OboToken) );
    OboToken.Type = OBO_USER;

    //
    // Get component's setup class reference.
    //

    hr = pnc->QueryNetCfgClass ( pguidClass,
                                 IID_INetCfgClassSetup,
                                 (void**)&pncClassSetup );
    if ( hr == S_OK ) {

        hr = pncClassSetup->Install( szComponentId,
                                     &OboToken,
                                     0,
                                     0,       // Upgrade from build number.
                                     NULL,    // Answerfile name
                                     NULL,    // Answerfile section name
                                     &pncc ); // Reference after the component
        if ( S_OK == hr ) {                   // is installed.

            //
            // we don't need to use pncc (INetCfgComponent), release it
            //

            ReleaseRef( pncc );
        }

        ReleaseRef( pncClassSetup );
    }

    return hr;
}

					 
HRESULT HrInstallNetComponent (IN INetCfg *pnc,
                               IN LPCWSTR lpszComponentId,
                               IN const GUID    *pguidClass,
                               IN LPCWSTR lpszInfFullPath)
{
    DWORD     dwError;
    HRESULT   hr = S_OK;
    WCHAR     Drive[_MAX_DRIVE];
    WCHAR     Dir[_MAX_DIR];
    WCHAR     DirWithDrive[_MAX_DRIVE+_MAX_DIR];

    //
    // If full path to INF has been specified, the INF
    // needs to be copied using Setup API to ensure that any other files
    // that the primary INF copies will be correctly found by Setup API
    //

    if ( lpszInfFullPath ) {

        //
        // Get the path where the INF file is.
        //

        _wsplitpath( lpszInfFullPath, Drive, Dir, NULL, NULL );

        wcscpy( DirWithDrive, Drive );
        wcscat( DirWithDrive, Dir );

        //
        // Copy the INF file and other files referenced in the INF file.
        //

        if ( !SetupCopyOEMInfW(lpszInfFullPath,
                               DirWithDrive,  // Other files are in the
                                              // same dir. as primary INF
                               SPOST_PATH,    // First param is path to INF
                               0,             // Default copy style
                               NULL,          // Name of the INF after
                                              // it's copied to %windir%\inf
                               0,             // Max buf. size for the above
                               NULL,          // Required size if non-null
                               NULL) ) {      // Optionally get the filename
                                              // part of Inf name after it is copied.
            dwError = GetLastError();

            hr = HRESULT_FROM_WIN32( dwError );
        }
    }

    if ( S_OK == hr ) {

        //
        // Install the network component.
        //

        hr = HrInstallComponent( pnc,
                                 lpszComponentId,
                                 pguidClass );
        if ( hr == S_OK ) {

            //
            // On success, apply the changes
            //

            hr = pnc->Apply();
        }
    }

    return hr;
}


HRESULT InstallSpecifiedComponent (LPWSTR lpszInfFile,
                                   LPWSTR lpszPnpID,
                                   const GUID *pguidClass)
{
    INetCfg    *pnc;
    LPWSTR     lpszApp;
    HRESULT    hr;

    hr = HrGetINetCfg( TRUE,
                       APP_NAME,
                       &pnc,
                       &lpszApp );

    if ( hr == S_OK ) {

        //
        // Install the network component.
        //

        hr = HrInstallNetComponent( pnc,
                                    lpszPnpID,
                                    pguidClass,
                                    lpszInfFile );
        if ( (hr == S_OK) || (hr == NETCFG_S_REBOOT) ) {

            hr = pnc->Apply();
        }
        else {
            if ( hr != HRESULT_FROM_WIN32(ERROR_CANCELLED) ) {
                AfxMessageBox("Couldn't install the network component.");
            }
        }

        HrReleaseINetCfg( pnc,
                          TRUE );
    }
    else {
        if ( (hr == NETCFG_E_NO_WRITE_LOCK) && lpszApp ) {
            AfxMessageBox("currently holds the lock, try later.");

            CoTaskMemFree( lpszApp );
        }
        else {
            AfxMessageBox("Couldn't the get notify object interface." );
        }
    }

    return hr;
}


