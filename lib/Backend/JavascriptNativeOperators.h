//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
#if ENABLE_NATIVE_CODEGEN
    template <typename T>
    class BranchDictionaryWrapper
    {
    public:
        class DictAllocator :public NativeCodeData::Allocator
        {
        public:
            char * Alloc(size_t requestedBytes)
            {
                char* dataBlock = __super::Alloc(requestedBytes);
#if DBG
                if (JITManager::GetJITManager()->IsJITServer())
                {
                    NativeCodeData::DataChunk* chunk = NativeCodeData::GetDataChunk(dataBlock);
                    chunk->dataType = "BranchDictionary::Bucket";
                    if (PHASE_TRACE1(Js::NativeCodeDataPhase))
                    {
                        Output::Print(_u("NativeCodeData BranchDictionary::Bucket: chunk: %p, data: %p, index: %d, len: %x, totalOffset: %x, type: %S\n"),
                            chunk, (void*)dataBlock, chunk->allocIndex, chunk->len, chunk->offset, chunk->dataType);
                    }
                }
#endif
                return dataBlock;
            }

            char * AllocZero(size_t requestedBytes)
            {
                char* dataBlock = __super::AllocZero(requestedBytes);
#if DBG
                if (JITManager::GetJITManager()->IsJITServer())
                {
                    NativeCodeData::DataChunk* chunk = NativeCodeData::GetDataChunk(dataBlock);
                    chunk->dataType = "BranchDictionary::Entries";
                    if (PHASE_TRACE1(Js::NativeCodeDataPhase))
                    {
                        Output::Print(_u("NativeCodeData BranchDictionary::Entries: chunk: %p, data: %p, index: %d, len: %x, totalOffset: %x, type: %S\n"),
                            chunk, (void*)dataBlock, chunk->allocIndex, chunk->len, chunk->offset, chunk->dataType);
                    }
                }
#endif
                return dataBlock;
            }
        };

        template <class TKey, class TValue>
        class SimpleDictionaryEntryWithFixUp : public JsUtil::SimpleDictionaryEntry<TKey, TValue>
        {
        public:
            void FixupWithRemoteKey(void* remoteKey)
            {
                this->key = (TKey)remoteKey;
            }
        };

        typedef JsUtil::BaseDictionary<T, void*, DictAllocator, PowerOf2SizePolicy, DefaultComparer, SimpleDictionaryEntryWithFixUp> BranchBaseDictionary;

        class BranchDictionary :public BranchBaseDictionary
        {
        public:
            BranchDictionary(DictAllocator* allocator, uint dictionarySize)
                : BranchBaseDictionary(allocator, dictionarySize)
            {
            }
            void Fixup(NativeCodeData::DataChunk* chunkList, void** remoteKeys)
            {
                for (int i = 0; i < this->Count(); i++)
                {
                    this->entries[i].FixupWithRemoteKey(remoteKeys[i]);
                }
                FixupNativeDataPointer(buckets, chunkList);
                FixupNativeDataPointer(entries, chunkList);
            }
        };

        BranchDictionaryWrapper(NativeCodeData::Allocator * allocator, uint dictionarySize, ArenaAllocator* remoteKeyAlloc) :
            defaultTarget(nullptr), dictionary((DictAllocator*)allocator, dictionarySize)
        {
            if (remoteKeyAlloc)
            {
                remoteKeys = AnewArrayZ(remoteKeyAlloc, void*, dictionarySize);
            }
            else
            {
                Assert(!JITManager::GetJITManager()->IsJITServer());
                remoteKeys = nullptr;
            }
        }

        BranchDictionary dictionary;
        void* defaultTarget;
        void** remoteKeys;

        static BranchDictionaryWrapper* New(NativeCodeData::Allocator * allocator, uint dictionarySize, ArenaAllocator* remoteKeyAlloc)
        {
            return NativeCodeDataNew(allocator, BranchDictionaryWrapper, allocator, dictionarySize, remoteKeyAlloc);
        }

        void AddEntry(uint32 offset, T key, void* remoteVar)
        {
            int index = dictionary.AddNew(key, (void**)offset);
            if (JITManager::GetJITManager()->IsJITServer())
            {
                Assert(remoteKeys);
                remoteKeys[index] = remoteVar;
            }
        }

        void Fixup(NativeCodeData::DataChunk* chunkList)
        {
            if (JITManager::GetJITManager()->IsJITServer())
            {
                dictionary.Fixup(chunkList, remoteKeys);
            }
        }
    };

    class JavascriptNativeOperators
    {
    public:
        static void * Op_SwitchStringLookUp(JavascriptString* str, Js::BranchDictionaryWrapper<Js::JavascriptString*>* stringDictionary, uintptr_t funcStart, uintptr_t funcEnd);

#if ENABLE_DEBUG_CONFIG_OPTIONS
        static void TracePropertyEquivalenceCheck(const JitEquivalentTypeGuard* guard, const Type* type, const Type* refType, bool isEquivalent, uint failedPropertyIndex);
#endif        
        static bool CheckIfTypeIsEquivalent(Type* type, JitEquivalentTypeGuard* guard);
        static bool CheckIfTypeIsEquivalentForFixedField(Type* type, JitEquivalentTypeGuard* guard);

        static Var OP_GetElementI_JIT_ExpectingNativeFloatArray(Var instance, Var index, ScriptContext *scriptContext);
        static Var OP_GetElementI_JIT_ExpectingVarArray(Var instance, Var index, ScriptContext *scriptContext);

        static Var OP_GetElementI_UInt32_ExpectingNativeFloatArray(Var instance, uint32 aElementIndex, ScriptContext* scriptContext);
        static Var OP_GetElementI_UInt32_ExpectingVarArray(Var instance, uint32 aElementIndex, ScriptContext* scriptContext);

        static Var OP_GetElementI_Int32_ExpectingNativeFloatArray(Var instance, int32 aElementIndex, ScriptContext* scriptContext);
        static Var OP_GetElementI_Int32_ExpectingVarArray(Var instance, int32 aElementIndex, ScriptContext* scriptContext);
    private:
        static bool IsStaticTypeObjTypeSpecEquivalent(const TypeEquivalenceRecord& equivalenceRecord, uint& failedIndex);
        static bool IsStaticTypeObjTypeSpecEquivalent(const EquivalentPropertyEntry *entry);
    };
#endif
};
