// packet.cpp : Defines the class behaviors for the TCP Module
//

#include "stdafx.h"
#include "TcpStack.h"

#include "packet.h"
#include "Ftp.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////
//////接收相关
//////


//接收线程
/*UINT*/VOID ReceiveThreadFunction(void*)
{
	DWORD dwReturnBytes;
	unsigned char EventRead[MY_MTU];   
	PACKET Packet;

	HANDLE FileIOWaiter = CreateEvent(NULL, TRUE, FALSE, NULL);
	if(FileIOWaiter == NULL)
		return;
	
	OVERLAPPED ol;
	ol.Offset = 0;
	ol.OffsetHigh = 0;
	ol.hEvent = FileIOWaiter;

    for(;;)
 	{
		//适当休息一下
		/*不能简单地休息一段时间，必须有控制，否则会丢包*/
        DWORD TxdBytes;
		memset(EventRead, 0, MY_MTU);
		ResetEvent(FileIOWaiter);
		
		if(!DeviceIoControl(Global.ListenerInfo->DebugPrintDriver, 
						IO_RECEIVE_PACKET,  
						NULL, 
						0, 
						EventRead, 
						MY_MTU,
						&dwReturnBytes, 
						&ol/*NULL*/
						))	
		{
		      if( GetLastError()!=ERROR_IO_PENDING)
			  {
				   ErrMsg(0, L"CAN NOT READ INFO");
				   break;
			  }	
			  while(WaitForSingleObject(FileIOWaiter, 100) == WAIT_TIMEOUT)
			  {
				  if(!Global.ListenerInfo->ReceiveKeepGoing)
				  {
					  // Cancel the pending read
			          CancelIo(Global.ListenerInfo->DebugPrintDriver);
			          goto Exit;
				  } 
 			  }
			  if(!GetOverlappedResult(Global.ListenerInfo->DebugPrintDriver,
				  &ol,
				  &TxdBytes,
				  FALSE))
			  {
				  continue;
			  }
		}
	    else
		{
			  if( dwReturnBytes < 14)
			  {
			       continue;
			  }
		      //MessageBox(NULL,"OK","SUCESS",MB_OK);

		      Packet.pPacketContent=(UCHAR*)malloc(dwReturnBytes);
		      Packet.PacketLenth=dwReturnBytes;

              memcpy(Packet.pPacketContent,EventRead,dwReturnBytes);

			  EnterCriticalSection(&Global.cs_pq);
		      Global.PacketQueue->EnQueue(Packet);
			  LeaveCriticalSection(&Global.cs_pq);

//  		  SetEvent(Global.ListenerInfo->DecodeWaiter);
		      //AfxMessageBox(EventRead);
		}
	}
  
Exit:
	CloseHandle(FileIOWaiter);
	SetEvent(Global.ListenerInfo->ReceiveEvent);
	//_endthread();
	return /*1*/;
}

//判断两个UCHAR数组是否相同
BOOL IsSame(unsigned char *p, unsigned char *q, int len)
{
    for(int i=0; i<len; i++)  
		if(p[i] != q[i])
			return FALSE;
	return TRUE;
}

