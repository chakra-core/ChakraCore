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
            TTString PropertyName;
        };

        //Inflate the given property record
        const Js::PropertyRecord* InflatePropertyRecord(const SnapPropertyRecord* pRecord, ThreadContext* threadContext);
        const Js::PropertyRecord* InflatePropertyRecord_CreateNew(const SnapPropertyRecord* pRecord, ThreadContext* threadContext);

        //serialize the record data
        void EmitPropertyRecordAsSnapPropertyRecord(const Js::PropertyRecord* pRecord, FileWriter* writer, NSTokens::Separator separator);
        void EmitSnapPropertyRecord(const SnapPropertyRecord* sRecord, FileWriter* writer, NSTokens::Separator separator);

        //de-serialize the data
        void ParseSnapPropertyRecord(SnapPropertyRecord* sRecord, bool readSeperator, FileReader* reader, SlabAllocator& alloc);

        //////////////////

        //A tag that we use to flag property entry attributes
        enum class SnapAttributeTag : uint8
        {
            Clear = 0x0,
            Enumerable   = 0x01, //same value as PropertyEnumerable for fast extraction
            Configurable = 0x02, //same value as PropertyConfigurable for fast extraction
            Writable     = 0x04, //same value as PropertyWritable for fast extraction
            AttributeMask = (Enumerable | Configurable | Writable) //used to mask out other uninteresting Js::Attributes
        };
        DEFINE_ENUM_FLAG_OPERATORS(SnapAttributeTag)

        //A tag that we use to flag data/getter/setters in the more complex types
        enum class SnapEntryDataKindTag : uint8
        {
            Clear = 0x0,
            Uninitialized,
            Data,   //the value in the location is a data entry
            Getter, //the value in the location is a getter function entry
            Setter  //the value in the location is a setter function entry
        };

        //A helper class that tracks all of the information for a single index entry in a snaphandler
        struct SnapHandlerPropertyEntry
        {
            //The property id associated with this location
            Js::PropertyId PropertyRecordId;

            //The kind of data this entry contains (data/getter/setter)
            SnapEntryDataKindTag DataKind;

            //Porperty attributes associated with this location (enumerable/configurable/writable)
            SnapAttributeTag AttributeInfo;
        };

        //A class that represents a single snapshot typehandler entry
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

            //The sealed/frozen/extensible information
            byte IsExtensibleFlag;
        };

        //Extract the info from a entry into the needed property information
        //pid and attr are simple but we check for deleted and internal proprty to set the datakind as Clear or the given dataKind as appropriate
        void ExtractSnapPropertyEntryInfo(SnapHandlerPropertyEntry* entry, Js::PropertyId pid, Js::PropertyAttributes attr, SnapEntryDataKindTag dataKind);

        //serialize the record data
        void EmitSnapHandler(const SnapHandler* snapHandler, FileWriter* writer, NSTokens::Separator separator);

        //de-serialize the data
        void ParseSnapHandler(SnapHandler* snapHandler, bool readSeperator, FileReader* reader, SlabAllocator& alloc);

#if ENABLE_SNAPSHOT_COMPARE 
        int64 ComputeLocationTagForAssertCompare(const SnapHandlerPropertyEntry& handlerEntry);
        void AssertSnapEquiv(const SnapHandler* h1, const SnapHandler* h2, TTDCompareMap& compareMap);
#endif

        //////////////////

        //A class that represents a single snapshot type entry
        struct SnapType
        {
            //The ptrid for the type
            TTD_PTR_ID TypePtrId;

            //The typeid given by Chakra
            Js::TypeId JsTypeId;

            //The prototype var
            TTDVar PrototypeVar;

            //The id of the script context that this object is associated with
            TTD_LOG_PTR_ID ScriptContextLogId;

            //The type descriptor which contains information on property layouts and other type information
            SnapHandler* TypeHandlerInfo;

            //The HasNoEnumerableProperties flag for the type
            bool HasNoEnumerableProperties;
        };

        //serialize the record data
        void EmitSnapType(const SnapType* sType, FileWriter* writer, NSTokens::Separator separator);

        //de-serialize the data
        void ParseSnapType(SnapType* sType, bool readSeperator, FileReader* reader, SlabAllocator& alloc, const TTDIdentifierDictionary<TTD_PTR_ID, SnapHandler*>& typeHandlerMap);

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SnapType* t1, const SnapType* t2, TTDCompareMap& compareMap);
#endif
    }
}

#endif

