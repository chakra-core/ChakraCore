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

        //An enumeration of tags for the SnapObjects (to support dispatch when parsing)
        //IMPORTANT: When adding a new SnapObject subclass you need to add a corresponding typeid here
        enum class SnapObjectType : int //need to change forwarde decls in runtime/runtimecore if you change this
        {
            Invalid = 0x0,

            SnapUnhandledObject,
            SnapDynamicObject,
            SnapScriptFunctionObject,
            SnapRuntimeFunctionObject,
            SnapExternalFunctionObject,
            SnapRuntimeRevokerFunctionObject,
            SnapBoundFunctionObject,
            SnapActivationObject,
            SnapBlockActivationObject,
            SnapPseudoActivationObject,
            SnapConsoleScopeActivationObject,
            SnapActivationObjectEx,
            SnapHeapArgumentsObject,
            SnapBoxedValueObject,
            SnapDateObject,
            SnapRegexObject,
            SnapErrorObject,
            SnapArrayObject,
            SnapNativeIntArrayObject,
            SnapNativeFloatArrayObject,
            SnapArrayBufferObject,
            SnapTypedArrayObject,
            SnapSetObject,
            SnapMapObject,
            SnapProxyObject,
            SnapPromiseObject,
            SnapPromiseResolveOrRejectFunctionObject,
            SnapPromiseReactionTaskFunctionObject,

            //objects that should always be well known but which may have other info we want to restore
            SnapWellKnownObject,

            Limit
        };
        DEFINE_ENUM_FLAG_OPERATORS(SnapObjectType);

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

            //The basic slots of the object the sizes are reproduced in a single array (VarArrayCount should be the same as MaxUsedSlotIndex from the type)
            uint32 VarArrayCount;
            TTDVar* VarArray;

            //The numeric indexed properties associated with the object (or invalid if no associated array)
            TTD_PTR_ID OptIndexedObjectArray;

            //The tag given to uniquely identify this object accross time (for logging and callback uses)
            TTD_LOG_TAG ObjectLogTag;

            //The tag given to uniquely identify this object accross time (for identity and value origin uses -- not always set)
            TTD_IDENTITY_TAG ObjectIdentityTag;

            //Objects this depends on when creating (or nullptr if no dependencies) 
            DependsOnInfo* OptDependsOnInfo;

            //A ptr for the remaining info which must be cast when needed by handler methods
            void* AddtlSnapObjectInfo;
        };

        //Access the AddtlSnapObjectInfo for the snap object as the specified kind (asserts on the tag and casts the data)
        template <typename T, SnapObjectType tag>
        T SnapObjectGetAddtlInfoAs(const SnapObject* snpObject)
        {
            AssertMsg(snpObject->SnapObjectTag == tag, "Tag does not match.");

            return reinterpret_cast<T>(snpObject->AddtlSnapObjectInfo);
        }

        template <typename T, SnapObjectType tag>
        void SnapObjectSetAddtlInfoAs(SnapObject* snpObject, T addtlInfo)
        {
            AssertMsg(sizeof(T) <= sizeof(void*), "Make sure your info fits into the space we have for it.");
            AssertMsg(snpObject->SnapObjectTag == tag, "Tag does not match.");

            snpObject->AddtlSnapObjectInfo = (void*)addtlInfo;
        }

        //The main method that should be called to extract the information from a snap object
        void ExtractCompoundObject(NSSnapObjects::SnapObject* sobj, Js::RecyclableObject* obj, bool isWellKnown, bool isLogged, const TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapType*>& idToTypeMap, SlabAllocator& alloc);

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

            AssertMsg(dependsOnArrayCount != 0 && dependsOnArray != nullptr, "Why are you calling this then?");

            snpObject->OptDependsOnInfo = alloc.SlabAllocateStruct<DependsOnInfo>();
            snpObject->OptDependsOnInfo->DepOnCount = dependsOnArrayCount;
            snpObject->OptDependsOnInfo->DepOnPtrArray = dependsOnArray;
        }

        //Check to see if we have an old version of this object around and, if so, clean up its type/handler/standard properties and return it
        Js::DynamicObject* ReuseObjectCheckAndReset(const SnapObject* snpObject, InflateMap* inflator);

        //Set all the general properties for the object 
        void StdPropertyRestore(const SnapObject* snpObject, Js::DynamicObject* obj, InflateMap* inflator);

        //serialize the object data 
        void EmitObject(const SnapObject* snpObject, FileWriter* writer, NSTokens::Separator separator, const SnapObjectVTable* vtable, ThreadContext* threadContext);

        //de-serialize a SnapObject
        void ParseObject(SnapObject* snpObject, bool readSeperator, FileReader* reader, SlabAllocator& alloc, const SnapObjectVTable* vtable, const TTDIdentifierDictionary<TTD_PTR_ID, NSSnapType::SnapType*>& ptrIdToTypeMap);

        ////
        //Functions for the VTable for DynamicObject tags

        Js::RecyclableObject* DoObjectInflation_SnapDynamicObject(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        //EmitAddtlInfo is a nop
        //ParseAddtlInfo is a nop

        //////////////////

        ////
        //Functions for the VTable for JSRTExternalObject tags

        Js::RecyclableObject* DoObjectInflation_SnapJSRTExternalInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        //EmitAddtlInfo is a nop
        //ParseAddtlInfo is a nop

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

            //The home object for the function (if it has one)
            TTD_PTR_ID HomeObjId;

            //If the function has computed name information and what it is (check if this is an invalid ptr)
            TTDVar ComputedNameInfo;

            //Flags matching the runtime definitions
            bool HasInlineCaches;
            bool HasSuperReference;
            bool IsActiveScript;
        };

        ////
        //Functions for the VTable for SnapScriptFunction tags

        Js::RecyclableObject* DoObjectInflation_SnapScriptFunctionInfo(const SnapObject* snpObject, InflateMap* inflator);
        void DoAddtlValueInstantiation_SnapScriptFunctionInfo(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator);
        void EmitAddtlInfo_SnapScriptFunctionInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapScriptFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

        ////
        //RuntimeFunction is resolved via a wellknowntag so we don't worry about it

        //DoObjectInflation is a nop (should either be wellknown or handled as a sub-type)
        //DoAddtlValueInstantiation is a nop
        //EmitAddtlInfo is a nop
        //ParseAddtlInfo is a nop

        ////
        //ExternalFunction always traps to log so we don't need any special information

        Js::RecyclableObject* DoObjectInflation_SnapExternalFunctionInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapExternalFunctionInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapExternalFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

        ////
        //RevokerFunction needs TTD_PTR_ID* for the proxy value

        Js::RecyclableObject* DoObjectInflation_SnapRevokerFunctionInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapRevokerFunctionInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapRevokerFunctionInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

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

        //////////////////

        ////
        //Functions for the VTable for ActivationObject tags

        Js::RecyclableObject* DoObjectInflation_SnapActivationInfo(const SnapObject* snpObject, InflateMap* inflator);
        Js::RecyclableObject* DoObjectInflation_SnapBlockActivationObject(const SnapObject* snpObject, InflateMap* inflator);
        Js::RecyclableObject* DoObjectInflation_SnapPseudoActivationObject(const SnapObject* snpObject, InflateMap* inflator);
        Js::RecyclableObject* DoObjectInflation_SnapConsoleScopeActivationObject(const SnapObject* snpObject, InflateMap* inflator);
        Js::RecyclableObject* DoObjectInflation_SnapActivationExInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        //EmitAddtlInfo is a nop
        //ParseAddtlInfo is a nop

        //////////////////

        //A class that represents an arguments object info
        struct SnapHeapArgumentsInfo
        {
            //number of arguments
            uint32 NumOfArguments;

            //The frame object 
            bool IsFrameNullPtr;
            bool IsFrameJsNull;
            TTD_PTR_ID FrameObject;

            uint32 FormalCount;
            byte* DeletedArgFlags;
        };

        ////
        //Functions for the VTable for ArgumentsObject tags

        Js::RecyclableObject* DoObjectInflation_SnapHeapArgumentsInfo(const SnapObject* snpObject, InflateMap* inflator);
        void EmitAddtlInfo_SnapHeapArgumentsInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapHeapArgumentsInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

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

        //////////////////

        ////
        //SnapBoxedValueObject is resolved via a TTDVar to the underlying value or ptrId

        Js::RecyclableObject* DoObjectInflation_SnapBoxedValue(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapBoxedValue(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapBoxedValue(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

        ////
        //SnapDateObject is resolved via a double* value

        Js::RecyclableObject* DoObjectInflation_SnapDate(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapDate(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapDate(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

        //A struct that represents a regular expression object
        struct SnapRegexInfo
        {
            //The underlying regex string value
            TTString RegexStr;

            //Regex flags value
            UnifiedRegex::RegexFlags Flags;

            //The char count or flag value from the regex object
            CharCount LastIndexOrFlag;
        };

        ////
        //Functions for the VTable for SnapRegexObject

        Js::RecyclableObject* DoObjectInflation_SnapRegexInfo(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        void EmitAddtlInfo_SnapRegexInfo(const SnapObject* snpObject, FileWriter* writer);
        void ParseAddtlInfo_SnapRegexInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc);

        ////
        //SnapErrorObject has no data currently so we nop on most things

        Js::RecyclableObject* DoObjectInflation_SnapError(const SnapObject* snpObject, InflateMap* inflator);
        //DoAddtlValueInstantiation is a nop
        //EmitAddtlInfo is a nop
        //ParseAddtlInfo is a nop

        //////////////////

        //A struct that represents Javascript arrays and Native arrays (T can be Var, int32, or double)
        template<typename T>
        struct SnapArrayInfo
        {
            //The index ranges that this info holds
            uint32 FirstIndex;
            uint32 LastIndex;

            //The contents of this array range [FirstIndex, LastIndex)
            T* ArrayRangeContents;
            byte* ArrayValidTags; //0 is invalid 1 is valid

                                  //The next slice of array elements
            SnapArrayInfo<T>* Next;
        };

        template<typename T, bool zeroFillValid>
        SnapArrayInfo<T>* AllocateArrayInfoBlock(SlabAllocator& alloc, uint32 firstIndex, uint32 lastIndex)
        {
            SnapArrayInfo<T>* sai = alloc.SlabAllocateStruct< SnapArrayInfo<T> >();
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

        template<typename T, SnapObjectType tag>
        SnapArrayInfo<T>* ExtractArrayValues(Js::JavascriptArray* arrayObject, SnapObject* objData, SlabAllocator& alloc)
        {
            SnapArrayInfo<T>* sai = nullptr;

            uint32 length = arrayObject->GetLength();
            if(length == 0)
            {
                ; //just leave it as a null ptr
            }
            else if(length < TTD_ARRAY_SMALL_ARRAY)
            {
                sai = AllocateArrayInfoBlock<T, false>(alloc, 0, length);
                for(uint32 i = 0; i < length; ++i)
                {
                    sai->ArrayValidTags[i] = (byte)arrayObject->DirectGetItemAt<T>(i, sai->ArrayRangeContents + i);
                }
            }
            else
            {
                SnapArrayInfo<T>* curr = nullptr;
                for(uint32 idx = arrayObject->GetNextIndex(Js::JavascriptArray::InvalidIndex); idx != Js::JavascriptArray::InvalidIndex; idx = arrayObject->GetNextIndex(idx))
                {
                    if(sai == nullptr)
                    {
                        sai = AllocateArrayInfoBlock<T, true>(alloc, idx, idx + TTD_ARRAY_BLOCK_SIZE);
                        curr = sai;
                    }

                    if(idx >= curr->LastIndex)
                    {
                        curr->Next = AllocateArrayInfoBlock<T, true>(alloc, idx, idx + TTD_ARRAY_BLOCK_SIZE);
                        curr = curr->Next;
                    }

                    curr->ArrayValidTags[idx - curr->FirstIndex] = TRUE;
                    arrayObject->DirectGetItemAt<T>(idx, curr->ArrayRangeContents + (idx - curr->FirstIndex));
                }
            }

            StdExtractSetKindSpecificInfo<SnapArrayInfo<T>*, tag>(objData, sai);

            return sai;
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

        ////
        //Functions for the VTable for SnapArrayObject tags

        Js::RecyclableObject* DoObjectInflation_SnapArrayInfo(const SnapObject* snpObject, InflateMap* inflator);

        template<typename T, typename U, SnapObjectType snapArrayKind>
        void DoAddtlValueInstantiation_SnapArrayInfo(const SnapObject* snpObject, Js::RecyclableObject* obj, InflateMap* inflator)
        {
            SnapArrayInfo<T>* arrayInfo = SnapObjectGetAddtlInfoAs<SnapArrayInfo<T>*, snapArrayKind>(snpObject);
            Js::JavascriptArray* arrayObj = static_cast<Js::JavascriptArray*>(obj);

            while(arrayInfo != nullptr)
            {
                for(uint32 i = 0; i < (arrayInfo->LastIndex - arrayInfo->FirstIndex); ++i)
                {
                    if(arrayInfo->ArrayValidTags[i])
                    {
                        T ttdVal = arrayInfo->ArrayRangeContents[i];
                        U jsVal = SnapArrayInfo_InflateValue(ttdVal, inflator);

                        arrayObj->DirectSetItemAt<U>(i + arrayInfo->FirstIndex, jsVal);
                    }
                }
                arrayInfo = arrayInfo->Next;
            }
        }

        template<typename T, SnapObjectType snapArrayKind>
        void EmitAddtlInfo_SnapArrayInfo(const SnapObject* snpObject, FileWriter* writer)
        {
            SnapArrayInfo<T>* arrayInfo = SnapObjectGetAddtlInfoAs<SnapArrayInfo<T>*, snapArrayKind>(snpObject);

            uint32 blockCount = 0;
            for(SnapArrayInfo<T>* currInfo = arrayInfo; currInfo != nullptr; currInfo = currInfo->Next)
            {
                blockCount++;
            }

            writer->WriteLengthValue(blockCount, NSTokens::Separator::CommaAndBigSpaceSeparator);
            writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
            writer->AdjustIndent(1);
            for(SnapArrayInfo<T>* currInfo = arrayInfo; currInfo != nullptr; currInfo = currInfo->Next)
            {
                writer->WriteRecordStart(currInfo == arrayInfo ? NSTokens::Separator::BigSpaceSeparator : NSTokens::Separator::CommaAndBigSpaceSeparator);
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
        void ParseAddtlInfo_SnapArrayInfo(SnapObject* snpObject, FileReader* reader, SlabAllocator& alloc)
        {
            SnapArrayInfo<T>* arrayInfo = nullptr;
            SnapArrayInfo<T>* curr = nullptr;

            uint32 blockCount = reader->ReadLengthValue(true);
            reader->ReadSequenceStart_WDefaultKey(true);
            for(uint32 k = 0; k < blockCount; ++k)
            {
                reader->ReadRecordStart(k != 0);

                SnapArrayInfo<T>* tmp = alloc.SlabAllocateStruct<SnapArrayInfo<T>>();
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

            SnapObjectSetAddtlInfoAs<SnapArrayInfo<T>*, snapArrayKind>(snpObject, arrayInfo);
        }

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
    }
}

#endif

