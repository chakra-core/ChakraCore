
//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

namespace TTD
{
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

        //Ensure a function is fully parsed/deserialized 
        Js::FunctionBody* ForceAndGetFunctionBody(Js::ParseableFunctionInfo* pfi);

        //Copy a string into the heap allocator
        void CopyStringToHeapAllocator(LPCWSTR string, TTString& into);
        void CopyStringToHeapAllocatorWLength(LPCWSTR string, uint32 length, TTString& into);
        void DeleteStringFromHeapAllocator(TTString& string);

        void WriteCodeToFile(IOStreamFunctions& streamFunctions, LPCWSTR srcDir, LPCWSTR docId, LPCWSTR sourceUri, const wchar* sourceBuffer, uint32 length);
        void ReadCodeFromFile(IOStreamFunctions& streamFunctions, LPCWSTR srcDir, LPCWSTR docId, LPCWSTR sourceUri, wchar* sourceBuffer, uint32 length);
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
        void AssertSnapEquivTTDVar_Helper(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, TTDComparePath::StepKind stepKind, const TTDComparePath::PathEntry& next);

        void AssertSnapEquivTTDVar_Property(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, Js::PropertyId pid);
        void AssertSnapEquivTTDVar_PropertyGetter(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, Js::PropertyId pid);
        void AssertSnapEquivTTDVar_PropertySetter(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, Js::PropertyId pid);
        void AssertSnapEquivTTDVar_Array(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, uint32 index);
        void AssertSnapEquivTTDVar_SlotArray(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, uint32 index);
        void AssertSnapEquivTTDVar_Special(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, LPCWSTR specialField);
        void AssertSnapEquivTTDVar_SpecialArray(const TTDVar v1, const TTDVar v2, TTDCompareMap& compareMap, LPCWSTR specialField, uint32 index);
#endif

        //////////////////

        //A structure for the primitive javascript types
        struct SnapPrimitiveValue
        {
            //The id (address of the object)
            TTD_PTR_ID PrimitiveValueId;

            //The type of the primitive value
            NSSnapType::SnapType* SnapType;

            //The tag given to uniquely identify this object accross time (for logging and callback uses)
            TTD_LOG_TAG ValueLogTag;

            //The optional well known token for this object (or INVALID)
            TTD_WELLKNOWN_TOKEN OptWellKnownToken;

            union
            {
                BOOL u_boolValue;
                int64 u_int64Value;
                uint64 u_uint64Value;
                double u_doubleValue;
                Js::PropertyId u_propertyIdValue; //for a symbol
            };

            //If this is a string then a ptr to the actual string contents
            TTString* m_optStringValue;
        };

        void ExtractSnapPrimitiveValue(SnapPrimitiveValue* snapValue, Js::RecyclableObject* jsValue, bool isWellKnown, bool isLogged, const TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapType*>& idToTypeMap, SlabAllocator& alloc);
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
            TTD_LOG_TAG ScriptContextTag;

            //The number of slots in the scope array
            uint32 SlotCount;

            //The data values for the slots in the scope entry
            TTDVar* Slots;

#if ENABLE_TTD_INTERNAL_DIAGNOSTICS
            Js::PropertyId* DebugPIDArray;
#endif

            //The meta-data for the slot array
            bool isFunctionBodyMetaData;

            TTD_PTR_ID OptFunctionBodyId;
            //TODO: add debugger scope meta-data info
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
            TTD_LOG_TAG ScriptContextTag;

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
            TTD_PTR_ID ResolveObjId;
            TTD_PTR_ID RejectObjId;
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

        //Information that is common to all top-level bodies
        struct TopLevelCommonBodyResolveInfo
        {
            //The id this body is associated with
            TTD_PTR_ID FunctionBodyId;

            //The context this body is associated with
            TTD_LOG_TAG ScriptContextTag;

            //The string name of the function (allocated into the heap allocator NOT slab)
            TTString FunctionName;

            //The module and document id
            Js::ModuleID ModuleId;
            DWORD_PTR DocumentID;

            //Src URI may be null (allocated into the heap allocator NOT slab)
            TTString SourceUri;

            //The source buffer (allocated into the heap allocator NOT slab)
            TTString SourceCode;
        };

