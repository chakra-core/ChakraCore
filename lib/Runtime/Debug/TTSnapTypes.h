//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

namespace TTD
{
    namespace NSSnapType
    {
        //typedef for the additional info each specific typehandler/type may need
        typedef void* SnapTypeHandlerSpecificData;
        typedef void* SnapTypeSpecificData;

        //////////////////

        //A struct that represents a single property record in a snapshot
        struct SnapPropertyRecord
        {
            //The property id given by the runtime
            Js::PropertyId PropertyId;

            //flags on various properties
            bool IsNumeric;
            bool IsBound;
            bool IsSymbol;

            //the name of the property
            LPCWSTR PropertyName;
        };

        //Inflate the given property record
        const Js::PropertyRecord* InflatePropertyRecord(const SnapPropertyRecord* pRecord, ThreadContext* threadContext);
        const Js::PropertyRecord* InflatePropertyRecord_CreateNew(const SnapPropertyRecord* pRecord, ThreadContext* threadContext);

        //serialize the record data
        void EmitSnapPropertyRecord(const SnapPropertyRecord* sRecord, FileWriter* writer, NSTokens::Separator separator);

        //de-serialize the data
        void ParseSnapPropertyRecord(SnapPropertyRecord* sRecord, bool readSeperator, FileReader* reader, SlabAllocator& alloc);

        //////////////////

        //A tag for us to track what typehandler this snap type corresponds to
        enum class SnapTypeHandlerTag : int //need to change forward decls in runtime/runtimecore if you change this
        {
            Invalid = 0x0,
            MissingHandlerTag,
            NullHandlerTag,
            SimplePathTypeHandlerTag,
            PathTypeHandlerTag,
            SimpleTypeHandlerTag,
            DictionaryTypeHandlerTag,
            SimpleDictionaryHandler,
            SimpleUnorderedDictionaryHandler,
            ES5GeneralDictionaryHandler
        };

        //A tag that we use to flag data/getter/setters in the more complex types
        //A tag for compact representation of sealed/frozen/extensible information
        enum class SnapAccessorTag : uint8
        {
            Clear = 0x0,
            Data,
            Getter,
            Setter
        };

        //A helper class that tracks all of the information for a single index entry in a snaphandler
        struct SnapHandlerPropertyEntry
        {
            Js::PropertyAttributes AttributeInfo;
            SnapAccessorTag AccessorInfo;

            Js::PropertyId PropertyRecordId;
        };

        //A class that represents a single snapshot typehandler entry
        //
        //TODO: this only tracks the propertyRecords in the type (in the order they enumerated at snapshot time) so we are missing any gaps or other 
        //information that is relevant to the externally observable behavior 
        //
        struct SnapHandler
        {
            //The ptrid for the type
            TTD_PTR_ID HandlerId;

            //Slot capacity information
            uint32 InlineSlotCapacity;
            uint32 TotalSlotCapacity;

            //the property record information array (with the info on the property stored in the location)
            uint32 MaxPropertyIndex;
            SnapHandlerPropertyEntry* PropertyInfoArray;

            //The kind of type this snaptype record is associated with
            SnapTypeHandlerTag SnapHandlerKind;

            //The sealed/frozen/extensible information
            byte IsExtensibleFlag;

            //A pointer to any additional data on the kind
            SnapTypeHandlerSpecificData AddtlData;
        };

        //
        //TODO: we just hope for the best during the inflate phase right now need to add this in (and how to set the value correctly) later
        //
        //Inflate the given type handler
        //Js::DynamicSnapHandler* InflateDynamicHandler(const SnapHandler& snapHandler, ThreadContext* threadContext, InflateMap* inflator);

        //serialize the record data
        void EmitSnapHandler(const SnapHandler* snapHandler, FileWriter* writer, NSTokens::Separator separator);

        //de-serialize the data
        void ParseSnapHandler(SnapHandler* snapHandler, bool readSeperator, FileReader* reader, SlabAllocator& alloc);

        //////////////////

        //A class that represents a single snapshot type entry
        struct SnapType
        {
            //The ptrid for the type
            TTD_PTR_ID TypePtrId;

            //If this is a well-known type
            TTD_WELLKNOWN_TOKEN OptWellKnownToken;

            //The typeid given by Chakra
            Js::TypeId JsTypeId;

            //The prototype ptr or invalid ptr id if we want to ignore the prototype info
            TTD_PTR_ID PrototypeId;

            //The id of the script context that this object is associated with
            TTD_LOG_TAG ScriptContextTag;

            //The type descriptor which contains information on property layouts and other type information
            SnapHandler* TypeHandlerInfo;

            //A pointer to any additional data on the kind (use JsTypeId to determine what this data is)
            SnapTypeSpecificData AddtlData;
        };

        //
        //TODO: we just hope for the best during the inflate phase right now need to add this in (and how to set the value correctly) later
        //
        //Inflate the given type
        //Js::DynamicType* InflateSnapType(const SnapHandler& snapHandler, ThreadContext* threadContext, InflateMap* inflator);

        //serialize the record data
        void EmitSnapType(const SnapType* sType, FileWriter* writer, NSTokens::Separator separator);

        //de-serialize the data
        void ParseSnapType(SnapType* sType, bool readSeperator, FileReader* reader, SlabAllocator& alloc, const TTDIdentifierDictionary<TTD_PTR_ID, SnapHandler*>& typeHandlerMap);
    }
}

#endif

