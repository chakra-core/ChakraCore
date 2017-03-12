//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    namespace NSSnapType
    {
        const Js::PropertyRecord* InflatePropertyRecord(const SnapPropertyRecord* pRecord, ThreadContext* threadContext)
        {
            const Js::PropertyRecord* newPropertyRecord = nullptr;

            if((pRecord->PropertyId != Js::Constants::NoProperty) & (!Js::IsInternalPropertyId(pRecord->PropertyId)))
            {
                switch(pRecord->PropertyId)
                {
                    // Do macro fill of wellknown property id set case statements
#define ENTRY_INTERNAL_SYMBOL(n) case Js::PropertyIds::##n:
#define ENTRY_SYMBOL(n, d) case Js::PropertyIds::##n:
#include "Base/JnDirectFields.h"
                    break; //if it is any of the well known symbols then we don't want to do anything since they are created automatically
                default:
                    newPropertyRecord = InflatePropertyRecord_CreateNew(pRecord, threadContext);
                }
            }

            return newPropertyRecord;
        }

        const Js::PropertyRecord* InflatePropertyRecord_CreateNew(const SnapPropertyRecord* pRecord, ThreadContext* threadContext)
        {
            const char16* pname = pRecord->PropertyName.Contents;
            int32 plen = pRecord->PropertyName.Length;

            const Js::PropertyRecord* newPropertyRecord = nullptr;
            if(pRecord->IsSymbol)
            {
                TTDAssert(pRecord->PropertyId == threadContext->GetNextPropertyId(), "We need to do these in the appropriate order!!!");

                newPropertyRecord = threadContext->UncheckedAddPropertyId(pname, plen, /*bind*/false, /*isSymbol*/true);
            }
            else
            {
                const Js::PropertyRecord* foundProperty = threadContext->FindPropertyRecord(pname, plen);

                if(foundProperty != nullptr)
                {
                    TTDAssert(pRecord->PropertyId == foundProperty->GetPropertyId(), "Someone is adding property ids before me and not in the same order as during record!!!");

                    newPropertyRecord = foundProperty;
                }
                else
                {
                    TTDAssert(pRecord->PropertyId == threadContext->GetNextPropertyId(), "We need to do these in the appropriate order to ensure property ids all match!!!");

                    newPropertyRecord = threadContext->UncheckedAddPropertyId(pname, plen, /*bind*/pRecord->IsBound, /*isSymbol*/false);
                }
            }

            return newPropertyRecord;
        }

        void EmitPropertyRecordAsSnapPropertyRecord(const Js::PropertyRecord* pRecord, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);

            writer->WriteUInt32(NSTokens::Key::propertyId, pRecord->GetPropertyId());

            writer->WriteBool(NSTokens::Key::isNumeric, pRecord->IsNumeric(), NSTokens::Separator::CommaSeparator);
            writer->WriteBool(NSTokens::Key::isBound, pRecord->IsBound(), NSTokens::Separator::CommaSeparator);
            writer->WriteBool(NSTokens::Key::isSymbol, pRecord->IsSymbol(), NSTokens::Separator::CommaSeparator);

            writer->WriteKey(NSTokens::Key::name, NSTokens::Separator::CommaSeparator);
            writer->WriteInlinePropertyRecordName(pRecord->GetBuffer(), pRecord->GetLength());

            writer->WriteRecordEnd();
        }

        void EmitSnapPropertyRecord(const SnapPropertyRecord* sRecord, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);

            writer->WriteUInt32(NSTokens::Key::propertyId, sRecord->PropertyId);

            writer->WriteBool(NSTokens::Key::isNumeric, sRecord->IsNumeric, NSTokens::Separator::CommaSeparator);
            writer->WriteBool(NSTokens::Key::isBound, sRecord->IsBound, NSTokens::Separator::CommaSeparator);
            writer->WriteBool(NSTokens::Key::isSymbol, sRecord->IsSymbol, NSTokens::Separator::CommaSeparator);

            writer->WriteString(NSTokens::Key::name, sRecord->PropertyName, NSTokens::Separator::CommaSeparator);

            writer->WriteRecordEnd();
        }

        void ParseSnapPropertyRecord(SnapPropertyRecord* sRecord, bool readSeperator, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadRecordStart(readSeperator);

            sRecord->PropertyId = reader->ReadUInt32(NSTokens::Key::propertyId);

            sRecord->IsNumeric = reader->ReadBool(NSTokens::Key::isNumeric, true);
            sRecord->IsBound = reader->ReadBool(NSTokens::Key::isBound, true);
            sRecord->IsSymbol = reader->ReadBool(NSTokens::Key::isSymbol, true);

            reader->ReadString(NSTokens::Key::name, alloc, sRecord->PropertyName, true);

            reader->ReadRecordEnd();
        }

        //////////////////

        void ExtractSnapPropertyEntryInfo(SnapHandlerPropertyEntry* entry, Js::PropertyId pid, Js::PropertyAttributes attr, SnapEntryDataKindTag dataKind)
        {
            entry->PropertyRecordId = pid;
            entry->AttributeInfo = ((SnapAttributeTag)attr) & SnapAttributeTag::AttributeMask;

            if((pid == Js::Constants::NoProperty) | Js::IsInternalPropertyId(pid) | ((attr & PropertyDeleted) == PropertyDeleted))
            {
                entry->DataKind = SnapEntryDataKindTag::Clear;
            }
            else
            {
                entry->DataKind = dataKind;
            }
        }

        void EmitSnapHandler(const SnapHandler* snapHandler, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);

            writer->WriteAddr(NSTokens::Key::handlerId, snapHandler->HandlerId);

            writer->WriteUInt32(NSTokens::Key::extensibleFlag, snapHandler->IsExtensibleFlag, NSTokens::Separator::CommaSeparator);

            writer->WriteUInt32(NSTokens::Key::inlineSlotCapacity, snapHandler->InlineSlotCapacity, NSTokens::Separator::CommaSeparator);
            writer->WriteUInt32(NSTokens::Key::totalSlotCapacity, snapHandler->TotalSlotCapacity, NSTokens::Separator::CommaSeparator);

            writer->WriteLengthValue(snapHandler->MaxPropertyIndex, NSTokens::Separator::CommaSeparator);

            if(snapHandler->MaxPropertyIndex > 0)
            {
                writer->WriteSequenceStart_DefaultKey(NSTokens::Separator::CommaSeparator);
                writer->AdjustIndent(1);
                for(uint32 i = 0; i < snapHandler->MaxPropertyIndex; ++i)
                {
                    writer->WriteRecordStart(i != 0 ? NSTokens::Separator::CommaAndBigSpaceSeparator : NSTokens::Separator::BigSpaceSeparator);
                    writer->WriteUInt32(NSTokens::Key::propertyId, snapHandler->PropertyInfoArray[i].PropertyRecordId);
                    writer->WriteTag<SnapEntryDataKindTag>(NSTokens::Key::dataKindTag, snapHandler->PropertyInfoArray[i].DataKind, NSTokens::Separator::CommaSeparator);
                    writer->WriteTag<SnapAttributeTag>(NSTokens::Key::attributeTag, snapHandler->PropertyInfoArray[i].AttributeInfo, NSTokens::Separator::CommaSeparator);
                    writer->WriteRecordEnd();
                }
                writer->AdjustIndent(-1);
                writer->WriteSequenceEnd(NSTokens::Separator::BigSpaceSeparator);
            }

            writer->WriteRecordEnd();
        }

        void ParseSnapHandler(SnapHandler* snapHandler, bool readSeperator, FileReader* reader, SlabAllocator& alloc)
        {
            reader->ReadRecordStart(readSeperator);

            snapHandler->HandlerId = reader->ReadAddr(NSTokens::Key::handlerId);

            snapHandler->IsExtensibleFlag = (byte)reader->ReadUInt32(NSTokens::Key::extensibleFlag, true);

            snapHandler->InlineSlotCapacity = reader->ReadUInt32(NSTokens::Key::inlineSlotCapacity, true);
            snapHandler->TotalSlotCapacity = reader->ReadUInt32(NSTokens::Key::totalSlotCapacity, true);

            snapHandler->MaxPropertyIndex = reader->ReadLengthValue(true);

            if(snapHandler->MaxPropertyIndex == 0)
            {
                snapHandler->PropertyInfoArray = nullptr;
            }
            else
            {
                snapHandler->PropertyInfoArray = alloc.SlabAllocateArray<SnapHandlerPropertyEntry>(snapHandler->MaxPropertyIndex);

                reader->ReadSequenceStart_WDefaultKey(true);
                for(uint32 i = 0; i < snapHandler->MaxPropertyIndex; ++i)
                {
                    reader->ReadRecordStart(i != 0);

                    snapHandler->PropertyInfoArray[i].PropertyRecordId = reader->ReadUInt32(NSTokens::Key::propertyId);
                    snapHandler->PropertyInfoArray[i].DataKind = reader->ReadTag<SnapEntryDataKindTag>(NSTokens::Key::dataKindTag, true);
                    snapHandler->PropertyInfoArray[i].AttributeInfo = reader->ReadTag<SnapAttributeTag>(NSTokens::Key::attributeTag, true);

                    reader->ReadRecordEnd();
                }
                reader->ReadSequenceEnd();
            }

            reader->ReadRecordEnd();
        }

