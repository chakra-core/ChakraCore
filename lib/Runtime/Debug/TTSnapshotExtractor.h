//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

namespace TTD
{
    //A class that is in charge of extracting a snapshot of the heap
    class SnapshotExtractor
    {
    private:
        //The mark table for all pure objects that we have seen (filled in during walk used during extract)
        MarkTable m_marks;

        //A worklist for processing the objects as we find them during the mark
        JsUtil::Queue<Js::RecyclableObject*, HeapAllocator> m_worklist;

        TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapHandler*> m_idToHandlerMap;
        TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapType*> m_idToTypeMap;

        //The snapshot that is being constructed
        SnapShot* m_pendingSnap;

        ////////
        //Mark code

        //Visit a handler/type during the walk-mark phase
        void MarkVisitHandler(Js::DynamicTypeHandler* handler);
        void MarkVisitType(Js::Type* type);

        //Visit all the named and dynamic indexed properties that can appear on any object
        void MarkVisitStandardProperties(Js::RecyclableObject* obj);

        //Ensure that a handler/type has been extracted
        void ExtractHandlerIfNeeded(Js::DynamicTypeHandler* handler, ThreadContext* threadContext);
        void ExtractTypeIfNeeded(Js::Type* jstype, ThreadContext* threadContext);

        //Ensure that a slot/scope has been extracted
        void ExtractSlotArrayIfNeeded(Js::ScriptContext* ctx, Js::Var* scope);
        void ExtractScopeIfNeeded(Js::ScriptContext* ctx, Js::FrameDisplay* environment);
        void ExtractScriptFunctionEnvironmentIfNeeded(Js::ScriptFunction* function);

        ////
        //Performance info
        uint32 m_snapshotsTakenCount;
        double m_totalMarkMillis;
        double m_totalExtractMillis;

        double m_maxMarkMillis;
        double m_maxExtractMillis;

        double m_lastMarkMillis;
        double m_lastExtractMillis;

        void UnloadDataFromExtractor();

        //
        //TODO: In the case of weak* types (WeakMap & WeakSet) the results of a snapshot -- and potentially later execution depends on when a GC has been run.
        //      I *think* we want to keep track of the number of weak types that *may* be live here and, if there are any, then always do a GC before doing a snapshot to put things in a uniform state.
        //

    public:
        SnapshotExtractor();
        ~SnapshotExtractor();

        //Get the current pending snapshot
        SnapShot* GetPendingSnapshot();

        //Return the slab allocatorr for the current active snapshot
        SlabAllocator& GetActiveSnapshotSlabAllocator();

        //Visit a variable value during the walk-mark phase, marking it and adding it to the appropriate worklist as needed
        void MarkVisitVar(Js::Var var);

        //Mark the function body
        void MarkFunctionBody(Js::FunctionBody* fb);

        //Mark/Extract the scope information for a function
        void MarkScriptFunctionScopeInfo(Js::FrameDisplay* environment);

        ////////
        //Do the actual snapshot extraction

        //Begin the snapshot by initializing the snapshot information
        void BeginSnapshot(ThreadContext* threadContext, double gcTime);

        //Do the walk of all objects caller need to to call MarkWalk on roots to initialize the worklist
        void DoMarkWalk(ThreadContext* threadContext);

        //Evacuate all the marked javascript objects into the snapshot (can do lazily/incrementally if desired)
        //All of the external elements are evacuated during the mark phase while propertyRecords and primitiveObjects are evacuated during the complete phase
        void EvacuateMarkedIntoSnapshot(ThreadContext* threadContext, JsUtil::BaseHashSet<Js::FunctionBody*, HeapAllocator>& liveTopLevelBodies);

        //Tidy up and save the snapshot return the completed snapshot
        SnapShot* CompleteSnapshot();

        //On replay we do a walk of the heap and re-populate the weak collection pin sets
        void DoResetWeakCollectionPinSet(ThreadContext* threadContext);
    };
}

#endif

