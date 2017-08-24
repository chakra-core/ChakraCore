//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"

namespace Js {
    // The VS2013 linker treats this as a redefinition of an already
    // defined constant and complains. So skip the declaration if we're compiling
    // with VS2013 or below.
#if !defined(_MSC_VER) || _MSC_VER >= 1900
    const uint TypePath::InitialTypePathSize;
#endif

    TypePath* TypePath::New(Recycler* recycler, uint size)
    {
        Assert(size <= MaxPathTypeHandlerLength);
        size = max(size, InitialTypePathSize);


        if (PHASE_OFF1(Js::TypePathDynamicSizePhase))
        {
            size = MaxPathTypeHandlerLength;
        }
        else
        {
            size = PowerOf2Policy::GetSize(size - TYPE_PATH_ALLOC_GRANULARITY_GAP);
            if (size < MaxPathTypeHandlerLength)
            {
                size += TYPE_PATH_ALLOC_GRANULARITY_GAP;
            }
        }

        Assert(size <= MaxPathTypeHandlerLength);

        TypePath * newTypePath = RecyclerNewPlusZ(recycler, sizeof(PropertyRecord *) * size, TypePath);
        // Allocate enough space for the "next" for the TinyDictionary;
        newTypePath->data = RecyclerNewPlusLeafZ(recycler, size, TypePath::Data, (uint8)size);

        return newTypePath;
    }

    PropertyIndex TypePath::Lookup(PropertyId propId,int typePathLength)
    {
        return LookupInline(propId,typePathLength);
    }

    PropertyIndex TypePath::LookupInline(PropertyId propId,int typePathLength)
    {
        if (propId == Constants::NoProperty)
        {
           return Constants::NoSlot;
        }

        PropertyIndex propIndex = Constants::NoSlot;
        if (this->GetData()->map.TryGetValue(propId, &propIndex, assignments) &&
            propIndex < typePathLength)
        {
            return propIndex;
        }

        return Constants::NoSlot;
    }

    TypePath * TypePath::Branch(Recycler * recycler, int pathLength, bool couldSeeProto)
    {
        AssertMsg(pathLength < this->GetPathLength(), "Why are we branching at the tip of the type path?");

        // Ensure there is at least one free entry in the new path, so we can extend it.
        // TypePath::New will take care of aligning this appropriately.
        TypePath * branchedPath = TypePath::New(recycler, pathLength + 1);

        for (PropertyIndex i = 0; i < pathLength; i++)
        {
            branchedPath->AddInternal(assignments[i]);

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
            if (couldSeeProto)
            {
                if (this->GetData()->usedFixedFields.Test(i))
                {
                    // We must conservatively copy all used as fixed bits if some prototype instance could also take
                    // this transition.  See comment in PathTypeHandlerBase::ConvertToSimpleDictionaryType.
                    // Yes, we could devise a more efficient way of copying bits 1 through pathLength, if performance of this
                    // code path proves important enough.
                    branchedPath->GetData()->usedFixedFields.Set(i);
                }
                else if (this->GetData()->fixedFields.Test(i))
                {
                    // We must clear any fixed fields that are not also used as fixed if some prototype instance could also take
                    // this transition.  See comment in PathTypeHandlerBase::ConvertToSimpleDictionaryType.
                    this->GetData()->fixedFields.Clear(i);
                }
            }
#endif

        }

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        // When branching, we must ensure that fixed field values on the prefix shared by the two branches are always
        // consistent.  Hence, we can't leave any of them uninitialized, because they could later get initialized to
        // different values, by two different instances (one on the old branch and one on the new branch).  If that happened
        // and the instance from the old branch later switched to the new branch, it would magically gain a different set
        // of fixed properties!
        if (this->GetMaxInitializedLength() < pathLength)
        {
            this->SetMaxInitializedLength(pathLength);
        }
        branchedPath->SetMaxInitializedLength(pathLength);
#endif

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        if (PHASE_VERBOSE_TRACE1(FixMethodPropsPhase))
        {
            Output::Print(_u("FixedFields: TypePath::Branch: singleton: 0x%p(0x%p)\n"), PointerValue(this->singletonInstance), this->singletonInstance->Get());
            Output::Print(_u("   fixed fields:"));

            for (PropertyIndex i = 0; i < GetPathLength(); i++)
            {
                Output::Print(_u(" %s %d%d%d,"), GetPropertyId(i)->GetBuffer(),
                    i < GetMaxInitializedLength() ? 1 : 0,
                    GetIsFixedFieldAt(i, GetPathLength()) ? 1 : 0,
                    GetIsUsedFixedFieldAt(i, GetPathLength()) ? 1 : 0);
            }

            Output::Print(_u("\n"));
        }
#endif

        return branchedPath;
    }

