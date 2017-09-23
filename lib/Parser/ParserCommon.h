//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// Common definitions used outside parser so that we don't have to include the whole Parser.h.

#pragma once

namespace Js
{
    typedef int32  ByteCodeLabel;       // Size of this match the offset size in layouts
    typedef uint32 RegSlot;
    typedef uint8  RegSlot_OneByte;
    typedef int8   RegSlot_OneSByte;
    typedef int16  RegSlot_TwoSByte;
    typedef uint16 RegSlot_TwoByte;
}

enum ErrorTypeEnum
{
    kjstError,
    kjstEvalError,
    kjstRangeError,
    kjstReferenceError,
    kjstSyntaxError,
    kjstTypeError,
    kjstURIError,
    kjstWebAssemblyCompileError,
    kjstWebAssemblyRuntimeError,
    kjstWebAssemblyLinkError,
    kjstCustomError,
#ifdef ENABLE_PROJECTION
    kjstWinRTError,
#endif
};

struct ParseNode;
typedef ParseNode *ParseNodePtr;

struct Ident;
typedef Ident *IdentPtr;

struct ModuleImportOrExportEntry
{
    IdentPtr moduleRequest;
    IdentPtr importName;
    IdentPtr localName;
    IdentPtr exportName;
};

typedef SList<ModuleImportOrExportEntry, ArenaAllocator> ModuleImportOrExportEntryList;
typedef SList<IdentPtr, ArenaAllocator> IdentPtrList;

//
// Below was moved from scrutil.h to share with chakradiag.
//
#define HR(sc) ((HRESULT)(sc))
