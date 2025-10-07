//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeBasePch.h"

#ifdef _CONTROL_FLOW_GUARD
#if !defined(DELAYLOAD_SET_CFG_TARGET)
extern "C"
WINBASEAPI
BOOL
WINAPI
SetProcessValidCallTargets(
    _In_ HANDLE hProcess,
    _In_ PVOID VirtualAddress,
    _In_ SIZE_T RegionSize,
    _In_ ULONG NumberOfOffsets,
    _In_reads_(NumberOfOffsets) PCFG_CALL_TARGET_INFO OffsetInformation
    );
#endif
#endif

namespace Js
{
    HRESULT DelayLoadWinRtString::WindowsCreateString(_In_reads_opt_(length) const WCHAR * sourceString, UINT32 length, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING * string)
    {
        if (m_hModule)
        {
            if (m_pfnWindowsCreateString == nullptr)
            {
                m_pfnWindowsCreateString = (PFNCWindowsCreateString)GetFunction("WindowsCreateString");
                if (m_pfnWindowsCreateString == nullptr)
                {
                    *string = nullptr;
                    return E_UNEXPECTED;
                }
            }

            Assert(m_pfnWindowsCreateString != nullptr);
            return m_pfnWindowsCreateString(sourceString, length, string);
        }

        *string = nullptr;
        return E_NOTIMPL;
    }

    HRESULT DelayLoadWinRtString::WindowsCreateStringReference(_In_reads_opt_(length + 1) const WCHAR *sourceString, UINT32 length, _Out_ HSTRING_HEADER *hstringHeader, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING * string)
    {
        if (m_hModule)
        {
            if (m_pfnWindowsCreateStringReference == nullptr)
            {
                m_pfnWindowsCreateStringReference = (PFNCWindowsCreateStringReference)GetFunction("WindowsCreateStringReference");
                if (m_pfnWindowsCreateStringReference == nullptr)
                {
                    *string = nullptr;
                    return E_UNEXPECTED;
                }
            }

            Assert(m_pfnWindowsCreateStringReference != nullptr);
            return m_pfnWindowsCreateStringReference(sourceString, length, hstringHeader, string);
        }

        *string = nullptr;
        return E_NOTIMPL;
    }

    HRESULT DelayLoadWinRtString::WindowsDeleteString(_In_opt_ HSTRING string)
    {
        if (m_hModule)
        {
            if (m_pfnWindowsDeleteString == nullptr)
            {
                m_pfnWindowsDeleteString = (PFNCWindowsDeleteString)GetFunction("WindowsDeleteString");
                if (m_pfnWindowsDeleteString == nullptr)
                {
                    return E_UNEXPECTED;
                }
            }

            Assert(m_pfnWindowsDeleteString != nullptr);
            HRESULT hr = m_pfnWindowsDeleteString(string);
            Assert(SUCCEEDED(hr));
            return hr;
        }

        return E_NOTIMPL;
    }

    PCWSTR DelayLoadWinRtString::WindowsGetStringRawBuffer(_In_opt_ HSTRING string, _Out_opt_ UINT32 * length)
    {
        if (m_hModule)
        {
            if (m_pfWindowsGetStringRawBuffer == nullptr)
            {
                m_pfWindowsGetStringRawBuffer = (PFNCWindowsGetStringRawBuffer)GetFunction("WindowsGetStringRawBuffer");
                if (m_pfWindowsGetStringRawBuffer == nullptr)
                {
                    if (length)
                    {
                        *length = 0;
                    }
                    return _u("\0");
                }
            }

            Assert(m_pfWindowsGetStringRawBuffer != nullptr);
            return m_pfWindowsGetStringRawBuffer(string, length);
        }

        if (length)
        {
            *length = 0;
        }
        return _u("\0");
    }

