//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

//#include <roapi.h>
#include "activation.h"
#include <winstring.h>

#include "RoParameterizedIID.h"

namespace Js
{
    class DelayLoadWinRtString : public DelayLoadLibrary
    {
    private:
        // WinRTString specific functions
        typedef HRESULT FNCWindowsCreateString(const WCHAR *, UINT32, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING *);
        typedef FNCWindowsCreateString* PFNCWindowsCreateString;
        PFNCWindowsCreateString m_pfnWindowsCreateString;

        typedef HRESULT FNCWindowsCreateStringReference(const WCHAR *, UINT32, HSTRING_HEADER *, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING *);
        typedef FNCWindowsCreateStringReference* PFNCWindowsCreateStringReference;
        PFNCWindowsCreateStringReference m_pfnWindowsCreateStringReference;

        typedef PCWSTR FNCWindowsGetStringRawBuffer(HSTRING, UINT32*);
        typedef FNCWindowsGetStringRawBuffer* PFNCWindowsGetStringRawBuffer;
        PFNCWindowsGetStringRawBuffer m_pfWindowsGetStringRawBuffer;

        typedef HRESULT FNCWindowsDeleteString(HSTRING);
        typedef FNCWindowsDeleteString* PFNCWindowsDeleteString;
        PFNCWindowsDeleteString m_pfnWindowsDeleteString;

        typedef HRESULT FNCWindowsCompareStringOrdinal(HSTRING,HSTRING,INT32*);
        typedef FNCWindowsCompareStringOrdinal* PFNCWindowsCompareStringOrdinal;
        PFNCWindowsCompareStringOrdinal m_pfnWindowsCompareStringOrdinal;

        typedef HRESULT FNCWindowsDuplicateString(HSTRING, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING*);
        typedef FNCWindowsDuplicateString* PFNCWindowsDuplicateString;
        PFNCWindowsDuplicateString m_pfnWindowsDuplicateString;

    public:
        DelayLoadWinRtString() : DelayLoadLibrary(),
            m_pfnWindowsCreateString(NULL),
            m_pfWindowsGetStringRawBuffer(NULL),
            m_pfnWindowsDeleteString(NULL),
            m_pfnWindowsCreateStringReference(NULL),
            m_pfnWindowsDuplicateString(NULL),
            m_pfnWindowsCompareStringOrdinal(NULL) { }

        virtual ~DelayLoadWinRtString() { }

        LPCTSTR GetLibraryName() const { return _u("api-ms-win-core-winrt-string-l1-1-0.dll"); }

        virtual HRESULT WindowsCreateString(_In_reads_opt_(length) const WCHAR * sourceString, UINT32 length, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING * string);
        virtual HRESULT WindowsCreateStringReference(_In_reads_opt_(length + 1) const WCHAR * sourceString, UINT32 length, _Out_ HSTRING_HEADER * header, _Outptr_result_maybenull_ _Result_nullonfailure_  HSTRING * string);
        virtual HRESULT WindowsDeleteString(_In_opt_ HSTRING string);
        virtual PCWSTR WindowsGetStringRawBuffer(_In_opt_ HSTRING string, _Out_opt_ UINT32 * length);
        virtual HRESULT WindowsCompareStringOrdinal(_In_opt_ HSTRING string1, _In_opt_ HSTRING string2, _Out_ INT32 * result);
        virtual HRESULT WindowsDuplicateString(_In_opt_ HSTRING original, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING * newString);
    };

#ifdef INTL_WINGLOB
    class DelayLoadWindowsGlobalization sealed : public DelayLoadWinRtString
    {
    private:
        // DelayLoadWindowsGlobalization specific functions
        typedef HRESULT FNCWDllGetActivationFactory(HSTRING clsid, IActivationFactory** factory);
        typedef FNCWDllGetActivationFactory* PFNCWDllGetActivationFactory;
        PFNCWDllGetActivationFactory m_pfnFNCWDllGetActivationFactory;