    TypePath * TypePath::Grow(Recycler * recycler)
    {
        uint currentPathLength = this->GetPathLength();
        AssertMsg(this->GetPathSize() == currentPathLength, "Why are we growing the type path?");

        // Ensure there is at least one free entry in the new path, so we can extend it.
        // TypePath::New will take care of aligning this appropriately.
        TypePath * clonedPath = TypePath::New(recycler, currentPathLength + 1);

        clonedPath->GetData()->pathLength = (uint8)currentPathLength;
        memcpy(&clonedPath->GetData()->map, &this->GetData()->map, sizeof(TinyDictionary) + currentPathLength);
        CopyArray(clonedPath->assignments, currentPathLength, this->assignments, currentPathLength);

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        // Copy fixed field info
        clonedPath->singletonInstance = this->singletonInstance;
        clonedPath->GetData()->maxInitializedLength = this->GetData()->maxInitializedLength;
        clonedPath->GetData()->fixedFields = this->GetData()->fixedFields;
        clonedPath->GetData()->usedFixedFields = this->GetData()->usedFixedFields;
#endif

        return clonedPath;
    }

#if ENABLE_FIXED_FIELDS
#if DBG
    bool TypePath::HasSingletonInstanceOnlyIfNeeded()
    {
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        return DynamicTypeHandler::AreSingletonInstancesNeeded() || this->singletonInstance == nullptr;
#else
        return true;
#endif
    }
#endif

    Var TypePath::GetSingletonFixedFieldAt(PropertyIndex index, int typePathLength, ScriptContext * requestContext)
    {
#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        Assert(index < this->GetPathLength());
        Assert(index < typePathLength);
        Assert(typePathLength <= this->GetPathLength());

        if (!CanHaveFixedFields(typePathLength))
        {
            return nullptr;
        }

        DynamicObject* localSingletonInstance = this->singletonInstance->Get();

        return localSingletonInstance != nullptr && localSingletonInstance->GetScriptContext() == requestContext && this->GetData()->fixedFields.Test(index) ? localSingletonInstance->GetSlot(index) : nullptr;
#else
        return nullptr;
#endif

    }
#endif

    int TypePath::Data::Add(const PropertyRecord* propId, Field(const PropertyRecord *)* assignments)
    {
        uint currentPathLength = this->pathLength;
        Assert(currentPathLength < this->pathSize);
        if (currentPathLength >= this->pathSize)
        {
            Throw::InternalError();
        }

#if DBG
        PropertyIndex temp;
        if (this->map.TryGetValue(propId->GetPropertyId(), &temp, assignments))
        {
            AssertMsg(false, "Adding a duplicate to the type path");
        }
#endif
        this->map.Add((unsigned int)propId->GetPropertyId(), (byte)currentPathLength);
        assignments[currentPathLength] = propId;
        this->pathLength++;
        return currentPathLength;
    }

