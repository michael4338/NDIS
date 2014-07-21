// Tcp.cpp : Defines the class behaviors for the TCP Module
//

#include "stdafx.h"
#include "TcpStack.h"

# include "Tcp.h"
# include "Ftp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif


///////////////////////////////////////////
///////////////TCB类实现///////////////////
///////////////////////////////////////////


TCB::TCB():link(NULL)
{
	//大规模初始化
	InputStru.InputType = 0;
	InputStru.LastInputType = 0;
	memset(InputStru.ParaInfo, 0, sizeof(InputStru.ParaInfo));
	InputStru.ParaLen = 0;
	memset(InputStru.SelfDirect, 0, sizeof(InputStru.SelfDirect));
	memset(InputStru.RemoteDirect, 0, sizeof(InputStru.RemoteDirect));
	InputStru.m_MaxSendSize = 0;
	memset(InputStru.InputBuffer, 0, PerPacketSize);
	InputStru.BufferSize = 0;
	InitializeCriticalSection(&InputStru.Cs_IBuffer); 
	InputStru.InputEvent = CreateEvent(NULL,TRUE,TRUE,NULL); //manual,set it

	//OutputStru.BufferSize = 0;
	//InitializeCriticalSection(&OutputStru.Cs_OBuffer);

    RecCtrler.LastAckNum = 0;
	RecCtrler.ISN = 0;
	RecCtrler.WindowSize = 0;
	RecCtrler.TempWinSize = 0;
	RecCtrler.DelayTime = TIME_RCV_DELAY;

    SendCtrler.ISN = 0;
	SendCtrler.CurAckNum = 0;
	SendCtrler.WindowSize = INIT_WIN_SIZE;
	SendCtrler.ReTranTime = INIT_RETRAN_TIME;

	m_Conn = NULL;

	m_bBlockAck = FALSE;
	m_bBlockData = FALSE;

	m_bQuit = FALSE;
	m_bConnected = FALSE;
	m_bContinueSend = FALSE;

	m_pMyStimer = new MyTimers(1, this, TRUE);
	m_pMyRtimer = new MyTimers(1, this, FALSE);

	m_CommEvent = CreateEvent(NULL,TRUE,FALSE,NULL); //manual,clear it 
	m_Comm_Data_Event = CreateEvent(NULL,TRUE,FALSE,NULL); //manual,clear it

    memset(m_IpAddr, 0, sizeof(m_IpAddr));
	memset(m_SubIpAddr, 0, sizeof(m_SubIpAddr));
	m_SrcPort = 1050;
	m_DstPort = 80;

	m_ReTranCount = RETRAN_COUNT;             
	dwStart = 0;                   
	
    CurStatus = STATUS_CLOSED;

	m_bCallTime = TRUE;                 
	m_bRcvDelay = TRUE;                 

	m_CurrISN = 0;

	m_TcbNum = -1;
	m_RemoteTcbNum = -1;
}


TCB::TCB(UINT TcbNum):link(NULL)
{
	//大规模初始化
	InputStru.InputType = 0;
	InputStru.LastInputType = 0;
	memset(InputStru.ParaInfo, 0, sizeof(InputStru.ParaInfo));
	InputStru.ParaLen = 0;
	memset(InputStru.SelfDirect, 0, sizeof(InputStru.SelfDirect));
	memset(InputStru.RemoteDirect, 0, sizeof(InputStru.RemoteDirect));
	InputStru.m_MaxSendSize = 0;
	memset(InputStru.InputBuffer, 0, PerPacketSize);
	InputStru.BufferSize = 0;
	InitializeCriticalSection(&InputStru.Cs_IBuffer); 
	InputStru.InputEvent = CreateEvent(NULL,TRUE,TRUE,NULL); //manual,set it

	//OutputStru.BufferSize = 0;
	//InitializeCriticalSection(&OutputStru.Cs_OBuffer);

    RecCtrler.LastAckNum = 0;
	RecCtrler.ISN = 0;
	RecCtrler.WindowSize = 0;
	RecCtrler.TempWinSize = 0;
	RecCtrler.DelayTime = TIME_RCV_DELAY;

    SendCtrler.ISN = 0;
	SendCtrler.CurAckNum = 0;
	SendCtrler.WindowSize = INIT_WIN_SIZE;
	SendCtrler.ReTranTime = INIT_RETRAN_TIME;      
	
	m_Conn = NULL;		
	
	m_bBlockAck = FALSE;
	m_bBlockData = FALSE;

	m_bQuit = FALSE;
	m_bConnected = FALSE;
	m_bContinueSend = FALSE;

	m_pMyStimer = new MyTimers(1, this, TRUE);
	m_pMyRtimer = new MyTimers(1, this, FALSE);

	m_CommEvent = CreateEvent(NULL,TRUE,FALSE,NULL); //manual,clear it 
	m_Comm_Data_Event = CreateEvent(NULL,TRUE,FALSE,NULL); //manual,clear it
  
	memset(m_IpAddr, 0, sizeof(m_IpAddr));
	memset(m_SubIpAddr, 0, sizeof(m_SubIpAddr));
	m_SrcPort = 1050;
	m_DstPort = 80;

	m_ReTranCount = RETRAN_COUNT;             
	dwStart = 0;                   
	
    CurStatus = STATUS_CLOSED;

	m_bCallTime = TRUE;                 
	m_bRcvDelay = TRUE;                 

	m_CurrISN = 0;

	m_TcbNum = TcbNum;
	m_RemoteTcbNum = -1;
}


TCB::~TCB()
{
	m_pMyRtimer->stopTimer();
	m_pMyStimer->stopTimer();
}
void TCB::InsertAfterSelf(TCB* tcb)
{
	tcb->link = link;
	link = tcb;
}

