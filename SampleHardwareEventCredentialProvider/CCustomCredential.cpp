//
// THIS CODE AND INFORMATION IS PROVIDED "AS IS" WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESSED OR IMPLIED, INCLUDING BUT NOT LIMITED TO
// THE IMPLIED WARRANTIES OF MERCHANTABILITY AND/OR FITNESS FOR A
// PARTICULAR PURPOSE.
//
// Copyright (c) 2006 Microsoft Corporation. All rights reserved.
//
//

#ifndef WIN32_NO_STATUS
#include <ntstatus.h>
#define WIN32_NO_STATUS
#endif
#include <unknwn.h>
#include <wincred.h>
#include "CCustomCredential.h"
#include "guid.h"

// CCustomCredential ////////////////////////////////////////////////////////

CCustomCredential::CCustomCredential(LoginCommander* pLoginCommander):
    _cRef(1),
    _pCredProvCredentialEvents(NULL),
	pLoginCommander(pLoginCommander)
{
    DllAddRef();

	FieldStrings.resize(SFI_NUM_FIELDS);

    ZeroMemory(_rgCredProvFieldDescriptors, sizeof(_rgCredProvFieldDescriptors));
    ZeroMemory(_rgFieldStatePairs, sizeof(_rgFieldStatePairs));
    //ZeroMemory(_rgFieldStrings, sizeof(_rgFieldStrings));
}

CCustomCredential::~CCustomCredential()
{
    /*if (_rgFieldStrings[SFI_PASSWORD])
    {
        // CoTaskMemFree (below) deals with NULL, but StringCchLength does not.
        size_t lenPassword;
        HRESULT hr = StringCchLengthW(_rgFieldStrings[SFI_PASSWORD], 128, &(lenPassword));
        if (SUCCEEDED(hr))
        {
            SecureZeroMemory(_rgFieldStrings[SFI_PASSWORD], lenPassword * sizeof(*_rgFieldStrings[SFI_PASSWORD]));
        }
        else
        {
            // TODO: Determine how to handle count error here.
        }
    }*/
    for (int i = 0; i < ARRAYSIZE(_rgCredProvFieldDescriptors); i++)
    {
        //CoTaskMemFree(_rgFieldStrings[i]);
        CoTaskMemFree(_rgCredProvFieldDescriptors[i].pszLabel);
    }

    DllRelease();
}
void CCustomCredential::SetLoginInfo(const std::wstring& UserName, const std::wstring& Password)
{
	FieldStrings[SFI_USERNAME] = std::vector<wchar_t>(UserName.begin(), UserName.end());
	FieldStrings[SFI_USERNAME].push_back(0);
	FieldStrings[SFI_PASSWORD] = std::vector<wchar_t>(Password.begin(), Password.end());
	FieldStrings[SFI_PASSWORD].push_back(0);
}
// Initializes one credential with the field information passed in.
// Set the value of the SFI_USERNAME field to pwzUsername.
HRESULT CCustomCredential::Initialize(
                                      CREDENTIAL_PROVIDER_USAGE_SCENARIO cpus,
                                      const CREDENTIAL_PROVIDER_FIELD_DESCRIPTOR* rgcpfd,
                                      const FIELD_STATE_PAIR* rgfsp,
                                      DWORD dwFlags
                                      )
{
    HRESULT hr = S_OK;
    _cpus = cpus;
    _dwFlags = dwFlags;
    // Copy the field descriptors for each field. This is useful if you want to vary the 
    // field descriptors based on what Usage scenario the credential was created for.
    for (DWORD i = 0; SUCCEEDED(hr) && i < ARRAYSIZE(_rgCredProvFieldDescriptors); i++)
    {
        _rgFieldStatePairs[i] = rgfsp[i];
        hr = FieldDescriptorCopy(rgcpfd[i], &_rgCredProvFieldDescriptors[i]);
    }

    // Initialize the String values of all the fields.
    /*if (SUCCEEDED(hr))
    {
        hr = SHStrDupW(pwzUsername, &FieldStrings[SFI_USERNAME].c_str());
    }
    if (SUCCEEDED(hr))
    {
        hr = SHStrDupW(pwzPassword ? pwzPassword : L"", &FieldStrings[SFI_PASSWORD].c_str());
    }
    if (SUCCEEDED(hr))
    {
        hr = SHStrDupW(L"Submit", &FieldStrings[SFI_SUBMIT_BUTTON].c_str());
    }*/
	FieldStrings[SFI_USERNAME].clear();
	FieldStrings[SFI_USERNAME].push_back(0);
	FieldStrings[SFI_PASSWORD].clear();
	FieldStrings[SFI_PASSWORD].push_back(0);

	auto submitstr = std::wstring(L"Submit!");
	FieldStrings[SFI_SUBMIT_BUTTON] = std::vector<wchar_t>(submitstr.begin(), submitstr.end());
	FieldStrings[SFI_SUBMIT_BUTTON].push_back(0);

    return S_OK;
}

