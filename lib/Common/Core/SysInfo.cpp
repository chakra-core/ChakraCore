//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonCorePch.h"
#ifdef _WIN32
#include <psapi.h>
#endif
#include <wincrypt.h>
#include <VersionHelpers.h>
#ifdef __APPLE__
#include <sys/sysctl.h> // sysctl*
#elif defined(__linux__)
#include <unistd.h> // sysconf
#endif
// Initialization order
//  AB AutoSystemInfo
//  AD PerfCounter
//  AE PerfCounterSet
//  AM Output/Configuration
//  AN MemProtectHeap
//  AP DbgHelpSymbolManager
//  AQ CFGLogger
//  AR LeakReport
//  AS JavascriptDispatch/RecyclerObjectDumper
//  AT HeapAllocator/RecyclerHeuristic
//  AU RecyclerWriteBarrierManager
#pragma warning(disable:4075)       // initializers put in unrecognized initialization area on purpose
#pragma init_seg(".CRT$XCAB")

#if SYSINFO_IMAGE_BASE_AVAILABLE
EXTERN_C IMAGE_DOS_HEADER __ImageBase;
#endif

AutoSystemInfo AutoSystemInfo::Data INIT_PRIORITY(300);

#if DBG
bool
AutoSystemInfo::IsInitialized()
{
    return AutoSystemInfo::Data.initialized;
}
#endif

bool
AutoSystemInfo::ShouldQCMoreFrequently()
{
    return Data.shouldQCMoreFrequently;
}

bool
AutoSystemInfo::SupportsOnlyMultiThreadedCOM()
{
    return Data.supportsOnlyMultiThreadedCOM;
}

bool
AutoSystemInfo::IsLowMemoryDevice()
{
    return Data.isLowMemoryDevice;
}

void
AutoSystemInfo::Initialize()
{
    Assert(!initialized);
#ifndef _WIN32
    PAL_InitializeChakraCore();
    majorVersion = CHAKRA_CORE_MAJOR_VERSION;
    minorVersion = CHAKRA_CORE_MINOR_VERSION;
#endif

    processHandle = GetCurrentProcess();
    GetSystemInfo(this);

    // Make the page size constant so calculation are faster.
    Assert(this->dwPageSize == AutoSystemInfo::PageSize);
#if defined(_M_IX86) || defined(_M_X64)
    get_cpuid(CPUInfo, 1);
    isAtom = CheckForAtom();
#endif
#if defined(_M_ARM32_OR_ARM64)
    armDivAvailable = IsProcessorFeaturePresent(PF_ARM_DIVIDE_INSTRUCTION_AVAILABLE) ? true : false;
#endif
    allocationGranularityPageCount = dwAllocationGranularity / dwPageSize;

    isWindows8OrGreater = IsWindows8OrGreater();

    binaryName[0] = _u('\0');

#if SYSINFO_IMAGE_BASE_AVAILABLE
    dllLoadAddress = (UINT_PTR)&__ImageBase;
    dllHighAddress = (UINT_PTR)&__ImageBase +
        ((PIMAGE_NT_HEADERS)(((char *)&__ImageBase) + __ImageBase.e_lfanew))->OptionalHeader.SizeOfImage;
#endif

    InitPhysicalProcessorCount();
#if DBG
    initialized = true;
#endif

    WCHAR DisableDebugScopeCaptureFlag[MAX_PATH];
    if (::GetEnvironmentVariable(_u("JS_DEBUG_SCOPE"), DisableDebugScopeCaptureFlag, _countof(DisableDebugScopeCaptureFlag)) != 0)
    {
        disableDebugScopeCapture = true;
    }
    else
    {
        disableDebugScopeCapture = false;
    }

    this->supportsOnlyMultiThreadedCOM = false;
#if defined(__ANDROID__) || defined(__IOS__)
    this->isLowMemoryDevice = true;
    this->shouldQCMoreFrequently = true;
#else
    this->shouldQCMoreFrequently = false;
    this->isLowMemoryDevice = false;
#endif

    // 0 indicates we haven't retrieved the available commit. We get it lazily.
    this->availableCommit = 0;

    ChakraBinaryAutoSystemInfoInit(this);
}


