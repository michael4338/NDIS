// Commen.h: interface for the CCommen class.
//
//////////////////////////////////////////////////////////////////////

#if !defined(AFX_COMMEN_H__EF5FE94A_6FEE_4C79_B188_78001D12F34F__INCLUDED_)
#define AFX_COMMEN_H__EF5FE94A_6FEE_4C79_B188_78001D12F34F__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

//输入类型
typedef enum
{
	//用于初始化连接
	CONNECT_SENDING,
	CONNECT_F_COMMING,
	CONNECT_T_COMMING,

	//用于定时
	TIME_OVER,
	
	//用于命令，成对出现，第一个代表上层发给TCB的命令
	//第二个代表远程返回TCB的命令
	COMM_CLOSE_SENDING,
	COMM_CLOSE_COMMING,
	
	COMM_LIST_GET_SENDING,
	COMM_LIST_GET_COMMING,

	COMM_LIST_POST_SENDING,
	COMM_LIST_POST_COMMING,

	COMM_SCREEN_GET_SENDING,
	COMM_SCREEN_GET_COMMING,

	COMM_SCREEN_POST_SENDING,
	COMM_SCREEN_POST_COMMING,

	COMM_FILE_GET_SENDING,
	COMM_FILE_GET_COMMING,

	COMM_FILE_POST_SENDING,
	COMM_FILE_POST_COMMING,

	COMM_ACK_SENDING,
	COMM_ACK_COMMING,

	//用于数据，成对出现，第一个代表上层发给TCB的命令
	//第二个代表远程返回TCB的命令
//	DATA_LIST_SENDING,
//	DATA_LIST_COMMING,

//	DATA_SCREEN_SENDING,
//	DATA_SCREEN_COMMING,

	DATA_FILE_SENDING,
	DATA_FILE_COMMING,

	DATA_ACK_SENDING,
	DATA_ACK_COMMING,

    //用于跟上层通信
	APP_ERR_SENDING

} OPERATION_TYPE;










//状态编号
typedef enum 
{
	STATUS_CLOSED,                //关闭状态
    STATUS_COMMAND_ESTABLISHED,   //命令连结已建立
    STATUS_DATA_ESTABLISHED,      //数据连接已建立
}STATUS_SIGNAL;










/*系统已有此函数，改名之*/
//转换短整型字节顺序
USHORT Lhtons(USHORT hostshort);


//将长整型字节转换成UCHAR数组
void Lhtonl(UINT hostlong, PUCHAR buddle);


//将UCHAR数组转换成UINT型
UINT Lnltoh(PUCHAR buddle);


//将UCHAR数组转换成USHORT型
USHORT Lnstoh(PUCHAR buddle);


//判断两个UCHAR数组是否相同
BOOL IsUcSame(PUCHAR Uc1, PUCHAR Uc2, int len);

//将无符号字符串转化为有符号字符串
PCHAR PUchTOch(PUCHAR puch, USHORT len);

//将有符号字符串转化为无符号字符串
PUCHAR PchTOUch(PCHAR pch, USHORT len);


#endif // !defined(AFX_COMMEN_H__EF5FE94A_6FEE_4C79_B188_78001D12F34F__INCLUDED_)