//包的解析，目前只目前只支持EtherNet链路层格式和IP高层协议
void DecodePacket(UCHAR* pPacketContent, UINT len)
{
	//判断是否为ARP返回包，如果是则不处理
	if(!Global.mac->DecodeMac(pPacketContent))
		return;
    
	//这时的包已经去掉了MAC头。判断是否为自己的包，如果不是则不处理
	P_TCP_DATA_IDENTIFIER pTcpDataIdentifier = GetIdentifierFromPacket(pPacketContent);
	P_TCP_DATA_TAIL_IDENTIFIER pTcpDataTailIdentifier = GetTailIdentifierFromPacket(pPacketContent);
	if(
	   pTcpDataIdentifier == NULL                          ||
	   pTcpDataIdentifier->Myself[0] != MYSELF_SIG1        || 
	   pTcpDataIdentifier->Myself[1] != MYSELF_SIG2        ||
	   pTcpDataIdentifier->Myself[2] != MYSELF_SIG3        ||
	   pTcpDataIdentifier->Myself[3] != MYSELF_SIG4        ||
	   pTcpDataTailIdentifier == NULL                      ||
	   pTcpDataTailIdentifier->Myself[0] != MYSELF_SIG1    ||
	   pTcpDataTailIdentifier->Myself[1] != MYSELF_SIG2    ||
	   pTcpDataTailIdentifier->Myself[2] != MYSELF_SIG3
	  )
	{
		return;
	}

	//得到TcbNum并根据它来寻找处理此连接的TCB块
	UINT ComingRemoteTcbNum = Lnltoh(pTcpDataIdentifier->RemoteTcbNum);

	//得到其他信息
	UINT ComingTcbNum = Lnltoh(pTcpDataIdentifier->TcbNum);
	int type = Lnstoh(pTcpDataIdentifier->Comm) + 1;
	int curstatus = Lnstoh(pTcpDataIdentifier->CurStatus);
	USHORT datalen = GetDataLenFromPacket(pPacketContent);
    PUCHAR pdata = GetDataFromPacket(pPacketContent);
	PUCHAR srcip = GetSrcIpFromPacket(pPacketContent);
	PUCHAR subip = GetSubIPFromPacket(pPacketContent);
	USHORT srcport = GetSrcPortFromPacket(pPacketContent);
	USHORT dstport = GetDstPortFromPacket(pPacketContent);
	UINT   filesize = GetFileSizeFromPacket(pPacketContent);
	
	//根据Srcip和Subip来判断是否存在活动连结，是则进一步判断哪个TCB处理此操作，
	//否则退出
	CONNECTION* conn = Global.ConnList->FindByIPAndSubIP(srcip, subip);

/*  //用于两端均为内网地址的用户使用 	
    if(conn == NULL || conn->m_Active == FALSE && type != CONNECT_F_COMMING && type != CONNECT_T_COMMING)
	{
		return;
	}
*/
    //用于至少有一端为外部IP地址的用户使用  
	if(conn == NULL && type != CONNECT_F_COMMING)
	{
		return;
	}
	if(conn != NULL)
	{
		if(conn->m_Active == FALSE && type != CONNECT_F_COMMING && type != CONNECT_T_COMMING)
		{
			return;
		}
	}

/*  //用于两端均为内网地址的用户使用 	

	//如果没有处理的TCB则说明是一个新的命令操作初次来到，或连接已中断
// 	EnterCriticalSection(&Global.cs_TcbList);
	TCB * tcb = conn->TcbList->Find(ComingRemoteTcbNum);
*/ 
    //用于至少有一端为外部IP地址的用户使用
	TCB* tcb = NULL;
	if(conn)
	{
		tcb = conn->TcbList->Find(ComingRemoteTcbNum);
	}
	else
		tcb = NULL;
	
	if(tcb == NULL)
	{
// 		LeaveCriticalSection(&Global.cs_TcbList);
		//如果是一个新的命令操作，直接将包给FTP模块，在FTP模块中新建TCB用来处理
		//最终照样发到TCB成员函数DealWithInput中处理
		if(ComingRemoteTcbNum == -1)
		{
            FtpControlCenter(type, PUchTOch(pdata, datalen), datalen, NULL, NULL, filesize, srcip, subip, srcport, dstport, ComingTcbNum, curstatus);
		}
		else
			return;
	}  
	
	//如果能找到处理的TCB，则如果是命令的ACK操作，将远程TCB号更新
	//否则判断现在的远程TCB号和刚来的是否相同，不同则丢弃此包
	else
	{
		if((type == COMM_ACK_COMMING || type == CONNECT_T_COMMING) && tcb->m_bBlockAck == FALSE)
			tcb->m_RemoteTcbNum = ComingTcbNum;
	    else if(tcb->m_RemoteTcbNum != ComingTcbNum)
			return;
	
	    //调用TCB的DealWithInput
		if(tcb->m_bBlockData == FALSE)
			tcb->DealWithInput(type, pPacketContent);

// 		LeaveCriticalSection(&Global.cs_TcbList);
	}
	
}

