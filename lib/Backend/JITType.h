//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

// forward declaration
class JITTypeHandler;

class JITType
{
public:
    JITType();
    JITType(TypeIDL * data);

    bool IsShared() const;
    Js::TypeId GetTypeId() const;
    intptr_t GetPrototypeAddr() const;
    intptr_t GetAddr() const;

    const JITTypeHandler* GetTypeHandler() const;

    TypeIDL * GetData();

    static void BuildFromJsType(__in Js::Type * jsType, __out JITType * jitType);

    bool operator!=(const JITType& bucket) const;
    bool operator==(const JITType& bucket) const;

private:
    TypeIDL m_data;
};