// LogonUI calls this in order to give us a callback in case we need to notify it of anything.
HRESULT CCustomCredential::Advise(ICredentialProviderCredentialEvents* pcpce)
{
    if (_pCredProvCredentialEvents != NULL)
    {
        _pCredProvCredentialEvents->Release();
    }
    _pCredProvCredentialEvents = pcpce;
    _pCredProvCredentialEvents->AddRef();
	
    return S_OK;
}

// LogonUI calls this to tell us to release the callback.
HRESULT CCustomCredential::UnAdvise()
{
    if (_pCredProvCredentialEvents)
    {
        _pCredProvCredentialEvents->Release();
    }
    _pCredProvCredentialEvents = NULL;
    return S_OK;
}

// LogonUI calls this function when our tile is selected (zoomed).
// If you simply want fields to show/hide based on the selected state,
// there's no need to do anything here - you can set that up in the 
// field definitions.  But if you want to do something
// more complicated, like change the contents of a field when the tile is
// selected, you would do it here.
HRESULT CCustomCredential::SetSelected(BOOL* pbAutoLogon)  
{
    *pbAutoLogon = false;

    return S_OK;
}

// Similarly to SetSelected, LogonUI calls this when your tile was selected
// and now no longer is. The most common thing to do here (which we do below)
// is to clear out the password field.
HRESULT CCustomCredential::SetDeselected()
{
    HRESULT hr = S_OK;
    /*if (_rgFieldStrings[SFI_PASSWORD])
    {
        // CoTaskMemFree (below) deals with NULL, but StringCchLength does not.
        size_t lenPassword;
        hr = StringCchLengthW(_rgFieldStrings[SFI_PASSWORD], 128, &(lenPassword));
        if (SUCCEEDED(hr))
        {
            SecureZeroMemory(_rgFieldStrings[SFI_PASSWORD], lenPassword * sizeof(*_rgFieldStrings[SFI_PASSWORD]));

            CoTaskMemFree(_rgFieldStrings[SFI_PASSWORD]);
            hr = SHStrDupW(L"", &_rgFieldStrings[SFI_PASSWORD]);
        }

        if (SUCCEEDED(hr) && _pCredProvCredentialEvents)
        {
            _pCredProvCredentialEvents->SetFieldString(this, SFI_PASSWORD, _rgFieldStrings[SFI_PASSWORD]);
        }
    }*/
	FieldStrings[SFI_PASSWORD].clear();
	FieldStrings[SFI_PASSWORD].push_back(0);
	if ( _pCredProvCredentialEvents )
		_pCredProvCredentialEvents->SetFieldString(this, SFI_PASSWORD, FieldStrings[SFI_PASSWORD].data());

    return hr;
}