bool
AutoSystemInfo::InitPhysicalProcessorCount()
{
    DWORD countPhysicalProcessor = 0;
#ifdef _WIN32
    DWORD size = 0;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION pBufferCurrent;
    PSYSTEM_LOGICAL_PROCESSOR_INFORMATION pBufferStart;
    BOOL bResult;
#endif // _WIN32
    Assert(!this->initialized);

    // Initialize physical processor to number of logical processors.
    // If anything below fails, we still need an approximate value

    this->dwNumberOfPhysicalProcessors = this->dwNumberOfProcessors;

#if defined(_WIN32)
    bResult = GetLogicalProcessorInformation(NULL, &size);

    if (bResult || GetLastError() != ERROR_INSUFFICIENT_BUFFER || !size)
    {
        return false;
    }

    DWORD count = (size) / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION);
    if (size != count * sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION))
    {
        Assert(false);
        return false;
    }

    pBufferCurrent = pBufferStart = NoCheckHeapNewArray(SYSTEM_LOGICAL_PROCESSOR_INFORMATION, (size_t)count);
    if (!pBufferCurrent)
    {
        return false;
    }

    bResult = GetLogicalProcessorInformation(pBufferCurrent, &size);
    if (!bResult)
    {
        NoCheckHeapDeleteArray(count, pBufferStart);
        return false;
    }

    while (pBufferCurrent < (pBufferStart + count))
    {
        if (pBufferCurrent->Relationship == RelationProcessorCore)
        {
            countPhysicalProcessor++;
        }
        pBufferCurrent++;
    }

    NoCheckHeapDeleteArray(count, pBufferStart);
#elif defined(__APPLE__)
    std::size_t szCount = sizeof(countPhysicalProcessor);
    sysctlbyname("hw.physicalcpu", &countPhysicalProcessor, &szCount, nullptr, 0);

    if (countPhysicalProcessor < 1)
    {
        int nMIB[2] = {CTL_HW, HW_NCPU}; // fallback. Depracated on latest OS
        sysctl(nMIB, 2, &countPhysicalProcessor, &szCount, nullptr, 0);
        if (countPhysicalProcessor < 1)
        {
            countPhysicalProcessor = 1;
        }
    }
#elif defined(__linux__)
    countPhysicalProcessor = sysconf(_SC_NPROCESSORS_ONLN);
#else
    // implementation for __linux__ should work for some others.
    // same applies to __APPLE__ implementation
    // instead of reimplementing, add corresponding preprocessors above
#error "NOT Implemented"
#endif
    this->dwNumberOfPhysicalProcessors = countPhysicalProcessor;

    return true;
}

#if SYSINFO_IMAGE_BASE_AVAILABLE
bool
AutoSystemInfo::IsJscriptModulePointer(void * ptr)
{
    return ((UINT_PTR)ptr >= Data.dllLoadAddress && (UINT_PTR)ptr < Data.dllHighAddress);
}

UINT_PTR
AutoSystemInfo::GetChakraBaseAddr() const
{
    return dllLoadAddress;
}
#endif

uint
AutoSystemInfo::GetAllocationGranularityPageCount() const
{
    Assert(initialized);
    return this->allocationGranularityPageCount;
}

uint
AutoSystemInfo::GetAllocationGranularityPageSize() const
{
    Assert(initialized);
    return this->allocationGranularityPageCount * PageSize;
}

#if defined(_M_IX86) || defined(_M_X64)
bool
AutoSystemInfo::VirtualSseAvailable(const int sseLevel) const
{
    #ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        return CONFIG_FLAG(Sse) < 0 || CONFIG_FLAG(Sse) >= sseLevel;
    #else
        return true;
    #endif
}
#endif

BOOL
AutoSystemInfo::SSE2Available() const
{
    Assert(initialized);

#if defined(_M_X64) || defined(_M_ARM32_OR_ARM64)
    return true;
#elif defined(_M_IX86)
#if defined(_WIN32)
    return VirtualSseAvailable(2) && (CPUInfo[3] & (1 << 26));
#else
    return false; // TODO: xplat support
#endif
#else
    return false;
#endif
}

#if defined(_M_IX86) || defined(_M_X64)
BOOL
AutoSystemInfo::SSE3Available() const
{
    Assert(initialized);
    return VirtualSseAvailable(3) && (CPUInfo[2] & 0x1);
}

BOOL
AutoSystemInfo::SSE4_1Available() const
{
    Assert(initialized);
    return VirtualSseAvailable(4) && (CPUInfo[2] & (0x1 << 19));
}

BOOL
AutoSystemInfo::SSE4_2Available() const
{
    Assert(initialized);
    return VirtualSseAvailable(4) && (CPUInfo[2] & (0x1 << 20));
}