TCB* TCB::RemoveAfterSelf()
{
	TCB*temp = link;
	if(link == NULL) return NULL;
	link = temp->link;
	return temp;
}

BOOL TCB::DealWithInput(int InputType, PUCHAR packet)
{      
	if(m_bQuit)
		return FALSE;

	InputStru.InputType = InputType;

	switch(InputType)
	{
	    //如果是收到连结相关信息，直接调用主处理流程
		case CONNECT_SENDING:
			{
				//同步处理，发送后等待回应消息
				if(m_bQuit)
					break;
                
				BOOL FirstTime = TRUE;

				for(;;)
				{
					ResetEvent(m_CommEvent);
				    //发送消息，通知对方自己要发送文件
				    Dispatch(CONNECT_SENDING, packet);
                    WaitForSingleObject(m_CommEvent, INFINITE);
				
				    m_Conn->m_Active = m_bConnected;

					if(m_Conn->m_Active)
					{
						if(FirstTime)
						{
							ErrMsg(0, L"连接成功！");
							FirstTime = FALSE;
						}
					}
					else
					{
						ErrMsg(0, L"连接失败！");
					}

					Sleep(INIT_RETRAN_TIME * 2);
				}

				break;
			}

	    case CONNECT_F_COMMING:
			{
				Dispatch(InputType, packet);
				break;
			}

	    case CONNECT_T_COMMING:
			{
				Dispatch(InputType, packet);
				SetEvent(m_CommEvent);
				break;
			}
			

	    //如果是收到数据相关信息，直接调用主处理流程
		case DATA_FILE_SENDING:
		case DATA_FILE_COMMING:
		case DATA_ACK_SENDING:
		case DATA_ACK_COMMING:
			{
				Dispatch(InputType, packet);
				break;
			}


		//如果是收到对方的请求,要求接收List
		case COMM_LIST_GET_COMMING:
			{
				break;
			}

        //如果是收到对方的请求,要求发送List
		case COMM_LIST_POST_COMMING:
			{
				break;
			}

		//如果是发送命令给对方，要求对方发送List
		case COMM_LIST_GET_SENDING:
			{
				break;
			}

		//如果是发送命令给对方，要求对方接收List
		case COMM_LIST_POST_SENDING:
			{
				break;
			}

		//如果是收到对方的请求,要求接收Screen
		case COMM_SCREEN_GET_COMMING:
			{
				break;
			}

        //如果是收到对方的请求,要求发送Screen
		case COMM_SCREEN_POST_COMMING:
			{
				break;
			}

		//如果是发送命令给对方，要求对方发送Screen
		case COMM_SCREEN_GET_SENDING:
			{
				break;
			}

		//如果是发送命令给对方，要求对方接收Screen
		case COMM_SCREEN_POST_SENDING:
			{
				break;
			}

		//如果是收到对方的请求,要求本方接收File
		case COMM_FILE_GET_COMMING:
			{
				//得到当前的路径
				//这种情况下，m_Conn->m_SelfCurDirect为本地当前路径，InputStru.ParaInfo为要接收的文件名
				//得到文件全名，并打开它
				strcat(strcat(InputStru.SelfDirect, "\\"), InputStru.ParaInfo);
				OutputStru.RcvFile = fopen(InputStru.SelfDirect,"wb");
				if(OutputStru.RcvFile == NULL)
				{
					ErrMsg(GetLastError(), L"CAN NOT OPEN THE RCV FILE");
					break;
				}
				
				//判断要接收的文件名，文件名长度和文件大小等信息是否已成功在FTP模块中赋值到TCB参数
                if(InputStru.ParaLen == 0 && InputStru.m_MaxSendSize == 0)
					break;
       
				//调用主处理流程，发送回应包，为了保险发两次
				Dispatch(InputType, packet);
				
				//等待一段时间
				Sleep(SendCtrler.ReTranTime * 2);

				//将现在的主流程的状态改为DATA_ESTABLISHED
							
				if(m_bQuit)
					break;

				ResetEvent(m_Comm_Data_Event);
				CurStatus = STATUS_DATA_ESTABLISHED;
				//等待接收过程完毕
				WaitForSingleObject(m_Comm_Data_Event, INFINITE);

				fclose(OutputStru.RcvFile);
				break;
			}

        //如果是收到对方的请求,要求本方发送File
		case COMM_FILE_POST_COMMING:
			{
				//判断文件名、文件名长度、文件大小是否已成功赋值到TCB参数中
				if(InputStru.ParaLen == 0)
					break;

				//得到当前路径，从而得到文件全路径，然后打开文件
				//这种情况下，文件全名在对方提供的包中的ParaInfo中
				strcpy(InputStru.SelfDirect, InputStru.ParaInfo);
				InputStru.TransFile = fopen(InputStru.SelfDirect, "rb");
				if(InputStru.TransFile == NULL)
				{
					ErrMsg(GetLastError(), L"CAN NOT OPEN THE SEND FILE");
					break;
				}

				//得到文件大小，并赋值给m_MaxSendSize
				struct stat st;
                if(stat(InputStru.SelfDirect, &st) == 0)
					InputStru.m_MaxSendSize = st.st_size;

				//发回应消息
				Dispatch(COMM_FILE_POST_COMMING, packet);
			
				//等待一段时间
				Sleep(SendCtrler.ReTranTime * 2);

				//可以发送数据了，将状态转变为DATA_ESTABLISHED
				CurStatus = STATUS_DATA_ESTABLISHED;
				
				//正式发送
				if(m_bQuit)
					break;

				ResetEvent(m_Comm_Data_Event);
				Dispatch(DATA_FILE_SENDING, packet);
				WaitForSingleObject(m_Comm_Data_Event, INFINITE);
				
				fclose(InputStru.TransFile);
				break;
			}

		//如果是发送命令给对方，要求对方接收File
		case COMM_FILE_GET_SENDING:
			{
				//判断文件名、文件名长度、文件大小是否已成功赋值到TCB参数中
				if(InputStru.ParaLen == 0 || InputStru.m_MaxSendSize == 0)
					break;

				//得到当前路径，从而得到文件全路径，然后打开文件
				//这种情况下，SelfDirect中存放本地文件全路径，ParaInfo中为要对方接收的文件名

				InputStru.TransFile = fopen(InputStru.SelfDirect, "rb");
				if(InputStru.TransFile == NULL)
				{
					ErrMsg(GetLastError(), L"CAN NOT OPEN THE SEND FILE");
					break;
				}

				//同步处理，发送后等待回应消息
				if(m_bQuit)
					break;

				ResetEvent(m_CommEvent);
				//发送消息，通知对方自己要发送文件
				Dispatch(COMM_FILE_GET_SENDING, packet);
                WaitForSingleObject(m_CommEvent, INFINITE);

				//ACK来了，这时可以发送数据了，将状态转变为DATA_ESTABLISHED，并禁止ACK再来
				m_bBlockAck = TRUE;
				CurStatus = STATUS_DATA_ESTABLISHED;

				//等待一段时间，待接收方变为可以接收数据时再发数据
				Sleep(SendCtrler.ReTranTime * 2);
				
				//正式发送
				if(m_bQuit)
					break;

				ResetEvent(m_Comm_Data_Event);
				Dispatch(DATA_FILE_SENDING, packet);
				WaitForSingleObject(m_Comm_Data_Event, INFINITE);

				fclose(InputStru.TransFile);
				
            	ErrMsg(0, L"发送文件已完成！");

				break;
			}

		//如果是发送命令给对方，要求对方发送File
		case COMM_FILE_POST_SENDING:
			{
				//判断文件名、文件名长度是否已成功赋值到TCB参数中
				if(InputStru.ParaLen == 0)
					break;

				//得到当前路径，从而得到文件全路径，然后打开文件
				//这种情况下，RemoteDirect为要求对方发送的全路径，InputStru.ParaInfo也要存放要对方发送的文件的全路径
                //SelfDirect中是本地的当前路径
				strcat(InputStru.SelfDirect, InputStru.ParaInfo);
				strcpy(InputStru.ParaInfo, InputStru.RemoteDirect);
				InputStru.TransFile = fopen(InputStru.SelfDirect, "wb");
				if(InputStru.TransFile == NULL)
				{
					ErrMsg(GetLastError(), L"CAN NOT OPEN THE SEND FILE");
					break;
				}

				//同步处理，发送后等待回应消息
				if(m_bQuit)
					break;

				ResetEvent(m_CommEvent);
				//发送消息，通知对方自己要发送文件
				Dispatch(COMM_FILE_POST_SENDING, packet);
                WaitForSingleObject(m_CommEvent, INFINITE);

				//ACK来了，这时可以接收数据了，将状态转变为DATA_ESTABLISHED，并禁止ACK再来
				m_bBlockAck = TRUE;

				//等待一段时间，待发送方变为可以发送数据时再接收数据
				//Sleep(SendCtrler.ReTranTime * 2);
				
				//正式接收
				if(m_bQuit)
					break;

				ResetEvent(m_Comm_Data_Event);
				CurStatus = STATUS_DATA_ESTABLISHED;
				//等待接收过程完毕
				WaitForSingleObject(m_Comm_Data_Event, INFINITE);

				fclose(OutputStru.RcvFile);
				break;
			}
        
	    //如果收到回应信息
		case COMM_ACK_COMMING:
			{
				Dispatch(COMM_ACK_COMMING, packet);
				break;
			}
			
		//如果收到超时消息
		case TIME_OVER:
			{
				Dispatch(TIME_OVER, packet);
				break;
			}

		//如果收到主动关闭消息
		case COMM_CLOSE_COMMING:
			{
				Dispatch(COMM_CLOSE_COMMING, packet);
				break;
			}

		//如果收到被动关闭消息
		case COMM_CLOSE_SENDING:
			{
				Dispatch(COMM_CLOSE_SENDING, packet);
				break;
			}

		default:
			break;
		}
		
	return TRUE;
}


