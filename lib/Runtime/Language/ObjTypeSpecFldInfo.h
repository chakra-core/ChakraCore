//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#if ENABLE_NATIVE_CODEGEN

// forward declaration
class JITTimeConstructorCache;

namespace Js
{

#define InitialObjTypeSpecFldInfoFlagValue 0x01

    struct FixedFieldInfo
    {
        Var fieldValue;
        Type* type;
        bool nextHasSameFixedField; // set to true if the next entry in the FixedFieldInfo array on ObjTypeSpecFldInfo has the same type
    };

    // Union with uint16 flags for fast default initialization
    union ObjTypeSpecFldInfoFlags
    {
        struct
        {
            bool falseReferencePreventionBit : 1;
            bool isPolymorphic : 1;
            bool isRootObjectNonConfigurableField : 1;
            bool isRootObjectNonConfigurableFieldLoad : 1;
            bool usesAuxSlot : 1;
            bool isLocal : 1;
            bool isLoadedFromProto : 1;
            bool usesAccessor : 1;
            bool hasFixedValue : 1;
            bool keepFieldValue : 1;
            bool isBeingStored : 1;
            bool isBeingAdded : 1;
            bool doesntHaveEquivalence : 1;
            bool isBuiltIn : 1;
        };
        struct
        {
            uint16 flags;
        };
        ObjTypeSpecFldInfoFlags(uint16 flags) : flags(flags) { }
    };

    class ObjTypeSpecFldInfo
    {
    private:
        DynamicObject* protoObject;
        PropertyGuard* propertyGuard;
        EquivalentTypeSet* typeSet;
        Type* initialType;
        JITTimeConstructorCache* ctorCache;
        FixedFieldInfo* fixedFieldInfoArray;

        PropertyId propertyId;
        Js::TypeId typeId;
        uint id;

        ObjTypeSpecFldInfoFlags flags;
        uint16 slotIndex;

        uint16 fixedFieldCount; // currently used only for fields that are functions

    public:
        ObjTypeSpecFldInfo() :
            id(0), typeId(TypeIds_Limit), typeSet(nullptr), initialType(nullptr), flags(InitialObjTypeSpecFldInfoFlagValue),
            slotIndex(Constants::NoSlot), propertyId(Constants::NoProperty), protoObject(nullptr), propertyGuard(nullptr),
            ctorCache(nullptr), fixedFieldInfoArray(nullptr) {}

        ObjTypeSpecFldInfo(uint id, TypeId typeId, Type* initialType,
            bool usesAuxSlot, bool isLoadedFromProto, bool usesAccessor, bool isFieldValueFixed, bool keepFieldValue, bool isBuiltIn,
            uint16 slotIndex, PropertyId propertyId, DynamicObject* protoObject, PropertyGuard* propertyGuard,
            JITTimeConstructorCache* ctorCache, FixedFieldInfo* fixedFieldInfoArray) :
            id(id), typeId(typeId), typeSet(nullptr), initialType(initialType), flags(InitialObjTypeSpecFldInfoFlagValue),
            slotIndex(slotIndex), propertyId(propertyId), protoObject(protoObject), propertyGuard(propertyGuard),
            ctorCache(ctorCache), fixedFieldInfoArray(fixedFieldInfoArray)
        {
            this->flags.isPolymorphic = false;
            this->flags.usesAuxSlot = usesAuxSlot;
            this->flags.isLocal = !isLoadedFromProto && !usesAccessor;
            this->flags.isLoadedFromProto = isLoadedFromProto;
            this->flags.usesAccessor = usesAccessor;
            this->flags.hasFixedValue = isFieldValueFixed;
            this->flags.keepFieldValue = keepFieldValue;
            this->flags.isBeingAdded = initialType != nullptr;
            this->flags.doesntHaveEquivalence = true; // doesn't mean anything for data from a monomorphic cache
            this->flags.isBuiltIn = isBuiltIn;
            this->fixedFieldCount = 1;
        }

