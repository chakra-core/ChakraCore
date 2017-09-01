//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

namespace TTD
{
    class TTDTimer
    {
    private:
        PlatformAgnostic::DateTime::HiResTimer m_timer;

    public:
        TTDTimer()
            : m_timer()
        {
            ;
        }

        ~TTDTimer()
        {
            ;
        }

        double Now()
        {
            return this->m_timer.Now();
        }
    };

    namespace JsSupport
    {
        //return true if the Var is a tagged number (inline)
        bool IsVarTaggedInline(Js::Var v);

        //return true if the Var is a ptr var value 
        bool IsVarPtrValued(Js::Var v);

        //return true if the Var is a primitive value (string, number, symbol, etc.)
        bool IsVarPrimitiveKind(Js::Var v);

        //return true if the Var is a richer value (enumerator, dynamicObject, array, etc.)
        bool IsVarComplexKind(Js::Var v);

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
        //Check if 2 inline values are equivalent -- JIT can use number vs. Int in interp mode but as long as values mean the same thing we are cool
        bool AreInlineVarsEquiv(Js::Var v1, Js::Var v2);
#endif

        //Ensure a function is fully parsed/deserialized 
        Js::FunctionBody* ForceAndGetFunctionBody(Js::ParseableFunctionInfo* pfi);

        void WriteCodeToFile(ThreadContext* threadContext, bool fromEvent, uint32 bodyId, bool isUtf8Source, byte* sourceBuffer, uint32 length);
        void ReadCodeFromFile(ThreadContext* threadContext, bool fromEvent, uint32 bodyId, bool isUtf8Source, byte* sourceBuffer, uint32 length);
    }

    namespace NSSnapValues
    {
        //////////////////
        //Helpers for TTDVars

        enum class TTDVarEmitTag
        {
            TTDVarInvalid = 0x0,
            TTDVarNull,
            TTDVarInt,
            TTDVarDouble,
            TTDVarAddr
        };

        //serialize the TTDVar
        void EmitTTDVar(TTDVar var, FileWriter* writer, NSTokens::Separator separator);

        //de-serialize the TTDVar
        TTDVar ParseTTDVar(bool readSeperator, FileReader* reader);

#if ENABLE_SNAPSHOT_COMPARE 
        bool CheckSnapEquivTTDDouble(double d1, double d2);
        void AssertSnapEquivTTDVar_Helper(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, TTDComparePath::StepKind stepKind, const TTDComparePath::PathEntry& next);

        void AssertSnapEquivTTDVar_Property(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, Js::PropertyId pid);
        void AssertSnapEquivTTDVar_PropertyGetter(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, Js::PropertyId pid);
        void AssertSnapEquivTTDVar_PropertySetter(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, Js::PropertyId pid);
        void AssertSnapEquivTTDVar_Array(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, uint32 index);
        void AssertSnapEquivTTDVar_SlotArray(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, uint32 index);
        void AssertSnapEquivTTDVar_Special(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, const char16* specialField);
        void AssertSnapEquivTTDVar_SpecialArray(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, const char16* specialField, uint32 index);
#endif

        //////////////////

        //A structure for the primitive javascript types
        struct SnapPrimitiveValue
        {
            //The id (address of the object)
            TTD_PTR_ID PrimitiveValueId;

            //The type of the primitive value
            NSSnapType::SnapType* SnapType;

            //The optional well known token for this object (or INVALID)
            TTD_WELLKNOWN_TOKEN OptWellKnownToken;

            union
            {
                BOOL u_boolValue;
                int64 u_int64Value;
                uint64 u_uint64Value;
                double u_doubleValue;
                TTString* u_stringValue;
                Js::PropertyId u_propertyIdValue;
            };
        };

        void ExtractSnapPrimitiveValue(SnapPrimitiveValue* snapValue, Js::RecyclableObject* jsValue, bool isWellKnown, const TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapType*>& idToTypeMap, SlabAllocator& alloc);
        void InflateSnapPrimitiveValue(const SnapPrimitiveValue* snapValue, InflateMap* inflator);

