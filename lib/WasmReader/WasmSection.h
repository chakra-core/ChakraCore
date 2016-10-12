//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#ifdef ENABLE_WASM
namespace Wasm
{

    enum SectionFlag
    {
        fSectNone,
        fSectIgnore,
    };

#define WASM_SECTION(name, id, flag, precendent) bSect ## name,
    enum SectionCode
    {
        bSectInvalid = -1,
#include "WasmSections.h"
        bSectLimit
    };

    struct SectionInfo
    {
        SectionFlag flag;
        SectionCode precedent;
        const wchar_t* name;
        const char* id;
        static SectionInfo All[bSectLimit];
    };
}
#endif
