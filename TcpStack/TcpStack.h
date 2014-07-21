// TcpStack.h : main header file for the TCPSTACK application
//

#if !defined(AFX_TCPSTACK_H__ABBF8518_E39D_4D14_B538_4383D025B93D__INCLUDED_)
#define AFX_TCPSTACK_H__ABBF8518_E39D_4D14_B538_4383D025B93D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"		// main symbols

/////////////////////////////////////////////////////////////////////////////
// CTcpStackApp:
// See TcpStack.cpp for the implementation of this class
//

class CTcpStackApp : public CWinApp
{
public:
	CTcpStackApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CTcpStackApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Implementation

	//{{AFX_MSG(CTcpStackApp)
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TCPSTACK_H__ABBF8518_E39D_4D14_B538_4383D025B93D__INCLUDED_)
