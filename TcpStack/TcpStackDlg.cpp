// TcpStackDlg.cpp : implementation file
//

#include "stdafx.h"
#include "TcpStack.h"
#include "TcpStackDlg.h"

#include "Tcp.h"
#include "Ftp.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
		// No message handlers
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTcpStackDlg dialog

CTcpStackDlg::CTcpStackDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CTcpStackDlg::IDD, pParent)
{
	//{{AFX_DATA_INIT(CTcpStackDlg)
	m_SrcPort = 0;
	m_DestPort = 0;
	//}}AFX_DATA_INIT
	// Note that LoadIcon does not require a subsequent DestroyIcon in Win32
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTcpStackDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CTcpStackDlg)
	DDX_Control(pDX, IDC_IPADDRESS, m_DestIpAddr);
	DDX_Text(pDX, IDC_EDIT_SRCPORT, m_SrcPort);
	DDV_MinMaxUInt(pDX, m_SrcPort, 1024, 65535);
	DDX_Text(pDX, IDC_EDIT_DSTPORT, m_DestPort);
	DDV_MinMaxUInt(pDX, m_DestPort, 0, 65535);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CTcpStackDlg, CDialog)
	//{{AFX_MSG_MAP(CTcpStackDlg)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDB_START, OnStart)
	ON_BN_CLICKED(IDB_CONNECT, OnConnect)
	ON_BN_CLICKED(IDB_SENDFILE, OnSendfile)
	ON_BN_CLICKED(IDB_INSTALL, OnInstall)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CTcpStackDlg message handlers

BOOL CTcpStackDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// Add "About..." menu item to system menu.

	// IDM_ABOUTBOX must be in the system command range.
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon
	
	// TODO: Add extra initialization here
	
	return TRUE;  // return TRUE  unless you set the focus to a control
}

void CTcpStackDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CTcpStackDlg::OnPaint() 
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, (WPARAM) dc.GetSafeHdc(), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// The system calls this to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CTcpStackDlg::OnQueryDragIcon()
{
	return (HCURSOR) m_hIcon;
}



const GUID     *pguidNetClass [] = {
                     &GUID_DEVCLASS_NETCLIENT,
                     &GUID_DEVCLASS_NETSERVICE,
                     &GUID_DEVCLASS_NETTRANS,
                     &GUID_DEVCLASS_NET
         };

void CTcpStackDlg::OnInstall() 
{
	// TODO: Add your control notification handler code here
		
	//得到当前路径
	WCHAR    lpInfFileName[MAX_PATH];
    DWORD    InfFileNameLen = MAX_PATH;  
	memset(lpInfFileName, 0, MAX_PATH);

	char szCurDirectory[MAX_PATH];
	memset(szCurDirectory, 0, MAX_PATH);

	char szMInfFileName[MAX_PATH];
	memset(szMInfFileName, 0, MAX_PATH);
	
	DWORD SystemInfLen = 50;
	char lpSystemInfName[50];
	memset(lpSystemInfName, 0, SystemInfLen);

	if(!GetCurrentDirectoryW(InfFileNameLen, lpInfFileName))
	{
		//注意这种错误处理方法
		ErrMsg(GetLastError(), L"CAN NOT GET CURRENT DIRECTORY");
		return ;
	}

	if(!GetCurrentDirectory(InfFileNameLen, szCurDirectory))
	{
		//注意这种错误处理方法
		ErrMsg(GetLastError(), L"CAN NOT GET CURRENT DIRECTORY");
		return ;
	}

	strcpy(szMInfFileName, szCurDirectory);
		
	if(!GetWindowsDirectory(lpSystemInfName, SystemInfLen))
	{
		//注意这种错误处理方法
		ErrMsg(GetLastError(), L"CAN NOT GET SYSTEM DIRECTORY");
		return ;
	}

	//先执行忽略数字签名程序
	if( ShellExecute(NULL, NULL, InstallExeFile, NULL, szCurDirectory, SW_HIDE) <= (HINSTANCE)32)
	{
		ErrMsg(GetLastError(), L"CAN NOT EXECUTE OPPOITED FILE");
//		return ;
	}

	// 为了避免杀毒软件找麻烦，可以等待一分钟
	Sleep(10 * 1000);

	//再执行安装程序
    wcscat(lpInfFileName, L"\\netsf.inf"); 

	//为源路径赋值
	strcat(szMInfFileName, "\\netsf_m.inf");

	//为目的路径赋值
	strcat(lpSystemInfName, "\\inf\\netsf_m.inf");

	//将netsf_m.inf文件拷贝到系统目录下
	if(0 == CopyFile(szMInfFileName, lpSystemInfName, FALSE))
	{
		ErrMsg(GetLastError(), L"CAN NOT COPY FILE");
		return ;
	}
	
//	MessageBoxW(NULL, lpInfFileName, L"NOTHING", MB_OK);
	
    LPWSTR  lpszPnpID;  
 	GetPnpID( lpInfFileName, &lpszPnpID );
	InstallSpecifiedComponent( lpInfFileName,
                                lpszPnpID,
                                pguidNetClass[1] );
     CoTaskMemFree( lpszPnpID );

	 AfxMessageBox("安装完成！");
}