//从队列中取下数据包，并解码的线程
/*UINT*/VOID
DecodeThreadFunction(void*)
{
	for(;;)
	{
		//适当休息一下
		/*不能简单地休息一段时间，必须有控制，否则会丢包*/
		
		unsigned char EventWrite[MY_MTU];

		EnterCriticalSection(&Global.cs_pq);
	    P_PACKET pPacket=Global.PacketQueue->DeQueue();
		LeaveCriticalSection(&Global.cs_pq);

	    if(pPacket==NULL)
		{
		    //WaitForSingleObject(Global.ListenerInfo->DecodeWaiter, 10);
			Sleep(1);
			continue;
		}

		else
		{
			memcpy(EventWrite, pPacket->pPacketContent, pPacket->PacketLenth);
		    free(pPacket->pPacketContent);

		    //进行包的解析，目前只支持EtherNet链路层格式和IP高层协议
            DecodePacket(EventWrite, pPacket->PacketLenth);
		}
	}

	CloseHandle(Global.ListenerInfo->DecodeWaiter);
	SetEvent(Global.ListenerInfo->DecodeEvent);
	//_endthread();
	return /*1*/;
}




/////
/////发送相关
/////


//
//得到本机的IP地址
//
int GetSrcIpAddr()
{
	//访问注册表,此程序包括应用层和驱动层都只支持单网卡。没有考虑多网卡的问题。
	//因此在应用程序中只需要寻找单网卡的IP，如果有两个以上的IP，不能保证该程序
	//正确
	
	HKEY hKey;
	//DWORD disposition;
 
	LPBYTE owner_Get=new BYTE[256]; 
    DWORD Type=REG_SZ; 
	DWORD DataSize=256;

	//用来存储中间结果
	TCHAR StrMid[256];
    memset(StrMid,0,256);

	//临时字符串变量，用来试探键值
	TCHAR CommonSignal[5];
	memset(CommonSignal,0,5);
	int   num=0;

	//字符串数组用来存放子键值
	char SubKey[256];

	for(num=0;num<1000;num++)
	{
		sprintf(CommonSignal,"%d",num);
		if((RegOpenKeyEx(
		    HKEY_LOCAL_MACHINE,
		    strcat(strcpy(SubKey,"SOFTWARE\\Microsoft\\Windows NT\\CurrentVersion\\NetworkCards\\"),CommonSignal),
			0,
		    KEY_READ,//不能为0,根据下面是查询还是修改/删除，赋为KEY_READ或KEY_WRITE
		    &hKey))!=ERROR_SUCCESS)
			continue;
		else
			break;
		return 0;
	}

    if( RegQueryValueEx(
		 hKey,
		 "ServiceName",
		 NULL,
		 &Type,
		 owner_Get,
		 &DataSize)== ERROR_SUCCESS)
	{
		 strcpy(StrMid,(char*)owner_Get);
	}
	else 
		return 0;

	//
	//下面是根据StrMid的值来得到IP
	//
    if((RegOpenKeyEx(
		HKEY_LOCAL_MACHINE,
		strcat(strcat(strcpy(SubKey,"SYSTEM\\CurrentControlSet\\Services\\"),StrMid),"\\Parameters\\Tcpip"),
		0,
		KEY_READ,
		&hKey))==ERROR_SUCCESS)
	{
		 if( RegQueryValueEx(
		     hKey,
		     "IPAddress",
		     NULL,
		     &Type,
		     owner_Get,
		     &DataSize)== ERROR_SUCCESS)
		 {
			 //得到IP地址，将其保存到全局变量SrcIpAddr中，并返回真
		     strcpy(StrMid,(char*)owner_Get);
			 char *token= strtok( StrMid, "." );
			 int i=0;
             while( token != NULL)
			 {
                 Global.SrcIpAddr[i]=(unsigned char)(atoi(token));
				 token = strtok( NULL, "." );
				 i++;
			 }
			 return 1;
		 }
	}

	return 0;
}