    HRESULT DelayLoadWinRtString::WindowsCompareStringOrdinal(_In_opt_ HSTRING string1, _In_opt_ HSTRING string2, _Out_ INT32 * result)
    {
        if (m_hModule)
        {
            if (m_pfnWindowsCompareStringOrdinal == nullptr)
            {
                m_pfnWindowsCompareStringOrdinal = (PFNCWindowsCompareStringOrdinal)GetFunction("WindowsCompareStringOrdinal");
                if (m_pfnWindowsCompareStringOrdinal == nullptr)
                {
                    return E_UNEXPECTED;
                }
            }

            Assert(m_pfnWindowsCompareStringOrdinal != nullptr);
            return m_pfnWindowsCompareStringOrdinal(string1,string2,result);
        }

        return E_NOTIMPL;
    }
    HRESULT DelayLoadWinRtString::WindowsDuplicateString(_In_opt_ HSTRING original, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING *newString)
    {
        if(m_hModule)
        {
            if(m_pfnWindowsDuplicateString == nullptr)
            {
                m_pfnWindowsDuplicateString = (PFNCWindowsDuplicateString)GetFunction("WindowsDuplicateString");
                if(m_pfnWindowsDuplicateString == nullptr)
                {
                    *newString = nullptr;
                    return E_UNEXPECTED;
                }
            }

            Assert(m_pfnWindowsDuplicateString != nullptr);
            return m_pfnWindowsDuplicateString(original, newString);
        }
        *newString = nullptr;
        return E_NOTIMPL;
    }

#ifdef INTL_WINGLOB
    bool DelayLoadWindowsGlobalization::HasGlobalizationDllLoaded()
    {
        return this->hasGlobalizationDllLoaded;
    }

    HRESULT DelayLoadWindowsGlobalization::DllGetActivationFactory(
        _In_ HSTRING activatableClassId,
        _Out_ IActivationFactory** factory)
    {
        if (m_hModule)
        {
            if (m_pfnFNCWDllGetActivationFactory == nullptr)
            {
                m_pfnFNCWDllGetActivationFactory = (PFNCWDllGetActivationFactory)GetFunction("DllGetActivationFactory");
                if (m_pfnFNCWDllGetActivationFactory == nullptr)
                {
                    return E_UNEXPECTED;
                }
            }

            Assert(m_pfnFNCWDllGetActivationFactory != nullptr);
            return m_pfnFNCWDllGetActivationFactory(activatableClassId, factory);
        }

        return E_NOTIMPL;
    }
#endif

