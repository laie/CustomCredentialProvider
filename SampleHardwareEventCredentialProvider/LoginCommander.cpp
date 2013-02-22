//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) 2006 Microsoft Corporation. All rights reserved.
//
//

#include "LoginCommander.h"
//#include <strsafe.h>
#include <boost/archive/binary_iarchive.hpp>
#include <Util.h>
#include <strstream>

inline void StartAccept(LoginCommander& This, boost::asio::ip::tcp::socket& Socket, boost::asio::ip::tcp::acceptor& Acceptor)
{
	Acceptor.async_accept(
		Socket,
		[&This, &Socket, &Acceptor](const boost::system::error_code& ErrorCode)
		{
			if ( ErrorCode ) throw ErrorCode;

			// read
			try
			{
				char readbuf[4096];
				size_t bytesread = Socket.read_some(boost::asio::buffer(readbuf, 4096));
				This.ReadHandler(readbuf, bytesread);
			}
			catch ( std::exception& )
			{
				// if socket is opened and closed immediately, it seems unable to do read_some()
			}

			Socket.close();

			StartAccept(This, Socket, Acceptor);
		}
	);
}

void LoginCommander::ThreadProc()
{
	boost::asio::ip::tcp::socket	serversock(IoService);

	boost::asio::ip::tcp::acceptor	acceptor(
		IoService,
		boost::asio::ip::tcp::endpoint(
			boost::asio::ip::address_v4(
				  (127 << 24)
				+ (0 << 16)
				+ (0 << 8)
				+ (1 << 0)
			),
			32695
		)
	);


	// start the accept
	StartAccept(*this, serversock, acceptor);
	IoService.run();
}

LoginCommander::LoginCommander(CCustomProvider *pProvider):
	pProvider(pProvider)
{
	IsLoggingIn = false;
}

bool LoginCommander::Init()
{
	Thread = boost::thread([this](){ this->ThreadProc(); });
	return true;
}

void LoginCommander::ReadHandler(char *pData, size_t BytesRead)
{
	try
	{
		Util::ibinarystream ib(pData, BytesRead);
		unsigned char packetcode;
		ib >> packetcode;
		switch ( packetcode )
		{
		case 0:
			{
				std::wstring username, passwd;

				DWORD sz;

				ib >> sz;
				username.resize(sz);
				ib.load_binary(const_cast<wchar_t*>(username.data()), sz*2);

				ib >> sz;
				passwd.resize(sz);
				ib.load_binary(const_cast<wchar_t*>(passwd.data()), sz*2);

				IsLoggingIn = true;
				pProvider->DoLoginWith(username, passwd);
				pProvider->OnConnectStatusChanged();
			}
			break;
		case 1:
			break;
		default:
			throw std::bad_exception("unknown packet type");
		}
	}
	catch ( boost::archive::archive_exception& )
	{

	}
}

void LoginCommander::LoginFailed()
{
	IsLoggingIn = false;
	pProvider->OnConnectStatusChanged();
}

bool LoginCommander::GetIsLoggingIn()
{
	return IsLoggingIn;
}


LoginCommander::~LoginCommander()
{
	IoService.stop();
}

