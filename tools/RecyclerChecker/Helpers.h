//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "llvm/Support/raw_ostream.h"
using std::string;
using llvm::raw_ostream;

class Log
{
public:
    enum class LogLevel { Error, Info, Verbose };

private:
    static LogLevel s_logLevel;

public:
    static LogLevel GetLevel() { return s_logLevel; }
    static void SetLevel(LogLevel level) { s_logLevel = level; }

    static raw_ostream& errs()
    {
        return llvm::outs();  // use same outs stream for better output order
    }

    static raw_ostream& outs()
    {
        return s_logLevel >= LogLevel::Info ? llvm::outs() : llvm::nulls();
    }

    static raw_ostream& verbose()
    {
        return s_logLevel >= LogLevel::Verbose ? llvm::outs() : llvm::nulls();
    }
};

template <size_t N>
bool StartsWith(const char* s, const char (&prefix)[N])
{
    return strncmp(s, prefix, N - 1) == 0;
}

template <size_t N>
bool StartsWith(const string& s, const char (&prefix)[N])
{
    return s.length() >= N - 1 && s.compare(0, N - 1, prefix, N - 1) == 0;
}

inline bool StartsWith(const char* s, const string& prefix)
{
    return strncmp(s, prefix.c_str(), prefix.length()) == 0;
}

template <class Set, class Item>
bool Contains(const Set& s, const Item& item)
{
    return s.find(item) != s.end();
}

inline bool Contains(const string& s, const char* sub)
{
    return s.find(sub) != string::npos;
}