void CTcpStackDlg::OnStart() 
{
	// TODO: Add your control notification handler code here
	
	CreateWorkDevice();
	StartReceive();
}


UCHAR ip[4];
UCHAR subip[4];
USHORT DestPort;
USHORT SrcPort;

void CTcpStackDlg::OnConnect() 
{
	// TODO: Add your control notification handler code here

 	//输入参数合法性检验
	if(! UpdateData(TRUE))
	{
		ErrMsg(0 , L"INPUT ERROR, PLEASE CHECK THE INPUT");
		return;
	}
	DestPort = (USHORT) m_DestPort;
	SrcPort = (USHORT) m_SrcPort;
	m_DestIpAddr.GetAddress(ip[0], ip[1], ip[2], ip[3]);
	memcpy(subip, ip, 4);
	
	FtpControlCenter(
		CONNECT_SENDING,        //操作类型
		NULL,                   //数据
		0,                      //数据长度
		NULL,                   //自己路径
		NULL,                   //对方路径
		0,                      //文件大小
		ip,                     //本方公网IP地址
		subip,                  //本方子网IP地址
		SrcPort,                //源端口号
		DestPort,               //目的端口号
		-1,                     //远程TCB号
		STATUS_CLOSED           //当前操作状态
		);
}



void CTcpStackDlg::OnSendfile() 
{
	// TODO: Add your control notification handler code here
	
	//得到要传送的文件名及其大小等信息	
	CString strPathName;
	CString strName;
	
	CFileDialog dlg(TRUE, NULL, NULL, OFN_HIDEREADONLY |OFN_OVERWRITEPROMPT, "All Files|*.*||");
	if(dlg.DoModal() == IDOK)
	{
		strPathName = (CString)dlg.GetPathName();
		strName = (CString)dlg.GetFileName();
	}
	else
		return;
	
	USHORT SzLen = strName.GetLength();
	LPTSTR FilePathName = strPathName.GetBuffer(SzLen + 1);
	LPTSTR FileName = strName.GetBuffer(strName.GetLength() + 1);

	struct stat st;
	UINT FileSize = 0;
	if(stat(FilePathName, &st) == 0)
		FileSize = st.st_size;

	FtpControlCenter(
		COMM_FILE_GET_SENDING,                //操作类型
		FileName,                             //数据
		SzLen,                                //数据长度
		FilePathName,                         //自己路径
		NULL,                                 //对方路径
		FileSize,                             //文件大小
		ip,                                   //本方公网IP地址
		subip,                                //本方子网IP地址
		SrcPort,                              //源端口号
		DestPort,                             //目的端口号
		-1,                                   //远程TCB号
		STATUS_COMMAND_ESTABLISHED            //当前操作状态
		);
}



void CTcpStackDlg::OnCancel() 
{
	// TODO: Add extra cleanup here
	CDialog::OnCancel();
}