    HRESULT DelayLoadWinRtFoundation::RoGetActivationFactory(
        _In_ HSTRING activatableClassId,
        _In_ REFIID iid,
        _Out_ IActivationFactory** factory)
    {
        if (m_hModule)
        {
            if (m_pfnFNCWRoGetActivationFactory == nullptr)
            {
                m_pfnFNCWRoGetActivationFactory = (PFNCWRoGetActivationFactory)GetFunction("RoGetActivationFactory");
                if (m_pfnFNCWRoGetActivationFactory == nullptr)
                {
                    return E_UNEXPECTED;
                }
            }

            Assert(m_pfnFNCWRoGetActivationFactory != nullptr);
            return m_pfnFNCWRoGetActivationFactory(activatableClassId, iid, factory);
        }

        return E_NOTIMPL;
    }

#ifdef INTL_WINGLOB
    void DelayLoadWindowsGlobalization::Ensure(Js::DelayLoadWinRtString *winRTStringLibrary)
    {
        if (!this->m_isInit)
        {
            DelayLoadLibrary::EnsureFromSystemDirOnly();

#if DBG
            // This unused variable is to allow one to see the value of lastError in case both LoadLibrary (DelayLoadLibrary::Ensure has one) fail.
            // As the issue might be with the first one, as opposed to the second
            DWORD errorWhenLoadingBluePlus = GetLastError();
            Unused(errorWhenLoadingBluePlus);
#endif
            //Perform a check to see if Windows.Globalization.dll was loaded; if not try loading jsIntl.dll as we are on Win7.
            if (m_hModule == nullptr)
            {
                m_hModule = LoadLibraryEx(GetWin7LibraryName(), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
            }

            // Set the flag depending on Windows.globalization.dll or jsintl.dll was loaded successfully or not
            if (m_hModule != nullptr)
            {
                hasGlobalizationDllLoaded = true;
            }
            this->winRTStringLibrary = winRTStringLibrary;
            this->winRTStringsPresent = GetFunction("WindowsDuplicateString") != nullptr;
        }
    }

    HRESULT DelayLoadWindowsGlobalization::WindowsCreateString(_In_reads_opt_(length) const WCHAR * sourceString, UINT32 length, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING * string)
    {
        //If winRtStringLibrary isn't nullptr, that means it is available and we are on Win8+
        if(!winRTStringsPresent && winRTStringLibrary->IsAvailable())
        {
            return winRTStringLibrary->WindowsCreateString(sourceString, length, string);
        }

        return DelayLoadWinRtString::WindowsCreateString(sourceString, length, string);
    }
    HRESULT DelayLoadWindowsGlobalization::WindowsCreateStringReference(_In_reads_opt_(length + 1) const WCHAR * sourceString, UINT32 length, _Out_ HSTRING_HEADER * header, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING * string)
    {
        //First, we attempt to use the WinStringRT api encapsulated in the globalization dll; if it is available then it is a downlevel dll.
        //Otherwise; we might run into an error where we are using the Win8 (because testing is being done for instance) with the downlevel dll, and that would cause errors.
        if(!winRTStringsPresent && winRTStringLibrary->IsAvailable())
        {
            return winRTStringLibrary->WindowsCreateStringReference(sourceString, length, header, string);
        }
        return DelayLoadWinRtString::WindowsCreateStringReference(sourceString, length, header, string);
    }
    HRESULT DelayLoadWindowsGlobalization::WindowsDeleteString(_In_opt_ HSTRING string)
    {
        //First, we attempt to use the WinStringRT api encapsulated in the globalization dll; if it is available then it is a downlevel dll.
        //Otherwise; we might run into an error where we are using the Win8 (because testing is being done for instance) with the downlevel dll, and that would cause errors.
        if(!winRTStringsPresent && winRTStringLibrary->IsAvailable())
        {
            return winRTStringLibrary->WindowsDeleteString(string);
        }
        return DelayLoadWinRtString::WindowsDeleteString(string);
    }
    PCWSTR DelayLoadWindowsGlobalization::WindowsGetStringRawBuffer(_In_opt_ HSTRING string, _Out_opt_ UINT32 * length)
    {
        //First, we attempt to use the WinStringRT api encapsulated in the globalization dll; if it is available then it is a downlevel dll.
        //Otherwise; we might run into an error where we are using the Win8 (because testing is being done for instance) with the downlevel dll, and that would cause errors.
        if(!winRTStringsPresent && winRTStringLibrary->IsAvailable())
        {
            return winRTStringLibrary->WindowsGetStringRawBuffer(string, length);
        }
        return DelayLoadWinRtString::WindowsGetStringRawBuffer(string, length);
    }
    HRESULT DelayLoadWindowsGlobalization::WindowsCompareStringOrdinal(_In_opt_ HSTRING string1, _In_opt_ HSTRING string2, _Out_ INT32 * result)
    {
        //First, we attempt to use the WinStringRT api encapsulated in the globalization dll; if it is available then it is a downlevel dll.
        //Otherwise; we might run into an error where we are using the Win8 (because testing is being done for instance) with the downlevel dll, and that would cause errors.
        if(!winRTStringsPresent && winRTStringLibrary->IsAvailable())
        {
            return winRTStringLibrary->WindowsCompareStringOrdinal(string1, string2, result);
        }
        return DelayLoadWinRtString::WindowsCompareStringOrdinal(string1, string2, result);
    }

    HRESULT DelayLoadWindowsGlobalization::WindowsDuplicateString(_In_opt_ HSTRING original, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING *newString)
    {
        //First, we attempt to use the WinStringRT api encapsulated in the globalization dll; if it is available then it is a downlevel dll.
        //Otherwise; we might run into an error where we are using the Win8 (because testing is being done for instance) with the downlevel dll, and that would cause errors.
        if(!winRTStringsPresent && winRTStringLibrary->IsAvailable())
        {
            return winRTStringLibrary->WindowsDuplicateString(original, newString);
        }
        return DelayLoadWinRtString::WindowsDuplicateString(original, newString);
    }
#endif

    BOOL DelayLoadWinCoreProcessThreads::GetProcessInformation(
        _In_ HANDLE hProcess,
        _In_ PROCESS_INFORMATION_CLASS ProcessInformationClass,
        __out_bcount(nLength) PVOID lpBuffer,
        _In_ SIZE_T nLength
    )
    {
#if defined(DELAYLOAD_SET_CFG_TARGET) || defined(_M_ARM)
        if (m_hModule)
        {
            if (m_pfnGetProcessInformation == nullptr)
            {
                m_pfnGetProcessInformation = (PFNCGetProcessInformation)GetFunction("GetProcessInformation");
                if (m_pfnGetProcessInformation == nullptr)
                {
                    return FALSE;
                }
            }

            Assert(m_pfnGetProcessInformation != nullptr);
            return m_pfnGetProcessInformation(hProcess, ProcessInformationClass, lpBuffer, nLength);
        }
#endif
        return FALSE;
    }
}