        void EmitSnapPrimitiveValue(const SnapPrimitiveValue* snapValue, FileWriter* writer, NSTokens::Separator separator);
        void ParseSnapPrimitiveValue(SnapPrimitiveValue* snapValue, bool readSeperator, FileReader* reader, SlabAllocator& alloc, const TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapType*>& ptrIdToTypeMap);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SnapPrimitiveValue* v1, const SnapPrimitiveValue* v2, TTDCompareMap& compareMap);
#endif

        //////////////////

        //If a scope is a slot array this class encodes the slot information and original address of the slot array
        struct SlotArrayInfo
        {
            //The unique id for the slot array scope entry 
            TTD_PTR_ID SlotId;

            //The tag of the script context that this slot array is associated with
            TTD_LOG_PTR_ID ScriptContextLogId;

            //The number of slots in the scope array
            uint32 SlotCount;

            //The data values for the slots in the scope entry
            TTDVar* Slots;

            //The property ids associated with each index
            Js::PropertyId* PIDArray;

            //The meta-data for the slot array
            bool isFunctionBodyMetaData;

            TTD_PTR_ID OptFunctionBodyId;
            TTD_PTR_ID OptDebugScopeId;
            TTD_WELLKNOWN_TOKEN OptWellKnownDbgScope;
        };

        Js::Var* InflateSlotArrayInfo(const SlotArrayInfo* slotInfo, InflateMap* inflator);

        void EmitSlotArrayInfo(const SlotArrayInfo* slotInfo, FileWriter* writer, NSTokens::Separator separator);
        void ParseSlotArrayInfo(SlotArrayInfo* slotInfo, bool readSeperator, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SlotArrayInfo* sai1, const SlotArrayInfo* sai2, TTDCompareMap& compareMap);
#endif

        //A struct to represent a single scope entry for a function
        struct ScopeInfoEntry
        {
            //The tag indicating what type of scope entry this is
            Js::ScopeType Tag;

            //The id for looking up the entry data
            //Either ptr object for Activation and With Scopes or the slotid for SlotArray Scopes
            TTD_PTR_ID IDValue;
        };

        //A class to represent a scope list for a script function
        struct ScriptFunctionScopeInfo
        {
            //The unique id for the scope array associated with this function
            TTD_PTR_ID ScopeId;

            //The id of the script context that this slot array is associated with
            TTD_LOG_PTR_ID ScriptContextLogId;

            //The number of scope entries
            uint16 ScopeCount;

            //The array of scope entries this function has
            ScopeInfoEntry* ScopeArray;
        };

        Js::FrameDisplay* InflateScriptFunctionScopeInfo(const ScriptFunctionScopeInfo* funcScopeInfo, InflateMap* inflator);

        void EmitScriptFunctionScopeInfo(const ScriptFunctionScopeInfo* funcScopeInfo, FileWriter* writer, NSTokens::Separator separator);
        void ParseScriptFunctionScopeInfo(ScriptFunctionScopeInfo* funcScopeInfo, bool readSeperator, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const ScriptFunctionScopeInfo* funcScopeInfo1, const ScriptFunctionScopeInfo* funcScopeInfo2, TTDCompareMap& compareMap);
#endif

        //////////////////

        //Capability info for a promise
        struct SnapPromiseCapabilityInfo
        {
            //The unique id for the capability entry
            TTD_PTR_ID CapabilityId;

            TTDVar PromiseVar;
            TTDVar ResolveVar;
            TTDVar RejectVar;
        };

        Js::JavascriptPromiseCapability* InflatePromiseCapabilityInfo(const SnapPromiseCapabilityInfo* capabilityInfo, Js::ScriptContext* ctx, InflateMap* inflator);

        void EmitPromiseCapabilityInfo(const SnapPromiseCapabilityInfo* capabilityInfo, FileWriter* writer, NSTokens::Separator separator);
        void ParsePromiseCapabilityInfo(SnapPromiseCapabilityInfo* capabilityInfo, bool readSeperator, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SnapPromiseCapabilityInfo* capabilityInfo1, const SnapPromiseCapabilityInfo* capabilityInfo2, TTDCompareMap& compareMap);
#endif

