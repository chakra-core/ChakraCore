//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

class JITTypeHandler
{
public:
    JITTypeHandler(TypeHandlerIDL * data);

    bool IsObjectHeaderInlinedTypeHandler() const;
    bool IsLocked() const;

    uint16 GetInlineSlotCapacity() const;
    uint16 GetOffsetOfInlineSlots() const;

    int GetSlotCapacity() const;

    static bool IsTypeHandlerCompatibleForObjectHeaderInlining(const JITTypeHandler * oldTypeHandler, const JITTypeHandler * newTypeHandler);
private:
    TypeHandlerIDL m_data;
};