BOOL TCB::DealWithOutput(int OutputType)
{
	//定义发送包时需要给定的变量
	PUCHAR Data = NULL;
	USHORT DataLen = 0;
	UINT   FileSize = 0;
	UINT   Isn = 0x3450;
	UINT   Ack = 0x1300;
	UCHAR  Mark = 0;            //ACK 0x10 , SYN 0x02 , RST 0x04
	USHORT Window = 0;
    UCHAR  Comm[2] = {0,0};
	UCHAR  CurTcbStatus[2] = {0,0};
	UINT   TcbNum = 0;
	UINT   RemoteTcbNum = 0;

	PUCHAR  DstIp = m_IpAddr;
	USHORT  DstPort = m_DstPort;
	USHORT  SrcPort = m_SrcPort;
				
	TcbNum = m_TcbNum;
	RemoteTcbNum = m_RemoteTcbNum;
    FileSize = InputStru.m_MaxSendSize;
	CurTcbStatus[0] = HIBYTE(CurStatus);
	CurTcbStatus[1] = LOBYTE(CurStatus);

	switch(OutputType)
	{
	case CONNECT_SENDING:
		{
			//将参数赋值
			Mark = 0x02;
			Comm[0] = HIBYTE(CONNECT_SENDING);
			Comm[1] = LOBYTE(CONNECT_SENDING);

			RemoteTcbNum = -1;

			//发送包
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND CONNECT_SENDING");

			break;
		}

	case CONNECT_F_COMMING:
		{
			//将参数赋值
			Mark = 0x02;
			Comm[0] = HIBYTE(CONNECT_F_COMMING);
			Comm[1] = LOBYTE(CONNECT_F_COMMING);
			
			//发送包
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND CONNECT_F_COMMING");

			break;
		}

	case COMM_CLOSE_SENDING:
		{
			//将参数赋值
			Mark = 0x04;
			Comm[0] = HIBYTE(COMM_CLOSE_SENDING);
			Comm[1] = LOBYTE(COMM_CLOSE_SENDING);

			//发送包
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_CLOSE_SENDING");

			break;
		}

	case COMM_LIST_GET_SENDING:
		{
			//将参数赋值
			Mark = 0x00;
			Comm[0] = HIBYTE(COMM_LIST_GET_SENDING);
			Comm[1] = LOBYTE(COMM_LIST_GET_SENDING);

			//发送包
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_LIST_GET_SENDING");

			break;
		}

	case COMM_LIST_POST_SENDING:
		{
			//将参数赋值
			Mark = 0x00;
			Comm[0] = HIBYTE(COMM_LIST_POST_SENDING);
			Comm[1] = LOBYTE(COMM_LIST_POST_SENDING);

			//发送包
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_LIST_POST_SENDING");

			break;
		}

	case COMM_SCREEN_GET_SENDING:
		{
			//将参数赋值
			Mark = 0x00;
			Comm[0] = HIBYTE(COMM_SCREEN_GET_SENDING);
			Comm[1] = LOBYTE(COMM_SCREEN_GET_SENDING);

			//发送包
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_SCREEN_GET_SENDING");

			break;
		}

	case COMM_SCREEN_POST_SENDING:
		{
			//将参数赋值
			Mark = 0x00;
			Comm[0] = HIBYTE(COMM_SCREEN_POST_SENDING);
			Comm[1] = LOBYTE(COMM_SCREEN_POST_SENDING);

			//发送包
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_SCREEN_POST_SENDING");

			break;
		}

	case COMM_FILE_GET_SENDING:
		{
			//将参数赋值
			Mark = 0x00;
			Comm[0] = HIBYTE(COMM_FILE_GET_SENDING);
			Comm[1] = LOBYTE(COMM_FILE_GET_SENDING);
 
			DataLen = InputStru.ParaLen;
			Data = PchTOUch(InputStru.ParaInfo, DataLen);
		
			//发送包
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_FILE_GET_SENDING");

			break;
		}

	case COMM_FILE_POST_SENDING:
		{
			//将参数赋值
			Mark = 0x00;
			Comm[0] = HIBYTE(COMM_FILE_POST_SENDING);
			Comm[1] = LOBYTE(COMM_FILE_POST_SENDING);

			DataLen = InputStru.ParaLen;
			Data = PchTOUch(InputStru.ParaInfo, DataLen);

			//发送包
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_FILE_POST_SENDING");

			break;
		}

	case COMM_ACK_SENDING:
		{
			//将参数赋值
			Mark = 0x10;
			Comm[0] = HIBYTE(COMM_ACK_SENDING);
			Comm[1] = LOBYTE(COMM_ACK_SENDING);

			//发送包
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND COMM_ACK_SENDING");

			break;
		}
		
	case DATA_FILE_SENDING:
		{
            //将参数赋值
			Data = InputStru.InputBuffer;
			DataLen = InputStru.BufferSize;
			Isn = SendCtrler.ISN;
			Ack = SendCtrler.ISN + 1;
			Mark = 0x00;
			Comm[0] = HIBYTE(DATA_FILE_SENDING);
			Comm[1] = LOBYTE(DATA_FILE_SENDING);
			Window = SendCtrler.WindowSize;

			//发送包
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND DATA_FILE_SENDING");

			break;
		}

	case DATA_ACK_SENDING:
		{
			//将参数赋值
			Mark = 0x10;
			Comm[0] = HIBYTE(DATA_ACK_SENDING);
			Comm[1] = LOBYTE(DATA_ACK_SENDING);
			Window = RecCtrler.WindowSize;
			Ack = RecCtrler.LastAckNum;

			//发送包
			if(!SendTcpPacket(SrcPort, DstPort, DstIp, Data, DataLen, FileSize, Isn, Ack, Mark, Window, Comm, CurTcbStatus, TcbNum, RemoteTcbNum))
				ErrMsg(0, L"CAN NOT SEND DATA_ACK_SENDING");

			break;
		}

	case APP_ERR_SENDING:
		{
			//对应用程序发消息，报告错误
			ErrMsg(0, L"THE OPERATION HAS CANCELED");
			break;
		}

	default:break;
	}
	return TRUE;
}