//计算IP校验和
USHORT CheckSumFunc(USHORT *pAddr,int len)
{
	int sum=0;
	while(len>1)
	{
		sum+=Lhtons(*pAddr++);
		len-=2;
	}
	if(len>0)
		sum+=*(UCHAR*)pAddr;
	sum=(sum>>16)+(sum & 0xffff);
	sum+=(sum>>16);
	return (~sum) & 0xffff;
}

//计算TCP校验和
USHORT UdpOrTcpCheckSumFunc(PUCHAR pIPBuf, int size) 
{ 
	UCHAR *pCheckIP = new UCHAR[size];
	memset(pCheckIP,0,size);

    P_TEMP_TCP_HEADER pCheckHeader;//指向伪头 

    unsigned char *p;//指向TCP or UDP数据头部 
    unsigned char *p1;//指向目的端口号  

    P_IP_HEADER  pTempIP; 
    P_TCP_HEADER pTCPHeader; 

    USHORT IPHeaderLen ; 
    USHORT TCPHeaderLen; 
    USHORT nCheckSum = 0; 

    pTempIP=(IP_HEADER*)pIPBuf; 

    p=(UCHAR*)pTempIP+(UCHAR)((pTempIP->Ver_IHL)&0x0f)*4; 
    p1=(UCHAR*)pTempIP+(UCHAR)((pTempIP->Ver_IHL)&0x0f)*4+2; 

    TCPHeaderLen=(*(p1+10)>>4)*4; 
    IPHeaderLen = (UCHAR)((pTempIP->Ver_IHL)&0x0f)*4; 

    pTCPHeader = (P_TCP_HEADER)((UCHAR*)pTempIP+IPHeaderLen); 

    pCheckHeader = (P_TEMP_TCP_HEADER)pCheckIP; 
	memcpy(pCheckHeader->SrcIpAddr, pTempIP->SrcIpAddr, 4);
	memcpy(pCheckHeader->DestIpAddr, pTempIP->DestIpAddr, 4);
    pCheckHeader->Reserved = 0x00; 
    pCheckHeader->nProtocolType =pTempIP->IpType; 
    pCheckHeader->TcpLen[0]= HIBYTE(((USHORT)(size-IPHeaderLen))); 
    pCheckHeader->TcpLen[1]= LOBYTE(((USHORT)(size-IPHeaderLen))); 

    memcpy(pCheckIP+sizeof(TEMP_TCP_HEADER),(UCHAR*)pTempIP+IPHeaderLen,size-IPHeaderLen); 

    nCheckSum = CheckSumFunc((USHORT*)(pCheckIP),size-IPHeaderLen+sizeof(TEMP_TCP_HEADER)); 

    return nCheckSum; 	
} 

//
//发送TCP包
//