        //Extract WITHOUT COPYING the info needed for this top level function -- use in script context when function is parsed to keep all the info together and then we do the copying later when doing snapshots
        void ExtractTopLevelCommonBodyResolveInfo_InScriptContext(TopLevelCommonBodyResolveInfo* fbInfo, Js::FunctionBody* fb, Js::ModuleID moduleId, DWORD_PTR documentID, bool isLibraryCodeLoad, LPCWSTR source, uint32 sourceLen);
        void UnloadTopLevelCommonBodyResolveInfo(TopLevelCommonBodyResolveInfo* fbInfo);

        void ExtractTopLevelCommonBodyResolveInfo_InShapshot(TopLevelCommonBodyResolveInfo* fbInfoDest, const TopLevelCommonBodyResolveInfo* fbInfoSrc, SlabAllocator& alloc);
        void EmitTopLevelCommonBodyResolveInfo(const TopLevelCommonBodyResolveInfo* fbInfo, bool emitInline, LPCWSTR sourceDir, IOStreamFunctions& streamFunctions, FileWriter* writer, NSTokens::Separator separator);
        void ParseTopLevelCommonBodyResolveInfo(TopLevelCommonBodyResolveInfo* fbInfo, bool readSeperator, bool parseInline, LPCWSTR sourceDir, IOStreamFunctions& streamFunctions, FileReader* reader, SlabAllocator& alloc);

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

        void ExtractTopLevelLoadedFunctionBodyInfo_InScriptContext(TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, Js::ModuleID moduleId, DWORD_PTR documentID, LPCWSTR source, uint32 sourceLen, LoadScriptFlag loadFlag);
        void UnloadTopLevelLoadedFunctionBodyInfo(TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo);

        void ExtractTopLevelLoadedFunctionBodyInfo_InShapshot(TopLevelScriptLoadFunctionBodyResolveInfo* fbInfoDest, const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfoSrc, SlabAllocator& alloc);
        Js::FunctionBody* InflateTopLevelLoadedFunctionBodyInfo(const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo, Js::ScriptContext* ctx);

        void EmitTopLevelLoadedFunctionBodyInfo(const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo, LPCWSTR sourceDir, IOStreamFunctions& streamFunctions, FileWriter* writer, NSTokens::Separator separator);
        void ParseTopLevelLoadedFunctionBodyInfo(TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo, bool readSeperator, LPCWSTR sourceDir, IOStreamFunctions& streamFunctions, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo1, const TopLevelScriptLoadFunctionBodyResolveInfo* fbInfo2, TTDCompareMap& compareMap);
#endif

        //A struct that we can use to resolve a top-level 'new Function' function bodies and info
        struct TopLevelNewFunctionBodyResolveInfo
        {
            //The base information for the top-level resolve info
            TopLevelCommonBodyResolveInfo TopLevelBase;
        };

        void ExtractTopLevelNewFunctionBodyInfo_InScriptContext(TopLevelNewFunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, Js::ModuleID moduleId, LPCWSTR source, uint32 sourceLen);
        void UnloadTopLevelNewFunctionBodyInfo(TopLevelNewFunctionBodyResolveInfo* fbInfo);

        void ExtractTopLevelNewFunctionBodyInfo_InShapshot(TopLevelNewFunctionBodyResolveInfo* fbInfoDest, const TopLevelNewFunctionBodyResolveInfo* fbInfoSrc, SlabAllocator& alloc);
        Js::FunctionBody* InflateTopLevelNewFunctionBodyInfo(const TopLevelNewFunctionBodyResolveInfo* fbInfo, Js::ScriptContext* ctx);

        void EmitTopLevelNewFunctionBodyInfo(const TopLevelNewFunctionBodyResolveInfo* fbInfo, LPCWSTR sourceDir, IOStreamFunctions& streamFunctions, FileWriter* writer, NSTokens::Separator separator);
        void ParseTopLevelNewFunctionBodyInfo(TopLevelNewFunctionBodyResolveInfo* fbInfo, bool readSeperator, LPCWSTR sourceDir, IOStreamFunctions& streamFunctions, FileReader* reader, SlabAllocator& alloc);

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

        void ExtractTopLevelEvalFunctionBodyInfo_InScriptContext(TopLevelEvalFunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, Js::ModuleID moduleId, LPCWSTR source, uint32 sourceLen, ulong grfscr, bool registerDocument, BOOL isIndirect, BOOL strictMode);
        void UnloadTopLevelEvalFunctionBodyInfo(TopLevelEvalFunctionBodyResolveInfo* fbInfo);