        ObjTypeSpecFldInfo(uint id, TypeId typeId, Type* initialType, EquivalentTypeSet* typeSet,
            bool usesAuxSlot, bool isLoadedFromProto, bool usesAccessor, bool isFieldValueFixed, bool keepFieldValue, bool doesntHaveEquivalence, bool isPolymorphic,
            uint16 slotIndex, PropertyId propertyId, DynamicObject* protoObject, PropertyGuard* propertyGuard,
            JITTimeConstructorCache* ctorCache, FixedFieldInfo* fixedFieldInfoArray, uint16 fixedFieldCount) :
            id(id), typeId(typeId), typeSet(typeSet), initialType(initialType), flags(InitialObjTypeSpecFldInfoFlagValue),
            slotIndex(slotIndex), propertyId(propertyId), protoObject(protoObject), propertyGuard(propertyGuard),
            ctorCache(ctorCache), fixedFieldInfoArray(fixedFieldInfoArray)
        {
            this->flags.isPolymorphic = isPolymorphic;
            this->flags.usesAuxSlot = usesAuxSlot;
            this->flags.isLocal = !isLoadedFromProto && !usesAccessor;
            this->flags.isLoadedFromProto = isLoadedFromProto;
            this->flags.usesAccessor = usesAccessor;
            this->flags.hasFixedValue = isFieldValueFixed;
            this->flags.keepFieldValue = keepFieldValue;
            this->flags.isBeingAdded = initialType != nullptr;
            this->flags.doesntHaveEquivalence = doesntHaveEquivalence;
            this->flags.isBuiltIn = false;
            this->fixedFieldCount = fixedFieldCount;
        }

        static ObjTypeSpecFldInfo* CreateFrom(uint id, InlineCache* cache, uint cacheId,
            EntryPointInfo *entryPoint, FunctionBody* const topFunctionBody, FunctionBody *const functionBody, FieldAccessStatsPtr inlineCacheStats);

        static ObjTypeSpecFldInfo* CreateFrom(uint id, PolymorphicInlineCache* cache, uint cacheId,
            EntryPointInfo *entryPoint, FunctionBody* const topFunctionBody, FunctionBody *const functionBody, FieldAccessStatsPtr inlineCacheStats);

        uint GetObjTypeSpecFldId() const
        {
            return this->id;
        }

        bool IsMono() const
        {
            return !this->flags.isPolymorphic;
        }

        bool IsPoly() const
        {
            return this->flags.isPolymorphic;
        }

        bool UsesAuxSlot() const
        {
            return this->flags.usesAuxSlot;
        }

        bool IsBuiltin() const
        {
            return this->flags.isBuiltIn;
        }

        void SetUsesAuxSlot(bool value)
        {
            this->flags.usesAuxSlot = value;
        }

        bool IsLoadedFromProto() const
        {
            return this->flags.isLoadedFromProto;
        }

        bool IsLocal() const
        {
            return this->flags.isLocal;
        }

        bool UsesAccessor() const
        {
            return this->flags.usesAccessor;
        }

        bool HasFixedValue() const
        {
            return this->flags.hasFixedValue;
        }

        void SetHasFixedValue(bool value)
        {
            this->flags.hasFixedValue = value;
        }

        bool IsBeingStored() const
        {
            return this->flags.isBeingStored;
        }

        void SetIsBeingStored(bool value)
        {
            this->flags.isBeingStored = value;
        }

        bool IsBeingAdded() const
        {
            return this->flags.isBeingAdded;
        }

        bool IsRootObjectNonConfigurableField() const
        {
            return this->flags.isRootObjectNonConfigurableField;
        }

        bool IsRootObjectNonConfigurableFieldLoad() const
        {
            return this->flags.isRootObjectNonConfigurableField && this->flags.isRootObjectNonConfigurableFieldLoad;
        }

        void SetRootObjectNonConfigurableField(bool isLoad)
        {
            this->flags.isRootObjectNonConfigurableField = true;
            this->flags.isRootObjectNonConfigurableFieldLoad = isLoad;
        }

        bool DoesntHaveEquivalence() const
        {
            return this->flags.doesntHaveEquivalence;
        }

        void ClearFlags()
        {
            this->flags = 0;
        }

        void SetFlags(uint16 flags)
        {
            this->flags = flags | 0x01;
        }

        uint16 GetFlags() const
        {
            return this->flags.flags;
        }

        uint16 GetSlotIndex() const
        {
            return this->slotIndex;
        }

        void SetSlotIndex(uint16 index)
        {
            this->slotIndex = index;
        }

        PropertyId GetPropertyId() const
        {
            return this->propertyId;
        }

        Js::DynamicObject* GetProtoObject() const
        {
            Assert(IsLoadedFromProto());
            return this->protoObject;
        }

        Var GetFieldValue() const
        {
            Assert(IsMono() || (IsPoly() && !DoesntHaveEquivalence()));
            return this->fixedFieldInfoArray[0].fieldValue;
        }