#if ENABLE_SNAPSHOT_COMPARE
        int64 ComputeLocationTagForAssertCompare(const SnapHandlerPropertyEntry& handlerEntry)
        {
            return (((int64)handlerEntry.PropertyRecordId) << 8) | ((int64)handlerEntry.DataKind);
        }

        void AssertSnapEquiv(const SnapHandler* h1, const SnapHandler* h2, TTDCompareMap& compareMap)
        {
            compareMap.DiagnosticAssert(h1->IsExtensibleFlag == h2->IsExtensibleFlag);

            JsUtil::BaseDictionary<int64, uint32, HeapAllocator> h1Dict(&HeapAllocator::Instance);
            for(uint32 i = 0; i < h1->MaxPropertyIndex; ++i)
            {
                if(h1->PropertyInfoArray[i].DataKind != SnapEntryDataKindTag::Clear)
                {
                    int64 locationTag = ComputeLocationTagForAssertCompare(h1->PropertyInfoArray[i]);
                    h1Dict.AddNew(locationTag, i);
                }
            }

            JsUtil::BaseDictionary<int64, uint32, HeapAllocator> h2Dict(&HeapAllocator::Instance);
            for(uint32 i = 0; i < h2->MaxPropertyIndex; ++i)
            {
                if(h2->PropertyInfoArray[i].DataKind != SnapEntryDataKindTag::Clear)
                {
                    int64 locationTag = ComputeLocationTagForAssertCompare(h2->PropertyInfoArray[i]);
                    h2Dict.AddNew(locationTag, i);
                }
            }

            compareMap.DiagnosticAssert(h1Dict.Count() == h2Dict.Count());

            for(auto iter = h1Dict.GetIterator(); iter.IsValid(); iter.MoveNext())
            {
                int64 locationTag = iter.CurrentKey();
                compareMap.DiagnosticAssert(h2Dict.ContainsKey(locationTag));

                uint32 h1Idx = h1Dict.Item(locationTag);
                uint32 h2Idx = h2Dict.Item(locationTag);
                compareMap.DiagnosticAssert(h1->PropertyInfoArray[h1Idx].AttributeInfo == h2->PropertyInfoArray[h2Idx].AttributeInfo);
                compareMap.DiagnosticAssert(h1->PropertyInfoArray[h1Idx].DataKind == h2->PropertyInfoArray[h2Idx].DataKind);
            }
        }
