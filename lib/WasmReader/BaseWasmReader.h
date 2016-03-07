//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{

    enum SectionFlag
    {
        fSectNone,
        fSectIgnore,
    };

#define WASM_SECTION(name, id, flag) bSect ## name,
    enum SectionCode
    {
        bSectInvalid = -1,
    #include "WasmSections.h"
        bSectLimit
    };

    enum ProcessSectionResult
    {
        psrContinue,
        psrEnd,
        psrInvalid
    };

    class BaseWasmReader
    {
    public:
        virtual void InitializeReader() = 0;
        virtual bool ReadNextSection(SectionCode nextSection) = 0;
        virtual ProcessSectionResult ProcessSection(SectionCode nextSection, bool isEntry = true) = 0;

        virtual WasmOp ReadExpr() = 0;
        virtual WasmOp ReadFromBlock() = 0;
        virtual WasmOp ReadFromCall() = 0;
        virtual bool IsBinaryReader() = 0;
        WasmNode    m_currentNode;
        ModuleInfo * m_moduleInfo;

    protected:
        WasmFunctionInfo *  m_funcInfo;
    };

} // namespace Wasm
