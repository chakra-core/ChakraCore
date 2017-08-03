//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

namespace TTD
{
    namespace NSSnapObjects
    {
        //////////////////
        //We basically build a fake vtable here and use it to fake class behavior without needing a virtual type features we don't need

        typedef Js::RecyclableObject*(*fPtr_DoObjectInflation)(const SnapObject* snpObject, InflateMap* inflator);
        typedef void(*fPtr_DoAddtlValueInstantiation)(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator);
        typedef void(*fPtr_EmitAddtlInfo)(const SnapObject* snpObject, FileWriter* writer);
        typedef void(*fPtr_ParseAddtlInfo)(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

        //Since may types don't need to do anything more in some cases we have a nop functions we set the fptr to null

        struct SnapObjectVTable
        {
            fPtr_DoObjectInflation InflationFunc;
            fPtr_DoAddtlValueInstantiation AddtlInstationationFunc;
            fPtr_EmitAddtlInfo EmitAddtlInfoFunc;
            fPtr_ParseAddtlInfo ParseAddtlInfoFunc;
        };

        struct DependsOnInfo
        {
            uint32 DepOnCount;
            TTD_PTR_ID* DepOnPtrArray;
        };

        //////////////////

        //Base struct for each kind of object we have in the system also used to represent user defined objects directly
        //Although enumerators are not technically subtypes of Dynamic Object we include them here since they have ptrs and such
        struct SnapObject
        {
            //The id (address of the object)
            TTD_PTR_ID ObjectPtrId;

            //The tag we use to distinguish between kinds of snap objects
            SnapObjectType SnapObjectTag;

            //The type for the object
            NSSnapType::SnapType* SnapType;

            //The optional well known token for this object (or INVALID)
            TTD_WELLKNOWN_TOKEN OptWellKnownToken;

            //Return true if the object has the cross site vtable
            BOOL IsCrossSite;

#if ENABLE_OBJECT_SOURCE_TRACKING
            DiagnosticOrigin DiagOriginInfo;
#endif

            //The basic slots of the object the sizes are reproduced in a single array (VarArrayCount should be the same as MaxUsedSlotIndex from the type)
            uint32 VarArrayCount;
            TTDVar* VarArray;

            //The numeric indexed properties associated with the object (or invalid if no associated array)
            TTD_PTR_ID OptIndexedObjectArray;

            //Objects this depends on when creating (or nullptr if no dependencies) 
            DependsOnInfo* OptDependsOnInfo;

            //A ptr for the remaining info which must be cast when needed by handler methods
            void* AddtlSnapObjectInfo;
        };

        //Access the AddtlSnapObjectInfo for the snap object as the specified kind (asserts on the tag and casts the data)
        template <typename T, SnapObjectType tag>
        T SnapObjectGetAddtlInfoAs(const SnapObject* snpObject)
        {
            TTDAssert(snpObject->SnapObjectTag == tag, "Tag does not match.");

            return reinterpret_cast<T>(snpObject->AddtlSnapObjectInfo);
        }

        template <typename T, SnapObjectType tag>
        void SnapObjectSetAddtlInfoAs(SnapObject* snpObject, T addtlInfo)
        {
            TTDAssert(sizeof(T) <= sizeof(void*), "Make sure your info fits into the space we have for it.");
            TTDAssert(snpObject->SnapObjectTag == tag, "Tag does not match.");

            snpObject->AddtlSnapObjectInfo = (void*)addtlInfo;
        }

        //The main method that should be called to extract the information from a snap object
        void ExtractCompoundObject(NSSnapObjects::SnapObject* sobj, Js::RecyclableObject* obj, bool isWellKnown, const TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapType*>& idToTypeMap, SlabAllocator& alloc);

        //Extract the basic information for a snap object (helpers)
        void StdPropertyExtract_StaticType(SnapObject* snpObject, Js::RecyclableObject* obj);
        void StdPropertyExtract_DynamicType(SnapObject* snpObject, Js::DynamicObject* dynObj, SlabAllocator& alloc);

        //a simple helper that we can call during the extract to make sure all needed fields are initialized (in cases where the are *no* depends on pointers)
        template <typename T, SnapObjectType tag>
        void StdExtractSetKindSpecificInfo(SnapObject* snpObject, T addtlInfo)
        {
            SnapObjectSetAddtlInfoAs<T, tag>(snpObject, addtlInfo);
        }

        //a simple helper that we can call during the extract to make sure all needed fields are initialized (in cases where the *are* depends on pointers)
        template <typename T, SnapObjectType tag>
        void StdExtractSetKindSpecificInfo(SnapObject* snpObject, T addtlInfo, SlabAllocator& alloc, uint32 dependsOnArrayCount, TTD_PTR_ID* dependsOnArray)
        {
            SnapObjectSetAddtlInfoAs<T, tag>(snpObject, addtlInfo);

            TTDAssert(dependsOnArrayCount != 0 && dependsOnArray != nullptr, "Why are you calling this then?");

            snpObject->OptDependsOnInfo = alloc.SlabAllocateStruct<DependsOnInfo>();
            snpObject->OptDependsOnInfo->DepOnCount = dependsOnArrayCount;
            snpObject->OptDependsOnInfo->DepOnPtrArray = dependsOnArray;
        }

        //Check to see if we have an old version of this object around and, if so, clean up its type/handler/standard properties and return it
        Js::DynamicObject* ReuseObjectCheckAndReset(const SnapObject* snpObject, InflateMap* inflator);

        //TODO: this is a workaround check until we can reliably reset objects -- allows us to early check for non-resetability and fall back to fully recreating script contexts
        bool DoesObjectBlockScriptContextReuse(const SnapObject* snpObject, Js::DynamicObject* dynObj, InflateMap* inflator);

        Js::DynamicObject* ObjectPropertyReset_WellKnown(const SnapObject* snpObject, Js::DynamicObject* dynObj, InflateMap* inflator);
        Js::DynamicObject* ObjectPropertyReset_General(const SnapObject* snpObject, Js::DynamicObject* dynObj, InflateMap* inflator);

        //Set all the general properties for the object 
        void StdPropertyRestore(const SnapObject* snpObject, Js::DynamicObject* obj, InflateMap* inflator);

        //serialize the object data 
        void EmitObject(const SnapObject* snpObject, FileWriter* writer, NSTokens::Separator separator, const SnapObjectVTable* vtable, ThreadContext* threadContext);

        //de-serialize a SnapObject
        void ParseObject(SnapObject* snpObject, bool readSeperator, FileReader* reader, SlabAllocator& alloc, const SnapObjectVTable* vtable, const TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapType*>& ptrIdToTypeMap);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        ////
        //Functions for the VTable for DynamicObject tags

        Js::RecyclableObject* DoObjectInflation_SnapDynamicObject(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        //EmitAddtlInfo is a nop
        //ParseAddtlInfo is a nop
        //AssertSnapEquiv is a nop

        Js::RecyclableObject* DoObjectInflation_SnapExternalObject(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        //EmitAddtlInfo is a nop
        //ParseAddtlInfo is a nop
        //AssertSnapEquiv is a nop

        //////////////////

        //A struct that represents a script function object
        struct SnapScriptFunctionInfo
        {
            //The display name of the function
            TTString DebugFunctionName;

            //The function body reference id (if not library)
            TTD_PTR_ID BodyRefId;

            //The scope info for this function
            TTD_PTR_ID ScopeId;

            //The cached scope object for the function (if it has one)
            TTD_PTR_ID CachedScopeObjId;

            //The home object for the function (if it has one)
            TTD_PTR_ID HomeObjId;

            //If the function has computed name information and what it is (check if this is an invalid ptr)
            TTDVar ComputedNameInfo;

            //Flags matching the runtime definitions
            bool HasSuperReference;
        };

        ////
        //Functions for the VTable for SnapScriptFunction tags

        Js::RecyclableObject* DoObjectInflation_SnapScriptFunctionInfo(const SnapObject* snpObject, InflateMap* inflator);
        void DoAddtlValueInstantiation_SnapScriptFunctionInfo(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator);
        void EmitAddtlInfo_SnapScriptFunctionInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapScriptFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv_SnapScriptFunctionInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        ////
        //RuntimeFunction is resolved via a wellknowntag so we don't worry about it

        //DoObjectInflation is a nop (should either be wellknown or handled as a sub-type)
        //DoAddtlValueInstantiation is a nop
        //EmitAddtlInfo is a nop
        //ParseAddtlInfo is a nop
        //AssertSnapEquiv is a nop

        ////
        //ExternalFunction always traps to log so we don't need any special information

        Js::RecyclableObject* DoObjectInflation_SnapExternalFunctionInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapExternalFunctionInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapExternalFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv_SnapExternalFunctionInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        ////
        //RevokerFunction needs TTD_PTR_ID* for the proxy value

        Js::RecyclableObject* DoObjectInflation_SnapRevokerFunctionInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapRevokerFunctionInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapRevokerFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv_SnapRevokerFunctionInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        ////
        //Functions for the VTable for SnapBoundFunctionObject tags

        //A class that represents a script function object
        struct SnapBoundFunctionInfo
        {
            //The function that is bound
            TTD_PTR_ID TargetFunction;

            //The "this" parameter to use
            TTD_PTR_ID BoundThis;

            //The count of bound arguments
            uint32 ArgCount;

            //The arguments
            TTDVar* ArgArray;
        };

        Js::RecyclableObject* DoObjectInflation_SnapBoundFunctionInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapBoundFunctionInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapBoundFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv_SnapBoundFunctionInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        //////////////////

        ////
        //Functions for the VTable for ActivationObject tags

        Js::RecyclableObject* DoObjectInflation_SnapActivationInfo(const SnapObject* snpObject, InflateMap* inflator);
        Js::RecyclableObject* DoObjectInflation_SnapBlockActivationObject(const SnapObject* snpObject, InflateMap* inflator);
        Js::RecyclableObject* DoObjectInflation_SnapPseudoActivationObject(const SnapObject* snpObject, InflateMap* inflator);
        Js::RecyclableObject* DoObjectInflation_SnapConsoleScopeActivationObject(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        //EmitAddtlInfo is a nop
        //ParseAddtlInfo is a nop
        //AssertSnapEquiv is a nop

        //////////////////

        //A class that represents an arguments object info
        struct SnapHeapArgumentsInfo
        {
            //number of arguments
            uint32 NumOfArguments;

            //The frame object 
            bool IsFrameNullPtr;
            TTD_PTR_ID FrameObject;

            uint32 FormalCount;
            byte* DeletedArgFlags;
        };

        ////
        //Functions for the VTable for ArgumentsObject tags

        Js::RecyclableObject* DoObjectInflation_SnapHeapArgumentsInfo(const SnapObject* snpObject, InflateMap* inflator);
        Js::RecyclableObject* DoObjectInflation_SnapES5HeapArgumentsInfo(const SnapObject* snpObject, InflateMap* inflator);

        template <SnapObjectType argsKind>
        void EmitAddtlInfo_SnapHeapArgumentsInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapHeapArgumentsInfo* argsInfo = SnapObjectGetAddtlInfoAs<SnapHeapArgumentsInfo*, argsKind>(snpObject);

            writer->WriteUInt32(NSTokens::Key::numberOfArgs, argsInfo->NumOfArguments, NSTokens::Separator::CommaAndBigSpaceSeparator);

            writer->WriteBool(NSTokens::Key::boolVal, argsInfo->IsFrameNullPtr, NSTokens::Separator::CommaSeparator);
            writer->WriteAddr(NSTokens::Key::objectId, argsInfo->FrameObject, NSTokens::Separator::CommaSeparator);

            writer->WriteLengthValue(argsInfo->FormalCount, NSTokens::Separator::CommaSeparator);

            writer->WriteKey(NSTokens::Key::deletedArgs, NSTokens::Separator::CommaSeparator);
            writer->WriteSequenceStart();
            for(uint32 i = 0; i < argsInfo->FormalCount; ++i)
            {
                writer->WriteNakedByte(argsInfo->DeletedArgFlags[i], i != 0 ? NSTokens::Separator::CommaSeparator : NSTokens::Separator::NoSeparator);
            }
            writer->WriteSequenceEnd();
        }

        template <SnapObjectType argsKind>
        void ParseAddtlInfo_SnapHeapArgumentsInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapHeapArgumentsInfo* argsInfo = alloc.SlabAllocateStruct<SnapHeapArgumentsInfo>();

            argsInfo->NumOfArguments = reader->ReadUInt32(NSTokens::Key::numberOfArgs, true);

            argsInfo->IsFrameNullPtr = reader->ReadBool(NSTokens::Key::boolVal, true);
            argsInfo->FrameObject = reader->ReadAddr(NSTokens::Key::objectId, true);

            argsInfo->FormalCount = reader->ReadLengthValue(true);

            if(argsInfo->FormalCount == 0)
            {
                argsInfo->DeletedArgFlags = nullptr;
            }
            else
            {
                argsInfo->DeletedArgFlags = alloc.SlabAllocateArray<byte>(argsInfo->FormalCount);
            }

            reader->ReadKey(NSTokens::Key::deletedArgs, true);
            reader->ReadSequenceStart();
            for(uint32 i = 0; i < argsInfo->FormalCount; ++i)
            {
                argsInfo->DeletedArgFlags[i] = reader->ReadNakedByte(i != 0);
            }
            reader->ReadSequenceEnd();

            SnapObjectSetAddtlInfoAs<SnapHeapArgumentsInfo*, argsKind>(snpObject, argsInfo);
        }

#if ENABLE_SNAPSHOT_COMPARE
        template <SnapObjectType argsKind>
        void AssertSnapEquiv_SnapHeapArgumentsInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            SnapHeapArgumentsInfo* argsInfo1 = SnapObjectGetAddtlInfoAs<SnapHeapArgumentsInfo*, argsKind>(sobj1);
            SnapHeapArgumentsInfo* argsInfo2 = SnapObjectGetAddtlInfoAs<SnapHeapArgumentsInfo*, argsKind>(sobj2);

            compareMap.DiagnosticAssert(argsInfo1->NumOfArguments == argsInfo2->NumOfArguments);

            compareMap.DiagnosticAssert(argsInfo1->IsFrameNullPtr == argsInfo2->IsFrameNullPtr);
            compareMap.CheckConsistentAndAddPtrIdMapping_Special(argsInfo1->FrameObject, argsInfo2->FrameObject, _u("frameObject"));

            compareMap.DiagnosticAssert(argsInfo1->FormalCount == argsInfo2->FormalCount);
            for(uint32 i = 0; i < argsInfo1->FormalCount; ++i)
            {
                compareMap.DiagnosticAssert(argsInfo1->DeletedArgFlags[i] == argsInfo2->DeletedArgFlags[i]);
            }
        }
#endif

        //////////////////

        ////
        //Promise Info
        struct SnapPromiseInfo
        {
            uint32 Status;
            TTDVar Result;

            //
            //We have the reaction info's inline even theought we want to preserve their pointer identity when inflating. 
            //So we duplicate data here but avoid needed to add more kinds to the mark/extract logic and will check on inflation.
            //

            uint32 ResolveReactionCount;
            NSSnapValues::SnapPromiseReactionInfo* ResolveReactions;

            uint32 RejectReactionCount;
            NSSnapValues::SnapPromiseReactionInfo* RejectReactions;
        };

        Js::RecyclableObject* DoObjectInflation_SnapPromiseInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapPromiseInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapPromiseInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv_SnapPromiseInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        ////
        //PromiseResolveOrRejectFunction Info
        struct SnapPromiseResolveOrRejectFunctionInfo
        {
            TTD_PTR_ID PromiseId;
            bool IsReject;

            //This has a pointer identity but we duplicate here and check on inflate
            TTD_PTR_ID AlreadyResolvedWrapperId;
            bool AlreadyResolvedValue;
        };

        Js::RecyclableObject* DoObjectInflation_SnapPromiseResolveOrRejectFunctionInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapPromiseResolveOrRejectFunctionInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapPromiseResolveOrRejectFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv_SnapPromiseResolveOrRejectFunctionInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        ////
        //ReactionTaskFunction Info
        struct SnapPromiseReactionTaskFunctionInfo
        {
            TTDVar Argument;

            TTD::NSSnapValues::SnapPromiseReactionInfo Reaction;
        };

        Js::RecyclableObject* DoObjectInflation_SnapPromiseReactionTaskFunctionInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapPromiseReactionTaskFunctionInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapPromiseReactionTaskFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv_SnapPromiseReactionTaskFunctionInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        ////
        //AllResolveElementFunctionObject Info
        struct SnapPromiseAllResolveElementFunctionInfo
        {
            NSSnapValues::SnapPromiseCapabilityInfo Capabilities;
            uint32 Index;

            TTD_PTR_ID RemainingElementsWrapperId;
            uint32 RemainingElementsValue;

            TTD_PTR_ID Values;
            bool AlreadyCalled;
        };

        Js::RecyclableObject* DoObjectInflation_SnapPromiseAllResolveElementFunctionInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapPromiseAllResolveElementFunctionInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapPromiseAllResolveElementFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv_SnapPromiseAllResolveElementFunctionInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        //////////////////

        ////
        //SnapBoxedValueObject is resolved via a TTDVar to the underlying value or ptrId

        Js::RecyclableObject* DoObjectInflation_SnapBoxedValue(const SnapObject* snpObject, InflateMap* inflator);
        void DoAddtlValueInstantiation_SnapBoxedValue(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator);
        void EmitAddtlInfo_SnapBoxedValue(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapBoxedValue(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv_SnapBoxedValue(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        ////
        //SnapDateObject is resolved via a double* value

        Js::RecyclableObject* DoObjectInflation_SnapDate(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapDate(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapDate(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv_SnapDate(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        //A struct that represents a regular expression object
        struct SnapRegexInfo
        {
            //The underlying regex string value
            TTString RegexStr;

            //Regex flags value
            UnifiedRegex::RegexFlags Flags;

            //The char count or flag value from the regex object
            CharCount LastIndexOrFlag;

            //The last index var from the regex object
            TTDVar LastIndexVar;
        };

        ////
        //Functions for the VTable for SnapRegexObject

        Js::RecyclableObject* DoObjectInflation_SnapRegexInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapRegexInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapRegexInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv_SnapRegexInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        ////
        //SnapErrorObject has no data currently so we nop on most things

        Js::RecyclableObject* DoObjectInflation_SnapError(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        //EmitAddtlInfo is a nop
        //ParseAddtlInfo is a nop
        //AssertSnapEquiv is a nop

        //////////////////

        //A struct that represents Javascript arrays and Native arrays (T can be Var, int32, or double)
        template<typename T>
        struct SnapArrayInfoBlock
        {
            //The index ranges that this info holds
            uint32 FirstIndex;
            uint32 LastIndex;

            //The contents of this array range [FirstIndex, LastIndex)
            T* ArrayRangeContents;
            byte* ArrayValidTags; //0 is invalid 1 is valid

                                  //The next slice of array elements
            SnapArrayInfoBlock<T>* Next;
        };

        //A struct that represents Javascript arrays and Native arrays (T can be Var, int32, or double)
        template<typename T>
        struct SnapArrayInfo
        {
            //The index ranges that this info holds
            uint32 Length;

            //The array elements or null if this is empty
            SnapArrayInfoBlock<T>* Data;
        };

        template<typename T, bool zeroFillValid>
        SnapArrayInfoBlock<T>* AllocateArrayInfoBlock(SlabAllocator& alloc, uint32 firstIndex, uint32 lastIndex)
        {
            SnapArrayInfoBlock<T>* sai = alloc.SlabAllocateStruct< SnapArrayInfoBlock<T> >();
            sai->FirstIndex = firstIndex;
            sai->LastIndex = lastIndex;

            sai->ArrayRangeContents = alloc.SlabAllocateArray<T>(lastIndex - firstIndex);
            sai->ArrayValidTags = alloc.SlabAllocateArray<byte>(lastIndex - firstIndex);

            //only zero valid flags (which guard the contents values)
            if(zeroFillValid)
            {
                memset(sai->ArrayValidTags, 0, lastIndex - firstIndex);
            }

            sai->Next = nullptr;

            return sai;
        }

        template<typename T>
        SnapArrayInfo<T>* ExtractArrayValues(Js::JavascriptArray* arrayObject, SlabAllocator& alloc)
        {
            SnapArrayInfoBlock<T>* sai = nullptr;

            uint32 length = arrayObject->GetLength();
            if(length == 0)
            {
                ; //just leave it as a null ptr
            }
            else if(length <= TTD_ARRAY_SMALL_ARRAY)
            {
                sai = AllocateArrayInfoBlock<T, false>(alloc, 0, length);
                for(uint32 i = 0; i < length; ++i)
                {
                    sai->ArrayValidTags[i] = (byte)arrayObject->DirectGetItemAt<T>(i, sai->ArrayRangeContents + i);
                }
            }
            else
            {
                SnapArrayInfoBlock<T>* curr = nullptr;
                for(uint32 idx = arrayObject->GetNextIndex(Js::JavascriptArray::InvalidIndex); idx != Js::JavascriptArray::InvalidIndex; idx = arrayObject->GetNextIndex(idx))
                {
                    if(sai == nullptr)
                    {
                        uint32 endIdx = (idx <= (Js::JavascriptArray::MaxArrayLength - TTD_ARRAY_BLOCK_SIZE)) ? (idx + TTD_ARRAY_BLOCK_SIZE) : Js::JavascriptArray::MaxArrayLength;
                        sai = AllocateArrayInfoBlock<T, true>(alloc, idx, endIdx);
                        curr = sai;
                    }

                    TTDAssert(curr != nullptr, "Should get set with variable sai above when needed!");
                    if(idx >= curr->LastIndex)
                    {
                        uint32 endIdx = (idx <= (Js::JavascriptArray::MaxArrayLength - TTD_ARRAY_BLOCK_SIZE)) ? (idx + TTD_ARRAY_BLOCK_SIZE) : Js::JavascriptArray::MaxArrayLength;
                        curr->Next = AllocateArrayInfoBlock<T, true>(alloc, idx, endIdx);
                        curr = curr->Next;
                    }

                    curr->ArrayValidTags[idx - curr->FirstIndex] = TRUE;
                    arrayObject->DirectGetItemAt<T>(idx, curr->ArrayRangeContents + (idx - curr->FirstIndex));
                }
            }

            SnapArrayInfo<T>* res = alloc.SlabAllocateStruct< SnapArrayInfo<T> >();
            res->Length = arrayObject->GetLength();
            res->Data = sai;

            return res;
        }

        int32 SnapArrayInfo_InflateValue(int32 value, InflateMap* inflator);
        void SnapArrayInfo_EmitValue(int32 value, FileWriter* writer);
        void SnapArrayInfo_ParseValue(int32* into, FileReader* reader, SlabAllocator& alloc);

        double SnapArrayInfo_InflateValue(double value, InflateMap* inflator);
        void SnapArrayInfo_EmitValue(double value, FileWriter* writer);
        void SnapArrayInfo_ParseValue(double* into, FileReader* reader, SlabAllocator& alloc);

        Js::Var SnapArrayInfo_InflateValue(TTDVar value, InflateMap* inflator);
        void SnapArrayInfo_EmitValue(TTDVar value, FileWriter* writer);
        void SnapArrayInfo_ParseValue(TTDVar* into, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        void SnapArrayInfo_EquivValue(int32 val1, int32 val2, TTDCompareMap& compareMap, int32 i);
        void SnapArrayInfo_EquivValue(double val1, double val2, TTDCompareMap& compareMap, int32 i);
        void SnapArrayInfo_EquivValue(TTDVar val1, TTDVar val2, TTDCompareMap& compareMap, int32 i);
#endif

        ////
        //Functions for the VTable for SnapArrayObject tags

        template<typename T, SnapObjectType snapArrayKind>
        Js::RecyclableObject* DoObjectInflation_SnapArrayInfo(const SnapObject* snpObject, InflateMap* inflator)
        {
            //Arrays can change type on us so seems easiest to always re-create them.
            //We can re-evaluate this choice later if needed and add checks for same type-ness.

            const SnapArrayInfo<T>* arrayInfo = SnapObjectGetAddtlInfoAs<SnapArrayInfo<T>*, snapArrayKind>(snpObject);
            const SnapArrayInfoBlock<T>* dataBlock = arrayInfo->Data;
            Js::ScriptContext* ctx = inflator->LookupScriptContext(snpObject->SnapType->ScriptContextLogId);

            Js::JavascriptLibrary* jslib = ctx->GetLibrary();
            uint32 preAllocSpace = 0;
            if(dataBlock != nullptr && dataBlock->Next == nullptr && dataBlock->FirstIndex == 0 && dataBlock->LastIndex <= TTD_ARRAY_SMALL_ARRAY)
            {
                preAllocSpace = dataBlock->LastIndex; //first index is 0
            }

            if(snpObject->SnapType->JsTypeId == Js::TypeIds_Array)
            {
                if(preAllocSpace == 0)
                {
                    return jslib->CreateArray();
                }
                else
                {
                    Js::DynamicObject* rcObj = ReuseObjectCheckAndReset(snpObject, inflator);
                    if(rcObj != nullptr)
                    {
                        Js::JavascriptArray::FromVar(rcObj)->SetLength(preAllocSpace);
                        return rcObj;
                    }
                    else
                    {
                        return jslib->CreateArray(preAllocSpace);
                    }
                }
            }
            else if(snpObject->SnapType->JsTypeId == Js::TypeIds_NativeIntArray)
            {
                return (preAllocSpace > 0) ? ctx->GetLibrary()->CreateNativeIntArray(preAllocSpace) : ctx->GetLibrary()->CreateNativeIntArray();
            }
            else if(snpObject->SnapType->JsTypeId == Js::TypeIds_NativeFloatArray)
            {
                return (preAllocSpace > 0) ? ctx->GetLibrary()->CreateNativeFloatArray(preAllocSpace) : ctx->GetLibrary()->CreateNativeFloatArray();
            }
            else
            {
                TTDAssert(false, "Unknown array type!");
                return nullptr;
            }
        }

        template<typename T, typename U>
        void DoAddtlValueInstantiation_SnapArrayInfoCore(SnapArrayInfo<T>* arrayInfo, Js::JavascriptArray* arrayObj, InflateMap* inflator)
        {
            const SnapArrayInfoBlock<T>* dataBlock = arrayInfo->Data;

            while(dataBlock != nullptr)
            {
                for(uint32 i = 0; i < (dataBlock->LastIndex - dataBlock->FirstIndex); ++i)
                {
                    if(dataBlock->ArrayValidTags[i])
                    {
                        T ttdVal = dataBlock->ArrayRangeContents[i];
                        U jsVal = SnapArrayInfo_InflateValue(ttdVal, inflator);

                        arrayObj->DirectSetItemAt<U>(i + dataBlock->FirstIndex, jsVal);
                    }
                }
                dataBlock = dataBlock->Next;
            }

            //Ensure this value is set correctly in case of sparse arrays
            arrayObj->SetLength(arrayInfo->Length);
        }

        template<typename T, typename U, SnapObjectType snapArrayKind>
        void DoAddtlValueInstantiation_SnapArrayInfo(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator)
        {
            SnapArrayInfo<T>* arrayInfo = SnapObjectGetAddtlInfoAs<SnapArrayInfo<T>*, snapArrayKind>(snpObject);
            Js::JavascriptArray* arrayObj = static_cast<Js::JavascriptArray*>(obj);

            DoAddtlValueInstantiation_SnapArrayInfoCore<T, U>(arrayInfo, arrayObj, inflator);
        }

        template<typename T>
        void EmitAddtlInfo_SnapArrayInfoCore(SnapArrayInfo<T>* arrayInfo, FileWriter* writer)
        {
            writer->WriteLengthValue(arrayInfo->Length, NSTokens::Separator::CommaSeparator);

            uint32 blockCount = 0;
            for(SnapArrayInfoBlock<T>* currInfo = arrayInfo->Data; currInfo != nullptr; currInfo = currInfo->Next)
            {
                blockCount++;
            }

            writer->WriteLengthValue(blockCount, NSTokens::Separator::CommaAndBigSpaceSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            writer->AdjustIndent(1);
            for(SnapArrayInfoBlock<T>* currInfo = arrayInfo->Data; currInfo != nullptr; currInfo = currInfo->Next)
            {
                writer->WriteRecordStart(currInfo == arrayInfo->Data ? NSTokens::Separator::BigSpaceSeparator : NSTokens::Separator::CommaAndBigSpaceSeparator);
                writer->WriteUInt32(NSTokens::Key::index, currInfo->FirstIndex);
                writer->WriteUInt32(NSTokens::Key::offset, currInfo->LastIndex, NSTokens::Separator::CommaSeparator);

                writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
                for(uint32 i = currInfo->FirstIndex; i < currInfo->LastIndex; ++i)
                {
                    uint32 j = (i - currInfo->FirstIndex);
                    writer->WriteRecordStart(j == 0 ? NSTokens::Separator::NoSeparator : NSTokens::Separator::CommaSeparator);

                    writer->WriteInt32(NSTokens::Key::isValid, currInfo->ArrayValidTags[j]);
                    if(currInfo->ArrayValidTags[j])
                    {
                        SnapArrayInfo_EmitValue(currInfo->ArrayRangeContents[j], writer);
                    }
                    writer->WriteRecordEnd();
                }
                writer->WriteSequenceEnd();
                writer->WriteRecordEnd();
            }
            writer->AdjustIndent(-1);
            writer->WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);
        }

        template<typename T, SnapObjectType snapArrayKind>
        void EmitAddtlInfo_SnapArrayInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapArrayInfo<T>* arrayInfo = SnapObjectGetAddtlInfoAs<SnapArrayInfo<T>*, snapArrayKind>(snpObject);

            EmitAddtlInfo_SnapArrayInfoCore<T>(arrayInfo, writer);
        }

        template<typename T>
        SnapArrayInfo<T>* ParseAddtlInfo_SnapArrayInfoCore(FileReader* reader, SlabAllocator& alloc)
        {
            uint32 alength = reader->ReadLengthValue(true);

            SnapArrayInfoBlock<T>* arrayInfo = nullptr;
            SnapArrayInfoBlock<T>* curr = nullptr;

            uint32 blockCount = reader->ReadLengthValue(true);
            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 k = 0; k < blockCount; ++k)
            {
                reader->ReadRecordStart(k != 0);

                SnapArrayInfoBlock<T>* tmp = alloc.SlabAllocateStruct< SnapArrayInfoBlock<T> >();
                tmp->FirstIndex = reader->ReadUInt32(NSTokens::Key::index);
                tmp->LastIndex = reader->ReadUInt32(NSTokens::Key::offset, true);

                tmp->ArrayValidTags = alloc.SlabAllocateArray<byte>(tmp->LastIndex - tmp->FirstIndex);
                tmp->ArrayRangeContents = alloc.SlabAllocateArray<T>(tmp->LastIndex - tmp->FirstIndex);
                tmp->Next = nullptr;

                if(curr != nullptr)
                {
                    curr->Next = tmp;
                }
                curr = tmp;
                TTDAssert(curr != nullptr, "Sanity assert failed.");

                if(arrayInfo == nullptr)
                {
                    arrayInfo = curr;
                }

                reader->ReadSequenceStart_WDefaultKey(true);
                for(uint32 i = curr->FirstIndex; i < curr->LastIndex; ++i)
                {
                    uint32 j = (i - curr->FirstIndex);
                    reader->ReadRecordStart(j != 0);

                    curr->ArrayValidTags[j] = (byte)reader->ReadInt32(NSTokens::Key::isValid);
                    if(curr->ArrayValidTags[j])
                    {
                        SnapArrayInfo_ParseValue(curr->ArrayRangeContents + j, reader, alloc);
                    }
                    reader->ReadRecordEnd();
                }
                reader->ReadSequenceEnd();
                reader->ReadRecordEnd();
            }
            reader->ReadSequenceEnd();

            SnapArrayInfo<T>* res = alloc.SlabAllocateStruct< SnapArrayInfo<T> >();
            res->Length = alength;
            res->Data = arrayInfo;

            return res;
        }

        template<typename T, SnapObjectType snapArrayKind>
        void ParseAddtlInfo_SnapArrayInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapArrayInfo<T>* arrayInfo = ParseAddtlInfo_SnapArrayInfoCore<T>(reader, alloc);

            SnapObjectSetAddtlInfoAs<SnapArrayInfo<T>*, snapArrayKind>(snpObject, arrayInfo);
        }

#if ENABLE_SNAPSHOT_COMPARE
        template<typename T>
        void AdvanceArrayIndex_SnapArrayInfoCompare(uint32* index, uint32* pos, const SnapArrayInfoBlock<T>** segment)
        {
            *index = *index + 1;
            if(*index >= (*segment)->LastIndex)
            {
                *segment = (*segment)->Next;
                *pos = 0;

                if(*segment != nullptr)
                {
                    *index = (*segment)->FirstIndex;
                }
            }
            else
            {
                TTDAssert(*index >= (*segment)->FirstIndex, "Something went wrong.");

                *pos = *index - (*segment)->FirstIndex;
            }
        }

        template<typename T>
        void AssertSnapEquiv_SnapArrayInfoCore(const SnapArrayInfoBlock<T>* arrayInfo1, const SnapArrayInfoBlock<T>* arrayInfo2, TTDCompareMap& compareMap)
        {
            uint32 index1 = (arrayInfo1 != nullptr) ? arrayInfo1->FirstIndex : 0;
            uint32 pos1 = 0;

            uint32 index2 = (arrayInfo2 != nullptr) ? arrayInfo2->FirstIndex : 0;
            uint32 pos2 = 0;

            while(arrayInfo1 != nullptr && arrayInfo2 != nullptr)
            {
                if(index1 < index2)
                {
                    compareMap.DiagnosticAssert(!arrayInfo1->ArrayValidTags[pos1]);
                    AdvanceArrayIndex_SnapArrayInfoCompare(&index1, &pos1, &arrayInfo1);
                }
                else if(index1 > index2)
                {
                    compareMap.DiagnosticAssert(!arrayInfo2->ArrayValidTags[pos2]);
                    AdvanceArrayIndex_SnapArrayInfoCompare(&index2, &pos2, &arrayInfo2);
                }
                else
                {
                    compareMap.DiagnosticAssert(arrayInfo1->ArrayValidTags[pos1] == arrayInfo2->ArrayValidTags[pos2]);
                    if(arrayInfo1->ArrayValidTags[pos1])
                    {
                        SnapArrayInfo_EquivValue(arrayInfo1->ArrayRangeContents[pos1], arrayInfo2->ArrayRangeContents[pos2], compareMap, index1);
                    }

                    AdvanceArrayIndex_SnapArrayInfoCompare(&index1, &pos1, &arrayInfo1);
                    AdvanceArrayIndex_SnapArrayInfoCompare(&index2, &pos2, &arrayInfo2);
                }
            }

            //make sure any remaining entries an empty
            while(arrayInfo1 != nullptr)
            {
                compareMap.DiagnosticAssert(!arrayInfo1->ArrayValidTags[pos1]);
                AdvanceArrayIndex_SnapArrayInfoCompare(&index1, &pos1, &arrayInfo1);
            }

            while(arrayInfo1 != nullptr)
            {
                compareMap.DiagnosticAssert(!arrayInfo2->ArrayValidTags[pos2]);
                AdvanceArrayIndex_SnapArrayInfoCompare(&index2, &pos2, &arrayInfo2);
            }
        }

        template<typename T, SnapObjectType snapArrayKind>
        void AssertSnapEquiv_SnapArrayInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap)
        {
            const SnapArrayInfo<T>* arrayInfo1 = SnapObjectGetAddtlInfoAs<SnapArrayInfo<T>*, snapArrayKind>(sobj1);
            const SnapArrayInfo<T>* arrayInfo2 = SnapObjectGetAddtlInfoAs<SnapArrayInfo<T>*, snapArrayKind>(sobj2);

            compareMap.DiagnosticAssert(arrayInfo1->Length == arrayInfo2->Length);

            AssertSnapEquiv_SnapArrayInfoCore<T>(arrayInfo1->Data, arrayInfo2->Data, compareMap);
        }
#endif

        //////////////////

        //A struct that represents a single getter/setter value in an ES5 array
        struct SnapES5ArrayGetterSetterEntry
        {
            uint32 Index;
            Js::PropertyAttributes Attributes;

            TTDVar Getter;
            TTDVar Setter;
        };

        //A struct that represents Javascript ES5 arrays
        struct SnapES5ArrayInfo
        {
            //Values copied from the ES5ArrayTypeHandler indexed data map
            uint32 GetterSetterCount;
            SnapES5ArrayGetterSetterEntry* GetterSetterEntries;

            //Values that are copied from the underlying data array
            SnapArrayInfo<TTDVar>* BasicArrayData;

            //True if the length is writable
            bool IsLengthWritable;
        };

        Js::RecyclableObject* DoObjectInflation_SnapES5ArrayInfo(const SnapObject* snpObject, InflateMap* inflator);
        void DoAddtlValueInstantiation_SnapES5ArrayInfo(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator);
        void EmitAddtlInfo_SnapES5ArrayInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapES5ArrayInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapES5ArrayInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        //////////////////

        //A struct that represents an array buffer
        struct SnapArrayBufferInfo
        {
            //The length of the array in bytes
            uint32 Length;

            //The byte array with the data
            byte* Buff;
        };

        ////
        //Functions for the VTable for SnapArrayBufferObject

        Js::RecyclableObject* DoObjectInflation_SnapArrayBufferInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapArrayBufferInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapArrayBufferInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapArrayBufferInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        //A struct that represents a typed array
        struct SnapTypedArrayInfo
        {
            //The byte offset that the data starts in in the buffer
            uint32 ByteOffset;

            //The length of the array (in terms of the underlying typed values)
            uint32 Length;

            //The array buffer that this typed array is a projection of -- need to fix this in cpp file too
            TTD_PTR_ID ArrayBufferAddr;
        };

        ////
        //Functions for the VTable for SnapTypedArrayObject

        Js::RecyclableObject* DoObjectInflation_SnapTypedArrayInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapTypedArrayInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapTypedArrayInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapTypedArrayInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        //////////////////

        //A struct that represents a set (or weakset) object
        struct SnapSetInfo
        {
            //The number of elements in the set
            uint32 SetSize;

            //The set values we want to store
            TTDVar* SetValueArray;
        };

        ////
        //Functions for the VTable for SnapSetObject

        Js::RecyclableObject* DoObjectInflation_SnapSetInfo(const SnapObject* snpObject, InflateMap* inflator);
        void DoAddtlValueInstantiation_SnapSetInfo(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator);
        void EmitAddtlInfo_SnapSetInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapSetInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapSetInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        //A struct that represents a map (or weakmap) object
        struct SnapMapInfo
        {
            //The number of elements in the set
            uint32 MapSize;

            //The keys/values we want to store (keys are at i, values at i + 1)
            TTDVar* MapKeyValueArray;
        };

        ////
        //Functions for the VTable for SnapMapObject

        Js::RecyclableObject* DoObjectInflation_SnapMapInfo(const SnapObject* snpObject, InflateMap* inflator);
        void DoAddtlValueInstantiation_SnapMapInfo(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator);
        void EmitAddtlInfo_SnapMapInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapMapInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapMapInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif

        //////////////////

        //A struct that represents a map (or weakmap) object
        struct SnapProxyInfo
        {
            //The handler ptrid (or invalid if revoked)
            TTD_PTR_ID HandlerId;

            //The target ptrid (or invalid if revoked)
            TTD_PTR_ID TargetId;
        };

        ////
        //Functions for the VTable for SnapProxyObject

        Js::RecyclableObject* DoObjectInflation_SnapProxyInfo(const SnapObject* snpObject, InflateMap* inflator);
        void EmitAddtlInfo_SnapProxyInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapProxyInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE
        void AssertSnapEquiv_SnapProxyInfo(const SnapObject* sobj1, const SnapObject* sobj2, TTDCompareMap& compareMap);
#endif
    }
}

#endif

