//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Memory
{

#ifdef LEAK_REPORT

class LeakReport
{
public:
    class UrlRecord
    {
    public:
        void * scriptEngine;
    private:
        char16 const * url;
#if _MSC_VER
        __time64_t time;
#else
        time_t time;
#endif
        DWORD tid;
        UrlRecord * next;

        void * globalObject;
        friend class LeakReport;
    };

    static void StartRedirectOutput();
    static void EndRedirectOutput();
    static void StartSection(char16 const * msg, ...);
    static void StartSection(char16 const * msg, va_list argptr);
    static void EndSection();
    static void Print(char16 const * msg, ...);

    static UrlRecord * LogUrl(char16 const * url, void * globalObject);
    static void DumpUrl(DWORD tid);
private:
    static CriticalSection s_cs;
    static AutoFILE file;
    static bool openReportFileFailed;
    static DWORD nestedSectionCount;
    static DWORD nestedRedirectOutputCount;

    static UrlRecord * urlRecordHead;
    static UrlRecord * urlRecordTail;

    static bool EnsureLeakReportFile();
};

class AutoLeakReportSection
{
public:
    AutoLeakReportSection(Js::ConfigFlagsTable& flags, char16 const * msg, ...);
    ~AutoLeakReportSection();

private:
    Js::ConfigFlagsTable& m_flags;
};

#define STRINGIFY2(x,y) x ## y
#define STRINGIFY(x,y) STRINGIFY2(x,y)
#define LEAK_REPORT_PRINT(msg, ...) if (Js::Configuration::Global.flags.IsEnabled(Js::LeakReportFlag)) LeakReport::Print(msg, __VA_ARGS__)
#define AUTO_LEAK_REPORT_SECTION(flags, msg, ...) AutoLeakReportSection STRINGIFY(__autoLeakReportSection, __COUNTER__)(flags, msg, __VA_ARGS__)
#define AUTO_LEAK_REPORT_SECTION_0(flags, msg) AutoLeakReportSection STRINGIFY(__autoLeakReportSection, __COUNTER__)(flags, msg, "")
#else
#define LEAK_REPORT_PRINT(msg, ...)
#define AUTO_LEAK_REPORT_SECTION(flags, msg, ...)
#define AUTO_LEAK_REPORT_SECTION_0(flags, msg)
#endif
}
