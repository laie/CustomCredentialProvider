//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) 2006 Microsoft Corporation. All rights reserved.
//
// CCustomProvider implements ICredentialProvider, which is the main
// interface that logonUI uses to decide which tiles to display.
// This sample illustrates processing asynchronous external events and 
// using them to provide the user with an appropriate set of credentials.
// In this sample, we provide two credentials: one for when the system
// is "connected" and one for when it isn't. When it's "connected", the
// tile provides the user with a field to log in as the administrator.
// Otherwise, the tile asks the user to connect first.
//

#include <credentialprovider.h>
#include "CCustomCredential.h"
#include "LoginCommander.h"
#include "guid.h"

// CCustomProvider ////////////////////////////////////////////////////////

CCustomProvider::CCustomProvider():
    _cRef(1)
{
    DllAddRef();

    _pcpe = NULL;
    pLoginCommander = NULL;
    _pCredential = NULL;
    _pMessageCredential = NULL;
}

CCustomProvider::~CCustomProvider()
{
    if (_pCredential != NULL)
    {
        _pCredential->Release();
        _pCredential = NULL;
    }
	
    if (pLoginCommander != NULL)
    {
        delete pLoginCommander;
    }

    DllRelease();
}

// This method acts as a callback for the hardware emulator. When it's called, it simply
// tells the infrastructure that it needs to re-enumerate the credentials.
void CCustomProvider::OnConnectStatusChanged()
{
    if (_pcpe != NULL)
    {
        _pcpe->CredentialsChanged(_upAdviseContext);
    }
}
void CCustomProvider::DoLoginWith(const std::wstring& UserName, const std::wstring& Password)
{
	_pCredential->SetLoginInfo(UserName, Password);
}

// SetUsageScenario is the provider's cue that it's going to be asked for tiles
// in a subsequent call.
HRESULT CCustomProvider::SetUsageScenario(
    CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
    DWORD dwFlags
    )
{
    UNREFERENCED_PARAMETER(dwFlags);
    HRESULT hr;

    // Decide which scenarios to support here. Returning E_NOTIMPL simply tells the caller
    // that we're not designed for that scenario.
    switch (cpus)
    {
    case CPUS_LOGON:
    case CPUS_UNLOCK_WORKSTATION:
        _cpus = cpus;

        // Create the CCustomCredential (for connected scenarios), the CMessageCredential
        // (for disconnected scenarios), and the CCommandWindow (to detect commands, such
        // as the connect/disconnect here).  We can get SetUsageScenario multiple times
        // (for example, cancel back out to the CAD screen, and then hit CAD again), 
        // but there's no point in recreating our creds, since they're the same all the
        // time
        if (!_pCredential && !_pMessageCredential && !pLoginCommander)
        {
            // For the locked case, a more advanced credprov might only enumerate tiles for the 
            // user whose owns the locked session, since those are the only creds that will work
			pLoginCommander = new LoginCommander(this);
            _pCredential = new CCustomCredential(pLoginCommander);
            if (_pCredential != NULL && pLoginCommander != NULL)
            {
                _pMessageCredential = new CMessageCredential();
                if (_pMessageCredential)
                {
                    // Initialize each of the object we've just created. 
                    // - The CCommandWindow needs a pointer to us so it can let us know 
                    // when to re-enumerate credentials.
                    // - The CCustomCredential needs field descriptors.
                    // - The CMessageCredential needs field descriptors and a message.
					if ( pLoginCommander->Init() ) hr = 0;
					else hr = -1;
                    if (SUCCEEDED(hr))
                    {
                        hr = _pCredential->Initialize(_cpus, s_rgCredProvFieldDescriptors, s_rgFieldStatePairs, 0);
                        if (SUCCEEDED(hr))
                        {
                            hr = _pMessageCredential->Initialize(s_rgMessageCredProvFieldDescriptors, s_rgMessageFieldStatePairs, L"Please connect");
                        }
                    }
                    else
                    {
                        hr = E_OUTOFMEMORY;
                    }
                }
                else
                {
                    hr = E_OUTOFMEMORY;
                }
            }
            else
            {
                hr = E_OUTOFMEMORY;
            }
            // If anything failed, clean up.
            if (FAILED(hr))
            {
                if (pLoginCommander != NULL)
                {
                    delete pLoginCommander;
                    pLoginCommander = NULL;
                }
                if (_pCredential != NULL)
                {
                    _pCredential->Release();
                    _pCredential = NULL;
                }
                if (_pMessageCredential != NULL)
                {
                    _pMessageCredential->Release();
                    _pMessageCredential = NULL;
                }
            }
        }
        else
        {
            //everything's already all set up
            hr = S_OK;
        }
        break;

    case CPUS_CREDUI:
    case CPUS_CHANGE_PASSWORD:
        hr = E_NOTIMPL;
        break;

    default:
        hr = E_INVALIDARG;
        break;
    }

    return hr;
}

