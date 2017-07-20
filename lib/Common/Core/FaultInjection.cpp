//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonCorePch.h"

#ifdef FAULT_INJECTION
#include "io.h"
#include "share.h"

#undef DBGHELP_TRANSLATE_TCHAR
#define _NO_CVCONST_H

// dbghelp.h is not clean with warning 4091
#pragma warning(push)
#pragma warning(disable: 4091) /* warning C4091: 'typedef ': ignored on left of '' when no variable is declared */
#include <dbghelp.h>
#pragma warning(pop)

namespace Js
{

#pragma region helpers

#define FIDELAYLOAD(fn) static decltype(fn)* pfn##fn = nullptr
    FIDELAYLOAD(SymInitialize);
    FIDELAYLOAD(SymCleanup);
    FIDELAYLOAD(SymFromAddrW);
    FIDELAYLOAD(SymFromNameW);
    FIDELAYLOAD(SymEnumSymbolsW);
    FIDELAYLOAD(SymGetModuleInfoW64);
    FIDELAYLOAD(SymMatchStringW);
    FIDELAYLOAD(SymSetOptions);
    FIDELAYLOAD(MiniDumpWriteDump);
    FIDELAYLOAD(SymFunctionTableAccess64);
    FIDELAYLOAD(SymGetModuleBase64);
    FIDELAYLOAD(StackWalk64);
#undef FIDELAYLOAD

    template<typename CharT>
    bool isEqualIgnoreCase(CharT c1, CharT c2)
    {
        return c1 == c2
            || ((c2 <= 'Z') && (c1 >= 'a') && (c1 - c2 == 'a' - 'A'))
            || ((c1 <= 'Z') && (c2 >= 'a') && (c2 - c1 == 'a' - 'A'));
    }

    template<typename CharT>
    CharT *stristr(const CharT * cs1,
        const CharT * cs2)
    {
        CharT *cp = (CharT *)cs1;
        CharT *s1, *s2;

        if (!*cs2)
            return (CharT *)cs1;

        while (*cp)
        {
            s1 = cp;
            s2 = (CharT *)cs2;

            while (*s1 && *s2 && isEqualIgnoreCase(*s1, *s2))
                s1++, s2++;

            if (!*s2)
                return cp;

            cp++;
        }

        return nullptr;
    }

    static char16* trimRight(_Inout_z_ char16* str)
    {
        auto tmp = str + wcslen(str);
        while (!isprint(*--tmp));
        *(tmp + 1) = _u('\0');
        return str;
    }

    static int8 const* hexTable = []()->int8*{
        static int8 hex[256] = { 0 };
        memset(hex, 0xff, 256);
        for (int8 i = '0'; i <= '9'; i++) hex[i] = i - '0';
        for (int8 i = 'a'; i <= 'f'; i++) hex[i] = i - 'a' + 10;
        for (int8 i = 'A'; i <= 'F'; i++) hex[i] = i - 'A' + 10;
        return hex;
    }();

    template<typename CharT>
    static UINT_PTR HexStrToAddress(const CharT* str)
    {
        UINT_PTR address = 0;
        while (*str == '0' || *str == '`' || *str == 'x' || *str == 'X')
            str++; // leading zero
        do
        {
            if (*str == '`') // amd64 address
                continue;
            if (hexTable[*str & 0xff] < 0)
                return address;
            address = 16 * address + hexTable[*str & 0xff];
        } while (*(++str));
        return address;
    }


#if _M_X64
    // for amd64 jit frame, RtlCaptureStackBackTrace stops walking after hitting jit frame on amd64
    _NOINLINE
        WORD StackTrace64(_In_ DWORD FramesToSkip,
        _In_ DWORD FramesToCapture,
        _Out_writes_to_(FramesToCapture, return) PVOID * BackTrace,
        _Out_opt_ PDWORD BackTraceHash,
        _In_opt_ const CONTEXT* pCtx = nullptr)
    {
        CONTEXT                         Context;
        KNONVOLATILE_CONTEXT_POINTERS   NvContext;
        UNWIND_HISTORY_TABLE            UnwindHistoryTable;
        PRUNTIME_FUNCTION               RuntimeFunction;
        PVOID                           HandlerData;
        ULONG64                         EstablisherFrame;
        ULONG64                         ImageBase;
        ULONG                           Frame = 0;

        if (BackTraceHash)
        {
            *BackTraceHash = 0;
        }
        if (pCtx == nullptr)
        {
            RtlCaptureContext(&Context);
        }
        else
        {
            memcpy(&Context, pCtx, sizeof(CONTEXT));
        }
        RtlZeroMemory(&UnwindHistoryTable, sizeof(UNWIND_HISTORY_TABLE));
        while (true)
        {
            RuntimeFunction = RtlLookupFunctionEntry(Context.Rip, &ImageBase, &UnwindHistoryTable);
            RtlZeroMemory(&NvContext, sizeof(KNONVOLATILE_CONTEXT_POINTERS));
            if (!RuntimeFunction)
            {
                Context.Rip = (ULONG64)(*(PULONG64)Context.Rsp);
                Context.Rsp += 8;
            }
            else
            {
                RtlVirtualUnwind(UNW_FLAG_NHANDLER, ImageBase, Context.Rip, RuntimeFunction,
                    &Context, &HandlerData, &EstablisherFrame, &NvContext);
            }

            if (!Context.Rip)
            {
                break;
            }

            if (FramesToSkip > 0)
            {
                FramesToSkip--;
                continue;
            }

            if (Frame >= FramesToCapture)
            {
                break;
            }

            BackTrace[Frame] = (PVOID)Context.Rip;
            if (BackTraceHash)
            {
                *BackTraceHash += (Context.Rip & 0xffffffff);
            }
            Frame++;
        }

        return (WORD)Frame;
    }

#define CaptureStack(FramesToSkip, FramesToCapture, BackTrace, BackTraceHash) \
    StackTrace64(FramesToSkip, FramesToCapture, BackTrace, BackTraceHash)
#elif defined (_M_IX86)
#pragma optimize( "g", off )
#pragma warning( push )
#pragma warning( disable : 4748 )
#pragma warning( disable : 4995 )
    WORD StackTrace86(
        _In_ DWORD FramesToSkip,
        _In_ DWORD FramesToCapture,
        _Out_writes_to_(FramesToCapture, return) PVOID * BackTrace,
        _Inout_opt_ PDWORD BackTraceHash,
        __in_opt CONST PCONTEXT InitialContext = NULL
        )
    {
        _Analysis_assume_(FramesToSkip >= 0);
        _Analysis_assume_(FramesToCapture >= 0);
        DWORD MachineType;
        CONTEXT Context;
        STACKFRAME64 StackFrame;

        if (InitialContext == NULL)
        {
            //RtlCaptureContext( &Context );
            ZeroMemory(&Context, sizeof(CONTEXT));
            Context.ContextFlags = CONTEXT_CONTROL;
            __asm
            {
            Label:
                mov[Context.Ebp], ebp;
                mov[Context.Esp], esp;
                mov eax, [Label];
                mov[Context.Eip], eax;
            }
        }
        else
        {
            CopyMemory(&Context, InitialContext, sizeof(CONTEXT));
        }

        ZeroMemory(&StackFrame, sizeof(STACKFRAME64));
        MachineType = IMAGE_FILE_MACHINE_I386;
        StackFrame.AddrPC.Offset = Context.Eip;
        StackFrame.AddrPC.Mode = AddrModeFlat;
        StackFrame.AddrFrame.Offset = Context.Ebp;
        StackFrame.AddrFrame.Mode = AddrModeFlat;
        StackFrame.AddrStack.Offset = Context.Esp;
        StackFrame.AddrStack.Mode = AddrModeFlat;

        WORD FrameCount = 0;
        while (FrameCount < FramesToSkip + FramesToCapture)
        {
            if (!pfnStackWalk64(MachineType, GetCurrentProcess(), GetCurrentThread(), &StackFrame,
                NULL, NULL, pfnSymFunctionTableAccess64, pfnSymGetModuleBase64, NULL))
            {
                break;
            }

            if (StackFrame.AddrPC.Offset != 0)
            {
                if (FrameCount >= FramesToSkip)
                {
#pragma warning(suppress: 22102)
#pragma warning(suppress: 26014)
                    BackTrace[FrameCount - FramesToSkip] = (PVOID)StackFrame.AddrPC.Offset;
                    if (BackTraceHash)
                    {
                        *BackTraceHash += (StackFrame.AddrPC.Offset & 0xffffffff);
                    }
                }
                FrameCount++;
            }
            else
            {
                break;
            }
        }

        if (FrameCount > FramesToSkip)
        {
            return (WORD)(FrameCount - FramesToSkip);
        }
        else
        {
            return 0;
        }
    }
#pragma warning( pop )
#pragma optimize( "", on )
#define CaptureStack(FramesToSkip, FramesToCapture, BackTrace, BackTraceHash) \
    RtlCaptureStackBackTrace(FramesToSkip, FramesToCapture, BackTrace, BackTraceHash)
#else
#define CaptureStack(FramesToSkip, FramesToCapture, BackTrace, BackTraceHash) \
    RtlCaptureStackBackTrace(FramesToSkip, FramesToCapture, BackTrace, BackTraceHash)
#endif
    struct SymbolInfoPackage : public SYMBOL_INFO_PACKAGEW
    {
        SymbolInfoPackage() { Init(); }
        void Init()
        {
            si.SizeOfStruct = sizeof(SYMBOL_INFOW);
            si.MaxNameLen = sizeof(name);
        }
    };