void TCB::Quit()
{
	//阻止进一步接收，阻止进一步发送数据
	m_bQuit = TRUE;

	//发送对外RST和对应用程序的错误报告
	for(int i=0; i<3; i++)
		DealWithOutput(COMM_CLOSE_SENDING);
    DealWithOutput(APP_ERR_SENDING);

	//为程序退出清除障碍
	SetEvent(m_CommEvent);
	SetEvent(m_Comm_Data_Event);
	SetEvent(InputStru.InputEvent);

	Sleep(SendCtrler.ReTranTime * 2);

	SetEvent(m_CommEvent);
	SetEvent(m_Comm_Data_Event);
	SetEvent(InputStru.InputEvent);

}

void TCB::Dispatch(int InputType, PUCHAR packet)
{
	if(m_bQuit)
		return;

	switch(CurStatus)
	{
	//
	//如果当前状态为关闭
	//
	case STATUS_CLOSED:
		{
			switch(InputType)
			{
			case CONNECT_SENDING:
				{
					if(!DealWithOutput(CONNECT_SENDING))
						break;
										
					//撤销此对象定时器
				    if(m_pMyStimer->getTimerId() != 0)
						m_pMyStimer->stopTimer();
				    //重新创建一个新的定时器
				    m_pMyStimer->startTimer(SendCtrler.ReTranTime, TRUE);
				
					dwStart = GetTickCount();

				    break;
				}

			case CONNECT_F_COMMING:          //SEND REQUEST
				{
					DealWithOutput(CONNECT_F_COMMING);
				    break;
				}

		    case CONNECT_T_COMMING:          //ACK
				{
					//撤销此对象定时器
				    if(m_pMyStimer->getTimerId() != 0)
						m_pMyStimer->stopTimer();
						
					m_ReTranCount = RETRAN_COUNT;
					
					if(m_bConnected == FALSE)
					{
						SendCtrler.ReTranTime = GetTickCount() - dwStart;
						if(SendCtrler.ReTranTime < TIME_RCV_DELAY)
							SendCtrler.ReTranTime += TIME_RCV_DELAY;
				
						m_Conn->m_ConnRetranTime = SendCtrler.ReTranTime;
					}

					m_bConnected = TRUE;

				    break;
				}

		    case TIME_OVER:
				{				
					//撤销此对象定时器
				    if(m_pMyStimer->getTimerId() != 0)
						m_pMyStimer->stopTimer();

					if(m_ReTranCount-- <= 0)
					{
						m_bConnected = FALSE;
						SetEvent(m_CommEvent);
						break;
					}

					dwStart = GetTickCount();

					if(!DealWithOutput(CONNECT_SENDING))
						break;

				    //重新创建一个新的定时器
				    m_pMyStimer->startTimer(SendCtrler.ReTranTime, TRUE);
					
				    break;
				}

			case COMM_CLOSE_SENDING:
			case COMM_CLOSE_COMMING:
				{
					Quit();
			        break;
				}

			default:break;	
			}

			break;
		}

	//
	//如果当前状态为已建立命令连接
	//
	case STATUS_COMMAND_ESTABLISHED:
		{
			switch(InputType)
			{
			//如果是收到对方的请求,要求接收List
			case COMM_LIST_GET_COMMING:
				{
					break;
				}

            //如果是收到对方的请求,要求发送List
			case COMM_LIST_POST_COMMING:
				{
					break;
				}

			//如果是发送命令给对方，要求对方发送List
			case COMM_LIST_GET_SENDING:
				{
					break;
				}

			//如果是发送命令给对方，要求对方接收List
			case COMM_LIST_POST_SENDING:
				{
					break;
				}
			//如果是收到对方的请求,要求接收Screen
			case COMM_SCREEN_GET_COMMING:
				{
					break;
				}

            //如果是收到对方的请求,要求发送Screen
			case COMM_SCREEN_POST_COMMING:
				{
					break;
				}

			//如果是发送命令给对方，要求对方发送Screen
			case COMM_SCREEN_GET_SENDING:
				{
					break;
				}

			//如果是发送命令给对方，要求对方接收Screen
			case COMM_SCREEN_POST_SENDING:
				{
					break;
				}

			//如果是收到对方的请求,要求本方接收File
			case COMM_FILE_GET_COMMING:
				{
					//连续发两次ACK回应
					DealWithOutput(COMM_ACK_SENDING);
					DealWithOutput(COMM_ACK_SENDING);
					
					break;
				}

            //如果是收到对方的请求,要求本方发送File
			case COMM_FILE_POST_COMMING:
				{
					//连续发两次ACK回应
					DealWithOutput(COMM_ACK_SENDING);
					DealWithOutput(COMM_ACK_SENDING);

					break;
				}

			//如果是发送命令给对方，要求对方接收File
			case COMM_FILE_GET_SENDING:
				{
					//发送消息，通知对方要其接收文件
					if(!DealWithOutput(COMM_FILE_GET_SENDING))
					{
						ErrMsg(GetLastError(), L"CAN NOT SEND COMM_FILE_GET_SENDING");
						break;
					}

					InputStru.LastInputType = COMM_FILE_GET_SENDING;
					
					//撤销此对象定时器
				    if(m_pMyStimer->getTimerId() != 0)
						m_pMyStimer->stopTimer();
				    //重新创建一个新的定时器
				    m_pMyStimer->startTimer(SendCtrler.ReTranTime, TRUE);

					break;
				}

			//如果是发送命令给对方，要求对方发送File
			case COMM_FILE_POST_SENDING:
				{
					break;
				}
        
		    //如果收到回应信息
			case COMM_ACK_COMMING:
				{
				    //撤销此对象定时器
				    if(m_pMyStimer->getTimerId() != 0)
						m_pMyStimer->stopTimer();

					//将定时器全局计数重置
					m_ReTranCount = RETRAN_COUNT;

					//将等待事件激活
					SetEvent(m_CommEvent);
					
					break;
				}
			
			//如果收到超时消息
			case TIME_OVER:
				{
					//撤销此对象定时器
				    if(m_pMyStimer->getTimerId() != 0)
						m_pMyStimer->stopTimer();

					//判断是否超过了最多重发次数
					if( m_ReTranCount -- <= 0)
					{
					    Quit();
						break;
					}

					//发送消息，通知对方要其接收文件
					if(!DealWithOutput(InputStru.LastInputType))
					{
						ErrMsg(GetLastError(), L"CAN NOT SEND COMM_FILE_GET_SENDING");
						break;
					}

				    //重新创建一个新的定时器
				    m_pMyStimer->startTimer(SendCtrler.ReTranTime * (RETRAN_COUNT - m_ReTranCount + 1), TRUE);
					
					break;
				}

			case COMM_CLOSE_SENDING:
			case COMM_CLOSE_COMMING:
				{
					Quit();
			        break;
				}

			//如果意外收到数据消息，发RST取消远程TCB
			case DATA_FILE_COMMING:
				{
					for(int i=0; i<3; i++)
						DealWithOutput(COMM_CLOSE_SENDING);
					break;
				}

			default:
				break;
			}
			break;
		}
	//
	//如果当前状态为已建立数据连接
	//
	case STATUS_DATA_ESTABLISHED:
		{
			switch(InputType)
			{
			//发送数据
			case DATA_FILE_SENDING:
				{				
	                while((SendCtrler.ISN < InputStru.m_MaxSendSize || !m_bContinueSend) && !m_bQuit )
					{
 						ResetEvent(InputStru.InputEvent);
						
					    EnterCriticalSection(&InputStru.Cs_IBuffer);

						m_bCallTime = TRUE;
						
				        while(SendCtrler.ISN < InputStru.m_MaxSendSize && SendCtrler.ISN / PerPacketSize < SendCtrler.CurAckNum / PerPacketSize + SendCtrler.WindowSize)
						{
							if(m_bQuit)
								break;
							if(m_bCallTime)
							{				            
					            //撤销此对象定时器
				                if(m_pMyStimer->getTimerId() != 0)
									m_pMyStimer->stopTimer();
				                //重新创建一个新的定时器
 				                m_pMyStimer->startTimer(SendCtrler.ReTranTime + TIME_RCV_DELAY, TRUE);

								if(m_CurrISN <= SendCtrler.ISN)
								{
									dwStart = GetTickCount();
								    m_CurrISN = SendCtrler.ISN;
								}
							
							    m_bCallTime = FALSE;
							}
					        //组包并发送

							ErrPtf("发包过程", SendCtrler.ISN, SendCtrler.CurAckNum, m_CurrISN, SendCtrler.WindowSize, SendCtrler.ReTranTime);

						    fseek(InputStru.TransFile, SendCtrler.ISN, 0);
						    fread(InputStru.InputBuffer,
								sizeof(UCHAR),
								InputStru.BufferSize = (PerPacketSize) > (InputStru.m_MaxSendSize - SendCtrler.ISN) ? (InputStru.m_MaxSendSize - SendCtrler.ISN) : (PerPacketSize),
							    InputStru.TransFile
								);
//							DWORD ddwStart = GetTickCount();
					        if(!DealWithOutput(DATA_FILE_SENDING))
								break;
//							ddwStart = GetTickCount() - ddwStart;
//						    ErrPtf("逻辑发送", ddwStart, 0, 0, 0, 0);
					        SendCtrler.ISN = ftell(InputStru.TransFile);
						}
				    
					    LeaveCriticalSection(&InputStru.Cs_IBuffer);
										
 					    WaitForSingleObject(InputStru.InputEvent, INFINITE);
					}

					break;
				}
				
			//收到确认，是针对发送的
		    case DATA_ACK_COMMING:
				{
					UINT AckNum = GetAckNumFromPacket(packet);
										   
					EnterCriticalSection(&InputStru.Cs_IBuffer);

		            if(AckNum > SendCtrler.CurAckNum)
					{
						if(AckNum > m_CurrISN)
						{
							//不再计算同一批次的时间
							m_CurrISN = SendCtrler.ISN;
 						    m_bCallTime = TRUE;
							
						    //计算RRT
						    DWORD TempRrt = GetTickCount() - dwStart;
				            SendCtrler.ReTranTime = (DWORD)(ALPHA * SendCtrler.ReTranTime) + (DWORD)((1 - ALPHA) * TempRrt);
						}
				         
						//撤销此对象定时器
				        if(m_pMyStimer->getTimerId() != 0)
							m_pMyStimer->stopTimer();
						    
						//重置重传次数
					    m_ReTranCount = RETRAN_COUNT;	

						//得到窗口大小,应该不小于1不大于15000，即每秒10M
						
						ErrPtf("回应过程", SendCtrler.ISN, SendCtrler.CurAckNum, m_CurrISN, SendCtrler.WindowSize, SendCtrler.ReTranTime);

						SendCtrler.WindowSize = (SendCtrler.WindowSize + GetWindowSizeFromPacket(packet)) / 2;
				        if(SendCtrler.WindowSize == 0)
							SendCtrler.WindowSize = 1;
						if(SendCtrler.WindowSize >= 15000)
							SendCtrler.WindowSize = 15000;

						//处理ISN
				        SendCtrler.CurAckNum = AckNum;
				        //继续发送
					    SetEvent(InputStru.InputEvent);	

						if(SendCtrler.CurAckNum >= InputStru.m_MaxSendSize)
						{
							m_bContinueSend = TRUE;
							SetEvent(m_Comm_Data_Event);
						}
					}
										  
					LeaveCriticalSection(&InputStru.Cs_IBuffer);

			        break;
				}

			//接收数据
		    case DATA_FILE_COMMING:
				{
					//得到发送端窗口大小
				    RecCtrler.WindowSize = GetWindowSizeFromPacket(packet);
				    if(m_bRcvDelay)
					{
						m_bRcvDelay = FALSE;

						RecCtrler.TempWinSize = 0;
					
					    //撤销此对象定时器
				        if(m_pMyRtimer->getTimerId() != 0)
							m_pMyRtimer->stopTimer();
				        //重新创建一个新的定时器
				        m_pMyRtimer->startTimer(TIME_RCV_DELAY, TRUE);

					}
				    //计算延迟发送确认的时间
				    //RecCtrler.DelayTime = GetWindowSizeFromPacket(packet);
                
				    RecCtrler.ISN = GetIsnFromPacket(packet);
			     	UINT DataLen = GetDataLenFromPacket(packet);

					if(RecCtrler.ISN > InputStru.m_MaxSendSize || RecCtrler.ISN < RecCtrler.LastAckNum)
						break;
		
				    //写入文件
				    fseek(OutputStru.RcvFile, RecCtrler.ISN, 0);
				    fwrite(GetDataFromPacket(packet), 
						sizeof(UCHAR),
					    DataLen,
					    OutputStru.RcvFile
						);
				   
					if(RecCtrler.LastAckNum == RecCtrler.ISN)
					{
						RecCtrler.TempWinSize ++;
					    RecCtrler.LastAckNum += DataLen;
					}
		
					if(RecCtrler.LastAckNum >= InputStru.m_MaxSendSize)
					{
						//睡一段时间，让接收例程有时间发回应消息
						m_bBlockData = TRUE;
						Sleep(RecCtrler.DelayTime *2);
						//将命令等待事件置位
						SetEvent(m_Comm_Data_Event);
					}
				     break;
				}
				
			//超时处理，是针对发送的
		    case TIME_OVER:
				{
					
					ErrPtf("超时过程", SendCtrler.ISN, SendCtrler.CurAckNum, m_CurrISN, SendCtrler.WindowSize, SendCtrler.ReTranTime);

					//撤销此对象定时器
				    if(m_pMyStimer->getTimerId() != 0)
						m_pMyStimer->stopTimer();
					
					UINT CurAck = SendCtrler.CurAckNum;
					
					if(m_ReTranCount -- > 0)
					{
						//重传,处理过程与发送相似，只是只重传一个包
					    EnterCriticalSection(&InputStru.Cs_IBuffer);
					   	
						//如果回应仍然没来的话
// 						if(CurAck == SendCtrler.CurAckNum)
						{
							//组包并发送
					        SendCtrler.ISN = SendCtrler.CurAckNum;
			
				            fseek(InputStru.TransFile, SendCtrler.ISN, 0);
				            fread(InputStru.InputBuffer,
								sizeof(UCHAR),
							    InputStru.BufferSize = (PerPacketSize) > (InputStru.m_MaxSendSize-SendCtrler.ISN) ? (InputStru.m_MaxSendSize-SendCtrler.ISN) : (PerPacketSize),
						        InputStru.TransFile
							    );
					        if(!DealWithOutput(DATA_FILE_SENDING))
								break;
					        SendCtrler.ISN = ftell(InputStru.TransFile);
				        
						    //重新创建一个新的定时器
						    //尽量避免超时操作
				            m_pMyStimer->startTimer(SendCtrler.ReTranTime * (RETRAN_COUNT - m_ReTranCount + 1) + TIME_RCV_DELAY, TRUE);
						}
						
					    LeaveCriticalSection(&InputStru.Cs_IBuffer);
					}
				    //如果真超时了，证明出错了
				    else
					{
						//对应用层发错误信息，对对方发RST
					    DealWithOutput(COMM_CLOSE_SENDING);
					    DealWithOutput(APP_ERR_SENDING);
					}
				    break;
				}

						
			case COMM_CLOSE_SENDING:
			case COMM_CLOSE_COMMING:
				{
					Quit();
			        break;
				}

		   default:break;

		}


		break;
		}
								

	//
	//否则
	//
	default:break;
	}


	return ;
}
///////////////////////////////////////////
//////////////TCB类实现结束////////////////
///////////////////////////////////////////