        Js::DelayLoadWinRtString *winRTStringLibrary;
        bool winRTStringsPresent;
        bool hasGlobalizationDllLoaded;

    public:
        DelayLoadWindowsGlobalization() : DelayLoadWinRtString(),
            m_pfnFNCWDllGetActivationFactory(nullptr),
            winRTStringLibrary(nullptr),
            winRTStringsPresent(false),
            hasGlobalizationDllLoaded(false) { }

        virtual ~DelayLoadWindowsGlobalization() { }

        LPCTSTR GetLibraryName() const
        {
            return _u("windows.globalization.dll");
        }
        LPCTSTR GetWin7LibraryName() const
        {
            return _u("jsIntl.dll");
        }
        void Ensure(Js::DelayLoadWinRtString *winRTStringLibrary);

        HRESULT DllGetActivationFactory(_In_ HSTRING activatibleClassId, _Out_ IActivationFactory** factory);
        bool HasGlobalizationDllLoaded();

        HRESULT WindowsCreateString(_In_reads_opt_(length) const WCHAR * sourceString, UINT32 length, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING * string) override;
        HRESULT WindowsCreateStringReference(_In_reads_opt_(length+1) const WCHAR * sourceString, UINT32 length, _Out_ HSTRING_HEADER * header, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING * string) override;
        HRESULT WindowsDeleteString(_In_opt_ HSTRING string) override;
        PCWSTR WindowsGetStringRawBuffer(_In_opt_ HSTRING string, _Out_opt_ UINT32 * length) override;
        HRESULT WindowsCompareStringOrdinal(_In_opt_ HSTRING string1, _In_opt_ HSTRING string2, _Out_ INT32 * result) override;
        HRESULT WindowsDuplicateString(_In_opt_ HSTRING original, _Outptr_result_maybenull_ _Result_nullonfailure_ HSTRING *newString) override;
    };
#endif

    class DelayLoadWinRtFoundation sealed : public DelayLoadLibrary
    {
    private:

        // DelayLoadWindowsFoundation specific functions
        typedef HRESULT FNCWRoGetActivationFactory(HSTRING clsid, REFIID iid, IActivationFactory** factory);

        typedef FNCWRoGetActivationFactory* PFNCWRoGetActivationFactory;
        PFNCWRoGetActivationFactory m_pfnFNCWRoGetActivationFactory;

    public:
        DelayLoadWinRtFoundation() : DelayLoadLibrary(),
            m_pfnFNCWRoGetActivationFactory(nullptr) { }

        virtual ~DelayLoadWinRtFoundation() { }

        LPCTSTR GetLibraryName() const { return _u("api-ms-win-core-winrt-l1-1-0.dll"); }

        HRESULT RoGetActivationFactory(
            _In_ HSTRING activatibleClassId,
            _In_ REFIID iid,
            _Out_ IActivationFactory** factory);
    };

    class DelayLoadWinCoreProcessThreads sealed : public DelayLoadLibrary
    {
    private:
        // LoadWinCoreMemory specific functions

        typedef BOOL FNCGetProcessInformation(HANDLE, PROCESS_INFORMATION_CLASS, PVOID, SIZE_T);
        typedef FNCGetProcessInformation* PFNCGetProcessInformation;
        PFNCGetProcessInformation m_pfnGetProcessInformation;

    public:
        DelayLoadWinCoreProcessThreads() :
            DelayLoadLibrary(),
            m_pfnGetProcessInformation(nullptr)
        {
        }

        LPCTSTR GetLibraryName() const { return _u("api-ms-win-core-processthreads-l1-1-3.dll"); }

        BOOL GetProcessInformation(
            _In_ HANDLE hProcess,
            _In_ PROCESS_INFORMATION_CLASS ProcessInformationClass,
            __out_bcount(nLength) PVOID lpBuffer,
            _In_ SIZE_T nLength
        );
    };
 }
