//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) 2006 Microsoft Corporation. All rights reserved.
//
// Standard dll required functions and class factory implementation.

#include <windows.h>
#include <unknwn.h>
#include "Dll.h"
#include "guid.h"

#include <Dbghelp.h>

void make_minidump(EXCEPTION_POINTERS* e)
{
    auto hDbgHelp = LoadLibraryA("dbghelp");
    if(hDbgHelp == nullptr)
        return;
    auto pMiniDumpWriteDump = (decltype(&MiniDumpWriteDump))GetProcAddress(hDbgHelp, "MiniDumpWriteDump");
    if(pMiniDumpWriteDump == nullptr)
        return;

    char name[MAX_PATH] = "c:\\log\\dmp.exe";
    {
        auto nameEnd = name + strlen(name); //GetModuleFileNameA(GetModuleHandleA(0), name, MAX_PATH);
        SYSTEMTIME t;
        GetSystemTime(&t);
        wsprintfA(nameEnd - strlen(".exe"),
            "_%4d%02d%02d_%02d%02d%02d.dmp",
            t.wYear, t.wMonth, t.wDay, t.wHour, t.wMinute, t.wSecond);
    }

    auto hFile = CreateFileA(name, GENERIC_WRITE, FILE_SHARE_READ, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0);
    if(hFile == INVALID_HANDLE_VALUE)
        return;

    MINIDUMP_EXCEPTION_INFORMATION exceptionInfo;
    exceptionInfo.ThreadId = GetCurrentThreadId();
    exceptionInfo.ExceptionPointers = e;
    exceptionInfo.ClientPointers = FALSE;

    auto dumped = pMiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        MINIDUMP_TYPE(MiniDumpWithIndirectlyReferencedMemory | MiniDumpScanMemory),
        e ? &exceptionInfo : nullptr,
        nullptr,
        nullptr);

    CloseHandle(hFile);

    return;
}

static LONG g_cRef = 0;   // global dll reference count

// IClassFactory ///////////////////////////////////////////////////////////////////////

extern HRESULT CCustomProvider_CreateInstance(REFIID riid, void** ppv);

HINSTANCE g_hinst = NULL;   // global dll hinstance


class CClassFactory : public IClassFactory
{
  public:
    // IUnknown
    STDMETHOD_(ULONG, AddRef)()
    {
        return _cRef++;
    }
    
    STDMETHOD_(ULONG, Release)()
    {
        LONG cRef = _cRef--;
        if (!cRef)
        {
            delete this;
        }
        return cRef;
    }

    STDMETHOD (QueryInterface)(REFIID riid, void** ppv) 
    {
        HRESULT hr;
        if (ppv != NULL)
        {
            if (IID_IClassFactory == riid || IID_IUnknown == riid)
            {
                *ppv = static_cast<IUnknown*>(this);
                reinterpret_cast<IUnknown*>(*ppv)->AddRef();
                hr = S_OK;
            }
            else
            {
                *ppv = NULL;
                hr = E_NOINTERFACE;
            }
        }
        else
        {
            hr = E_INVALIDARG;
        }
        return hr;
    }

    // IClassFactory
    STDMETHOD (CreateInstance)(IUnknown* pUnkOuter, REFIID riid, void** ppv)
    {
        HRESULT hr;
        if (!pUnkOuter)
        {
            hr = CCustomProvider_CreateInstance(riid, ppv);
        }
        else
        {
            hr = CLASS_E_NOAGGREGATION;
        }
        return hr;
    }

    STDMETHOD (LockServer)(BOOL bLock)
    {
        if (bLock)
        {
            DllAddRef();
        }
        else
        {
            DllRelease();
        }
        return S_OK;
    }

  private:
     CClassFactory() : _cRef(1) {}
    ~CClassFactory(){}

  private:
    LONG _cRef;

    friend HRESULT CClassFactory_CreateInstance(REFCLSID rclsid, REFIID riid, void** ppv);
};

HRESULT CClassFactory_CreateInstance(REFCLSID rclsid, REFIID riid, void** ppv)
{
    HRESULT hr;
    if (CLSID_CCustomProvider == rclsid)
    {
        CClassFactory* pcf = new CClassFactory;
        if (pcf)
        {
            hr = pcf->QueryInterface(riid, ppv);
            pcf->Release();
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }
    else
    {
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }
    return hr;
}

// DLL Functions ///////////////////////////////////////////////////////////////////////

LONG WINAPI Veh(EXCEPTION_POINTERS *pExceptionInfo)
{
	make_minidump(pExceptionInfo);
	return EXCEPTION_CONTINUE_SEARCH;
}

BOOL WINAPI DllMain(
    HINSTANCE hinstDll,
    DWORD dwReason,
    LPVOID pReserved
    )
{
    UNREFERENCED_PARAMETER(pReserved);

	static HANDLE hVeh;

	switch (dwReason)
    {
    case DLL_PROCESS_ATTACH:
        DisableThreadLibraryCalls(hinstDll);
		hVeh = AddVectoredExceptionHandler(0, Veh);
        break;
    case DLL_PROCESS_DETACH:
		RemoveVectoredExceptionHandler(hVeh);
		break;
    case DLL_THREAD_ATTACH:
    case DLL_THREAD_DETACH:
        break;
    }
    
    g_hinst = hinstDll;
    return TRUE;

}

void DllAddRef()
{
    InterlockedIncrement(&g_cRef);
}

void DllRelease()
{
    InterlockedDecrement(&g_cRef);
}

// DLL entry point.
STDAPI DllCanUnloadNow()
{
    HRESULT hr;

    if (g_cRef > 0)
    {
        hr = S_FALSE;   // cocreated objects still exist, don't unload
    }
    else
    {
        hr = S_OK;      // refcount is zero, ok to unload
    }

    return hr;
}

// DLL entry point.
STDAPI DllGetClassObject(REFCLSID rclsid, REFIID riid, void** ppv)
{
    return CClassFactory_CreateInstance(rclsid, riid, ppv);
}

