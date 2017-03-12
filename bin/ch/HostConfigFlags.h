//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#include "Core/ICustomConfigFlags.h"
class HostConfigFlags : public ICustomConfigFlags
{
public:
#define FLAG(Type, Name, Desc, Default) \
    Type Name; \
    bool Name##IsEnabled;
#include "HostConfigFlagsList.h"

    static HostConfigFlags flags;
    static LPWSTR* argsVal;
    static int argsCount;
    static void(__stdcall *pfnPrintUsage)();

    static void HandleArgsFlag(int& argc, _Inout_updates_to_(argc, argc) LPWSTR argv[]);
    static void RemoveArg(int& argc, _Inout_updates_to_(argc, argc) PWSTR argv[], int index);
    static int FindArg(int argc, _In_reads_(argc) PWSTR argv[], PCWSTR targetArg, size_t targetArgLen);

    template <class Func> static int FindArg(int argc, _In_reads_(argc) PWSTR argv[], Func func);
    template <int LEN> static int FindArg(int argc, _In_reads_(argc) PWSTR argv[], const char16(&targetArg)[LEN]);

    virtual bool ParseFlag(LPCWSTR flagsString, ICmdLineArgsParser * parser) override;
    virtual void PrintUsage() override;
    static void PrintUsageString();

private:
    int nDummy;
    HostConfigFlags();

    template <typename T>
    void Parse(ICmdLineArgsParser * parser, T * value);
};

// Find an arg in the arg list that satisfies func. Return the arg index if found.
template <class Func>
int HostConfigFlags::FindArg(int argc, _In_reads_(argc) PWSTR argv[], Func func)
{
    for (int i = 1; i < argc; ++i)
    {
        if (func(argv[i]))
        {
            return i;
        }
    }

    return -1;
}

template <int LEN>
int HostConfigFlags::FindArg(int argc, _In_reads_(argc) PWSTR argv[], const char16(&targetArg)[LEN])
{
    return FindArg(argc, argv, targetArg, LEN - 1); // -1 to exclude null terminator
}