// Gets info for a particular field of a tile. Called by logonUI to get information to 
// display the tile.
HRESULT CCustomCredential::GetFieldState(
    DWORD dwFieldID,
    CREDENTIAL_PROVIDER_FIELD_STATE* pcpfs,
    CREDENTIAL_PROVIDER_FIELD_INTERACTIVE_STATE* pcpfis
    )
{
    HRESULT hr;

    // Validate paramters.
    if ((dwFieldID < ARRAYSIZE(_rgFieldStatePairs)) && pcpfs && pcpfis)
    {
        *pcpfs = _rgFieldStatePairs[dwFieldID].cpfs;
        *pcpfis = _rgFieldStatePairs[dwFieldID].cpfis;

        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

// Sets ppwz to the string value of the field at the index dwFieldID.
HRESULT CCustomCredential::GetStringValue(
    DWORD dwFieldID, 
    PWSTR* ppwz
    )
{
    HRESULT hr;

    // Check to make sure dwFieldID is a legitimate index.
    if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors) && ppwz) 
    {
        // Make a copy of the string and return that. The caller
        // is responsible for freeing it.
        hr = SHStrDupW(FieldStrings[dwFieldID].data(), ppwz);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

// Gets the image to show in the user tile.
HRESULT CCustomCredential::GetBitmapValue(
    DWORD dwFieldID, 
    HBITMAP* phbmp
    )
{
    HRESULT hr;
    if ((SFI_TILEIMAGE == dwFieldID) && phbmp)
    {
        HBITMAP hbmp = LoadBitmap(HINST_THISDLL, MAKEINTRESOURCE(IDB_TILE_IMAGE));
        if (hbmp != NULL)
        {
            hr = S_OK;
            *phbmp = hbmp;
        }
        else
        {
            hr = HRESULT_FROM_WIN32(GetLastError());
        }
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

// Sets pdwAdjacentTo to the index of the field the submit button should be 
// adjacent to. We recommend that the submit button is placed next to the last
// field which the user is required to enter information in. Optional fields
// should be below the submit button.
HRESULT CCustomCredential::GetSubmitButtonValue(
    DWORD dwFieldID,
    DWORD* pdwAdjacentTo
    )
{
    HRESULT hr;

    // Validate parameters.
    if ((SFI_SUBMIT_BUTTON == dwFieldID) && pdwAdjacentTo)
    {
        // pdwAdjacentTo is a pointer to the fieldID you want the submit button to appear next to.
        *pdwAdjacentTo = SFI_PASSWORD;
        hr = S_OK;
    }
    else
    {
        hr = E_INVALIDARG;
    }
    return hr;
}

// Sets the value of a field which can accept a string as a value.
// This is called on each keystroke when a user types into an edit field.
HRESULT CCustomCredential::SetStringValue(
    DWORD dwFieldID, 
    PCWSTR pwz      
    )
{
    HRESULT hr;

    // Validate parameters.
    if (dwFieldID < ARRAYSIZE(_rgCredProvFieldDescriptors) && 
       (CPFT_EDIT_TEXT == _rgCredProvFieldDescriptors[dwFieldID].cpft || 
        CPFT_PASSWORD_TEXT == _rgCredProvFieldDescriptors[dwFieldID].cpft)) 
    {
		auto str = std::wstring(pwz);
		FieldStrings[dwFieldID] = std::vector<wchar_t>(str.begin(), str.end());
		FieldStrings[dwFieldID].push_back(0);
        //PWSTR* ppwzStored = &FieldStrings[dwFieldID].c_str();
        //CoTaskMemFree(*ppwzStored);
        //hr = SHStrDupW(pwz, ppwzStored);
    }
    else
    {
        hr = E_INVALIDARG;
    }

    return hr;
}

//------------- 
// The following methods are for logonUI to get the values of various UI elements and then communicate
// to the credential about what the user did in that field.  However, these methods are not implemented
// because our tile doesn't contain these types of UI elements
HRESULT CCustomCredential::GetCheckboxValue(
    DWORD dwFieldID, 
    BOOL* pbChecked,
    PWSTR* ppwzLabel
    )
{
    UNREFERENCED_PARAMETER(dwFieldID);
    UNREFERENCED_PARAMETER(pbChecked);
    UNREFERENCED_PARAMETER(ppwzLabel);

    return E_NOTIMPL;
}

HRESULT CCustomCredential::GetComboBoxValueCount(
    DWORD dwFieldID, 
    DWORD* pcItems, 
    DWORD* pdwSelectedItem
    )
{
    UNREFERENCED_PARAMETER(dwFieldID);
    UNREFERENCED_PARAMETER(pcItems);
    UNREFERENCED_PARAMETER(pdwSelectedItem);
    return E_NOTIMPL;
}

HRESULT CCustomCredential::GetComboBoxValueAt(
    DWORD dwFieldID, 
    DWORD dwItem,
    PWSTR* ppwzItem
    )
{
    UNREFERENCED_PARAMETER(dwFieldID);
    UNREFERENCED_PARAMETER(dwItem);
    UNREFERENCED_PARAMETER(ppwzItem);
    return E_NOTIMPL;
}

HRESULT CCustomCredential::SetCheckboxValue(
    DWORD dwFieldID, 
    BOOL bChecked
    )
{
    UNREFERENCED_PARAMETER(dwFieldID);
    UNREFERENCED_PARAMETER(bChecked);

    return E_NOTIMPL;
}

HRESULT CCustomCredential::SetComboBoxSelectedValue(
    DWORD dwFieldId,
    DWORD dwSelectedItem
    )
{
    UNREFERENCED_PARAMETER(dwFieldId);
    UNREFERENCED_PARAMETER(dwSelectedItem);
    return E_NOTIMPL;
}

HRESULT CCustomCredential::CommandLinkClicked(DWORD dwFieldID)
{
    UNREFERENCED_PARAMETER(dwFieldID);
    return E_NOTIMPL;
}
//------ end of methods for controls we don't have in our tile ----//


// Collect the username and password into a serialized credential for the correct usage scenario 
// (logon/unlock is what's demonstrated in this sample).  LogonUI then passes these credentials 
// back to the system to log on.
HRESULT CCustomCredential::GetSerialization(
    CREDENTIAL_PROVIDER_GET_SERIALIZATION_RESPONSE* pcpgsr,
    CREDENTIAL_PROVIDER_CREDENTIAL_SERIALIZATION* pcpcs, 
    PWSTR* ppwzOptionalStatusText, 
    CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon
    )
{
    UNREFERENCED_PARAMETER(ppwzOptionalStatusText);
    UNREFERENCED_PARAMETER(pcpsiOptionalStatusIcon);

    HRESULT hr;

    WCHAR wsz[MAX_COMPUTERNAME_LENGTH+1];
    DWORD cch = ARRAYSIZE(wsz);

    DWORD cb = 0;
    BYTE* rgb = NULL;

    if (GetComputerNameW(wsz, &cch))
    {
        PWSTR pwzProtectedPassword;

		UserNameSerialized = FieldStrings[SFI_USERNAME];
		PasswordProtected = FieldStrings[SFI_PASSWORD];
        hr = ProtectIfNecessaryAndCopyPassword(PasswordProtected, _cpus, &pwzProtectedPassword);

        // Only CredUI scenarios should use CredPackAuthenticationBuffer.  Custom packing logic is necessary for
        // logon and unlock scenarios in order to specify the correct MessageType.
        if (CPUS_CREDUI == _cpus)
        {
            if (SUCCEEDED(hr))
            {
                PWSTR pwzDomainUsername = NULL;
                hr = DomainUsernameStringAlloc(wsz, UserNameSerialized.data(), &pwzDomainUsername);
                if (SUCCEEDED(hr))
                {
                    // We use KERB_INTERACTIVE_UNLOCK_LOGON in both unlock and logon scenarios.  It contains a
                    // KERB_INTERACTIVE_LOGON to hold the creds plus a LUID that is filled in for us by Winlogon
                    // as necessary.
                    if (!CredPackAuthenticationBufferW((CREDUIWIN_PACK_32_WOW & _dwFlags) ? CRED_PACK_WOW_BUFFER : 0, pwzDomainUsername, pwzProtectedPassword, rgb, &cb))
                    {
                        if (ERROR_INSUFFICIENT_BUFFER == GetLastError())
                        {
                            rgb = (BYTE*)HeapAlloc(GetProcessHeap(), 0, cb);
                            if (rgb)
                            {
                                // If the CREDUIWIN_PACK_32_WOW flag is set we need to return 32 bit buffers to our caller we do this by 
                                // passing CRED_PACK_WOW_BUFFER to CredPacAuthenticationBufferW.
                                if (!CredPackAuthenticationBufferW((CREDUIWIN_PACK_32_WOW & _dwFlags) ? CRED_PACK_WOW_BUFFER : 0, pwzDomainUsername, pwzProtectedPassword, rgb, &cb))
                                {
                                    HeapFree(GetProcessHeap(), 0, rgb);
                                    hr = HRESULT_FROM_WIN32(GetLastError());
                                }
                                else
                                {
                                    hr = S_OK;
                                }
                            }
                            else
                            {
                                hr = E_OUTOFMEMORY;
                            }
                        }
                        else
                        {
                            hr = E_FAIL;
                        }
                        HeapFree(GetProcessHeap(), 0, pwzDomainUsername);
                    }
                    else
                    {
                        hr = E_FAIL;
                    }
                }
                CoTaskMemFree(pwzProtectedPassword);
            }
        }
        else
        {
            KERB_INTERACTIVE_UNLOCK_LOGON kiul;

            // Initialize kiul with weak references to our credential.
            hr = KerbInteractiveUnlockLogonInit(wsz, UserNameSerialized.data(), pwzProtectedPassword, _cpus, &kiul);

            if (SUCCEEDED(hr))
            {
                // We use KERB_INTERACTIVE_UNLOCK_LOGON in both unlock and logon scenarios.  It contains a
                // KERB_INTERACTIVE_LOGON to hold the creds plus a LUID that is filled in for us by Winlogon
                // as necessary.
                hr = KerbInteractiveUnlockLogonPack(kiul, &pcpcs->rgbSerialization, &pcpcs->cbSerialization);
            }
        }

        if (SUCCEEDED(hr))
        {
            ULONG ulAuthPackage;
            hr = RetrieveNegotiateAuthPackage(&ulAuthPackage);
            if (SUCCEEDED(hr))
            {
                pcpcs->ulAuthenticationPackage = ulAuthPackage;
                pcpcs->clsidCredentialProvider = CLSID_CCustomProvider;

                // In CredUI scenarios, we must pass back the buffer constructed with CredPackAuthenticationBuffer.
                if (CPUS_CREDUI == _cpus)
                {
                    pcpcs->rgbSerialization = rgb;
                    pcpcs->cbSerialization = cb;
                }

                // At this point the credential has created the serialized credential used for logon
                // By setting this to CPGSR_RETURN_CREDENTIAL_FINISHED we are letting logonUI know
                // that we have all the information we need and it should attempt to submit the 
                // serialized credential.
                *pcpgsr = CPGSR_RETURN_CREDENTIAL_FINISHED;
            }
            else 
            {
                HeapFree(GetProcessHeap(), 0, rgb);
            }
        }
    }
    else
    {
        DWORD dwErr = GetLastError();
        hr = HRESULT_FROM_WIN32(dwErr);
    }

	pLoginCommander->LoginFailed();
	return hr;
}
struct REPORT_RESULT_STATUS_INFO
{
    NTSTATUS ntsStatus;
    NTSTATUS ntsSubstatus;
    PWSTR     pwzMessage;
    CREDENTIAL_PROVIDER_STATUS_ICON cpsi;
};

static const REPORT_RESULT_STATUS_INFO s_rgLogonStatusInfo[] =
{
    { STATUS_LOGON_FAILURE, STATUS_SUCCESS, L"Incorrect password or username: Try set another username and make sure password wan't changed so far.", CPSI_ERROR, },
    { STATUS_ACCOUNT_RESTRICTION, STATUS_ACCOUNT_DISABLED, L"The account is disabled. Enable your account first.", CPSI_WARNING },
};

// ReportResult is completely optional.  Its purpose is to allow a credential to customize the string
// and the icon displayed in the case of a logon failure.  For example, we have chosen to 
// customize the error shown in the case of bad username/password and in the case of the account
// being disabled.
HRESULT CCustomCredential::ReportResult(
                                        NTSTATUS ntsStatus, 
                                        NTSTATUS ntsSubstatus,
                                        PWSTR* ppwzOptionalStatusText, 
                                        CREDENTIAL_PROVIDER_STATUS_ICON* pcpsiOptionalStatusIcon
                                        )
{
    *ppwzOptionalStatusText = NULL;
    *pcpsiOptionalStatusIcon = CPSI_NONE;

    DWORD dwStatusInfo = (DWORD)-1;

    // Look for a match on status and substatus.
    for (DWORD i = 0; i < ARRAYSIZE(s_rgLogonStatusInfo); i++)
    {
        if (s_rgLogonStatusInfo[i].ntsStatus == ntsStatus && s_rgLogonStatusInfo[i].ntsSubstatus == ntsSubstatus)
        {
            dwStatusInfo = i;
            break;
        }
    }

    if ((DWORD)-1 != dwStatusInfo)
    {
        if (SUCCEEDED(SHStrDupW(s_rgLogonStatusInfo[dwStatusInfo].pwzMessage, ppwzOptionalStatusText)))
        {
            *pcpsiOptionalStatusIcon = s_rgLogonStatusInfo[dwStatusInfo].cpsi;
        }
    }
    // If we failed the logon, try to erase the password field.
    if (!SUCCEEDED(HRESULT_FROM_NT(ntsStatus)))
    {
        if (_pCredProvCredentialEvents)
        {
            _pCredProvCredentialEvents->SetFieldString(this, SFI_PASSWORD, L"");
        }
    }

    // Since NULL is a valid value for *ppwzOptionalStatusText and *pcpsiOptionalStatusIcon
    // this function can't fail.
    return S_OK;
}
