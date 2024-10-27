
#if !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
#define AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_

#pragma once

#define WIN32_LEAN_AND_MEAN	
#define _CRT_SECURE_NO_DEPRECATE

#include <windows.h>
#include <objbase.h>
#ifdef _DEBUG
#include <crtdbg.h>
#endif // _DEBUG

#include "..\DuiLib\UIlib.h"

using namespace DuiLib;

#ifdef _WIN64
    #ifdef _DEBUG
        #ifdef _UNICODE
            #pragma comment(lib, "..\\bin\\DuiLib_ud_x64.lib")
        #else
            #pragma comment(lib, "..\\bin\\DuiLib_d_x64.lib")
        #endif
    #else
        #ifdef _UNICODE
        #pragma comment(lib, "..\\bin\\DuiLib_u_x64.lib")
        #else
        #pragma comment(lib, "..\\bin\\DuiLib_x64.lib")
        #endif
    #endif
#else
    #ifdef _DEBUG
        #ifdef _UNICODE
            #pragma comment(lib, "..\\bin\\DuiLib_ud.lib")
        #else
            #pragma comment(lib, "..\\bin\\DuiLib_d.lib")
        #endif
    #else
        #ifdef _UNICODE
        #pragma comment(lib, "..\\bin\\DuiLib_u.lib")
        #else
        #pragma comment(lib, "..\\bin\\DuiLib.lib")
        #endif
    #endif
#endif


//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_STDAFX_H__A9DB83DB_A9FD_11D0_BFD1_444553540000__INCLUDED_)
