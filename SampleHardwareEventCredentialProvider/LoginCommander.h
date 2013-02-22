//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) 2006 Microsoft Corporation. All rights reserved.
//
// CCommandWindow provides a way to emulate external "connect" and "disconnect" 
// events, which are invoked via toggle button on a window. The window is launched
// and managed on a separate thread, which is necessary to ensure it gets pumped.
//

#pragma once

#ifndef _WIN32_WINNT
#define _WIN32_WINNT 0x0501
#endif

#include "CCustomProvider.h"
#include <boost/thread.hpp>
#include <boost/asio.hpp>

class LoginCommander
{
private:
	boost::asio::io_service	IoService;
	bool				IsLoggingIn;
	boost::thread		Thread;
	CCustomProvider*	pProvider;

	void ThreadProc();

public:
	
	LoginCommander(CCustomProvider *pProvider);
	~LoginCommander();

	bool Init();

	void ReadHandler(char *pData, size_t BytesRead);
	void LoginFailed();

	bool GetIsLoggingIn();
};