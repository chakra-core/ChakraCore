//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
namespace Js
{
#if ENABLE_NATIVE_CODEGEN
    class OOPJITBranchDictAllocator :public NativeCodeData::Allocator
    {
    public:
        char * Alloc(size_t requestedBytes)
        {
            char* dataBlock = __super::Alloc(requestedBytes);
#if DBG
            NativeCodeData::DataChunk* chunk = NativeCodeData::GetDataChunk(dataBlock);
            chunk->dataType = "BranchDictionary::Bucket";
            if (PHASE_TRACE1(Js::NativeCodeDataPhase))
            {
                Output::Print(_u("NativeCodeData BranchDictionary::Bucket: chunk: %p, data: %p, index: %d, len: %x, totalOffset: %x, type: %S\n"),
                    chunk, (void*)dataBlock, chunk->allocIndex, chunk->len, chunk->offset, chunk->dataType);
            }
#endif
            return dataBlock;
        }

        char * AllocZero(size_t requestedBytes)
        {
            char* dataBlock = __super::AllocZero(requestedBytes);
#if DBG
            NativeCodeData::DataChunk* chunk = NativeCodeData::GetDataChunk(dataBlock);
            chunk->dataType = "BranchDictionary::Entries";
            if (PHASE_TRACE1(Js::NativeCodeDataPhase))
            {
                Output::Print(_u("NativeCodeData BranchDictionary::Entries: chunk: %p, data: %p, index: %d, len: %x, totalOffset: %x, type: %S\n"),
                    chunk, (void*)dataBlock, chunk->allocIndex, chunk->len, chunk->offset, chunk->dataType);
            }
#endif
            return dataBlock;
        }
    };


    template <typename T, class DictAllocT>
    class BranchDictionaryWrapper
    {
    public:

        template <class TKey, class TValue>
        class SimpleDictionaryEntryWithFixUp : public JsUtil::SimpleDictionaryEntry<TKey, TValue>
        {
        public:
            void FixupWithRemoteKey(void* remoteKey)
            {
                this->key = (TKey)remoteKey;
            }
        };

        typedef JsUtil::BaseDictionary<T, void*, DictAllocT, PowerOf2SizePolicy, DefaultComparer, SimpleDictionaryEntryWithFixUp> BranchBaseDictionary;

        class BranchDictionary :public BranchBaseDictionary
        {
        public:
            BranchDictionary(DictAllocT* allocator, uint dictionarySize)
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

        BranchDictionaryWrapper(DictAllocT * allocator, uint dictionarySize, ArenaAllocator* remoteKeyAlloc) :
            defaultTarget(nullptr), dictionary((DictAllocT*)allocator, dictionarySize)
        {
            Assert(remoteKeyAlloc);
            remoteKeys = AnewArrayZ(remoteKeyAlloc, void*, dictionarySize);
        }

        BranchDictionaryWrapper(DictAllocT * allocator, uint dictionarySize) :
            defaultTarget(nullptr), dictionary((DictAllocT*)allocator, dictionarySize), remoteKeys(nullptr)
        {

        }

        BranchDictionary dictionary;
        void* defaultTarget;
        void** remoteKeys;

        bool IsOOPJit()
        {
            return remoteKeys != nullptr;
        }

        static BranchDictionaryWrapper* New(Func* func, uint dictionarySize, ArenaAllocator* remoteKeyAlloc)
        {
            Assert(func->IsOOPJIT());
            return NativeCodeDataNew(func, BranchDictionaryWrapper, (OOPJITBranchDictAllocator*)func->GetNativeCodeDataAllocator(), dictionarySize, remoteKeyAlloc);
        }
        static BranchDictionaryWrapper* New(Func* func, uint dictionarySize)
        {
            Assert(!func->IsOOPJIT());
            return NativeCodeDataNew(func, BranchDictionaryWrapper, func->GetNativeCodeDataNoFixupAllocator(), dictionarySize);
        }

        void AddEntry(uint32 offset, T key, void* remoteVar)
        {
            int index = dictionary.AddNew(key, (void**)offset);
            if (IsOOPJit())
            {
                remoteKeys[index] = remoteVar;
            }
        }

        void Fixup(NativeCodeData::DataChunk* chunkList)
        {
            if (IsOOPJit())
            {
                dictionary.Fixup(chunkList, remoteKeys);
            }
        }
    };

    class JavascriptNativeOperators
    {
    public:
        static void * Op_SwitchStringLookUp_OOP(JavascriptString* str, Js::BranchDictionaryWrapper<Js::JavascriptString*, OOPJITBranchDictAllocator>* stringDictionary, uintptr_t funcStart, uintptr_t funcEnd);
        static void * Op_SwitchStringLookUp(JavascriptString* str, Js::BranchDictionaryWrapper<Js::JavascriptString*, NativeCodeDataNoFixup::Allocator>* stringDictionary, uintptr_t funcStart, uintptr_t funcEnd);
    };
#endif
};
