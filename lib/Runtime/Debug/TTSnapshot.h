//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

namespace TTD
{
    //A class that represents a heap snapshot the page context
    class SnapShot
    {
    private:
        ////
        //The slab allocator we use for storing the data extracted
        SlabAllocator m_slabAllocator;

        ////
        //List containing the "context" information for the objects in this snapshot
        UnorderedArrayList<NSSnapValues::SnapContext, TTD_ARRAY_LIST_SIZE_XSMALL> m_ctxList;

        //List containing all the propertyids for the symbols that are in the thread context symbolRegistrationMap
        UnorderedArrayList<Js::PropertyId, TTD_ARRAY_LIST_SIZE_XSMALL> m_tcSymbolRegistrationMapContents;

        //The active script context
        TTD_LOG_PTR_ID m_activeScriptContext;

        //A list of all the global root objects and local root objects
        UnorderedArrayList<NSSnapValues::SnapRootInfoEntry, TTD_ARRAY_LIST_SIZE_MID> m_rootList;

        ////
        //Lists containing the "type" information for the objects in this snapshot

        //The list of all the dynamichandler definitions that the snapshot uses
        UnorderedArrayList<NSSnapType::SnapHandler, TTD_ARRAY_LIST_SIZE_DEFAULT> m_handlerList;

        //The list of all the type definitions that the snapshot uses
        UnorderedArrayList<NSSnapType::SnapType, TTD_ARRAY_LIST_SIZE_DEFAULT> m_typeList;

        ////
        //Lists containing the objects in this snapshot

        //The list of all the type definitions that the snapshot uses
        UnorderedArrayList<NSSnapValues::FunctionBodyResolveInfo, TTD_ARRAY_LIST_SIZE_DEFAULT> m_functionBodyList;

        //The list of all the primitive objects (empty until we convert all the primitive pinned objects during extractor unlink)
        UnorderedArrayList<NSSnapValues::SnapPrimitiveValue, TTD_ARRAY_LIST_SIZE_LARGE> m_primitiveObjectList;

        //The list of all the non-primitive javascript objects in the snapshot
        UnorderedArrayList<NSSnapObjects::SnapObject, TTD_ARRAY_LIST_SIZE_LARGE> m_compoundObjectList;

        //pseudo vtable for working with snap objects
        NSSnapObjects::SnapObjectVTable* m_snapObjectVTableArray;

        ////
        //Function scope info 

        //All the scopes in the snapshot
        UnorderedArrayList<NSSnapValues::ScriptFunctionScopeInfo, TTD_ARRAY_LIST_SIZE_DEFAULT> m_scopeEntries;

        //All the slot arrays in the snapshot
        UnorderedArrayList<NSSnapValues::SlotArrayInfo, TTD_ARRAY_LIST_SIZE_DEFAULT> m_slotArrayEntries;

        //Emit the json file for the snapshot into the given directory 
        void EmitSnapshotToFile(FileWriter* writer, ThreadContext* threadContext) const;
        static SnapShot* ParseSnapshotFromFile(FileReader* reader);