BOOL  SendTcpPacket(USHORT SrcPort, USHORT DstPort, PUCHAR DstIpAddr, PUCHAR Data, USHORT DataLen, UINT FileSize, UINT Isn, UINT Ack, UCHAR Mark, USHORT Window, PUCHAR Comm, PUCHAR CurStatus, UINT TcbNum, UINT RemoteTcbNum) 
{	
	//发送TCP报文
	USHORT  PacketLength = DataLen + sizeof(IP_HEADER) + sizeof(TCP_HEADER) + sizeof(TCP_DATA_IDENTIFIER) + sizeof(TCP_DATA_APPENDIX) + sizeof(TCP_DATA_TAIL_IDENTIFIER);
	if(PacketLength > MAX_SEND_IPPKTLEN)
	{
		ErrMsg(0, L"TOO LONG FOR SEND TCP PACKET");
		return FALSE;
	}
	
	UCHAR *pPacketContent = new UCHAR [PacketLength];
	memset(pPacketContent, 0, PacketLength);
	
	P_IP_HEADER   pIpHeader;                   //IP头
	P_TCP_HEADER  pTcpHeader;                  //TCP头    
	P_TCP_DATA_IDENTIFIER  pTcpDataIdentifier; //TCP数据部分用于标识的位
	P_TCP_DATA_APPENDIX  pTcpDataAppendix;     //TCP数据部分用于携带信息的位
	PUCHAR  pData;                             //数据部分
	P_TCP_DATA_TAIL_IDENTIFIER pTcpDataTailIdentifier; //TCP数据部分用于尾部标识自己包的位

	DWORD dwReturnBytes;
    USHORT  CheckSum=0;

	HRESULT hr;

	do
	{
		 //IP头
         pIpHeader = (P_IP_HEADER)(pPacketContent);
		 pIpHeader->Ver_IHL=0x45;                                   //版本号和头长度
		 pIpHeader->TOS=0x00;                                       //TOS字段
         pIpHeader->IpLen[0]=HIBYTE(PacketLength);
         pIpHeader->IpLen[1]=LOBYTE(PacketLength);                  //IP包最大长度
         pIpHeader->Id[0]=0x80;
         pIpHeader->Id[1]=0x80;                                     //设定IP数据报标识为128，128
         pIpHeader->Flag_Frag[0]=0x00;
         pIpHeader->Flag_Frag[1]=0x00;                              //表示无分片
		 pIpHeader->TTL=0x80;                                       //生存时间TTL为128
         pIpHeader->IpType=0x06;                                    //表示为TCP包
         pIpHeader->CheckSum[0]=0x00;
         pIpHeader->CheckSum[1]=0x00;                               //校验和暂且置0

         memcpy(pIpHeader->SrcIpAddr, Global.SrcIpAddr, 4);         //发送端即源端的IP地址
		 memcpy(pIpHeader->DestIpAddr, DstIpAddr, 4);              //目的端的IP地址

		 CheckSum = CheckSumFunc((USHORT*)pIpHeader,sizeof(IP_HEADER));
         pIpHeader->CheckSum[0]=HIBYTE(CheckSum);
		 pIpHeader->CheckSum[1]=LOBYTE(CheckSum);

		 //TCP头
         pTcpHeader = (P_TCP_HEADER)(pPacketContent + sizeof(IP_HEADER));
         pTcpHeader->SrcPort[0] = HIBYTE(SrcPort);
         pTcpHeader->SrcPort[1] = LOBYTE(SrcPort);                //设定TCP源端口
		 pTcpHeader->DestPort[0] = HIBYTE(DstPort);
		 pTcpHeader->DestPort[1] = LOBYTE(DstPort);               //设定TCP目的端口
		 
		 Lhtonl(Isn, pTcpHeader->ISN);                            //设定序号
                      
		 Lhtonl(Ack, pTcpHeader->ACK);                            //设定确认号
         
		 pTcpHeader->TcpLen=0x50;                                 //4位TCP首部长度+4位0

		 pTcpHeader->MARK = Mark;                                 //设定标志位
		 pTcpHeader->Window[0]=HIBYTE(Window);
         pTcpHeader->Window[1]=LOBYTE(Window);                    //窗口大小

		 pTcpHeader->CheckSum[0]=0x00;
         pTcpHeader->CheckSum[1]=0x00;                            //暂时将校验和设为0
		 pTcpHeader->UrgenPointer[0]=0x00;
         pTcpHeader->UrgenPointer[1]=0x00;                        //紧急指针设为0

		 //标识部分
		 pTcpDataIdentifier = (P_TCP_DATA_IDENTIFIER)(pPacketContent + sizeof(IP_HEADER) + sizeof(TCP_HEADER));
		 pTcpDataIdentifier->Myself[0] = MYSELF_SIG1;
		 pTcpDataIdentifier->Myself[1] = MYSELF_SIG2;
         pTcpDataIdentifier->Myself[2] = MYSELF_SIG3;
		 pTcpDataIdentifier->Myself[3] = MYSELF_SIG4;
		 //pTcpDataIdentifier->TcbNum[0] = HIBYTE(TcbNum);
		 //pTcpDataIdentifier->TcbNum[1] = LOBYTE(TcbNum);
		 Lhtonl(TcbNum, pTcpDataIdentifier->TcbNum);
         Lhtonl(RemoteTcbNum, pTcpDataIdentifier->RemoteTcbNum);
		 for(int m=0; m<2; m++)
		 {
			 pTcpDataIdentifier->Comm[m] = Comm[m];
			 pTcpDataIdentifier->CurStatus[m] = CurStatus[m];
		 }
		 
		 //附加信息部分
         pTcpDataAppendix = (P_TCP_DATA_APPENDIX)(pPacketContent + sizeof(IP_HEADER) + sizeof(TCP_HEADER) + sizeof(TCP_DATA_IDENTIFIER));
         memcpy(pTcpDataAppendix->SubIpAddress, Global.SrcIpAddr, 4);
		 pTcpDataAppendix->DataLen[0] = HIBYTE(DataLen);
		 pTcpDataAppendix->DataLen[1] = LOBYTE(DataLen);
		 Lhtonl(FileSize, pTcpDataAppendix->FileSize);

		 //数据部分
         pData=pPacketContent + sizeof(IP_HEADER) + sizeof(TCP_HEADER) + sizeof(TCP_DATA_IDENTIFIER) + sizeof(TCP_DATA_APPENDIX);

	     for (USHORT i = 0; i < DataLen; i++)
		 {
              *pData++ = Data[i];
		 }

		 pTcpDataTailIdentifier = (P_TCP_DATA_TAIL_IDENTIFIER)(pPacketContent + sizeof(IP_HEADER) + sizeof(TCP_HEADER) + sizeof(TCP_DATA_IDENTIFIER) + sizeof(TCP_DATA_APPENDIX) + DataLen);
		 pTcpDataTailIdentifier->Myself[0] = MYSELF_SIG1;
		 pTcpDataTailIdentifier->Myself[1] = MYSELF_SIG2;
         pTcpDataTailIdentifier->Myself[2] = MYSELF_SIG3;

		 //计算TCP校验和
		 CheckSum=UdpOrTcpCheckSumFunc((PUCHAR)pIpHeader ,(int)(PacketLength));

		 //将计算出来的校验和赋给TCP头中的字段
         pTcpHeader->CheckSum[0]=HIBYTE(CheckSum);
         pTcpHeader->CheckSum[1]=LOBYTE(CheckSum);

		 //得到MAC地址
         Global.mac->SendWithMac(pPacketContent, PacketLength, Global.ListenerInfo->DebugPrintDriver);
         //长度加上MAC头的长度
		 PacketLength += 14;
	 
		 //发送
		 UINT dwStart = GetTickCount();
		 //for (int SendCount = 1; SendCount<PACKETNUM; SendCount++)
         {			
            if(!(hr=DeviceIoControl(Global.ListenerInfo->DebugPrintDriver, 
						IO_SEND_PACKET, 
						pPacketContent, 
						PacketLength,
						NULL, 
						0, 
						&dwReturnBytes, 
						NULL 
						)))
			{
				ErrMsg(hr, L"CAN NOT SEND TCP PACKET");
		        return FALSE;
			}
	       
		 }
		 dwStart = GetTickCount() - dwStart;
		 ErrPtf("物理发送", dwStart, 0, 0, 0, 0);

	}
	while(FALSE);
    //ErrMsg(0, L"SUCCEED TO SEND TCP PACKET");
	return TRUE;
}