BOOL
AutoSystemInfo::PopCntAvailable() const
{
    Assert(initialized);
    return VirtualSseAvailable(4) && (CPUInfo[2] & (1 << 23));
}

BOOL
AutoSystemInfo::LZCntAvailable() const
{
    Assert(initialized);
    int CPUInfo[4];
    get_cpuid(CPUInfo, 0x80000001);

    return VirtualSseAvailable(4) && (CPUInfo[2] & (1 << 5));
}

BOOL
AutoSystemInfo::TZCntAvailable() const
{
    Assert(initialized);
    int CPUInfo[4];
    get_cpuid(CPUInfo, 7);

    return VirtualSseAvailable(4) && (CPUInfo[1] & (1 << 3));
}

bool
AutoSystemInfo::IsAtomPlatform() const
{
    return isAtom;
}

bool
AutoSystemInfo::CheckForAtom() const
{
    int CPUInfo[4];
    const int GENUINE_INTEL_0 = 0x756e6547,
              GENUINE_INTEL_1 = 0x49656e69,
              GENUINE_INTEL_2 = 0x6c65746e;
    const int PLATFORM_MASK = 0x0fff3ff0;
    const int ATOM_PLATFORM_A = 0x0106c0, /* bonnell - extended model 1c, type 0, family code 6 */
              ATOM_PLATFORM_B = 0x020660, /* lincroft - extended model 26, type 0, family code 6 */
              ATOM_PLATFORM_C = 0x020670, /* saltwell - extended model 27, type 0, family code 6 */
              ATOM_PLATFORM_D = 0x030650, /* tbd - extended model 35, type 0, family code 6 */
              ATOM_PLATFORM_E = 0x030660, /* tbd - extended model 36, type 0, family code 6 */
              ATOM_PLATFORM_F = 0x030670; /* tbd - extended model 37, type 0, family code 6 */
    int platformSignature;

    get_cpuid(CPUInfo, 0);

    // See if CPU is ATOM HW. First check if CPU is genuine Intel.
    if( CPUInfo[1]==GENUINE_INTEL_0 &&
        CPUInfo[3]==GENUINE_INTEL_1 &&
        CPUInfo[2]==GENUINE_INTEL_2)
    {
        get_cpuid(CPUInfo, 1);
        // get platform signature
        platformSignature = CPUInfo[0];
        if((( PLATFORM_MASK & platformSignature) == ATOM_PLATFORM_A) ||
            ((PLATFORM_MASK & platformSignature) == ATOM_PLATFORM_B) ||
            ((PLATFORM_MASK & platformSignature) == ATOM_PLATFORM_C) ||
            ((PLATFORM_MASK & platformSignature) == ATOM_PLATFORM_D) ||
            ((PLATFORM_MASK & platformSignature) == ATOM_PLATFORM_E) ||
            ((PLATFORM_MASK & platformSignature) == ATOM_PLATFORM_F))
        {
            return true;
        }

    }
    return false;
}
#endif

bool
AutoSystemInfo::IsCFGEnabled()
{
#if defined(_CONTROL_FLOW_GUARD)
    return true
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        && IsWinThresholdOrLater() && !PHASE_OFF1(Js::CFGPhase)
#endif //ENABLE_DEBUG_CONFIG_OPTIONS
        ;
#else
    return false;
#endif //_CONTROL_FLOW_GUARD
}

bool
AutoSystemInfo::IsWin8OrLater()
{
    return isWindows8OrGreater;
}

#if defined(_CONTROL_FLOW_GUARD)
bool
AutoSystemInfo::IsWinThresholdOrLater()
{
    return IsWindowsThresholdOrGreater();
}
#endif

DWORD AutoSystemInfo::SaveModuleFileName(HANDLE hMod)
{
    return ::GetModuleFileNameW((HMODULE)hMod, Data.binaryName, MAX_PATH);
}

LPCWSTR AutoSystemInfo::GetJscriptDllFileName()
{
    return (LPCWSTR)Data.binaryName;
}

#ifdef _WIN32
/* static */
HMODULE AutoSystemInfo::GetCRTHandle()
{
    return GetModuleHandle(_u("msvcrt.dll"));
}

bool
AutoSystemInfo::IsCRTModulePointer(uintptr_t ptr)
{
    HMODULE base = GetCRTHandle();
    if (Data.crtSize == 0)
    {
        MODULEINFO info;
        if (!GetModuleInformation(GetCurrentProcess(), base, &info, sizeof(MODULEINFO)))
        {
            AssertOrFailFast(UNREACHED);
        }
        Data.crtSize = info.SizeOfImage;
        Assert(base == info.lpBaseOfDll);
    }
    return (ptr >= (uintptr_t)base && ptr < (uintptr_t)base + Data.crtSize);
}
#endif