    int TypePath::AddInternal(const PropertyRecord * propId)
    {
        int propertyIndex = this->GetData()->Add(propId, assignments);

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
        if (PHASE_VERBOSE_TRACE1(FixMethodPropsPhase))
        {
            Output::Print(_u("FixedFields: TypePath::AddInternal: singleton = 0x%p(0x%p)\n"),
                PointerValue(this->singletonInstance), this->singletonInstance != nullptr ? this->singletonInstance->Get() : nullptr);
            Output::Print(_u("   fixed fields:"));

            for (PropertyIndex i = 0; i < GetPathLength(); i++)
            {
                Output::Print(_u(" %s %d%d%d,"), GetPropertyId(i)->GetBuffer(),
                    i < GetMaxInitializedLength() ? 1 : 0,
                    GetIsFixedFieldAt(i, GetPathLength()) ? 1 : 0,
                    GetIsUsedFixedFieldAt(i, GetPathLength()) ? 1 : 0);
            }

            Output::Print(_u("\n"));
        }
#endif

        return propertyIndex;
    }

#ifdef SUPPORT_FIXED_FIELDS_ON_PATH_TYPES
    void TypePath::AddBlankFieldAt(PropertyIndex index, int typePathLength)
    {
        Assert(index >= this->GetMaxInitializedLength());
        this->SetMaxInitializedLength(index + 1);

        if (PHASE_VERBOSE_TRACE1(FixMethodPropsPhase))
        {
            Output::Print(_u("FixedFields: TypePath::AddBlankFieldAt: singleton = 0x%p(0x%p)\n"),
                PointerValue(this->singletonInstance), this->singletonInstance != nullptr ? this->singletonInstance->Get() : nullptr);
            Output::Print(_u("   fixed fields:"));

            for (PropertyIndex i = 0; i < GetPathLength(); i++)
            {
                Output::Print(_u(" %s %d%d%d,"), GetPropertyId(i)->GetBuffer(),
                    i < GetMaxInitializedLength() ? 1 : 0,
                    GetIsFixedFieldAt(i, GetPathLength()) ? 1 : 0,
                    GetIsUsedFixedFieldAt(i, GetPathLength()) ? 1 : 0);
            }

            Output::Print(_u("\n"));
        }
    }

    void TypePath::AddSingletonInstanceFieldAt(DynamicObject* instance, PropertyIndex index, bool isFixed, int typePathLength)
    {
        Assert(index < this->GetPathLength());
        Assert(typePathLength >= this->GetMaxInitializedLength());
        Assert(index >= this->GetMaxInitializedLength());
        // This invariant is predicated on the properties getting initialized in the order of indexes in the type handler.
        Assert(instance != nullptr);
        Assert(this->singletonInstance == nullptr || this->singletonInstance->Get() == instance);
        Assert(!this->GetData()->fixedFields.Test(index) && !this->GetData()->usedFixedFields.Test(index));

        if (this->singletonInstance == nullptr)
        {
            this->singletonInstance = instance->CreateWeakReferenceToSelf();
        }

        this->SetMaxInitializedLength(index + 1);

        if (isFixed)
        {
            this->GetData()->fixedFields.Set(index);
        }

        if (PHASE_VERBOSE_TRACE1(FixMethodPropsPhase))
        {
            Output::Print(_u("FixedFields: TypePath::AddSingletonInstanceFieldAt: singleton = 0x%p(0x%p)\n"),
                PointerValue(this->singletonInstance), this->singletonInstance != nullptr ? this->singletonInstance->Get() : nullptr);
            Output::Print(_u("   fixed fields:"));

            for (PropertyIndex i = 0; i < GetPathLength(); i++)
            {
                Output::Print(_u(" %s %d%d%d,"), GetPropertyId(i)->GetBuffer(),
                    i < GetMaxInitializedLength() ? 1 : 0,
                    GetIsFixedFieldAt(i, GetPathLength()) ? 1 : 0,
                    GetIsUsedFixedFieldAt(i, GetPathLength()) ? 1 : 0);
            }

            Output::Print(_u("\n"));
        }
    }

    void TypePath::AddSingletonInstanceFieldAt(PropertyIndex index, int typePathLength)
    {
        Assert(index < this->GetPathLength());
        Assert(typePathLength >= this->GetMaxInitializedLength());
        Assert(index >= this->GetMaxInitializedLength());
        Assert(!this->GetData()->fixedFields.Test(index) && !this->GetData()->usedFixedFields.Test(index));

        this->SetMaxInitializedLength(index + 1);

        if (PHASE_VERBOSE_TRACE1(FixMethodPropsPhase))
        {
            Output::Print(_u("FixedFields: TypePath::AddSingletonInstanceFieldAt: singleton = 0x%p(0x%p)\n"),
                PointerValue(this->singletonInstance), this->singletonInstance != nullptr ? this->singletonInstance->Get() : nullptr);
            Output::Print(_u("   fixed fields:"));

            for (PropertyIndex i = 0; i < GetPathLength(); i++)
            {
                Output::Print(_u(" %s %d%d%d,"), GetPropertyId(i)->GetBuffer(),
                    i < GetMaxInitializedLength() ? 1 : 0,
                    GetIsFixedFieldAt(i, GetPathLength()) ? 1 : 0,
                    GetIsUsedFixedFieldAt(i, GetPathLength()) ? 1 : 0);
            }

            Output::Print(_u("\n"));
        }
    }
#endif

}