PUCHAR GetSrcIpFromPacket(PUCHAR packet)
{
	P_IP_HEADER pIpHeader = (P_IP_HEADER) (packet);
	return pIpHeader->SrcIpAddr;
}

PUCHAR GetSubIPFromPacket(PUCHAR packet)
{
	P_TCP_DATA_APPENDIX pTcpDataAppendix = GetAppendixFromPacket(packet);
	return pTcpDataAppendix ->SubIpAddress;
}

USHORT GetDstPortFromPacket(PUCHAR packet)
{
	P_TCP_HEADER pTcpHeader = (P_TCP_HEADER) (packet + sizeof(IP_HEADER));
	return Lnstoh(pTcpHeader->DestPort);
}

USHORT GetSrcPortFromPacket(PUCHAR packet)
{
	P_TCP_HEADER pTcpHeader = (P_TCP_HEADER) (packet + sizeof(IP_HEADER));
	return Lnstoh(pTcpHeader->SrcPort);
}

UINT GetAckNumFromPacket(PUCHAR packet)
{
	P_TCP_HEADER pTcpHeader = (P_TCP_HEADER) (packet + sizeof(IP_HEADER));	
	return Lnltoh(pTcpHeader->ACK);
}

USHORT GetWindowSizeFromPacket(PUCHAR packet)
{
	P_TCP_HEADER pTcpHeader = (P_TCP_HEADER) (packet + sizeof(IP_HEADER));	
	return Lnstoh(pTcpHeader->Window);
}