bool AutoSystemInfo::IsLowMemoryProcess()
{
    ULONG64 commit = ULONG64(-1);
    this->GetAvailableCommit(&commit);
    return commit <= CONFIG_FLAG(LowMemoryCap);
}

BOOL AutoSystemInfo::GetAvailableCommit(ULONG64 *pCommit)
{
    Assert(initialized);

    // Non-zero value indicates we've been here before.
    if (this->availableCommit == 0)
    {
        return false;
    }

    *pCommit = this->availableCommit;
    return true;
}

void AutoSystemInfo::SetAvailableCommit(ULONG64 commit)
{
    ::InterlockedCompareExchange64((volatile LONG64 *)&this->availableCommit, commit, 0);
}

//
// Returns the major and minor version of the loaded binary. If the version info has been fetched once, it will be cached
// and returned without any system calls to find the version number.
//
HRESULT AutoSystemInfo::GetJscriptFileVersion(DWORD* majorVersion, DWORD* minorVersion, DWORD *buildDateHash, DWORD *buildTimeHash)
{
    HRESULT hr = E_FAIL;
    if(AutoSystemInfo::Data.majorVersion == 0 && AutoSystemInfo::Data.minorVersion == 0)
    {
        // uninitialized state  - call the system API to get the version info.
        LPCWSTR jscriptDllName = GetJscriptDllFileName();
        hr = GetVersionInfo(jscriptDllName, majorVersion, minorVersion);

        AutoSystemInfo::Data.majorVersion = *majorVersion;
        AutoSystemInfo::Data.minorVersion = *minorVersion;
    }
    else if(AutoSystemInfo::Data.majorVersion != INVALID_VERSION)
    {
        // if the cached copy is valid, use it and return S_OK.
        *majorVersion = AutoSystemInfo::Data.majorVersion;
        *minorVersion = AutoSystemInfo::Data.minorVersion;
        hr = S_OK;
    }

    if (buildDateHash)
    {
        *buildDateHash = AutoSystemInfo::Data.buildDateHash;
    }

    if (buildTimeHash)
    {
        *buildTimeHash = AutoSystemInfo::Data.buildTimeHash;
    }
    return hr;
}

//
// Returns the major and minor version of the binary passed as argument.
//
HRESULT AutoSystemInfo::GetVersionInfo(__in LPCWSTR pszPath, DWORD* majorVersion, DWORD* minorVersion)
{
#ifdef _WIN32
    DWORD   dwTemp;
    DWORD   cbVersionSz;
    HRESULT hr = E_FAIL;
    BYTE*    pVerBuffer = NULL;
    VS_FIXEDFILEINFO* pFileInfo = NULL;
    cbVersionSz = GetFileVersionInfoSizeEx(FILE_VER_GET_LOCALISED, pszPath, &dwTemp);
    if(cbVersionSz > 0)
    {
        pVerBuffer = NoCheckHeapNewArray(BYTE, cbVersionSz);
        if(pVerBuffer)
        {
            if(GetFileVersionInfoEx(FILE_VER_GET_LOCALISED|FILE_VER_GET_NEUTRAL, pszPath, 0, cbVersionSz, pVerBuffer))
            {
                UINT    uiSz = sizeof(VS_FIXEDFILEINFO);
                if(!VerQueryValue(pVerBuffer, _u("\\"), (LPVOID*)&pFileInfo, &uiSz))
                {
                    hr = HRESULT_FROM_WIN32(GetLastError());
                }
                else
                {
                    hr = S_OK;
                }
            }
            else
            {
                hr = HRESULT_FROM_WIN32(GetLastError());
            }
        }
        else
        {
            hr = E_OUTOFMEMORY;
        }
    }

    if(SUCCEEDED(hr))
    {
        *majorVersion = pFileInfo->dwFileVersionMS;
        *minorVersion = pFileInfo->dwFileVersionLS;
    }
    else
    {
        *majorVersion = INVALID_VERSION;
        *minorVersion = INVALID_VERSION;
    }
    if(pVerBuffer)
    {
        NoCheckHeapDeleteArray(cbVersionSz, pVerBuffer);
    }
    return hr;
#else // !_WIN32
    // xplat-todo: how to handle version resource?
    *majorVersion = INVALID_VERSION;
    *minorVersion = INVALID_VERSION;
    return NOERROR;
#endif
}