///////////////////////////////////////////
///////////////TCBLIST类实现///////////////
///////////////////////////////////////////

TCBLIST::~TCBLIST(){}
	
void TCBLIST::MakeEmpty()
{
	TCB *temp;
	while(First->link != NULL)
	{
		temp = First->link;
		First->link = temp->link;
		delete temp;
	}	
	Last = First;
}
	
int TCBLIST::Length() const
{
	TCB *temp = First->link;
	int count = 0;
	while(temp != NULL)
	{
		temp = temp->link;
		count++;
	}
	return count;
}
	
	
TCB* TCBLIST::Find(UINT TcbNum)
{
	TCB *temp = First->link;
	while(temp != NULL && temp->m_TcbNum != TcbNum)
		temp = temp->link;
	return temp;
}
	
void TCBLIST::AppendList(TCB *add)
{
	Last->link = add;
	add->link = NULL;
	Last = add;
}

void TCBLIST::RemoveNode(TCB *tcb)
{
	TCB *temp = First->link;
	TCB *pretemp = First;
	while(temp != NULL && temp != tcb)
	{
		pretemp = temp;
		temp = temp->link;
	}
	if(temp != NULL)
	{
		if(temp == Last)
			Last = pretemp;
		pretemp->RemoveAfterSelf(); 
		delete temp;
	}
}
///////////////////////////////////////////
///////////////TCBLIST类实现结束///////////
///////////////////////////////////////////