        //A struct to represent a PromiseReaction
        struct SnapPromiseReactionInfo
        {
            //The unique id for PromiseReaction
            TTD_PTR_ID PromiseReactionId;

            TTD_PTR_ID HandlerObjId;
            SnapPromiseCapabilityInfo Capabilities;
        };

        Js::JavascriptPromiseReaction* InflatePromiseReactionInfo(const SnapPromiseReactionInfo* reactionInfo, Js::ScriptContext* ctx, InflateMap* inflator);

        void EmitPromiseReactionInfo(const SnapPromiseReactionInfo* reactionInfo, FileWriter* writer, NSTokens::Separator separator);
        void ParsePromiseReactionInfo(SnapPromiseReactionInfo* reactionInfo, bool readSeperator, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SnapPromiseReactionInfo* reactionInfo1, const SnapPromiseReactionInfo* reactionInfo2, TTDCompareMap& compareMap);
#endif

        //////////////////

        //Information on the scopechain for a function body
        struct SnapFunctionBodyScopeChain
        {
            //The number of scopes associated with this function body
            uint32 ScopeCount;

            //The Ids of the scopes
            TTD_PTR_ID* ScopeArray;
        };

        void ExtractSnapFunctionBodyScopeChain(bool isWellKnownFunction, SnapFunctionBodyScopeChain& scopeChain, Js::FunctionBody* fb, SlabAllocator& alloc);

        void EmitSnapFunctionBodyScopeChain(const SnapFunctionBodyScopeChain& scopeChain, FileWriter* writer);
        void ParseSnapFunctionBodyScopeChain(SnapFunctionBodyScopeChain& scopeChain, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SnapFunctionBodyScopeChain& chain1, const SnapFunctionBodyScopeChain& chain2, TTDCompareMap& compareMap);
#endif

        //////////////////

        //Information that is common to all top-level bodies
        struct TopLevelCommonBodyResolveInfo
        {
            //The context this body is associated with
            TTD_LOG_PTR_ID ScriptContextLogId;

            //A unique counter we produce for every top-level body as it is loaded
            uint32 TopLevelBodyCtr;

            //The string name of the function
            TTString FunctionName;

            //The module id and source context
            Js::ModuleID ModuleId;
            uint64 SourceContextId;

            //Src URI may be null
            TTString SourceUri;

            //The source length/buffer and if it is utf8 or char16 encoded
            bool IsUtf8;
            uint32 ByteLength;
            byte* SourceBuffer;

            //The (possibly empty) scope chain info
            SnapFunctionBodyScopeChain ScopeChainInfo;

            //The number of bytes (or -1 if not set) and the buffer for the serialized bytecode
            mutable DWORD DbgSerializedBytecodeSize;
            mutable byte* DbgSerializedBytecodeBuffer;
        };

        //Extract WITHOUT COPYING the info needed for this top level function -- use in script context when function is parsed to keep all the info together and then we do the copying later when doing snapshots
        void ExtractTopLevelCommonBodyResolveInfo(TopLevelCommonBodyResolveInfo* fbInfo, Js::FunctionBody* fb, uint32 topLevelCtr, Js::ModuleID moduleId, uint64 sourceContextId, bool isUtf8source, const byte* source, uint32 sourceLen, SlabAllocator& alloc);
        void EmitTopLevelCommonBodyResolveInfo(const TopLevelCommonBodyResolveInfo* fbInfo, bool emitInline, ThreadContext* threadContext, FileWriter* writer, NSTokens::Separator separator);
        void ParseTopLevelCommonBodyResolveInfo(TopLevelCommonBodyResolveInfo* fbInfo, bool readSeperator, bool parseInline, ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const TopLevelCommonBodyResolveInfo* fbInfo1, const TopLevelCommonBodyResolveInfo* fbInfo2, TTDCompareMap& compareMap);
#endif

        //A struct that we can use to resolve a top-level script load function bodies and info
        struct TopLevelScriptLoadFunctionBodyResolveInfo
        {
            //The base information for the top-level resolve info
            TopLevelCommonBodyResolveInfo TopLevelBase;

            //The script flag this was loaded with
            LoadScriptFlag LoadFlag;
        };

