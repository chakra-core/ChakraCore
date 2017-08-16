//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------
// Automatic system info getter at startup
//----------------------------------------------------------------------------

class AutoSystemInfo : public SYSTEM_INFO
{
    friend void ChakraBinaryAutoSystemInfoInit(AutoSystemInfo *);  // The hosting DLL provides the implementation of this function.
public:
    static AutoSystemInfo Data;
    uint GetAllocationGranularityPageCount() const;
    uint GetAllocationGranularityPageSize() const;

    bool DisableDebugScopeCapture() const { return this->disableDebugScopeCapture; }
    bool IsCFGEnabled();
    bool IsWin8OrLater();
#if defined(_CONTROL_FLOW_GUARD)
    bool IsWinThresholdOrLater();
#endif
#if defined(_M_IX86) || defined(_M_X64)
    bool VirtualSseAvailable(const int sseLevel) const;
#endif
    BOOL SSE2Available() const;
#if defined(_M_IX86) || defined(_M_X64)
    BOOL SSE3Available() const;
    BOOL SSE4_1Available() const;
    BOOL SSE4_2Available() const;
    BOOL PopCntAvailable() const;
    BOOL LZCntAvailable() const;
    BOOL TZCntAvailable() const;
    bool IsAtomPlatform() const;
#endif
    bool IsLowMemoryProcess();
    BOOL GetAvailableCommit(ULONG64 *pCommit);
    void SetAvailableCommit(ULONG64 commit);
    DWORD GetNumberOfLogicalProcessors() const { return this->dwNumberOfProcessors; }
    DWORD GetNumberOfPhysicalProcessors() const { return this->dwNumberOfPhysicalProcessors; }

#ifdef _WIN32
    bool IsCRTModulePointer(uintptr_t ptr);
#endif

#if SYSINFO_IMAGE_BASE_AVAILABLE
    UINT_PTR GetChakraBaseAddr() const;
#endif

#if defined(_M_ARM32_OR_ARM64)
    bool ArmDivAvailable() const { return this->armDivAvailable; }
#endif
    static DWORD SaveModuleFileName(HANDLE hMod);
    static LPCWSTR GetJscriptDllFileName();
    static HRESULT GetJscriptFileVersion(DWORD* majorVersion, DWORD* minorVersion, DWORD *buildDateHash = nullptr, DWORD *buildTimeHash = nullptr);
#if DBG
    static bool IsInitialized();
#endif
#if SYSINFO_IMAGE_BASE_AVAILABLE
    static bool IsJscriptModulePointer(void * ptr);
#endif
#ifdef _WIN32
    static HMODULE GetCRTHandle();
#endif
    static DWORD const PageSize = 4096;

#ifdef STACK_ALIGN
    static DWORD const StackAlign = STACK_ALIGN;
#else
# if defined(_WIN64)
    static DWORD const StackAlign = 16;
# elif defined(_M_ARM)
    static DWORD const StackAlign = 8;
# elif defined(_M_IX86)
    static DWORD const StackAlign = 4;
# else
    # error missing_target
# endif
#endif

#if SYSINFO_IMAGE_BASE_AVAILABLE
    UINT_PTR dllLoadAddress;
    UINT_PTR dllHighAddress;
#endif
    
private:
    AutoSystemInfo() : majorVersion(0), minorVersion(0), buildDateHash(0), buildTimeHash(0), crtSize(0) { Initialize(); }
    void Initialize();
    bool isWindows8OrGreater;
    uint allocationGranularityPageCount;
    HANDLE processHandle;
    DWORD crtSize;
#if defined(_M_IX86) || defined(_M_X64)
    int CPUInfo[4];
#endif
#if defined(_M_ARM32_OR_ARM64)
    bool armDivAvailable;
#endif
    DWORD dwNumberOfPhysicalProcessors;

    bool disableDebugScopeCapture;
#if DBG
    bool initialized;
#endif

private:
#if defined(_M_IX86) || defined(_M_X64)
    bool isAtom;
    bool CheckForAtom() const;
#endif

    bool InitPhysicalProcessorCount();

    WCHAR binaryName[MAX_PATH + 1];
    DWORD majorVersion;
    DWORD minorVersion;
    DWORD buildDateHash;
    DWORD buildTimeHash;
    static HRESULT GetVersionInfo(__in LPCWSTR pszPath, DWORD* majorVersion, DWORD* minorVersion);

    static const DWORD INVALID_VERSION = (DWORD)-1;

    ULONG64 availableCommit;
    bool shouldQCMoreFrequently;
    bool supportsOnlyMultiThreadedCOM;
    bool isLowMemoryDevice;

public:
    static bool ShouldQCMoreFrequently();
    static bool SupportsOnlyMultiThreadedCOM();
    static bool IsLowMemoryDevice();
};


// For Prefast where it doesn't like symbolic constants
CompileAssert(AutoSystemInfo::PageSize == 4096);
#define __in_ecount_pagesize __in_ecount(4096)
#define __in_ecount_twopagesize __in_ecount(8192)