//对CMMTimers类的继承
MyTimers::MyTimers(UINT x, TCB* tcb, BOOL IsSend) : CMMTimers(x)
{
	m_tcb = tcb;
	m_IsSend = IsSend;
}

void MyTimers::timerProc()
{
//	AfxMessageBox("TIME OVER");
	//发送超时信息
	if(m_IsSend)
		m_tcb->DealWithInput(TIME_OVER, NULL);
	else
	{
		if(m_tcb->RecCtrler.WindowSize <= 5000)
			m_tcb->RecCtrler.WindowSize = (m_tcb->RecCtrler.TempWinSize >= m_tcb->RecCtrler.WindowSize ? m_tcb->RecCtrler.WindowSize * 3 : m_tcb->RecCtrler.TempWinSize);
		else
            m_tcb->RecCtrler.WindowSize = m_tcb->RecCtrler.TempWinSize;
		
		m_tcb->DealWithOutput(DATA_ACK_SENDING);
	    m_tcb->m_bRcvDelay = TRUE;
	}
		
}




///////////////////////////////////////////
//////////////CONNECTION类实现/////////////
///////////////////////////////////////////

CONNECTION::CONNECTION():link(NULL)
{
	memset(m_IP, 0, 4);
	memset(m_SubIP, 0, 4);
	m_SrcPort = 1080;
	m_DstPort = 80;
	m_Active = FALSE;
	
	memset(m_SelfCurDirect, 0, MAX_PATH);
    memset(m_RemoteCurDirect, 0, MAX_PATH);

	TcbList = new TCBLIST(); 

	m_ConnRetranTime = INIT_RETRAN_TIME;

	m_CurType = -1;
	memset(m_pszAppendix, 0, MAX_PATH);
	m_SzLen = 0;
	m_FileSize = 0;
	m_TcbNum = 0;
	m_RemoteTcbNum = -1;
	m_CurStatus = 0;
	
    InitializeCriticalSection(&Cs_ConPara1); 
    InitializeCriticalSection(&Cs_ConPara2); 
}