        void ExtractTopLevelLoadedFunctionBodyInfo(TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, uint32 topLevelCtr, Js::ModuleID moduleId, uint64 sourceContextId, bool isUtf8, const byte* source, uint32 sourceLen, LoadScriptFlag loadFlag, SlabAllocator& alloc);
        Js::FunctionBody* InflateTopLevelLoadedFunctionBodyInfo(const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo, Js::ScriptContext* ctx);

        void EmitTopLevelLoadedFunctionBodyInfo(const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo, ThreadContext* threadContext, FileWriter* writer, NSTokens::Separator separator);
        void ParseTopLevelLoadedFunctionBodyInfo(TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo, bool readSeperator, ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo1, const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo2, TTDCompareMap& compareMap);
#endif

        //A struct that we can use to resolve a top-level 'new Function' function bodies and info
        struct TopLevelNewFunctionBodyResolveInfo
        {
            //The base information for the top-level resolve info
            TopLevelCommonBodyResolveInfo TopLevelBase;
        };

        void ExtractTopLevelNewFunctionBodyInfo(TopLevelNewFunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, uint32 topLevelCtr, Js::ModuleID moduleId, const char16* source, uint32 sourceLen, SlabAllocator& alloc);
        Js::FunctionBody* InflateTopLevelNewFunctionBodyInfo(const TopLevelNewFunctionBodyResolveInfo* fbInfo, Js::ScriptContext* ctx);

        void EmitTopLevelNewFunctionBodyInfo(const TopLevelNewFunctionBodyResolveInfo* fbInfo, ThreadContext* threadContext, FileWriter* writer, NSTokens::Separator separator);
        void ParseTopLevelNewFunctionBodyInfo(TopLevelNewFunctionBodyResolveInfo* fbInfo, bool readSeperator, ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const TopLevelNewFunctionBodyResolveInfo* fbInfo1, const TopLevelNewFunctionBodyResolveInfo* fbInfo2, TTDCompareMap& compareMap);
#endif

        //A struct that we can use to resolve a top-level 'eval' function bodies and info
        struct TopLevelEvalFunctionBodyResolveInfo
        {
            //The base information for the top-level resolve info
            TopLevelCommonBodyResolveInfo TopLevelBase;

            //Additional data for handling the eval
            uint64 EvalFlags;
            bool RegisterDocument;
            bool IsIndirect; 
            bool IsStrictMode;
        };

        void ExtractTopLevelEvalFunctionBodyInfo(TopLevelEvalFunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, uint32 topLevelCtr, Js::ModuleID moduleId, const char16* source, uint32 sourceLen, uint32 grfscr, bool registerDocument, BOOL isIndirect, BOOL strictMode, SlabAllocator& alloc);
        Js::FunctionBody* InflateTopLevelEvalFunctionBodyInfo(const TopLevelEvalFunctionBodyResolveInfo* fbInfo, Js::ScriptContext* ctx);

        void EmitTopLevelEvalFunctionBodyInfo(const TopLevelEvalFunctionBodyResolveInfo* fbInfo, ThreadContext* threadContext, FileWriter* writer, NSTokens::Separator separator);
        void ParseTopLevelEvalFunctionBodyInfo(TopLevelEvalFunctionBodyResolveInfo* fbInfo, bool readSeperator, ThreadContext* threadContext, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const TopLevelEvalFunctionBodyResolveInfo* fbInfo1, const TopLevelEvalFunctionBodyResolveInfo* fbInfo2, TTDCompareMap& compareMap);
#endif

        //A struct that we can use to resolve a function body during inflation
        struct FunctionBodyResolveInfo
        {
            //The id this body is associated with
            TTD_PTR_ID FunctionBodyId;

            //The context this body is associated with
            TTD_LOG_PTR_ID ScriptContextLogId;

            //The string name of the function
            TTString FunctionName;

            //The known path to a function with the desired body
            TTD_WELLKNOWN_TOKEN OptKnownPath;

            //The ptr id of the parent function body
            TTD_PTR_ID OptParentBodyId;

            //The line number the function is def starts on
            int64 OptLine;

            //The column number the function is def starts on
            int64 OptColumn;