// SetSerialization takes the kind of buffer that you would normally return to LogonUI for
// an authentication attempt.  It's the opposite of ICredentialProviderCredential::GetSerialization.
// GetSerialization is implement by a credential and serializes that credential.  Instead,
// SetSerialization takes the serialization and uses it to create a tile.
//
// SetSerialization is called for two main scenarios.  The first scenario is in the credui case
// where it is prepopulating a tile with credentials that the user chose to store in the OS.
// The second situation is in a remote logon case where the remote client may wish to 
// prepopulate a tile with a username, or in some cases, completely populate the tile and
// use it to logon without showing any UI.
//
// If you wish to see an example of SetSerialization, please see either the SampleCredentialProvider
// sample or the SampleCredUICredentialProvider sample.  [The logonUI team says, "The original sample that
// this was built on top of didn't have SetSerialization.  And when we decided SetSerialization was
// important enough to have in the sample, it ended up being a non-trivial amount of work to integrate
// it into the main sample.  We felt it was more important to get these samples out to you quickly than to
// hold them in order to do the work to integrate the SetSerialization changes from SampleCredentialProvider 
// into this sample.]
STDMETHODIMP CCustomProvider::SetSerialization(
    const CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs
    )
{
    UNREFERENCED_PARAMETER(pcpcs);
    return E_NOTIMPL;
}

// Called by LogonUI to give you a callback. Providers often use the callback if they
// some event would cause them to need to change the set of tiles that they enumerated
HRESULT CCustomProvider::Advise(
    ICredentialProviderEvents* pcpe,
    UINT_PTR upAdviseContext
    )
{
    if (_pcpe != NULL)
    {
        _pcpe->Release();
    }
    _pcpe = pcpe;
    _pcpe->AddRef();
    _upAdviseContext = upAdviseContext;
    return S_OK;
}

// Called by LogonUI when the ICredentialProviderEvents callback is no longer valid.
HRESULT CCustomProvider::UnAdvise()
{
    if (_pcpe != NULL)
    {
        _pcpe->Release();
        _pcpe = NULL;
    }
    return S_OK;
}

// Called by LogonUI to determine the number of fields in your tiles. We return the number
// of fields to be displayed on our active tile, which depends on our connected state. The
// "connected" CCustomCredential has SFI_NUM_FIELDS fields, whereas the "disconnected" 
// CMessageCredential has SMFI_NUM_FIELDS fields.
HRESULT CCustomProvider::GetFieldDescriptorCount(
    DWORD* pdwCount
    )
{
	if (pLoginCommander->GetIsLoggingIn())
    {
        *pdwCount = SFI_NUM_FIELDS;
    }
    else
    {
        *pdwCount = SMFI_NUM_FIELDS;
    }
  
    return S_OK;
}

// Gets the field descriptor for a particular field. Note that we need to determine which
// tile to use based on the "connected" status.
HRESULT CCustomProvider::GetFieldDescriptorAt(
    DWORD dwIndex, 
    CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR** ppcpfd
    )
{    
    HRESULT hr;

    if (pLoginCommander->GetIsLoggingIn())
    {
        // Verify dwIndex is a valid field.
        if ((dwIndex < SFI_NUM_FIELDS) && ppcpfd)
        {
            hr = FieldDescriptorCoAllocCopy(s_rgCredProvFieldDescriptors[dwIndex], ppcpfd);
        }
        else
        { 
            hr = E_INVALIDARG;
        }
    }
    else
    {
        // Verify dwIndex is a valid field.
        if ((dwIndex < SMFI_NUM_FIELDS) && ppcpfd)
        {
            hr = FieldDescriptorCoAllocCopy(s_rgMessageCredProvFieldDescriptors[dwIndex], ppcpfd);
        }
        else
        { 
            hr = E_INVALIDARG;
        }
    }

    return hr;
}

// We only use one tile at any given time since the system can either be "connected" or 
// "disconnected". If we decided that there were multiple valid ways to be connected with
// different sets of credentials, we would provide a combobox in the "connected" tile so
// that the user could pick which one they want to use.
// The last cred prov used gets to select the default user tile
HRESULT CCustomProvider::GetCredentialCount(
    DWORD* pdwCount,
    DWORD* pdwDefault,
    BOOL* pbAutoLogonWithDefault
    )
{
    *pdwCount = 1;
    *pdwDefault = 0;
    *pbAutoLogonWithDefault = pLoginCommander->GetIsLoggingIn();//FALSE;
    return S_OK;
}

// Returns the credential at the index specified by dwIndex. This function is called
// to enumerate the tiles. Note that we need to return the right credential, which depends
// on whether we're connected or not.
HRESULT CCustomProvider::GetCredentialAt(
    DWORD dwIndex, 
    ICredentialProviderCredential** ppcpc
    )
{
    HRESULT hr;
    // Make sure the parameters are valid.
    if ((dwIndex == 0) && ppcpc)
    {
        if (pLoginCommander->GetIsLoggingIn())
        {
            hr = _pCredential->QueryInterface(IID_ICredentialProviderCredential, reinterpret_cast<void**>(ppcpc));
        }
        else
        {
            hr = _pMessageCredential->QueryInterface(IID_ICredentialProviderCredential, reinterpret_cast<void**>(ppcpc));
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }
        
    return hr;
}

// Boilerplate method to create an instance of our provider. 
HRESULT CCustomProvider_CreateInstance(REFIID riid, void** ppv)
{
    HRESULT hr;

    CCustomProvider* pProvider = new CCustomProvider();

    if (pProvider)
    {
        hr = pProvider->QueryInterface(riid, ppv);
        pProvider->Release();
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    
    return hr;
}