CONNECTION::CONNECTION(UINT nIP, UINT nSubIP, USHORT SrcPort, USHORT DstPort):link(NULL)
{
	Lhtonl(nIP, m_IP);
	Lhtonl(nSubIP, m_SubIP);
	m_SrcPort = SrcPort;
	m_DstPort = DstPort;
	m_Active = FALSE;
	
	memset(m_SelfCurDirect, 0, MAX_PATH);
    memset(m_RemoteCurDirect, 0, MAX_PATH);

	TcbList = new TCBLIST(); 

	m_ConnRetranTime = INIT_RETRAN_TIME;

	m_CurType = -1;
	memset(m_pszAppendix, 0, MAX_PATH);
	m_SzLen = 0;
	m_FileSize = 0;
	m_TcbNum = 0;
	m_RemoteTcbNum = -1;
	m_CurStatus = 0;
	
    InitializeCriticalSection(&Cs_ConPara1); 
    InitializeCriticalSection(&Cs_ConPara2); 
}

CONNECTION::CONNECTION(PUCHAR puIP, PUCHAR puSubIP, USHORT SrcPort, USHORT DstPort)
{
    memcpy(m_IP, puIP, 4);
	memcpy(m_SubIP, puSubIP, 4);
	m_SrcPort = SrcPort;
	m_DstPort = DstPort;
	m_Active = FALSE;
		
	memset(m_SelfCurDirect, 0, MAX_PATH);
    memset(m_RemoteCurDirect, 0, MAX_PATH);
	
	TcbList = new TCBLIST(); 

	m_ConnRetranTime = INIT_RETRAN_TIME;

	m_CurType = -1;
	memset(m_pszAppendix, 0, MAX_PATH);
	m_SzLen = 0;
	m_FileSize = 0;
	m_TcbNum = 0;
	m_RemoteTcbNum = -1;
	m_CurStatus = 0;
	
    InitializeCriticalSection(&Cs_ConPara1); 
    InitializeCriticalSection(&Cs_ConPara2); 
}