            //The (possibly empty) scope chain info
            SnapFunctionBodyScopeChain ScopeChainInfo;
        };

        void ExtractFunctionBodyInfo(FunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, bool isWellKnown, SlabAllocator& alloc);
        void InflateFunctionBody(const FunctionBodyResolveInfo* fbInfo, InflateMap* inflator, const TTDIdentifierDictionary<TTD_PTR_ID, FunctionBodyResolveInfo*>& idToFbResolveMap);

        void EmitFunctionBodyInfo(const FunctionBodyResolveInfo* fbInfo, FileWriter* writer, NSTokens::Separator separator);
        void ParseFunctionBodyInfo(FunctionBodyResolveInfo* fbInfo, bool readSeperator, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const FunctionBodyResolveInfo* fbInfo1, const FunctionBodyResolveInfo* fbInfo2, TTDCompareMap& compareMap);
#endif

        //////////////////

        struct SnapRootInfoEntry
        {
            //The log id value 
            TTD_LOG_PTR_ID LogId;

            //The object that this log id is mapped to
            TTD_PTR_ID LogObject;

            //True if the root maybe long lived/false otherwise
            bool MaybeLongLivedRoot;
        };

        struct SnapPendingAsyncBufferModification
        {
            //The log id value 
            TTD_LOG_PTR_ID LogId;

            //The index value associated with this modification
            uint32 Index;
        };

        //A structure for serializing/tracking script contexts
        struct SnapContext
        {
            //The tag id of the script context (actually the global object associated with this context)
            TTD_LOG_PTR_ID ScriptContextLogId;

            //The random seed for the context
            bool IsPNRGSeeded;
            uint64 RandomSeed0;
            uint64 RandomSeed1;

            //The main URI of the context
            TTString ContextSRC;

            //A list of all *root* scripts that have been loaded into this context
            uint32 LoadedTopLevelScriptCount;
            TopLevelFunctionInContextRelation* LoadedTopLevelScriptArray;

            uint32 NewFunctionTopLevelScriptCount;
            TopLevelFunctionInContextRelation* NewFunctionTopLevelScriptArray;

            uint32 EvalTopLevelScriptCount;
            TopLevelFunctionInContextRelation* EvalTopLevelScriptArray;

            //A list of all the pending async buffer modifications
            uint32 PendingAsyncModCount;
            SnapPendingAsyncBufferModification* PendingAsyncModArray;
        };

        void ExtractScriptContext(SnapContext* snapCtx, Js::ScriptContext* ctx, const JsUtil::BaseDictionary<Js::RecyclableObject*, TTD_LOG_PTR_ID, HeapAllocator>& objToLogIdMap, const JsUtil::BaseHashSet<Js::FunctionBody*, HeapAllocator>& liveTopLevelBodies, SlabAllocator& alloc);

        void InflateScriptContext(const SnapContext* snpCtx, Js::ScriptContext* intoCtx, InflateMap* inflator,
            const TTDIdentifierDictionary<uint64, TopLevelScriptLoadFunctionBodyResolveInfo*>& topLevelLoadScriptMap,
            const TTDIdentifierDictionary<uint64, TopLevelNewFunctionBodyResolveInfo*>& topLevelNewScriptMap,
            const TTDIdentifierDictionary<uint64, TopLevelEvalFunctionBodyResolveInfo*>& topLevelEvalScriptMap);

        void ResetPendingAsyncBufferModInfo(const SnapContext* snpCtx, Js::ScriptContext* intoCtx, InflateMap* inflator);

        void EmitSnapContext(const SnapContext* snapCtx, FileWriter* writer, NSTokens::Separator separator);
        void ParseSnapContext(SnapContext* intoCtx, bool readSeperator, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SnapContext* snapCtx1, const SnapContext* snapCtx2, const JsUtil::BaseDictionary<TTD_LOG_PTR_ID, NSSnapValues::SnapRootInfoEntry*, HeapAllocator>& allRootMap1, const JsUtil::BaseDictionary<TTD_LOG_PTR_ID, NSSnapValues::SnapRootInfoEntry*, HeapAllocator>& allRootMap2, TTDCompareMap& compareMap);
#endif
    }
}

#endif