    struct ModuleInfo : public IMAGEHLP_MODULEW64
    {
        ModuleInfo() { Init(); }
        void Init()
        {
            SizeOfStruct = sizeof(IMAGEHLP_MODULEW64);
        }
    };

    bool FaultInjection::InitializeSym()
    {
        if (symInitialized)
        {
            return true;
        }

        // load dbghelp APIs
        if (hDbgHelp == NULL)
        {
            hDbgHelp = LoadLibraryEx(_u("dbghelp.dll"), 0, 0);
        }
        if (hDbgHelp == NULL)
        {
            fwprintf(stderr, _u("Failed to load dbghelp.dll for stack walking, gle=0x%08x\n"), GetLastError());
            fflush(stderr);
            return false;
        }
#define FIDELAYLOAD(fn) pfn##fn = (decltype(fn)*)GetProcAddress(hDbgHelp, #fn); \
        if (pfn##fn == nullptr){\
            fwprintf(stderr, _u("Failed to load sigs:%s\n"), _u(#fn)); \
            fflush(stderr); \
            return false; \
        }
        FIDELAYLOAD(SymInitialize);
        FIDELAYLOAD(SymCleanup);
        FIDELAYLOAD(SymFromAddrW);
        FIDELAYLOAD(SymFromNameW);
        FIDELAYLOAD(SymEnumSymbolsW);
        FIDELAYLOAD(SymGetModuleInfoW64);
        FIDELAYLOAD(SymMatchStringW);
        FIDELAYLOAD(SymSetOptions);
        FIDELAYLOAD(MiniDumpWriteDump);
        FIDELAYLOAD(SymFunctionTableAccess64);
        FIDELAYLOAD(SymGetModuleBase64);
        FIDELAYLOAD(StackWalk64);
#undef FIDELAYLOAD

        // TODO: StackBackTrace.cpp also call SymInitialize, but this can only be called once before cleanup
        if (!pfnSymInitialize(GetCurrentProcess(), NULL, TRUE))
        {
            fwprintf(stderr, _u("SymInitialize failed, gle=0x%08x\n"), GetLastError());
            fflush(stderr);
            return false;
        }
        symInitialized = true;
        return true;
    }
#pragma endregion helpers

    FaultInjection FaultInjection::Global;
    static CriticalSection cs_Sym; // for Sym* method is not thread safe
    const auto& globalFlags = Js::Configuration::Global.flags;
    PVOID FaultInjection::vectoredExceptionHandler = nullptr;
    DWORD FaultInjection::exceptionFilterRemovalLastError = 0;
    THREAD_LOCAL int(*Js::FaultInjection::pfnHandleAV)(int, PEXCEPTION_POINTERS) = nullptr;
    static SymbolInfoPackage sip;
    static ModuleInfo mi;

    const char16* crashStackStart = _u("=====Callstack for this exception=======\n");
    const char16* crashStackEnd = _u("=====End of callstack for this exception=======\n");
    const char16* injectionStackStart = _u("=====Fault injecting record=====\n");
    const char16* injectionStackEnd = _u("=====End of Fault injecting record=====\n");

    typedef struct _RANGE{
        UINT_PTR startAddress;
        UINT_PTR endAddress;
    }RANGE, *PRANGE;

    typedef struct _FUNCTION_SIGNATURES
    {
        int count;
        RANGE signatures[ANYSIZE_ARRAY];
    } FUNCTION_SIGNATURES, *PFUNCTION_SIGNATURES;

    // function address ranges of each signature
    // use for faster address matching instead of symbol table lookup when reproing
    PFUNCTION_SIGNATURES baselineFuncSigs[FaultInjection::MAX_FRAME_COUNT] = { 0 };
    // record hit count of each frame when Faults are injected.
    unsigned int stackMatchRank[FaultInjection::MAX_FRAME_COUNT] = { 0 };

#define FAULT_TYPE(x) _u(#x),\

    char16 *FaultInjection::FaultTypeNames[] =
    {
#include "FaultTypes.h"
    };
#undef FAULT_TYPE
    static_assert(sizeof(FaultInjection::FaultTypeNames) == FaultInjection::FaultType::FaultTypeCount*sizeof(char16*),
        "FaultTypeNames count is wrong");

    void FaultInjection::FaultInjectionTypes::EnableType(FaultType type)
    {
        Assert(type >= 0 && type < FaultType::FaultTypeCount);
        setBit(type, 1);
    }
    bool FaultInjection::FaultInjectionTypes::IsEnabled(FaultType type)
    {
        Assert(type >= 0 && type < FaultType::FaultTypeCount);
        return getBit(type) == 0x1;
    }
    bool FaultInjection::FaultInjectionTypes::IsEnabled(const char16* name)
    {
        for (int type = 0; type < FaultType::FaultTypeCount; type++)
        {
            if (wcscmp(FaultTypeNames[type], name) == 0)
                return getBit(type) == 0x1;
        }
        AssertMsg(false, "Unknown fault type name");
        return false;
    }

    FaultInjection::FaultInjection()
    {
        stackMatchInitialized = Uninitialized;
        countOfInjectionPoints = 0;
        hDbgHelp = NULL;
        InjectionFirstRecord = nullptr;
        InjectionLastRecordRef = &InjectionFirstRecord;
        InjectionRecordsCount = 0;
        FaultInjectionCookie = 0;
        baselineFrameCount = 0;
        stackHashOfAllInjectionPointsSize = 256;
        stackHashOfAllInjectionPoints = (ULONG_PTR*)malloc(stackHashOfAllInjectionPointsSize*sizeof(ULONG_PTR));
        faultInjectionTypes = nullptr;
        symInitialized = false;

        for (int i = 0; i < MAX_FRAME_COUNT; i++)
        {
            baselineStack[i] = nullptr;
            baselineAddresses[i] = 0;
        }
    }

    FaultInjection::~FaultInjection()
    {
        RemoveExceptionFilters();

        // when fault injection count only is passing from jscript.config(in case of running on 3rd part host)
        // and the host don't have code to output the fault injection count, we still able to do the fault injection test
        if (globalFlags.FaultInjection == FaultMode::CountOnly
            || globalFlags.FaultInjection == FaultMode::StackMatchCountOnly)
        {
            fprintf(stderr, "FaultInjection - Total Allocation Count:%u\n", countOfInjectionPoints);
            fflush(stderr);
            FILE *fp;
            char countFileName[64];
            sprintf_s(countFileName, "ChakraFaultInjectionCount_%u.txt", GetCurrentProcessId());
            if (fopen_s(&fp, countFileName, "w") == 0)
            {
                fprintf(fp, "FaultInjection - Total Allocation Count:%u\n", countOfInjectionPoints);
                fflush(fp);
                fclose(fp);
            }
            for (int i = 0; i < MAX_FRAME_COUNT; i++)
            {
                if (stackMatchRank[i] == 0)
                {
                    break;
                }
                fwprintf(stderr, _u("FaultInjection stack matching rank %d: %u\n"), i + 1, stackMatchRank[i]);
            }
            fflush(stderr);

        }

        if (globalFlags.FaultInjection == StackHashCountOnly)
        {
            FILE *fp;
            if (fopen_s(&fp, "ChakraFaultInjectionHashes.txt", "w") == 0)
            {
                for (uint i = 0; i < countOfInjectionPoints; i++)
                {
                    fprintf(fp, "%p\n", (void*)stackHashOfAllInjectionPoints[i]);
                }
                fflush(fp);
                fclose(fp);
            }
        }

        free(stackHashOfAllInjectionPoints);
        stackHashOfAllInjectionPoints = nullptr;

        if (globalFlags.FaultInjection == FaultMode::DisplayAvailableFaultTypes)
        {
            Output::Print(_u("Available Fault Types:\n"));
            for (int i = 0; i < FaultType::FaultTypeCount; i++)
            {
                Output::Print(_u("%d-%s\n"), i, FaultTypeNames[i]);
            }
            Output::Flush();
        }

        InjectionRecord* head = InjectionFirstRecord;
        while (head != nullptr)
        {
            InjectionRecord* next = head->next;
            if (head->StackData)
            {
                free(head->StackData);
            }
            free(head);
            head = next;
        }

        for (int i = 0; i < MAX_FRAME_COUNT; i++)
        {
            if (baselineStack[i])
            {
                free(baselineStack[i]);
            }
            if (baselineFuncSigs[i])
            {
                free(baselineFuncSigs[i]);
            }
        }

        if (stackMatchInitialized == Succeeded)
        {
            pfnSymCleanup(GetCurrentProcess());
        }

        if (hDbgHelp)
        {
            FreeLibrary(hDbgHelp);
        }

        if (faultInjectionTypes)
        {
            faultInjectionTypes->~FaultInjectionTypes();
            NoCheckHeapDelete(faultInjectionTypes);
        }
    }

    bool FaultInjection::IsFaultEnabled(FaultType faultType)
    {
        if (!faultInjectionTypes)
        {
            faultInjectionTypes = NoCheckHeapNew(FaultInjectionTypes);
            if ((const char16*)globalFlags.FaultInjectionType == nullptr)
            {
                // no -FaultInjectionType specified, inject all
                faultInjectionTypes->EnableAll();
            }
            else
            {
                ParseFaultTypes(globalFlags.FaultInjectionType);
            }
        }

        return faultInjectionTypes->IsEnabled(faultType);
    }

    bool FaultInjection::IsFaultInjectionOn(FaultType faultType)
    {
        return globalFlags.FaultInjection >= 0 //-FaultInjection switch
            && IsFaultEnabled(faultType);
    }

    void FaultInjection::ParseFaultTypes(const char16* szFaultTypes)
    {
        auto charCount = wcslen(szFaultTypes) + 1;
        char16* szTypes = (char16*)malloc(charCount*sizeof(char16));
        AssertMsg(szTypes, "OOM in FaultInjection Infra");
        wcscpy_s(szTypes, charCount, szFaultTypes);
        const char16* delims = _u(",");
        char16 *nextTok = nullptr;
        char16* tok = wcstok_s(szTypes, delims, &nextTok);
        while (tok != NULL)
        {
            if (wcslen(tok) > 0)
            {
                if (iswdigit(tok[0]))
                {
                    auto numType = _wtoi(tok);
                    for (int i = 0; i< FaultType::FaultTypeCount; i++)
                    {
                        if (numType & (1 << i))
                        {
                            faultInjectionTypes->EnableType(i);
                        }
                    }
                }
                else if (tok[0] == _u('#'))
                {
                    // FaultInjectionType:#1-4,#6 format, not flags
                    auto tok1 = tok + 1;
                    if (wcslen(tok1)>0 && iswdigit(tok1[0]))
                    {
                        char16* pDash = wcschr(tok1, _u('-'));
                        if (pDash)
                        {
                            for (int i = _wtoi(tok1); i <= _wtoi(pDash + 1); i++)
                            {
                                faultInjectionTypes->EnableType(i);
                            }
                        }
                        else
                        {
                            faultInjectionTypes->EnableType(_wtoi(tok1));
                        }
                    }
                }
                else
                {
                    for (int i = 0; i < FaultType::FaultTypeCount; i++)
                    {
                        if (_wcsicmp(FaultTypeNames[i], tok) == 0)
                        {
                            faultInjectionTypes->EnableType(i);
                            break;
                        }
                    }
                }
            }
            tok = wcstok_s(NULL, delims, &nextTok);
        }
        free(szTypes);
    }

    static void SmashLambda(_Inout_z_ char16* str)
    {
        //jscript9test!<lambda_dc7f9e8c591f1832700d6567e43faa6c>::operator()
        const char16 lambdaSig[] = _u("<lambda_");
        const int lambdaSigLen = (int)wcslen(lambdaSig);
        auto temp = str;
        while (temp != nullptr)
        {
            auto lambdaStart = wcsstr(temp, lambdaSig);
            temp = nullptr;
            if (lambdaStart != nullptr)
            {
                auto lambdaEnd = wcschr(lambdaStart, _u('>'));
                temp = lambdaEnd;
                if (lambdaEnd != nullptr && lambdaEnd - lambdaStart == lambdaSigLen + 32)
                {
                    lambdaStart += lambdaSigLen;
                    while (lambdaStart < lambdaEnd)
                    {
                        *(lambdaStart++) = _u('?');
                    }
                }
            }
        }
    }

    bool FaultInjection::EnsureStackMatchInfraInitialized()
    {
        if (stackMatchInitialized == Succeeded)
        {
            return true;
        }
        else if (stackMatchInitialized == FailedToInitialize)
        {
            // previous try to initialize and failed
            return false;
        }
        else if (stackMatchInitialized == Uninitialized)
        {
            stackMatchInitialized = FailedToInitialize; //tried

            if (!InitializeSym())
            {
                return false;
            }

            // read baseline stack file
            FILE *fp = nullptr;
            const char16 *stackFile = globalFlags.FaultInjectionStackFile;//default: _u("stack.txt");
            auto err = _wfopen_s(&fp, stackFile, _u("r"));
            if (err != 0 || fp == nullptr)
            {
                fwprintf(stderr, _u("Failed to load %s, gle=0x%08x\n"), stackFile, GetLastError());
                fflush(stderr);
                return false;
            }

            char16 buffer[MAX_SYM_NAME]; // assume the file is normal
            unsigned int maxLineCount =
                (globalFlags.FaultInjectionStackLineCount < 0
                || globalFlags.FaultInjectionStackLineCount > MAX_FRAME_COUNT
                || globalFlags.FaultInjection == FaultMode::StackMatchCountOnly)
                ? MAX_FRAME_COUNT : globalFlags.FaultInjectionStackLineCount;

            while (fgetws(buffer, MAX_SYM_NAME, fp))
            {
                if (wcscmp(buffer, injectionStackStart) == 0)
                {
                    baselineFrameCount = 0;
                    continue;
                }

                if (baselineFrameCount >= maxLineCount)
                {
                    continue; // don't break because we can hit the start marker and reset
                }

                const char16 jscript9test[] = _u("jscript9test!");
                const char16 jscript9[] = _u("jscript9!");
                char16* symbolStart = stristr(buffer, jscript9test);
                if (symbolStart == nullptr)
                {
                    symbolStart = stristr(buffer, jscript9);
                }
                if (symbolStart == nullptr)
                {
                    continue;// no "jscript9test!", skip this line
                }

                if (wcsstr(symbolStart, _u("Js::FaultInjection")) != NULL)
                { // skip faultinjection infra frames.
                    continue;
                }

                auto plus = wcschr(symbolStart, _u('+'));
                if (plus)
                {
                    *plus = _u('\0');
                }
                else
                {
                    trimRight(symbolStart);
                }
                SmashLambda(symbolStart);
                size_t len = wcslen(symbolStart);
                if (baselineStack[baselineFrameCount] == nullptr)
                {
                    baselineStack[baselineFrameCount] = (char16*)malloc((len + 1)*sizeof(char16));
                    AssertMsg(baselineStack[baselineFrameCount], "OOM in FaultInjection Infra");
                }
                else
                {
                    auto tmp = (char16*)realloc(baselineStack[baselineFrameCount], (len + 1)*sizeof(char16));
                    AssertMsg(tmp, "OOM in FaultInjection Infra");
                    baselineStack[baselineFrameCount] = tmp;
                }
                wcscpy_s(baselineStack[baselineFrameCount], len + 1, symbolStart);
                baselineFrameCount++;
            }
            fclose(fp);

            OutputDebugString(_u("Fault will be injected when hit following stack:\n"));
            for (uint i = 0; i<baselineFrameCount; i++)
            {
                OutputDebugString(baselineStack[i]);
                OutputDebugString(_u("\n"));
                if (wcschr(baselineStack[i], '*') != nullptr || wcschr(baselineStack[i], '?') != nullptr)
                {
                    continue; // there's wildcard in this line, don't use address matching
                }

                // enum symbols, if succeed we compare with address when doing stack matching
                pfnSymEnumSymbolsW(GetCurrentProcess(), 0, baselineStack[i],
                    [](_In_ PSYMBOL_INFOW pSymInfo, _In_ ULONG SymbolSize, _In_opt_  PVOID UserContext)->BOOL
                {
                    Assert(UserContext != nullptr); // did passed in the user context
                    if (pSymInfo->Size > 0)
                    {
                        PFUNCTION_SIGNATURES* sigs = (PFUNCTION_SIGNATURES*)UserContext;
                        int count = (*sigs) == nullptr ? 0 : (*sigs)->count;
                        auto tmp = (PFUNCTION_SIGNATURES)realloc(*sigs, sizeof(FUNCTION_SIGNATURES) + count*sizeof(RANGE));
                        AssertMsg(tmp, "OOM when allocating for FaultInjection Stack matching objects");
                        *sigs = tmp;
                        (*sigs)->count = count;
                        (*sigs)->signatures[count].startAddress = (UINT_PTR)pSymInfo->Address;
                        (*sigs)->signatures[count].endAddress = (UINT_PTR)(pSymInfo->Address + pSymInfo->Size);
                        (*sigs)->count++;
                    }
                    return TRUE;
                }, &baselineFuncSigs[i]);
            }

            stackMatchInitialized = Succeeded; // initialized
            return true;
        }
        return false;
    }

    bool FaultInjection::IsCurrentStackMatch()
    {
        AutoCriticalSection autocs(&cs_Sym); // sym* API is thread unsafe

        if (!EnsureStackMatchInfraInitialized())
        {
            return false;
        }

        DWORD64 dwSymDisplacement = 0;
        auto hProcess = GetCurrentProcess();
        static void* framesBuffer[FaultInjection::MAX_FRAME_COUNT];
        auto frameCount = CaptureStack(0, MAX_FRAME_COUNT, framesBuffer, 0);

        uint n = 0;
        for (uint i = 0; i < frameCount; i++)
        {
            if (n >= baselineFrameCount)
            {
                return true;
            }

            if (!AutoSystemInfo::Data.IsJscriptModulePointer(framesBuffer[i]))
            { // skip non-Chakra frame
                continue;
            }

            bool match = false;
            if (baselineFuncSigs[n] != nullptr)
            {
                for (int j = 0; j<baselineFuncSigs[n]->count; j++)
                {
                    match = baselineFuncSigs[n]->signatures[j].startAddress <= (UINT_PTR)framesBuffer[i]
                        && (UINT_PTR)framesBuffer[i] < baselineFuncSigs[n]->signatures[j].endAddress;
                    if (match)
                    {
                        break;
                    }
                }
            }
            else
            {
                // fallback to symbol name matching
                sip.Init();
                if (!pfnSymFromAddrW(hProcess, (DWORD64)framesBuffer[i], &dwSymDisplacement, &sip.si))
                {
                    continue;
                }
                SmashLambda(sip.si.Name);
                // Only search sigs name, can use wildcard in baseline file
                match = stristr(baselineStack[n], sip.si.Name) != nullptr
                    || pfnSymMatchStringW(sip.si.Name, baselineStack[n], false);// wildcard
            }

            if (match)
            {
                stackMatchRank[n]++;
                if (n == 0)
                {
                    n++;
                    continue;
                }
            }
            else if (n > 0)
            {
                return false;
            }

            // First line in baseline is found, moving forward.
            if (n > 0)
            {
                n++;
            }
        }
        return false;
    }

    static bool faultInjectionDebug = false;
    static bool triedToInstallExceptionFilter = false;
    static CriticalSection csFautInjection;
    void FaultInjection::InstallExceptionFilters()
    {
        if (triedToInstallExceptionFilter)
        {
            return;
        }
        
        AutoCriticalSection autoCS(&csFautInjection);
        if (triedToInstallExceptionFilter)
        {
            return;
        }
        triedToInstallExceptionFilter = true;

        if (GetEnvironmentVariable(_u("FAULTINJECTION_DEBUG"), nullptr, 0) != 0)
        {
            faultInjectionDebug = true;
        }
        if (globalFlags.FaultInjection >= 0)
        {
            // initialize symbol system here instead of inside the exception filter
            // because some hard stack overflow can happen in SymInitialize
            // when the exception filter is handling stack overflow exception
            if (!FaultInjection::Global.InitializeSym())
            {
                return;
            }
            //C28725:    Use Watson instead of this SetUnhandledExceptionFilter.
#pragma prefast(suppress: 28725)
            SetUnhandledExceptionFilter([](_In_  struct _EXCEPTION_POINTERS *ExceptionInfo)->LONG
            {
                return FaultInjectionExceptionFilter(ExceptionInfo);
            });
            vectoredExceptionHandler = AddVectoredExceptionHandler(0, [](_In_  struct _EXCEPTION_POINTERS *ExceptionInfo)->LONG
            {
                switch (ExceptionInfo->ExceptionRecord->ExceptionCode)
                {
                    // selected fatal exceptions:
                case STATUS_ACCESS_VIOLATION:
                {
                    if (pfnHandleAV
                        && pfnHandleAV(ExceptionInfo->ExceptionRecord->ExceptionCode, ExceptionInfo) == EXCEPTION_CONTINUE_EXECUTION)
                    {
                        return EXCEPTION_CONTINUE_EXECUTION;
                    }
                }
                case STATUS_ASSERTION_FAILURE:
                case STATUS_STACK_OVERFLOW:
                    FaultInjectionExceptionFilter(ExceptionInfo);
                    TerminateProcess(::GetCurrentProcess(), ExceptionInfo->ExceptionRecord->ExceptionCode);
                default:
                    return EXCEPTION_CONTINUE_SEARCH;
                }
            });
        }
    }

    void FaultInjection::RemoveExceptionFilters()
    {
        //C28725:    Use Watson instead of this SetUnhandledExceptionFilter.
#pragma prefast(suppress: 28725)
        SetUnhandledExceptionFilter(nullptr);
        if (vectoredExceptionHandler != nullptr)
        {
            RemoveVectoredExceptionHandler(vectoredExceptionHandler);

            // remove the handler from the list second time. 
            // This code is called inside an exception handler, when the exception handler is called, 
            // the refcount of the handler in ntdll!LdrpVectorHandlerList is increased, 
            // so need to call RemoveVectoredExceptionHandler twice to really remove the handler from the list
            // otherwise the exception from the handler itself will re-enter the handler
            RemoveVectoredExceptionHandler(vectoredExceptionHandler);

            vectoredExceptionHandler = nullptr;
        }
    }

    // Calculate stack hash by adding the addresses (only jscript9 frames)
    UINT_PTR FaultInjection::CalculateStackHash(void* frames[], WORD frameCount, WORD framesToSkip)
    {
        UINT_PTR hash = 0;
        for (int i = framesToSkip; i < frameCount; i++)
        {
            if (AutoSystemInfo::Data.IsJscriptModulePointer(frames[i]))
            {
                hash += (UINT_PTR)frames[i] - AutoSystemInfo::Data.dllLoadAddress;
            }
        }
        return hash;
    }

    // save the stack data for dump debugging use
    // to get list of fault injection points:
    // !list -t jscript9test!Js::FaultInjection::InjectionRecord.next -e -x "dps @$extret @$extret+0x128" poi(@@c++(&jscript9test!Js::FaultInjection::Global.InjectionFirstRecord))
    // to rebuild the stack (locals are available)
    // .cxr @@C++(&jscript9test!Js::FaultInjection::Global.InjectionFirstRecord->Context)
    _NOINLINE void FaultInjection::dumpCurrentStackData(LPCWSTR name /*= nullptr*/, size_t size /*= 0*/)
    {

#if !defined(_M_ARM32_OR_ARM64)

        static bool keepBreak = true; // for disabling following breakpoint by editing the value
        if (keepBreak && IsDebuggerPresent())
        {
            DebugBreak();
        }

        InjectionRecord* record = (InjectionRecord*)malloc(sizeof(InjectionRecord));
        if (record == nullptr) return;
        ZeroMemory(record, sizeof(InjectionRecord));
        auto _stackbasepointer = ((PNT_TIB)NtCurrentTeb())->StackBase;

        // context
        RtlCaptureContext(&record->Context);
#if _M_X64
        auto& _stackpointer = record->Context.Rsp;
        auto& _basepointer = record->Context.Rbp;
#elif _M_IX86
        auto& _stackpointer = record->Context.Esp;
        auto& _basepointer = record->Context.Ebp;
#endif
        typedef decltype(_stackpointer) spType;
        record->StackDataLength = (spType)_stackbasepointer - _stackpointer;
        record->StackData = malloc(record->StackDataLength);
        if (record->StackData)
        {
            memcpy(record->StackData, (void*)_stackpointer, record->StackDataLength);
            _basepointer = _basepointer + (spType)record->StackData - _stackpointer;
            _stackpointer = (spType)record->StackData; // for .cxr switching to this state
        }

        if (name)
        {
            wcscpy_s(record->name, name);
        }
        record->allocSize = size;

        // stack frames
        record->FrameCount = CaptureStack(0, MAX_FRAME_COUNT, record->StackFrames, 0);

        // hash
        record->hash = CalculateStackHash(record->StackFrames, record->FrameCount, 2);
        fwprintf(stderr, _u("***FI: Fault Injected, StackHash:%p\n"), (void*)record->hash);
        fflush(stderr);

        *InjectionLastRecordRef = record;
        InjectionLastRecordRef = &record->next;
        InjectionRecordsCount++;

#endif // _M_ARM || _M_ARM64
    }

    bool FaultInjection::ShouldInjectFault(FaultType fType, LPCWSTR name, size_t size)
    {
        bool shouldInjectionFault = ShouldInjectFaultHelper(fType, name, size);
        if (shouldInjectionFault && fType != FaultType::ScriptTerminationOnDispose)
        {
            dumpCurrentStackData(name, size);
        }
        return shouldInjectionFault;
    }

    bool FaultInjection::ShouldInjectFaultHelper(FaultType fType, LPCWSTR name, size_t size)
    {
        if (globalFlags.FaultInjection < 0)
        {
            return false; // no -FaultInjection switch
        }
        if (globalFlags.FaultInjectionFilter && _wcsicmp(globalFlags.FaultInjectionFilter, name) != 0)
        {
            return false;
        }
        if (globalFlags.FaultInjectionAllocSize >= 0 && size != (size_t)globalFlags.FaultInjectionAllocSize)
        {
            return false;
        }

        // install exception filter to smart dump for faultinjection
        // when reproing in debugger, only let debugger catch the exception
        // can't do this in ctor because the global flags are not initialized yet
        InstallExceptionFilters();

        bool validInjectionPoint = IsFaultEnabled(fType);
        if (!validInjectionPoint)
        {
            return false;
        }

        bool shouldInjectionFault = false;
        switch (globalFlags.FaultInjection)
        {
        case CountEquals:
            //Fault inject on count only when equal
            if (countOfInjectionPoints == (uint)globalFlags.FaultInjectionCount)
            {
                shouldInjectionFault = true;
            }
            break;

        case CountEqualsOrAbove:
            //Fault inject on count greater than or equal
            if (countOfInjectionPoints >= (uint)globalFlags.FaultInjectionCount)
            {
                shouldInjectionFault = true;
            }
            break;
        case StackMatch:
            // We don't care about the fault if we already passed in terms of count, or the stack doesn't match
            if (countOfInjectionPoints > (uint)globalFlags.FaultInjectionCount || !IsCurrentStackMatch())
            {
                validInjectionPoint = false;
            }
            else // otherwise determine if we will be injecting this time around
            {
                shouldInjectionFault = countOfInjectionPoints == (uint)globalFlags.FaultInjectionCount || globalFlags.FaultInjectionCount == -1;
            }
            break;
        case StackMatchCountOnly:
            validInjectionPoint = IsCurrentStackMatch();
            break;
        case StackHashCountOnly:
        {
            // extend the storage when necessary
            if (countOfInjectionPoints > stackHashOfAllInjectionPointsSize)
            {
                stackHashOfAllInjectionPointsSize += 1024;
                auto extended = (ULONG_PTR*)realloc(stackHashOfAllInjectionPoints,
                    stackHashOfAllInjectionPointsSize*sizeof(ULONG_PTR));
                AssertMsg(extended, "OOM in FaultInjection Infra");
                stackHashOfAllInjectionPoints = extended;
            }
            void* StackFrames[MAX_FRAME_COUNT];
            auto FrameCount = CaptureStack(0, MAX_FRAME_COUNT, StackFrames, 0);
            UINT_PTR hash = CalculateStackHash(StackFrames, FrameCount, 2);
            stackHashOfAllInjectionPoints[countOfInjectionPoints] = hash;
            break;
        }
        case CountOnly:
            break;
        case DisplayAvailableFaultTypes:
        case InstallExceptionHandlerOnly:        
            return false;
        default:
            AssertMsg(false, "Invalid FaultInjection mode");
            break;
        }

        if (validInjectionPoint)
        {
            countOfInjectionPoints++;
        }

        // try to lookup stack hash, to see if it matches
        if (!shouldInjectionFault)
        {
            const UINT_PTR expectedHash = HexStrToAddress((LPCWSTR)globalFlags.FaultInjectionStackHash);
            if (expectedHash != 0)
            {
                void* StackFrames[MAX_FRAME_COUNT];
                auto FrameCount = CaptureStack(0, MAX_FRAME_COUNT, StackFrames, 0);
                UINT_PTR hash = CalculateStackHash(StackFrames, FrameCount, 2);
                if (hash == expectedHash)
                {
                    shouldInjectionFault = true;
                }
            }
        }

        return shouldInjectionFault;
    }

    // For faster fault injection test run, filter out the AVs on same IP/hash
    void FaultInjection::FaultInjectionAnalyzeException(_EXCEPTION_POINTERS *ep)
    {
#if !defined(_M_ARM32_OR_ARM64) // not support ARM for now, add support in case we run fault injection on ARM
        AutoCriticalSection autocs(&cs_Sym);
        CONTEXT* pContext = ep->ContextRecord;

        // always show stack for crash and fault injection points in console,
        // this can be used for additional stack matching repro
        HANDLE hProcess = GetCurrentProcess();
        DWORD64 dwSymDisplacement = 0;
        auto printFrame = [&](LPVOID addr)
        {
            sip.Init();
            if (pfnSymFromAddrW(hProcess, (DWORD64)addr, &dwSymDisplacement, &sip.si))
            {
                mi.Init();
                pfnSymGetModuleInfoW64(hProcess, (DWORD64)addr, &mi);
                fwprintf(stderr, _u("%s!%s+0x%llx\n"), mi.ModuleName, sip.si.Name, (ULONGLONG)dwSymDisplacement);
            }
            else
            {
                fwprintf(stderr, _u("0x%p\n"), addr);
            }
        };

        LPVOID backTrace[MAX_FRAME_COUNT] = { 0 };
        DWORD64 displacements[MAX_FRAME_COUNT] = { 0 };
#if _M_IX86
        WORD nStackCount = StackTrace86(0, MAX_FRAME_COUNT, backTrace, 0, pContext);
#elif _M_X64
        WORD nStackCount = StackTrace64(0, MAX_FRAME_COUNT, backTrace, 0, pContext);
#else
        WORD nStackCount = CaptureStack(0, MAX_FRAME_COUNT, backTrace, 0);
#endif

        // Print current crash stacks
        fwprintf(stderr, crashStackStart);

        for (int i = 0; i < nStackCount; i++)
        {
            printFrame(backTrace[i]);
            displacements[i] = dwSymDisplacement;
        }

        LPVOID internalExceptionAddr = nullptr;
        for (int i = 0; i < nStackCount - 1 && internalExceptionAddr == nullptr; i++)
        {
            if (backTrace[i] == (char*)Js::Throw::FatalInternalError + displacements[i])
            {
                internalExceptionAddr = backTrace[i + 1];
            }
            else if (backTrace[i] == (char*)Js::Throw::ReportAssert + displacements[i])
            {
                if (backTrace[i + 1] == (char*)Js::Throw::InternalError + displacements[i + 1])
                {
                    // skip to next frame
                }
                else
                {
                    internalExceptionAddr = backTrace[i + 1];
                }
            }
            else if (backTrace[i] == (char*)Js::Throw::InternalError + displacements[i])
            {
                internalExceptionAddr = backTrace[i + 1];
            }
        }

        fwprintf(stderr, crashStackEnd);

        // Print fault injecting point stacks
        auto record = InjectionFirstRecord;
        while (record)
        {
            if (record->StackFrames)
            {
                fwprintf(stderr, injectionStackStart);
                for (int i = 0; i < record->FrameCount; i++)
                {
                    printFrame(backTrace[i]);
                }
                fwprintf(stderr, injectionStackEnd);
            }
            record = record->next;
        }

        // we called RaiseException() which always use RaiseException as exception address, restore the real exception addr
        if (internalExceptionAddr != nullptr)
        {
            ep->ExceptionRecord->ExceptionAddress = internalExceptionAddr;
        }

        bool needDump = true;
        uintptr_t ip = (uintptr_t)ep->ExceptionRecord->ExceptionAddress;
        uintptr_t offset = 0;       

        // static to not use local stack space since stack space might be low at this point
        THREAD_LOCAL static char16 modulePath[MAX_PATH + 1];
        THREAD_LOCAL static WCHAR filename[MAX_PATH + 1];

        HMODULE mod = nullptr;
        GetModuleHandleEx(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS, reinterpret_cast<LPCTSTR>(ip), &mod);
        offset = ip - (uintptr_t)mod;
        auto& faultModule = modulePath;
        GetModuleFileName(mod, faultModule, MAX_PATH);
        fwprintf(stderr, _u("***FI: Exception: %08x, module: %s, offset: 0x%p\n"),
            ep->ExceptionRecord->ExceptionCode, faultModule, (void*)offset);

        //analyze duplication
        uintptr_t savedOffset = 0;
        auto& mainModule = modulePath;
        GetModuleFileName(NULL, mainModule, MAX_PATH);
        // multiple session of Fault Injection run shares the single crash offset recording file
        _snwprintf_s(filename, _TRUNCATE, _u("%s.FICrashes.txt"), mainModule);

        auto fp = _wfsopen(filename, _u("a+t"), _SH_DENYNO);
        if (fp != nullptr)
        {
            HANDLE hFile = (HANDLE)_get_osfhandle(_fileno(fp));
            OVERLAPPED overlapped;
            memset(&overlapped, 0, sizeof(overlapped));
            const int lockSize = 1024 * 64;
            if (!LockFileEx(hFile, LOCKFILE_EXCLUSIVE_LOCK, 0, lockSize, 0, &overlapped))
            {
                fwprintf(stderr, _u("LockFileEx(%ls) Failed when saving offset to file, gle=%8x\n"), filename, GetLastError());
                fclose(fp);
            }
            else
            { // file locked
                char16 content[32] = { 0 };
                while (fgetws(content, 31, fp))
                {
                    savedOffset = HexStrToAddress(content);
                    if (offset == savedOffset)
                    {
                        // found duplicate so not creating dump
                        needDump = false;
                    }
                }

                if (needDump)
                {
                    fwprintf(stderr, _u("This is new Exception\n"));
                    fwprintf(fp, _u("0x%p\n"), (void*)offset);
                }
                else
                {
                    fwprintf(stderr, _u("This is not a new Exception\n"));
                }
                fflush(fp);

                // save the hit count to a file, for bug prioritizing
                _snwprintf_s(filename, _TRUNCATE, _u("%s.HitCount_%llx.txt"), mainModule, (long long)offset);
                auto hcfp = _wfsopen(filename, _u("r+"), _SH_DENYNO);
                if (!hcfp)
                {
                    hcfp = _wfsopen(filename, _u("w+"), _SH_DENYNO);
                }
                if (hcfp)
                {
                    auto count = 0;
                    fscanf_s(hcfp, "%d", &count);
                    count++;
                    fseek(hcfp, -ftell(hcfp), SEEK_CUR);
                    fwprintf(hcfp, _u("%d"), count);
                    fclose(hcfp);
                }

                fclose(fp);
                UnlockFileEx(hFile, 0, lockSize, 0, &overlapped);
            }
            fflush(stderr);
        }

        if (globalFlags.FaultInjection == InstallExceptionHandlerOnly)
        {
            needDump = true;
        }

        // create dump for this crash
        if (needDump)
        {
            THREAD_LOCAL static char16 dumpName[MAX_PATH + 1];
            wcscpy_s(filename, globalFlags.Filename);
            char16* jsFile = filename;
            char16 *pch = jsFile;
            // remove path and keep only alphabet and number to make a valid filename
            while (*pch)
            {
                if (*pch == _u(':') || *pch == _u('\\'))
                {
                    jsFile = pch + 1;
                }
                else if (!isalnum(*pch))
                {
                    *pch = _u('_');
                }
                pch++;
            }


            // get dump file name
            int suffix = 1;
            const char16* fiType = _u("undefined");
            if (globalFlags.FaultInjectionType != nullptr)
            {
                fiType = (LPCWSTR)globalFlags.FaultInjectionType;
            }
            while (true)
            {
                _snwprintf_s(dumpName, _TRUNCATE, _u("%s_%s_M%d_T%s_C%d_%llx_%llx_%d.dmp"),
                    mainModule, jsFile,
                    globalFlags.FaultInjection, fiType, globalFlags.FaultInjectionCount,
                    (ULONGLONG)offset, (ULONGLONG)ep->ExceptionRecord->ExceptionCode, suffix);
                WIN32_FIND_DATAW data;
                HANDLE hExist = FindFirstFile(dumpName, &data);
                if (hExist == INVALID_HANDLE_VALUE)
                {
                    FindClose(hExist);
                    break;
                }
                FindClose(hExist);
                suffix++;
            }

            // writing the dump file
            HANDLE hFile = CreateFile(dumpName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
            if ((hFile == NULL) || (hFile == INVALID_HANDLE_VALUE))
            {
                fwprintf(stderr, _u("CreateFile <%s> failed. gle=0x%08x\n"), dumpName, GetLastError());
            }
            else
            {
                MINIDUMP_EXCEPTION_INFORMATION mdei;
                mdei.ThreadId = GetCurrentThreadId();
                mdei.ExceptionPointers = ep;
                mdei.ClientPointers = FALSE;
                MINIDUMP_TYPE mdt = (MINIDUMP_TYPE)(MiniDumpNormal
                    | MiniDumpWithFullMemory
                    | MiniDumpWithPrivateReadWriteMemory
                    | MiniDumpWithIndirectlyReferencedMemory
                    | MiniDumpWithThreadInfo);


                // removing extension for windbg module name style
                auto& jscript9Path = modulePath;
                wcsncpy_s(jscript9Path, AutoSystemInfo::Data.GetJscriptDllFileName(),
                    wcslen(AutoSystemInfo::Data.GetJscriptDllFileName()) - 4);
                char16* jscript9Name = jscript9Path + wcslen(jscript9Path);
                while (*(jscript9Name - 1) != _u('\\') && jscript9Name > jscript9Path)
                {
                    jscript9Name--;
                }

                // This buffer will be written to a dump stream when creating the minidump file.
                // It contains windbg debugging instructions on how to figure out the injected faults,
                // And the message will be showing in windbg while loading the minidump.
                // If you need to add more instructions please increase the buffer capacity accordingly
                THREAD_LOCAL static char16 dbgTip[1024];
                if (InjectionFirstRecord == nullptr)
                {
                    wcsncpy_s(dbgTip,
                        _u("\n")
                        _u("************************************************************\n")
                        _u("* The dump is made by FaultInjection framework, however, the fault is not actually injected yet.\n")
                        _u("************************************************************\n"), _TRUNCATE);
                }
                else
                {
                    _snwprintf_s(dbgTip, _TRUNCATE, _u("\n")
                        _u("************************************************************\n")
                        _u("* To find the Fault Injecting points run following command: \n")
                        _u("* !list -t %s!Js::FaultInjection::InjectionRecord.next -e -x \"dps @$extret @$extret+0x128\" poi(@@c++(&%s!Js::FaultInjection::Global.InjectionFirstRecord))\n")
                        _u("* To rebuild the stack (locals are available):\n")
                        _u("* .cxr @@C++(&%s!Js::FaultInjection::Global.InjectionFirstRecord->Context)\n")
                        _u("************************************************************\n"), jscript9Name, jscript9Name, jscript9Name);
                }

                MINIDUMP_USER_STREAM UserStreams[1];
                UserStreams[0].Type = CommentStreamW;
                UserStreams[0].Buffer = dbgTip;
                UserStreams[0].BufferSize = (ULONG)wcslen(dbgTip)*sizeof(char16);
                MINIDUMP_USER_STREAM_INFORMATION musi;
                musi.UserStreamCount = 1;
                musi.UserStreamArray = UserStreams;

                BOOL rv = pfnMiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, mdt, (ep != 0) ? &mdei : 0, &musi, 0);
                if (rv)
                {
                    fwprintf(stderr, _u("Minidump created: %s\n"), dumpName);
                }
                else
                {
                    fwprintf(stderr, _u("MiniDumpWriteDump failed. gle=0x%08x\n"), GetLastError());
                }
                CloseHandle(hFile);
            }
        }

        fflush(stderr);

#endif  //_M_ARM and _M_ARM64
    }

    static volatile bool inExceptionHandler = false;
    LONG WINAPI FaultInjection::FaultInjectionExceptionFilter(_In_  struct _EXCEPTION_POINTERS *ExceptionInfo)
    {
        if (inExceptionHandler)
        {
            // re-entering, this can happen if RemoveExceptionFilters() failed because of stack overflow
            // Let it crash and the postmortem debugger can catch it.
            DebugBreak();
        }

        inExceptionHandler = true;

        RemoveExceptionFilters();

        // for debugging, can't hit here in windbg because of using vectored exception handling
        if (faultInjectionDebug)
        {
            DebugBreak();
        }

        if (ExceptionInfo->ExceptionRecord->ExceptionCode == STATUS_STACK_OVERFLOW) // hard stack overflow
        {
            DebugBreak(); // let the postmortem debugger to create the dump, make sure they are filing bug with same bucket
        }        

        __try
        {
            // sometimes the OS is really low memory and can't commit page for stack expanding
            // even stack is not deep yet
            FaultInjection::Global.FaultInjectionAnalyzeException(ExceptionInfo);
        }
        __except (EXCEPTION_EXECUTE_HANDLER)
        {
            DebugBreak();
        }
        inExceptionHandler = false;

        return EXCEPTION_EXECUTE_HANDLER;
    }

} //namespace Js
#endif //FAULT_INJECTION