void CONNECTION::InsertAfterSelf(CONNECTION* conn)
{
	conn->link = link;
	link = conn;
}

CONNECTION* CONNECTION::RemoveAfterSelf()
{
	CONNECTION* temp = link;
	if(link == NULL) return NULL;
	link = temp->link;
	return temp;
}





///////////////////////////////////////////
//////////////CONNECTION类实现结束/////////
///////////////////////////////////////////



///////////////////////////////////////////
//////////////CONNECTIONLIST类实现/////////
///////////////////////////////////////////

CONNECTIONLIST::~CONNECTIONLIST(){}
	
void CONNECTIONLIST::MakeEmpty()
{
	CONNECTION *temp;
	while(First->link != NULL)
	{
		temp = First->link;
		First->link = temp->link;
		delete temp;
	}	
	Last = First;
}
	
int CONNECTIONLIST::Length() const
{
	CONNECTION *temp = First->link;
	int count = 0;
	while(temp != NULL)
	{
		temp = temp->link;
		count++;
	}
	return count;
}
	
void CONNECTIONLIST::AppendList(CONNECTION *add)
{
	Last->link = add;
	add->link = NULL;
	Last = add;
}

CONNECTION* CONNECTIONLIST::FindByIPAndSubIP(PUCHAR puIP, PUCHAR puSubIP)
{
	CONNECTION *temp = First->link;
	while(temp != NULL && !(IsUcSame(temp->m_IP, puIP, 4) && IsUcSame(temp->m_SubIP, puSubIP, 4)))
		temp = temp->link;
	return temp;
}

void CONNECTIONLIST::RemoveNode(CONNECTION *del)
{
	CONNECTION *temp = First->link;
	CONNECTION *pretemp = First;
	while(temp != NULL && temp != del)
	{
		pretemp = temp;
		temp = temp->link;
	}
	if(temp != NULL)
	{
		if(temp == Last)
			Last = pretemp;
		pretemp->RemoveAfterSelf(); 
	}
}
///////////////////////////////////////////
//////////////CONNECTIONLIST类实现结束/////
///////////////////////////////////////////