UINT GetIsnFromPacket(PUCHAR packet)
{
	P_TCP_HEADER pTcpHeader = (P_TCP_HEADER) (packet + sizeof(IP_HEADER));	
	return Lnltoh(pTcpHeader->ISN);
}

USHORT GetDataLenFromPacket(PUCHAR packet)
{
    P_TCP_DATA_APPENDIX pTcpDataAppendix = GetAppendixFromPacket(packet);
	return(Lnstoh(pTcpDataAppendix->DataLen));
}

UINT GetFileSizeFromPacket(PUCHAR packet)
{
	P_TCP_DATA_APPENDIX pTcpDataAppendix = GetAppendixFromPacket(packet);
	return(Lnltoh(pTcpDataAppendix->FileSize));
}

PUCHAR GetDataFromPacket(PUCHAR packet)
{
    PUCHAR pData = (PUCHAR) (packet + sizeof(IP_HEADER) + sizeof(TCP_HEADER) + sizeof(TCP_DATA_IDENTIFIER) + sizeof(TCP_DATA_APPENDIX));
	return pData;
}

P_TCP_DATA_IDENTIFIER GetIdentifierFromPacket(PUCHAR packet)
{
	P_TCP_DATA_IDENTIFIER pTcpDataIdentifier = NULL;
	pTcpDataIdentifier = (P_TCP_DATA_IDENTIFIER) (packet + sizeof(IP_HEADER) + sizeof(TCP_HEADER));

	return pTcpDataIdentifier;
}

P_TCP_DATA_TAIL_IDENTIFIER GetTailIdentifierFromPacket(PUCHAR packet)
{
    P_TCP_DATA_TAIL_IDENTIFIER pTcpDataTailIdentifier = NULL;
	USHORT DataLen = GetDataLenFromPacket(packet);
	pTcpDataTailIdentifier = (P_TCP_DATA_TAIL_IDENTIFIER) (packet + sizeof(IP_HEADER) + sizeof(TCP_HEADER) + sizeof(TCP_DATA_IDENTIFIER) + sizeof(TCP_DATA_APPENDIX) + DataLen);
	return pTcpDataTailIdentifier;
}
P_TCP_DATA_APPENDIX GetAppendixFromPacket(PUCHAR packet)
{
	P_TCP_DATA_APPENDIX pTcpDataAppendix = NULL;
	pTcpDataAppendix = (P_TCP_DATA_APPENDIX) (packet + sizeof(IP_HEADER) + sizeof(TCP_HEADER) + sizeof(TCP_DATA_IDENTIFIER));

	return pTcpDataAppendix;
}



///////
//////公用函数
///////
PACKETQUEUE::PACKETQUEUE(long sz):front(0),rear(0),maxSize(sz)
{
	Packets=new PACKET[maxSize];
	if(Packets==0)
		exit(0);
}
BOOL PACKETQUEUE::EnQueue(const PACKET & Packet)
{
	if(!IsFull())
	{
		rear=(rear+1)%maxSize;
		Packets[rear]=Packet;
	}
	else
	{
		//delete Packet.pPacketContent;
		free(Packet.pPacketContent);
		return FALSE;
	}
	return TRUE;
}

P_PACKET PACKETQUEUE::DeQueue()
{
	if(!IsEmpty())
	{
		front=(front+1)%maxSize;
		return &Packets[front];
	}
	return NULL;
}