        void ExtractTopLevelEvalFunctionBodyInfo_InShapshot(TopLevelEvalFunctionBodyResolveInfo* fbInfoDest, const TopLevelEvalFunctionBodyResolveInfo* fbInfoSrc, SlabAllocator& alloc);
        Js::FunctionBody* InflateTopLevelEvalFunctionBodyInfo(const TopLevelEvalFunctionBodyResolveInfo* fbInfo, Js::ScriptContext* ctx);

        void EmitTopLevelEvalFunctionBodyInfo(const TopLevelEvalFunctionBodyResolveInfo* fbInfo, LPCWSTR sourceDir, IOStreamFunctions& streamFunctions, FileWriter* writer, NSTokens::Separator separator);
        void ParseTopLevelEvalFunctionBodyInfo(TopLevelEvalFunctionBodyResolveInfo* fbInfo, bool readSeperator, LPCWSTR sourceDir, IOStreamFunctions& streamFunctions, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const TopLevelEvalFunctionBodyResolveInfo* fbInfo1, const TopLevelEvalFunctionBodyResolveInfo* fbInfo2, TTDCompareMap& compareMap);
#endif

        //A struct that we can use to resolve a function body during inflation
        struct FunctionBodyResolveInfo
        {
            //The id this body is associated with
            TTD_PTR_ID FunctionBodyId;

            //The context this body is associated with
            TTD_LOG_TAG ScriptContextTag;

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
        };

        void ExtractFunctionBodyInfo(FunctionBodyResolveInfo* fbInfo, Js::FunctionBody* fb, bool isWellKnown, SlabAllocator& alloc);
        void InflateFunctionBody(const FunctionBodyResolveInfo* fbInfo, InflateMap* inflator, const TTDIdentifierDictionary<TTD_PTR_ID, FunctionBodyResolveInfo*>& idToFbResolveMap);

        void EmitFunctionBodyInfo(const FunctionBodyResolveInfo* fbInfo, LPCWSTR sourceDir, IOStreamFunctions& streamFunctions, FileWriter* writer, NSTokens::Separator separator);
        void ParseFunctionBodyInfo(FunctionBodyResolveInfo* fbInfo, bool readSeperator, LPCWSTR sourceDir, IOStreamFunctions& streamFunctions, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const FunctionBodyResolveInfo* fbInfo1, const FunctionBodyResolveInfo* fbInfo2, TTDCompareMap& compareMap);
#endif

        //////////////////

        //A structure for serializing/tracking script contexts
        struct SnapContext
        {
            //The tag id of the script context (actually the global object associated with this context)
            TTD_LOG_TAG m_scriptContextTagId;

            //The random seed for the context
            uint64 m_randomSeed0;
            uint64 m_randomSeed1;

            //The main URI of the context
            TTString m_contextSRC;

            //A list of all *root* scripts that have been loaded into this context
            uint32 m_loadedScriptCount;
            TopLevelScriptLoadFunctionBodyResolveInfo* m_loadedScriptArray;

            uint32 m_newScriptCount;
            TopLevelNewFunctionBodyResolveInfo* m_newScriptArray;

            uint32 m_evalScriptCount;
            TopLevelEvalFunctionBodyResolveInfo* m_evalScriptArray;

            //A list of all the root objects in this context
            uint32 m_rootCount;
            TTD_PTR_ID* m_rootArray;
        };

        void ExtractScriptContext(SnapContext* snapCtx, Js::ScriptContext* ctx, SlabAllocator& alloc);
        void InflateScriptContext(const SnapContext* snpCtx, Js::ScriptContext* intoCtx, InflateMap* inflator);
        void ReLinkRoots(const SnapContext* snpCtx, Js::ScriptContext* intoCtx, InflateMap* inflator);

        void EmitSnapContext(const SnapContext* snapCtx, LPCWSTR sourceDir, IOStreamFunctions& streamFunctions, FileWriter* writer, NSTokens::Separator separator);
        void ParseSnapContext(SnapContext* intoCtx, bool readSeperator, LPCWSTR sourceDir, IOStreamFunctions& streamFunctions, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SnapContext* snapCtx1, const SnapContext* snapCtx2, TTDCompareMap& compareMap);
#endif
    }
}

#endif
