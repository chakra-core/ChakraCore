//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class AsmJsJITInfo
{
public:
    AsmJsJITInfo(AsmJsDataIDL * data);

    int GetIntConstCount() const;
    int GetDoubleConstCount() const;
    int GetFloatConstCount() const;
    int GetSimdConstCount() const;
    int GetIntTmpCount() const;
    int GetDoubleTmpCount() const;
    int GetFloatTmpCount() const;
    int GetSimdTmpCount() const;
    int GetIntVarCount() const;
    int GetDoubleVarCount() const;
    int GetFloatVarCount() const;
    int GetSimdVarCount() const;
    int GetIntByteOffset() const;
    int GetDoubleByteOffset() const;
    int GetFloatByteOffset() const;
    int GetSimdByteOffset() const;
    int GetTotalSizeInBytes() const;

    Js::ArgSlot GetArgCount() const;
    Js::ArgSlot GetArgByteSize() const;
    Js::AsmJsRetType::Which GetRetType() const;
    Js::AsmJsVarType::Which * GetArgTypeArray() const;
    Js::AsmJsVarType::Which GetArgType(Js::ArgSlot argNum) const;

    bool IsHeapBufferConst() const;
    bool UsesHeapBuffer() const;
    bool AccessNeedsBoundCheck(uint offset) const;

private:
    AsmJsDataIDL m_data;
};