#endif

        //////////////////

        void EmitSnapType(const SnapType* sType, FileWriter* writer, NSTokens::Separator separator)
        {
            writer->WriteRecordStart(separator);

            writer->WriteAddr(NSTokens::Key::typeId, sType->TypePtrId);

            writer->WriteTag<Js::TypeId>(NSTokens::Key::jsTypeId, sType->JsTypeId, NSTokens::Separator::CommaSeparator);
            writer->WriteLogTag(NSTokens::Key::ctxTag, sType->ScriptContextLogId, NSTokens::Separator::CommaSeparator);

            writer->WriteKey(NSTokens::Key::prototypeVar, NSTokens::Separator::CommaSeparator);
            NSSnapValues::EmitTTDVar(sType->PrototypeVar, writer, NSTokens::Separator::NoSeparator);

            TTD_PTR_ID handlerId = (sType->TypeHandlerInfo != nullptr) ? sType->TypeHandlerInfo->HandlerId : TTD_INVALID_PTR_ID;
            writer->WriteAddr(NSTokens::Key::handlerId, handlerId, NSTokens::Separator::CommaSeparator);

            writer->WriteBool(NSTokens::Key::boolVal, sType->HasNoEnumerableProperties, NSTokens::Separator::CommaSeparator);

            writer->WriteRecordEnd();
        }

        void ParseSnapType(SnapType* sType, bool readSeperator, FileReader* reader, SlabAllocator& alloc, const TTDIdentifierDictionary<TTD_PTR_ID, SnapHandler*>& typeHandlerMap)
        {
            reader->ReadRecordStart(readSeperator);

            sType->TypePtrId = reader->ReadAddr(NSTokens::Key::typeId);

            sType->JsTypeId = reader->ReadTag<Js::TypeId>(NSTokens::Key::jsTypeId, true);
            sType->ScriptContextLogId = reader->ReadLogTag(NSTokens::Key::ctxTag, true);

            reader->ReadKey(NSTokens::Key::prototypeVar, true);
            sType->PrototypeVar = NSSnapValues::ParseTTDVar(false, reader);

            TTD_PTR_ID handlerId = reader->ReadAddr(NSTokens::Key::handlerId, true);
            if(handlerId == TTD_INVALID_PTR_ID)
            {
                sType->TypeHandlerInfo = nullptr;
            }
            else
            {
                sType->TypeHandlerInfo = typeHandlerMap.LookupKnownItem(handlerId);
            }

            sType->HasNoEnumerableProperties = reader->ReadBool(NSTokens::Key::boolVal, true);

            reader->ReadRecordEnd();
        }

#if ENABLE_SNAPSHOT_COMPARE 
        void AssertSnapEquiv(const SnapType* t1, const SnapType* t2, TTDCompareMap& compareMap)
        {
            compareMap.DiagnosticAssert(t1->JsTypeId == t2->JsTypeId);
            compareMap.DiagnosticAssert(t1->ScriptContextLogId == t2->ScriptContextLogId);

            NSSnapValues::AssertSnapEquivTTDVar_Special(t1->PrototypeVar, t2->PrototypeVar, compareMap, _u("prototype"));

            if(t1->TypeHandlerInfo == nullptr || t2->TypeHandlerInfo == nullptr)
            {
                compareMap.DiagnosticAssert(t1->TypeHandlerInfo == nullptr && t2->TypeHandlerInfo == nullptr);
            }
            else
            {
                AssertSnapEquiv(t1->TypeHandlerInfo, t2->TypeHandlerInfo, compareMap);
            }

            //
            //Disable until we have fix for set internal property changes state bug.
            //
            //compareMap.DiagnosticAssert(t1->HasNoEnumerableProperties == t2->HasNoEnumerableProperties);
        }
#endif
    }
}

#endif
