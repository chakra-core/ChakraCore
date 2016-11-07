//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class JITTimeConstructorCache
{
public:
    JITTimeConstructorCache(const Js::JavascriptFunction* constructor, Js::ConstructorCache* runtimeCache);
    JITTimeConstructorCache(const JITTimeConstructorCache* other);

    JITTimeConstructorCache* Clone(JitArenaAllocator* allocator) const;
    BVSparse<JitArenaAllocator>* GetGuardedPropOps() const;
    void EnsureGuardedPropOps(JitArenaAllocator* allocator);
    void SetGuardedPropOp(uint propOpId);
    void AddGuardedPropOps(const BVSparse<JitArenaAllocator>* propOps);

    intptr_t GetRuntimeCacheAddr() const;
    intptr_t GetRuntimeCacheGuardAddr() const;
    JITTypeHolder GetType() const;
    int GetSlotCount() const;
    int16 GetInlineSlotCount() const;
    bool SkipNewScObject() const;
    bool CtorHasNoExplicitReturnValue() const;
    bool IsTypeFinal() const;
    bool IsUsed() const;
    void SetUsed(bool val);

    JITTimeConstructorCacheIDL * GetData();

private:
    Field(JITTimeConstructorCacheIDL) m_data;
};

#pragma once
