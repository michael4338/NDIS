// Commen.cpp: implementation of the CCommen class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "TcpStack.h"
#include "Commen.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////




/*系统已有此函数，改名之*/
//转换短整型字节顺序
USHORT Lhtons(USHORT hostshort)
{
	return ((hostshort<<8) | (hostshort>>8));
}
//将长整型字节转换成UCHAR数组
void Lhtonl(UINT hostlong, PUCHAR buddle)
{
	buddle[0] = (UCHAR)(hostlong >> 24);
	buddle[1] = (UCHAR)((hostlong >> 16) & 0x000000FF);
	buddle[2] = (UCHAR)((hostlong >> 8) & 0x000000FF);
	buddle[3] = (UCHAR)(hostlong & 0x000000FF);
}
//将UCHAR数组转换成UINT型
UINT Lnltoh(PUCHAR buddle)
{
	UINT temp1 = buddle[0], temp2 = buddle[1], temp3 = buddle[2], temp4 = buddle[3];
	return (((temp1 << 24) & 0xFF000000) | ((temp2 << 16) & 0x00FF0000) | ((temp3 << 8) & 0x0000FF00) | (temp4 & 0x000000FF));
}
//将UCHAR数组转换成USHORT型
USHORT Lnstoh(PUCHAR buddle)
{
	USHORT temp1 = buddle[0], temp2 = buddle[1];
	return (((temp1 << 8) & 0xFF00) | (temp2 & 0x00FF));
}
//判断两个UCHAR数组是否相同
BOOL IsUcSame(PUCHAR Uc1, PUCHAR Uc2, int len)
{
    for(int i=0; i<len; i++)  
		if(Uc1[i] != Uc2[i])
			return FALSE;
	return TRUE;
}
//将无符号字符串转化为有符号字符串
PCHAR PUchTOch(PUCHAR puch, USHORT len)
{
	PCHAR pch = new CHAR[len];
	for(USHORT i=0; i<len; i++)
		pch[i] = (char)(puch[i]);
	return pch;
}
//将有符号字符串转化为无符号字符串
PUCHAR PchTOUch(PCHAR pch, USHORT len)
{
	PUCHAR puch = new UCHAR[len];
	for(USHORT i=0; i<len; i++)
		puch[i] = (char)(pch[i]);
	return puch;
}