        template<typename Fn, typename T, size_t allocSize>
        static void EmitListHelper(Fn emitFunc, const UnorderedArrayList<T, allocSize>& list, FileWriter* snapwriter)
        {
            snapwriter->WriteLengthValue(list.Count(), NSTokens::Separator::CommaAndBigSpaceSeparator);
            snapwriter->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaAndBigSpaceSeparator);
            snapwriter->AdjustIndent(1);
            bool firstElement = true;
            for(auto iter = list.GetIterator(); iter.IsValid(); iter.MoveNext())
            {
                (*emitFunc)(iter.Current(), snapwriter, firstElement ? NSTokens::Separator::BigSpaceSeparator : NSTokens::Separator::CommaAndBigSpaceSeparator);
                firstElement = false;
            }
            snapwriter->AdjustIndent(-1);
            snapwriter->WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);
        }

        template<typename Fn, typename T, size_t allocSize>
        static void ParseListHelper(Fn parseFunc, UnorderedArrayList<T, allocSize>& list, FileReader* snapreader, SlabAllocator& alloc)
        {
            uint32 count = snapreader->ReadLengthValue(true);
            snapreader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < count; ++i)
            {
                T* into = list.NextOpenEntry();
                (*parseFunc)(into, i != 0, snapreader, alloc);
            }
            snapreader->ReadSequenceEnd();
        }

        template<typename Fn, typename T, typename U, size_t allocSize>
        static void ParseListHelper_WMap(Fn parseFunc, UnorderedArrayList<T, allocSize>& list, FileReader* snapreader, SlabAllocator& alloc, const TTDIdentifierDictionary<TTD_PTR_ID, U>& infoMap)
        {
            uint32 count = snapreader->ReadLengthValue(true);
            snapreader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 i = 0; i < count; ++i)
            {
                T* into = list.NextOpenEntry();
                (*parseFunc)(into, i != 0, snapreader, alloc, infoMap);
            }
            snapreader->ReadSequenceEnd();
        }

        //Inflate a single JS object
        void InflateSingleObject(const NSSnapObjects::SnapObject* snpObject, InflateMap* inflator, const TTDIdentifierDictionary<TTD_PTR_ID, NSSnapObjects::SnapObject*>& idToSnpObjectMap) const;

        void ReLinkThreadContextInfo(InflateMap* inflator, ThreadContextTTD* intoCtx) const;

        static void SnapRootPinEntryEmit(const NSSnapValues::SnapRootInfoEntry* spe, FileWriter* snapwriter, NSTokens::Separator separator);
        static void SnapRootPinEntryParse(NSSnapValues::SnapRootInfoEntry* spe, bool readSeperator, FileReader* reader, SlabAllocator& alloc);

    public:
        //Performance counter values
        double GCTime;
        double MarkTime;
        double ExtractTime;

        //Compute the memory used by this snapshot
        void ComputeSnapshotMemory(uint64* usedSpace, uint64* reservedSpace) const;

        SnapShot(double gcTime);
        ~SnapShot();

        //Get the counts for the various list sizes (not constant time -- so use carefully!!!!)
        uint32 ContextCount() const;
        uint32 HandlerCount() const;
        uint32 TypeCount() const;
        uint32 BodyCount() const;
        uint32 PrimitiveCount() const;
        uint32 ObjectCount() const;
        uint32 EnvCount() const;
        uint32 SlotArrayCount() const;
        uint32 GetDbgScopeCountNonTopLevel() const;

        //Get the context list for this snapshot
        UnorderedArrayList<NSSnapValues::SnapContext, TTD_ARRAY_LIST_SIZE_XSMALL>& GetContextList();
        const UnorderedArrayList<NSSnapValues::SnapContext, TTD_ARRAY_LIST_SIZE_XSMALL>& GetContextList() const;

        //Get the thread context symbol map info list for this snapshot
        UnorderedArrayList<Js::PropertyId, TTD_ARRAY_LIST_SIZE_XSMALL>& GetTCSymbolMapInfoList();

        TTD_LOG_PTR_ID GetActiveScriptContext() const;
        void SetActiveScriptContext(TTD_LOG_PTR_ID activeCtx);

        //Get the root lists
        UnorderedArrayList<NSSnapValues::SnapRootInfoEntry, TTD_ARRAY_LIST_SIZE_MID>& GetRootList();

        //Get a pointer to the next open handler slot that we can fill
        NSSnapType::SnapHandler* GetNextAvailableHandlerEntry();

        //Get a pointer to the next open handler slot that we can fill
        NSSnapType::SnapType* GetNextAvailableTypeEntry();

        //Get a pointer to the next open FunctionBodyResolveInfo slot that we can fill
        NSSnapValues::FunctionBodyResolveInfo* GetNextAvailableFunctionBodyResolveInfoEntry();

        //Get a pointer to the next open primitive object slot that we can fill
        NSSnapValues::SnapPrimitiveValue* GetNextAvailablePrimitiveObjectEntry();

        //Get a pointer to the next open compound object slot that we can fill
        NSSnapObjects::SnapObject* GetNextAvailableCompoundObjectEntry();

        //Get a pointer to the next open scope entry slot that we can fill
        NSSnapValues::ScriptFunctionScopeInfo* GetNextAvailableFunctionScopeEntry();

        //Get a pointer to the next open slot array entry that we can fill
        NSSnapValues::SlotArrayInfo* GetNextAvailableSlotArrayEntry();

        //Get the slab allocator for this snapshot context
        SlabAllocator& GetSnapshotSlabAllocator();

        //Make sure that all well known objects can be re-used -- if not we will need to recreate all script contexts from scratch
        bool AllWellKnownObjectsReusable(InflateMap* inflator) const;

        //Inflate the snapshot
        void Inflate(InflateMap* inflator, ThreadContextTTD* tCtx) const;

        //serialize the snapshot data 
        void EmitSnapshot(int64 snapId, ThreadContext* threadContext) const;

        //de-serialize the snapshot data
        static SnapShot* Parse(int64 snapId, ThreadContext* threadContext);

#if ENABLE_SNAPSHOT_COMPARE
        static void InitializeForSnapshotCompare(const SnapShot* snap1, const SnapShot* snap2, TTDCompareMap& compareMap);

        static void DoSnapshotCompare(const SnapShot* snap1, const SnapShot* snap2, TTDCompareMap& compareMap);
#endif
    };
}

#endif