        Var GetFieldValue(uint i) const
        {
            Assert(IsPoly());
            return this->fixedFieldInfoArray[i].fieldValue;
        }

        void SetFieldValue(Var value)
        {
            Assert(IsMono() || (IsPoly() && !DoesntHaveEquivalence()));
            this->fixedFieldInfoArray[0].fieldValue = value;
        }

        Var GetFieldValueAsFixedDataIfAvailable() const;

        Js::JavascriptFunction* GetFieldValueAsFixedFunction() const;
        Js::JavascriptFunction* GetFieldValueAsFixedFunction(uint i) const;

        Js::JavascriptFunction* GetFieldValueAsFunction() const;

        Js::JavascriptFunction* GetFieldValueAsFunctionIfAvailable() const;

        Js::JavascriptFunction* GetFieldValueAsFixedFunctionIfAvailable() const;
        Js::JavascriptFunction* GetFieldValueAsFixedFunctionIfAvailable(uint i) const;

        bool GetKeepFieldValue() const
        {
            return this->flags.keepFieldValue;
        }

        JITTimeConstructorCache* GetCtorCache() const
        {
            return this->ctorCache;
        }

        Js::PropertyGuard* GetPropertyGuard() const
        {
            return this->propertyGuard;
        }

        bool IsObjTypeSpecCandidate() const
        {
            return true;
        }

        bool IsMonoObjTypeSpecCandidate() const
        {
            return IsObjTypeSpecCandidate() && IsMono();
        }

        bool IsPolyObjTypeSpecCandidate() const
        {
            return IsObjTypeSpecCandidate() && IsPoly();
        }

        Js::TypeId GetTypeId() const
        {
            Assert(typeId != TypeIds_Limit);
            return this->typeId;
        }

        Js::TypeId GetTypeId(uint i) const
        {
            Assert(IsPoly());
            return this->fixedFieldInfoArray[i].type->GetTypeId();
        }

        Js::Type * GetType() const
        {
            Assert(IsObjTypeSpecCandidate() && IsMono());
            return this->fixedFieldInfoArray[0].type;
        }

        Js::Type * GetType(uint i) const
        {
            Assert(IsPoly());
            return this->fixedFieldInfoArray[i].type;
        }

        bool HasInitialType() const
        {
            return IsObjTypeSpecCandidate() && IsMono() && !IsLoadedFromProto() && this->initialType != nullptr;
        }

        Js::Type * GetInitialType() const
        {
            Assert(IsObjTypeSpecCandidate() && IsMono() && !IsLoadedFromProto());
            return this->initialType;
        }

        Js::EquivalentTypeSet * GetEquivalentTypeSet() const
        {
            Assert(IsObjTypeSpecCandidate());
            return this->typeSet;
        }

        JITTypeHolder GetFirstEquivalentType() const;

        Js::FixedFieldInfo* GetFixedFieldInfoArray()
        {
            return this->fixedFieldInfoArray;
        }

        uint16 GetFixedFieldCount() const
        {
            return this->fixedFieldCount;
        }

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        const char16 *GetCacheLayoutString() const;
#endif
    };

    class ObjTypeSpecFldInfoArray
    {
    private:
        ObjTypeSpecFldInfo** infoArray;
#if DBG
        uint infoCount;
#endif
    public:
        ObjTypeSpecFldInfoArray();

    private:
        void EnsureArray(Recycler *const recycler, FunctionBody *const functionBody);

    public:
        ObjTypeSpecFldInfo* GetInfo(FunctionBody *const functionBody, const uint index) const;
        ObjTypeSpecFldInfo* GetInfo(const uint index) const;
        ObjTypeSpecFldInfo** GetInfoArray() const { return infoArray; }

        void SetInfo(Recycler *const recycler, FunctionBody *const functionBody,
            const uint index, ObjTypeSpecFldInfo* info);

        void Reset();

        template <class Fn>
        void Map(Fn fn, uint count) const
        {
            if (this->infoArray != nullptr)
            {
                for (uint i = 0; i < count; i++)
                {
                    ObjTypeSpecFldInfo* info = this->infoArray[i];

                    if (info != nullptr)
                    {
                        fn(info);
                    }
                }
            }
        };

        PREVENT_COPY(ObjTypeSpecFldInfoArray)
    };
}
#endif // ENABLE_NATIVE_CODEGEN

