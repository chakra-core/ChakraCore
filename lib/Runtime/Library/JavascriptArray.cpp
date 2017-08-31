//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"
#include "Types/PathTypeHandler.h"
#include "Types/SpreadArgument.h"

// TODO: Change this generic fatal error to the descriptive one.
#define AssertAndFailFast(x) if (!(x)) { Assert(x); Js::Throw::FatalInternalError(); }

namespace Js
{
    // Make sure EmptySegment points to read-only memory.
    // Can't do this the easy way because SparseArraySegment has a constructor...
    static const char EmptySegmentData[sizeof(SparseArraySegmentBase)] = {0};
    const SparseArraySegmentBase *JavascriptArray::EmptySegment = (SparseArraySegmentBase *)&EmptySegmentData;

    // col0 : allocation bucket
    // col1 : No. of missing items to set during initialization depending on bucket.
    // col2 : allocation size for elements in given bucket.
    // col1 and col2 is calculated at runtime
    uint JavascriptNativeFloatArray::allocationBuckets[][AllocationBucketsInfoSize] =
    {
        { 3, 0, 0 },    // allocate space for 3 elements for array of length 0,1,2,3
        { 5, 0, 0 },    // allocate space for 5 elements for array of length 4,5
        { 8, 0, 0 },    // allocate space for 8 elements for array of length 6,7,8
    };
#if defined(_M_X64_OR_ARM64)
    const Var JavascriptArray::MissingItem = (Var)0x8000000280000002;
    uint JavascriptNativeIntArray::allocationBuckets[][AllocationBucketsInfoSize] =
    {
        // See comments above on how to read this
        {2, 0, 0},
        {6, 0, 0},
        {8, 0, 0},
    };
    uint JavascriptArray::allocationBuckets[][AllocationBucketsInfoSize] =
    {
        // See comments above on how to read this
        {4, 0, 0},
        {6, 0, 0},
        {8, 0, 0},
    };
#else
    const Var JavascriptArray::MissingItem = (Var)0x80000002;
    uint JavascriptNativeIntArray::allocationBuckets[][AllocationBucketsInfoSize] =
    {
        // See comments above on how to read this
        { 3, 0, 0 },
        { 7, 0, 0 },
        { 8, 0, 0 },
    };
    uint JavascriptArray::allocationBuckets[][AllocationBucketsInfoSize] =
    {
        // See comments above on how to read this
        { 4, 0, 0 },
        { 8, 0, 0 },
    };
#endif

    const int32 JavascriptNativeIntArray::MissingItem = 0x80000002;
    static const uint64 FloatMissingItemPattern = 0x8000000280000002ull;
    const double JavascriptNativeFloatArray::MissingItem = *(double*)&FloatMissingItemPattern;

    // Allocate enough space for 4 inline property slots and 16 inline element slots
    const size_t JavascriptArray::StackAllocationSize = DetermineAllocationSize<JavascriptArray, 4>(16);
    const size_t JavascriptNativeIntArray::StackAllocationSize = DetermineAllocationSize<JavascriptNativeIntArray, 4>(16);
    const size_t JavascriptNativeFloatArray::StackAllocationSize = DetermineAllocationSize<JavascriptNativeFloatArray, 4>(16);

    SegmentBTree::SegmentBTree()
        : segmentCount(0),
          segments(nullptr),
          keys(nullptr),
          children(nullptr)
    {
    }

    uint32 SegmentBTree::GetLazyCrossOverLimit()
    {
#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.DisableArrayBTree)
        {
            return Js::JavascriptArray::InvalidIndex;
        }
        else if (Js::Configuration::Global.flags.ForceArrayBTree)
        {
            return ARRAY_CROSSOVER_FOR_VALIDATE;
        }
#endif
#ifdef VALIDATE_ARRAY
        if (Js::Configuration::Global.flags.ArrayValidate)
        {
            return ARRAY_CROSSOVER_FOR_VALIDATE;
        }
#endif
        return SegmentBTree::MinDegree * 3;
    }

    BOOL SegmentBTree::IsLeaf() const
    {
        return children == NULL;
    }
    BOOL SegmentBTree::IsFullNode() const
    {
        return segmentCount == MaxKeys;
    }

    void SegmentBTree::InternalFind(SegmentBTree* node, uint32 itemIndex, SparseArraySegmentBase*& prev, SparseArraySegmentBase*& matchOrNext)
    {
        uint32 i = 0;

        for(; i < node->segmentCount; i++)
        {
            Assert(node->keys[i] == node->segments[i]->left);
            if (itemIndex <  node->keys[i])
            {
                break;
            }
        }

        // i indicates the 1st segment in the node past any matching segment.
        // the i'th child is the children to the 'left' of the i'th segment.

        // If itemIndex matches segment i-1 (note that left is always a match even when length == 0)
        bool matches = i > 0 && (itemIndex == node->keys[i-1] || itemIndex < node->keys[i-1] + node->segments[i-1]->length);

        if (matches)
        {
            // Find prev segment
            if (node->IsLeaf())
            {
                if (i > 1)
                {
                    // Previous is either sibling or set in a parent
                    prev = node->segments[i-2];
                }
            }
            else
            {
                // prev is the right most leaf in children[i-1] tree
                SegmentBTree* child = &node->children[i - 1];
                while (!child->IsLeaf())
                {
                    child = &child->children[child->segmentCount];
                }
                prev = child->segments[child->segmentCount - 1];
            }

            // Return the matching segment
            matchOrNext = node->segments[i-1];
        }
        else // itemIndex in between segment i-1 and i
        {
            if (i > 0)
            {
                // Store in previous in case a match or next is the first segment in a child.
                prev = node->segments[i-1];
            }

            if (node->IsLeaf())
            {
                matchOrNext = (i == 0 ? node->segments[0] : PointerValue(prev->next));
            }
            else
            {
                InternalFind(node->children + i, itemIndex, prev, matchOrNext);
            }
        }
    }

    void SegmentBTreeRoot::Find(uint32 itemIndex, SparseArraySegmentBase*& prev, SparseArraySegmentBase*& matchOrNext)
    {
        prev = matchOrNext = NULL;
        InternalFind(this, itemIndex, prev, matchOrNext);
        Assert(prev == NULL || (prev->next == matchOrNext));// If prev exists it is immediately before matchOrNext in the list of arraysegments
        Assert(prev == NULL || (prev->left < itemIndex && prev->left + prev->length <= itemIndex)); // prev should never be a match (left is a match if length == 0)
        Assert(matchOrNext == NULL || (matchOrNext->left >= itemIndex || matchOrNext->left + matchOrNext->length > itemIndex));
    }

    void SegmentBTreeRoot::Add(Recycler* recycler, SparseArraySegmentBase* newSeg)
    {

        if (IsFullNode())
        {
            SegmentBTree * children = AllocatorNewArrayZ(Recycler, recycler, SegmentBTree, MaxDegree);
            children[0] = *this;

            // Even though the segments point to a GC pointer, the main array should keep a references
            // as well.  So just make it a leaf allocation
            this->segmentCount = 0;
            this->segments = AllocatorNewArrayLeafZ(Recycler, recycler, SparseArraySegmentBase*, MaxKeys);
            this->keys = AllocatorNewArrayLeafZ(Recycler,recycler,uint32,MaxKeys);
            this->children = children;

            // This split is the only way the tree gets deeper
            SplitChild(recycler, this, 0, &children[0]);
        }
        InsertNonFullNode(recycler, this, newSeg);
    }

    void SegmentBTree::SwapSegment(uint32 originalKey, SparseArraySegmentBase* oldSeg, SparseArraySegmentBase* newSeg)
    {
        // Find old segment
        uint32 itemIndex = originalKey;
        uint32 i = 0;

        for(; i < segmentCount; i++)
        {
            Assert(keys[i] == segments[i]->left || (oldSeg == newSeg && newSeg == segments[i]));
            if (itemIndex <  keys[i])
            {
                break;
            }
        }

        // i is 1 past any match

        if (i > 0)
        {
            if (oldSeg == segments[i-1])
            {
                segments[i-1] = newSeg;
                keys[i-1] = newSeg->left;
                return;
            }
        }

        Assert(!IsLeaf());
        children[i].SwapSegment(originalKey, oldSeg, newSeg);
    }


    void SegmentBTree::SplitChild(Recycler* recycler, SegmentBTree* parent, uint32 iChild, SegmentBTree* child)
    {
        // Split child in two, move it's median key up to parent, and put the result of the split
        // on either side of the key moved up into parent

        Assert(child != NULL);
        Assert(parent != NULL);
        Assert(!parent->IsFullNode());
        Assert(child->IsFullNode());

        SegmentBTree newNode;
        newNode.segmentCount = MinKeys;

        // Even though the segments point to a GC pointer, the main array should keep a references
        // as well.  So just make it a leaf allocation
        newNode.segments = AllocatorNewArrayLeafZ(Recycler, recycler, SparseArraySegmentBase*, MaxKeys);
        newNode.keys = AllocatorNewArrayLeafZ(Recycler,recycler,uint32,MaxKeys);

        // Move the keys above the median into the new node
        for(uint32 i = 0; i < MinKeys; i++)
        {
            newNode.segments[i] = child->segments[i+MinDegree];
            newNode.keys[i] = child->keys[i+MinDegree];

            // Do not leave false positive references around in the b-tree
            child->segments[i+MinDegree] = nullptr;
        }

        // If children exist move those as well.
        if (!child->IsLeaf())
        {
            newNode.children = AllocatorNewArrayZ(Recycler, recycler, SegmentBTree, MaxDegree);
            for(uint32 j = 0; j < MinDegree; j++)
            {
                newNode.children[j] = child->children[j+MinDegree];

                // Do not leave false positive references around in the b-tree
                child->children[j+MinDegree].segments = nullptr;
                child->children[j+MinDegree].children = nullptr;
            }
        }
        child->segmentCount = MinKeys;

        // Make room for the new child in parent
        for(uint32 j = parent->segmentCount; j > iChild; j--)
        {
            parent->children[j+1] = parent->children[j];
        }
        // Copy the contents of the new node into the correct place in the parent's child array
        parent->children[iChild+1] = newNode;

        // Move the keys to make room for the median key
        for(uint32 k = parent->segmentCount; k > iChild; k--)
        {
            parent->segments[k] = parent->segments[k-1];
            parent->keys[k] = parent->keys[k-1];
        }

        // Move the median key into the proper place in the parent node
        parent->segments[iChild] = child->segments[MinKeys];
        parent->keys[iChild] = child->keys[MinKeys];

        // Do not leave false positive references around in the b-tree
        child->segments[MinKeys] = nullptr;

        parent->segmentCount++;
    }

    void SegmentBTree::InsertNonFullNode(Recycler* recycler, SegmentBTree* node, SparseArraySegmentBase* newSeg)
    {
        Assert(!node->IsFullNode());
        AnalysisAssert(node->segmentCount < MaxKeys);       // Same as !node->IsFullNode()
        Assert(newSeg != NULL);

        if (node->IsLeaf())
        {
            // Move the keys
            uint32 i = node->segmentCount - 1;
            while( (i != -1) && (newSeg->left < node->keys[i]))
            {
                node->segments[i+1] = node->segments[i];
                node->keys[i+1] = node->keys[i];
                i--;
            }
            if (!node->segments)
            {
                // Even though the segments point to a GC pointer, the main array should keep a references
                // as well.  So just make it a leaf allocation
                node->segments = AllocatorNewArrayLeafZ(Recycler, recycler, SparseArraySegmentBase*, MaxKeys);
                node->keys = AllocatorNewArrayLeafZ(Recycler, recycler, uint32, MaxKeys);
            }
            node->segments[i + 1] = newSeg;
            node->keys[i + 1] = newSeg->left;
            node->segmentCount++;
        }
        else
        {
            // find the correct child node
            uint32 i = node->segmentCount-1;

            while((i != -1) && (newSeg->left < node->keys[i]))
            {
                i--;
            }
            i++;

            // Make room if full
            if(node->children[i].IsFullNode())
            {
                // This split doesn't make the tree any deeper as node already has children.
                SplitChild(recycler, node, i, node->children+i);
                Assert(node->keys[i] == node->segments[i]->left);
                if (newSeg->left > node->keys[i])
                {
                    i++;
                }
            }
            InsertNonFullNode(recycler, node->children+i, newSeg);
        }
    }

    inline void ThrowTypeErrorOnFailureHelper::ThrowTypeErrorOnFailure(BOOL operationSucceeded)
    {
        if (IsThrowTypeError(operationSucceeded))
        {
            ThrowTypeErrorOnFailure();
        }
    }

    inline void ThrowTypeErrorOnFailureHelper::ThrowTypeErrorOnFailure()
    {
        JavascriptError::ThrowTypeError(m_scriptContext, VBSERR_ActionNotSupported, m_functionName);
    }

    inline BOOL ThrowTypeErrorOnFailureHelper::IsThrowTypeError(BOOL operationSucceeded)
    {
        return !operationSucceeded;
    }

    // Make sure EmptySegment points to read-only memory.
    // Can't do this the easy way because SparseArraySegment has a constructor...
    JavascriptArray::JavascriptArray(DynamicType * type)
        : ArrayObject(type, false, 0)
    {
        Assert(type->GetTypeId() == TypeIds_Array || type->GetTypeId() == TypeIds_NativeIntArray || type->GetTypeId() == TypeIds_NativeFloatArray || ((type->GetTypeId() == TypeIds_ES5Array || type->GetTypeId() == TypeIds_Object) && type->GetPrototype() == GetScriptContext()->GetLibrary()->GetArrayPrototype()));
        Assert(EmptySegment->length == 0 && EmptySegment->size == 0 && EmptySegment->next == NULL);
        InitArrayFlags(DynamicObjectFlags::InitialArrayValue);
        SetHeadAndLastUsedSegment(const_cast<SparseArraySegmentBase *>(EmptySegment));

    }

    JavascriptArray::JavascriptArray(uint32 length, DynamicType * type)
        : ArrayObject(type, false, length)
    {
        Assert(JavascriptArray::Is(type->GetTypeId()));
        Assert(EmptySegment->length == 0 && EmptySegment->size == 0 && EmptySegment->next == NULL);
        InitArrayFlags(DynamicObjectFlags::InitialArrayValue);
        SetHeadAndLastUsedSegment(const_cast<SparseArraySegmentBase *>(EmptySegment));
    }

    JavascriptArray::JavascriptArray(uint32 length, uint32 size, DynamicType * type)
        : ArrayObject(type, false, length)
    {
        Assert(type->GetTypeId() == TypeIds_Array);
        InitArrayFlags(DynamicObjectFlags::InitialArrayValue);
        Recycler* recycler = GetRecycler();
        SetHeadAndLastUsedSegment(SparseArraySegment<Var>::AllocateSegment(recycler, 0, 0, size, nullptr));
    }

    JavascriptArray::JavascriptArray(DynamicType * type, uint32 size)
        : ArrayObject(type, false)
    {
        InitArrayFlags(DynamicObjectFlags::InitialArrayValue);
        SetHeadAndLastUsedSegment(DetermineInlineHeadSegmentPointer<JavascriptArray, 0, false>(this));
        head->size = size;
        head->CheckLengthvsSize();
        Var fill = Js::JavascriptArray::MissingItem;
        for (uint i = 0; i < size; i++)
        {
            SparseArraySegment<Var>::From(head)->elements[i] = fill;
        }
    }

    JavascriptNativeIntArray::JavascriptNativeIntArray(uint32 length, uint32 size, DynamicType * type)
        : JavascriptNativeArray(type)
    {
        Assert(type->GetTypeId() == TypeIds_NativeIntArray);
        this->length = length;
        Recycler* recycler = GetRecycler();
        SetHeadAndLastUsedSegment(SparseArraySegment<int32>::AllocateSegment(recycler, 0, 0, size, nullptr));
    }

    JavascriptNativeIntArray::JavascriptNativeIntArray(DynamicType * type, uint32 size)
        : JavascriptNativeArray(type)
    {
        SetHeadAndLastUsedSegment(DetermineInlineHeadSegmentPointer<JavascriptNativeIntArray, 0, false>(this));
        head->size = size;
        head->CheckLengthvsSize();
        SparseArraySegment<int32>::From(head)->FillSegmentBuffer(0, size);
    }

    JavascriptNativeFloatArray::JavascriptNativeFloatArray(uint32 length, uint32 size, DynamicType * type)
        : JavascriptNativeArray(type)
    {
        Assert(type->GetTypeId() == TypeIds_NativeFloatArray);
        this->length = length;
        Recycler* recycler = GetRecycler();
        SetHeadAndLastUsedSegment(SparseArraySegment<double>::AllocateSegment(recycler, 0, 0, size, nullptr));
    }

    JavascriptNativeFloatArray::JavascriptNativeFloatArray(DynamicType * type, uint32 size)
        : JavascriptNativeArray(type)
    {
        SetHeadAndLastUsedSegment(DetermineInlineHeadSegmentPointer<JavascriptNativeFloatArray, 0, false>(this));
        head->size = size;
        head->CheckLengthvsSize();
        SparseArraySegment<double>::From(head)->FillSegmentBuffer(0, size);
    }

    bool JavascriptArray::Is(Var aValue)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(aValue);
        return JavascriptArray::Is(typeId);
    }

    bool JavascriptArray::Is(TypeId typeId)
    {
        return typeId >= TypeIds_ArrayFirst && typeId <= TypeIds_ArrayLast;
    }

    bool JavascriptArray::IsVarArray(Var aValue)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(aValue);
        return JavascriptArray::IsVarArray(typeId);
    }

    bool JavascriptArray::IsVarArray(TypeId typeId)
    {
        return typeId == TypeIds_Array;
    }

    template<typename T>
    bool JavascriptArray::IsMissingItemAt(uint32 index) const
    {
        SparseArraySegment<T>* headSeg = SparseArraySegment<T>::From(this->head);

        return SparseArraySegment<T>::IsMissingItem(&headSeg->elements[index]);
    }

    bool JavascriptArray::IsMissingItem(uint32 index)
    {
        if (this->length <= index)
        {
            return false;
        }

        bool isIntArray = false, isFloatArray = false;
        this->GetArrayTypeAndConvert(&isIntArray, &isFloatArray);

        if (isIntArray)
        {
            return IsMissingItemAt<int32>(index);
        }
        else if (isFloatArray)
        {
            return IsMissingItemAt<double>(index);
        }
        else
        {
            return IsMissingItemAt<Var>(index);
        }
    }

    JavascriptArray* JavascriptArray::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptArray'");

        return static_cast<JavascriptArray *>(RecyclableObject::FromVar(aValue));
    }

    // Get JavascriptArray* from a Var, which is either a JavascriptArray* or ESArray*.
    JavascriptArray* JavascriptArray::FromAnyArray(Var aValue)
    {
        AssertMsg(Is(aValue) || ES5Array::Is(aValue), "Ensure var is actually a 'JavascriptArray' or 'ES5Array'");

        return static_cast<JavascriptArray *>(RecyclableObject::FromVar(aValue));
    }

    // Check if a Var is a direct-accessible (fast path) JavascriptArray.
    bool JavascriptArray::IsDirectAccessArray(Var aValue)
    {
        return RecyclableObject::Is(aValue) &&
            (VirtualTableInfo<JavascriptArray>::HasVirtualTable(aValue) ||
                VirtualTableInfo<JavascriptNativeIntArray>::HasVirtualTable(aValue) ||
                VirtualTableInfo<JavascriptNativeFloatArray>::HasVirtualTable(aValue));
    }

    bool JavascriptArray::IsInlineSegment(SparseArraySegmentBase *seg, JavascriptArray *pArr)
    {
        if (seg == nullptr)
        {
            return false;
        }

        SparseArraySegmentBase* inlineHeadSegment = nullptr;
        if (JavascriptNativeArray::Is(pArr))
        {
            if (JavascriptNativeFloatArray::Is(pArr))
            {
                inlineHeadSegment = DetermineInlineHeadSegmentPointer<JavascriptNativeFloatArray, 0, true>((JavascriptNativeFloatArray*)pArr);
            }
            else
            {
                AssertOrFailFast(JavascriptNativeIntArray::Is(pArr));
                inlineHeadSegment = DetermineInlineHeadSegmentPointer<JavascriptNativeIntArray, 0, true>((JavascriptNativeIntArray*)pArr);
            }

            Assert(inlineHeadSegment);
            return (seg == inlineHeadSegment);
        }

        // This will result in false positives. It is used because DetermineInlineHeadSegmentPointer
        // does not handle Arrays that change type e.g. from JavascriptNativeIntArray to JavascriptArray
        // This conversion in particular is problematic because JavascriptNativeIntArray is larger than JavascriptArray
        // so the returned head segment ptr never equals pArr->head. So we will default to using this and deal with
        // false positives. It is better than always doing a hard copy.
        return pArr->head != nullptr && HasInlineHeadSegment(pArr->head->length);
    }

    DynamicObjectFlags JavascriptArray::GetFlags() const
    {
        return GetArrayFlags();
    }

    DynamicObjectFlags JavascriptArray::GetFlags_Unchecked() const // do not use except in extreme circumstances
    {
        return GetArrayFlags_Unchecked();
    }

    void JavascriptArray::SetFlags(const DynamicObjectFlags flags)
    {
        SetArrayFlags(flags);
    }

    DynamicType * JavascriptArray::GetInitialType(ScriptContext * scriptContext)
    {
        return scriptContext->GetLibrary()->GetArrayType();
    }

    JavascriptArray *JavascriptArray::GetArrayForArrayOrObjectWithArray(const Var var)
    {
        bool isObjectWithArray;
        TypeId arrayTypeId;
        return GetArrayForArrayOrObjectWithArray(var, &isObjectWithArray, &arrayTypeId);
    }

    JavascriptArray *JavascriptArray::GetArrayForArrayOrObjectWithArray(
        const Var var,
        bool *const isObjectWithArrayRef,
        TypeId *const arrayTypeIdRef)
    {
        // This is a helper function used by jitted code. The array checks done here match the array checks done by jitted code
        // (see Lowerer::GenerateArrayTest) to minimize bailouts.

        Assert(var);
        Assert(isObjectWithArrayRef);
        Assert(arrayTypeIdRef);

        *isObjectWithArrayRef = false;
        *arrayTypeIdRef = TypeIds_Undefined;

        if(!RecyclableObject::Is(var))
        {
            return nullptr;
        }

        JavascriptArray *array = nullptr;
        INT_PTR vtable = VirtualTableInfoBase::GetVirtualTable(var);
        if(vtable == VirtualTableInfo<DynamicObject>::Address)
        {
            ArrayObject* objectArray = DynamicObject::FromVar(var)->GetObjectArray();
            array = (objectArray && Is(objectArray)) ? FromVar(objectArray) : nullptr;
            if(!array)
            {
                return nullptr;
            }
            *isObjectWithArrayRef = true;
            vtable = VirtualTableInfoBase::GetVirtualTable(array);
        }

        if(vtable == VirtualTableInfo<JavascriptArray>::Address)
        {
            *arrayTypeIdRef = TypeIds_Array;
        }
        else if(vtable == VirtualTableInfo<JavascriptNativeIntArray>::Address)
        {
            *arrayTypeIdRef = TypeIds_NativeIntArray;
        }
        else if(vtable == VirtualTableInfo<JavascriptNativeFloatArray>::Address)
        {
            *arrayTypeIdRef = TypeIds_NativeFloatArray;
        }
        else
        {
            return nullptr;
        }

        if(!array)
        {
            array = FromVar(var);
        }
        return array;
    }

    const SparseArraySegmentBase *JavascriptArray::Jit_GetArrayHeadSegmentForArrayOrObjectWithArray(const Var var)
    {
        // This is a helper function used by jitted code

        JavascriptArray *const array = GetArrayForArrayOrObjectWithArray(var);
        return array ? array->head : nullptr;
    }

    uint32 JavascriptArray::Jit_GetArrayHeadSegmentLength(const SparseArraySegmentBase *const headSegment)
    {
        // This is a helper function used by jitted code

        return headSegment ? headSegment->length : 0;
    }

    bool JavascriptArray::Jit_OperationInvalidatedArrayHeadSegment(
        const SparseArraySegmentBase *const headSegmentBeforeOperation,
        const uint32 headSegmentLengthBeforeOperation,
        const Var varAfterOperation)
    {
        // This is a helper function used by jitted code

        Assert(varAfterOperation);

        if(!headSegmentBeforeOperation)
        {
            return false;
        }

        const SparseArraySegmentBase *const headSegmentAfterOperation =
            Jit_GetArrayHeadSegmentForArrayOrObjectWithArray(varAfterOperation);
        return
            headSegmentAfterOperation != headSegmentBeforeOperation ||
            headSegmentAfterOperation->length != headSegmentLengthBeforeOperation;
    }

    uint32 JavascriptArray::Jit_GetArrayLength(const Var var)
    {
        // This is a helper function used by jitted code

        bool isObjectWithArray;
        TypeId arrayTypeId;
        JavascriptArray *const array = GetArrayForArrayOrObjectWithArray(var, &isObjectWithArray, &arrayTypeId);
        return array && !isObjectWithArray ? array->GetLength() : 0;
    }

    bool JavascriptArray::Jit_OperationInvalidatedArrayLength(const uint32 lengthBeforeOperation, const Var varAfterOperation)
    {
        // This is a helper function used by jitted code

        return Jit_GetArrayLength(varAfterOperation) != lengthBeforeOperation;
    }

    DynamicObjectFlags JavascriptArray::Jit_GetArrayFlagsForArrayOrObjectWithArray(const Var var)
    {
        // This is a helper function used by jitted code

        JavascriptArray *const array = GetArrayForArrayOrObjectWithArray(var);
        return array && array->UsesObjectArrayOrFlagsAsFlags() ? array->GetFlags() : DynamicObjectFlags::None;
    }

    bool JavascriptArray::Jit_OperationCreatedFirstMissingValue(
        const DynamicObjectFlags flagsBeforeOperation,
        const Var varAfterOperation)
    {
        // This is a helper function used by jitted code

        Assert(varAfterOperation);

        return
            !!(flagsBeforeOperation & DynamicObjectFlags::HasNoMissingValues) &&
            !(Jit_GetArrayFlagsForArrayOrObjectWithArray(varAfterOperation) & DynamicObjectFlags::HasNoMissingValues);
    }

    bool JavascriptArray::HasNoMissingValues() const
    {
        return !!(GetFlags() & DynamicObjectFlags::HasNoMissingValues);
    }

    bool JavascriptArray::HasNoMissingValues_Unchecked() const // do not use except in extreme circumstances
    {
        return !!(GetFlags_Unchecked() & DynamicObjectFlags::HasNoMissingValues);
    }

    void JavascriptArray::SetHasNoMissingValues(const bool hasNoMissingValues)
    {
        SetFlags(
            hasNoMissingValues
                ? GetFlags() | DynamicObjectFlags::HasNoMissingValues
                : GetFlags() & ~DynamicObjectFlags::HasNoMissingValues);
    }

    template<class T>
    bool JavascriptArray::IsMissingHeadSegmentItemImpl(const uint32 index) const
    {
        Assert(index < head->length);

        return SparseArraySegment<T>::IsMissingItem(&SparseArraySegment<T>::From(head)->elements[index]);
    }

    bool JavascriptArray::IsMissingHeadSegmentItem(const uint32 index) const
    {
        return IsMissingHeadSegmentItemImpl<Var>(index);
    }

#if ENABLE_COPYONACCESS_ARRAY
    void JavascriptCopyOnAccessNativeIntArray::ConvertCopyOnAccessSegment()
    {
        Assert(this->GetScriptContext()->GetLibrary()->cacheForCopyOnAccessArraySegments->IsValidIndex(::Math::PointerCastToIntegral<uint32>(this->GetHead())));
        SparseArraySegment<int32> *seg = this->GetScriptContext()->GetLibrary()->cacheForCopyOnAccessArraySegments->GetSegmentByIndex(::Math::PointerCastToIntegral<byte>(this->GetHead()));
        SparseArraySegment<int32> *newSeg = SparseArraySegment<int32>::AllocateLiteralHeadSegment(this->GetRecycler(), seg->length);

#if ENABLE_DEBUG_CONFIG_OPTIONS
        if (Js::Configuration::Global.flags.TestTrace.IsEnabled(Js::CopyOnAccessArrayPhase))
        {
            Output::Print(_u("Convert copy-on-access array: index(%d) length(%d)\n"), this->GetHead(), seg->length);
            Output::Flush();
        }
#endif

        newSeg->CopySegment(this->GetRecycler(), newSeg, 0, seg, 0, seg->length);
        this->SetHeadAndLastUsedSegment(newSeg);

        VirtualTableInfo<JavascriptNativeIntArray>::SetVirtualTable(this);
        this->type = JavascriptNativeIntArray::GetInitialType(this->GetScriptContext());

        ArrayCallSiteInfo *arrayInfo = this->GetArrayCallSiteInfo();
        if (arrayInfo && !arrayInfo->isNotCopyOnAccessArray)
        {
            arrayInfo->isNotCopyOnAccessArray = 1;
        }
    }

    uint32 JavascriptCopyOnAccessNativeIntArray::GetNextIndex(uint32 index) const
    {
        if (this->length == 0 || (index != Js::JavascriptArray::InvalidIndex && index >= this->length))
        {
            return Js::JavascriptArray::InvalidIndex;
        }
        else if (index == Js::JavascriptArray::InvalidIndex)
        {
            return 0;
        }
        else
        {
            return index + 1;
        }
    }

    BOOL JavascriptCopyOnAccessNativeIntArray::DirectGetItemAt(uint32 index, int* outVal)
    {
        Assert(this->GetScriptContext()->GetLibrary()->cacheForCopyOnAccessArraySegments->IsValidIndex(::Math::PointerCastToIntegral<uint32>(this->GetHead())));
        SparseArraySegment<int32> *seg = this->GetScriptContext()->GetLibrary()->cacheForCopyOnAccessArraySegments->GetSegmentByIndex(::Math::PointerCastToIntegral<byte>(this->GetHead()));

        if (this->length == 0 || index == Js::JavascriptArray::InvalidIndex || index >= this->length)
        {
            return FALSE;
        }
        else
        {
            *outVal = seg->elements[index];
            return TRUE;
        }
    }
#endif

    bool JavascriptNativeIntArray::IsMissingHeadSegmentItem(const uint32 index) const
    {
        return IsMissingHeadSegmentItemImpl<int32>(index);
    }

    bool JavascriptNativeFloatArray::IsMissingHeadSegmentItem(const uint32 index) const
    {
        return IsMissingHeadSegmentItemImpl<double>(index);
    }

    void JavascriptArray::InternalFillFromPrototype(JavascriptArray *dstArray, uint32 dstIndex, JavascriptArray *srcArray, uint32 start, uint32 end, uint32 count)
    {
        RecyclableObject* prototype = srcArray->GetPrototype();
        while (start + count != end && JavascriptOperators::GetTypeId(prototype) != TypeIds_Null)
        {
            ForEachOwnMissingArrayIndexOfObject(srcArray, dstArray, prototype, start, end, dstIndex, [&](uint32 index, Var value) {
                uint32 n = dstIndex + (index - start);
                dstArray->SetItem(n, value, PropertyOperation_None);

                count++;
            });

            prototype = prototype->GetPrototype();
        }
    }

    /* static */
    bool JavascriptArray::HasInlineHeadSegment(uint32 length)
    {
        return length <= SparseArraySegmentBase::INLINE_CHUNK_SIZE;
    }

    Var JavascriptArray::OP_NewScArray(uint32 elementCount, ScriptContext* scriptContext)
    {
        // Called only to create array literals: size is known.
        return scriptContext->GetLibrary()->CreateArrayLiteral(elementCount);
    }

    Var JavascriptArray::OP_NewScArrayWithElements(uint32 elementCount, Var *elements, ScriptContext* scriptContext)
    {
        // Called only to create array literals: size is known.
        JavascriptArray *arr = scriptContext->GetLibrary()->CreateArrayLiteral(elementCount);

        SparseArraySegment<Var> *head = SparseArraySegment<Var>::From(arr->head);
        Assert(elementCount <= head->length);
        CopyArray(head->elements, head->length, elements, elementCount);

#ifdef VALIDATE_ARRAY
        arr->ValidateArray();
#endif

        return arr;
    }

    Var JavascriptArray::OP_NewScArrayWithMissingValues(uint32 elementCount, ScriptContext* scriptContext)
    {
        // Called only to create array literals: size is known.
        JavascriptArray *const array = static_cast<JavascriptArray *>(OP_NewScArray(elementCount, scriptContext));
        array->SetHasNoMissingValues(false);
        SparseArraySegment<Var> *head = SparseArraySegment<Var>::From(array->head);
        head->FillSegmentBuffer(0, elementCount);

        return array;
    }

#if ENABLE_PROFILE_INFO
    Var JavascriptArray::ProfiledNewScArray(uint32 elementCount, ScriptContext *scriptContext, ArrayCallSiteInfo *arrayInfo, RecyclerWeakReference<FunctionBody> *weakFuncRef)
    {
        if (arrayInfo->IsNativeIntArray())
        {
            JavascriptNativeIntArray *arr = scriptContext->GetLibrary()->CreateNativeIntArrayLiteral(elementCount);
            arr->SetArrayProfileInfo(weakFuncRef, arrayInfo);
            return arr;
        }

        if (arrayInfo->IsNativeFloatArray())
        {
            JavascriptNativeFloatArray *arr = scriptContext->GetLibrary()->CreateNativeFloatArrayLiteral(elementCount);
            arr->SetArrayProfileInfo(weakFuncRef, arrayInfo);
            return arr;
        }

        JavascriptArray *arr = scriptContext->GetLibrary()->CreateArrayLiteral(elementCount);
        return arr;
    }
#endif
    Var JavascriptArray::OP_NewScIntArray(AuxArray<int32> *ints, ScriptContext* scriptContext)
    {
        uint32 count = ints->count;
        JavascriptArray *arr = scriptContext->GetLibrary()->CreateArrayLiteral(count);
        SparseArraySegment<Var> *head = SparseArraySegment<Var>::From(arr->head);
        Assert(count > 0 && count == head->length);
        for (uint i = 0; i < count; i++)
        {
            head->elements[i] = JavascriptNumber::ToVar(ints->elements[i], scriptContext);
        }
        return arr;
    }

#if ENABLE_PROFILE_INFO
    Var JavascriptArray::ProfiledNewScIntArray(AuxArray<int32> *ints, ScriptContext* scriptContext, ArrayCallSiteInfo *arrayInfo, RecyclerWeakReference<FunctionBody> *weakFuncRef)
    {
        // Called only to create array literals: size is known.
        uint32 count = ints->count;

        if (arrayInfo->IsNativeIntArray())
        {
            JavascriptNativeIntArray *arr;

#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary *lib = scriptContext->GetLibrary();
            FunctionBody *functionBody = weakFuncRef->Get();

            if (JavascriptLibrary::IsCopyOnAccessArrayCallSite(lib, arrayInfo, count))
            {
                Assert(lib->cacheForCopyOnAccessArraySegments);
                arr = scriptContext->GetLibrary()->CreateCopyOnAccessNativeIntArrayLiteral(arrayInfo, functionBody, ints);
            }
            else
#endif
            {
                arr = scriptContext->GetLibrary()->CreateNativeIntArrayLiteral(count);
                SparseArraySegment<int32> *head = SparseArraySegment<int32>::From(arr->head);
                Assert(count > 0 && count == head->length);
                CopyArray(head->elements, head->length, ints->elements, count);
            }

            arr->SetArrayProfileInfo(weakFuncRef, arrayInfo);

            return arr;
        }

        if (arrayInfo->IsNativeFloatArray())
        {
            JavascriptNativeFloatArray *arr = scriptContext->GetLibrary()->CreateNativeFloatArrayLiteral(count);
            SparseArraySegment<double> *head = SparseArraySegment<double>::From(arr->head);
            Assert(count > 0 && count == head->length);
            for (uint i = 0; i < count; i++)
            {
                head->elements[i] = (double)ints->elements[i];
            }
            arr->SetArrayProfileInfo(weakFuncRef, arrayInfo);

            return arr;
        }

        return OP_NewScIntArray(ints, scriptContext);
    }
#endif

    Var JavascriptArray::OP_NewScFltArray(AuxArray<double> *doubles, ScriptContext* scriptContext)
    {
        uint32 count = doubles->count;
        JavascriptArray *arr = scriptContext->GetLibrary()->CreateArrayLiteral(count);
        SparseArraySegment<Var> *head = SparseArraySegment<Var>::From(arr->head);
        Assert(count > 0 && count == head->length);
        for (uint i = 0; i < count; i++)
        {
            double dval = doubles->elements[i];
            int32 ival;
            if (JavascriptNumber::TryGetInt32Value(dval, &ival) && !TaggedInt::IsOverflow(ival))
            {
                head->elements[i] = TaggedInt::ToVarUnchecked(ival);
            }
            else
            {
                head->elements[i] = JavascriptNumber::ToVarNoCheck(dval, scriptContext);
            }
        }
        return arr;
    }

#if ENABLE_PROFILE_INFO
    Var JavascriptArray::ProfiledNewScFltArray(AuxArray<double> *doubles, ScriptContext* scriptContext, ArrayCallSiteInfo *arrayInfo, RecyclerWeakReference<FunctionBody> *weakFuncRef)
    {
        // Called only to create array literals: size is known.
        if (arrayInfo->IsNativeFloatArray())
        {
            arrayInfo->SetIsNotNativeIntArray();
            uint32 count = doubles->count;
            JavascriptNativeFloatArray *arr = scriptContext->GetLibrary()->CreateNativeFloatArrayLiteral(count);
            SparseArraySegment<double> *head = SparseArraySegment<double>::From(arr->head);
            Assert(count > 0 && count == head->length);
            CopyArray(head->elements, head->length, doubles->elements, count);
            arr->SetArrayProfileInfo(weakFuncRef, arrayInfo);

            return arr;
        }

        return OP_NewScFltArray(doubles, scriptContext);
    }

    Var JavascriptArray::ProfiledNewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);

        Assert(JavascriptFunction::Is(function) &&
               JavascriptFunction::FromVar(function)->GetFunctionInfo() == &JavascriptArray::EntryInfo::NewInstance);
        Assert(callInfo.Count >= 2);

        ArrayCallSiteInfo *arrayInfo = (ArrayCallSiteInfo*)args[0];
        JavascriptArray* pNew = nullptr;

        if (callInfo.Count == 2)
        {
            // Exactly one argument, which is the array length if it's a uint32.
            Var firstArgument = args[1];
            int elementCount;
            if (TaggedInt::Is(firstArgument))
            {
                elementCount = TaggedInt::ToInt32(firstArgument);
                if (elementCount < 0)
                {
                    JavascriptError::ThrowRangeError(function->GetScriptContext(), JSERR_ArrayLengthConstructIncorrect);
                }
                if (arrayInfo && arrayInfo->IsNativeArray())
                {
                    if (arrayInfo->IsNativeIntArray())
                    {
                        pNew = function->GetLibrary()->CreateNativeIntArray(elementCount);
                    }
                    else
                    {
                        pNew = function->GetLibrary()->CreateNativeFloatArray(elementCount);
                    }
                }
                else
                {
                    pNew = function->GetLibrary()->CreateArray(elementCount);
                }
            }
            else if (JavascriptNumber::Is_NoTaggedIntCheck(firstArgument))
            {
                // Non-tagged-int number: make sure the double value is really a uint32.
                double value = JavascriptNumber::GetValue(firstArgument);
                uint32 uvalue = JavascriptConversion::ToUInt32(value);
                if (value != uvalue)
                {
                    JavascriptError::ThrowRangeError(function->GetScriptContext(), JSERR_ArrayLengthConstructIncorrect);
                }
                if (arrayInfo && arrayInfo->IsNativeArray())
                {
                    if (arrayInfo->IsNativeIntArray())
                    {
                        pNew = function->GetLibrary()->CreateNativeIntArray(uvalue);
                    }
                    else
                    {
                        pNew = function->GetLibrary()->CreateNativeFloatArray(uvalue);
                    }
                }
                else
                {
                    pNew = function->GetLibrary()->CreateArray(uvalue);
                }
            }
            else
            {
                //
                // First element is not int/double
                // create an array of length 1.
                // Set first element as the passed Var
                //

                pNew = function->GetLibrary()->CreateArray(1);
                pNew->DirectSetItemAt<Var>(0, firstArgument);
            }
        }
        else
        {
            // Called with a list of initial element values.
            // Create an array of the appropriate length and walk the list.

            if (arrayInfo && arrayInfo->IsNativeArray())
            {
                if (arrayInfo->IsNativeIntArray())
                {
                    pNew = function->GetLibrary()->CreateNativeIntArray(callInfo.Count - 1);
                }
                else
                {
                    pNew = function->GetLibrary()->CreateNativeFloatArray(callInfo.Count - 1);
                }
            }
            else
            {
                pNew = function->GetLibrary()->CreateArray(callInfo.Count - 1);
            }
            pNew->FillFromArgs(callInfo.Count - 1, 0, args.Values, arrayInfo);
        }

#ifdef VALIDATE_ARRAY
        pNew->ValidateArray();
#endif
        return pNew;
    }
#endif

    Var JavascriptArray::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);
        return NewInstance(function, args);
    }

    Var JavascriptArray::NewInstance(RecyclableObject* function, Arguments args)
    {
        // Call to new Array(), possibly under another name.
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        // SkipDefaultNewObject function flag should have prevented the default object
        // being created, except when call true a host dispatch.
        const CallInfo &callInfo = args.Info;
        Var newTarget = callInfo.Flags & CallFlags_NewTarget ? args.Values[args.Info.Count] : args[0];
        bool isCtorSuperCall = (callInfo.Flags & CallFlags_New) && newTarget != nullptr && !JavascriptOperators::IsUndefined(newTarget);
        Assert( isCtorSuperCall || !(callInfo.Flags & CallFlags_New) || args[0] == nullptr
            || JavascriptOperators::GetTypeId(args[0]) == TypeIds_HostDispatch);

        ScriptContext* scriptContext = function->GetScriptContext();
        JavascriptArray* pNew = nullptr;

        if (callInfo.Count < 2)
        {
            // No arguments passed to Array(), so create with the default size (0).
            pNew = CreateArrayFromConstructorNoArg(function, scriptContext);

            return isCtorSuperCall ?
                JavascriptOperators::OrdinaryCreateFromConstructor(RecyclableObject::FromVar(newTarget), pNew, nullptr, scriptContext) :
                pNew;
        }

        if (callInfo.Count == 2)
        {
            // Exactly one argument, which is the array length if it's a uint32.
            Var firstArgument = args[1];
            int elementCount;

            if (TaggedInt::Is(firstArgument))
            {
                elementCount = TaggedInt::ToInt32(firstArgument);
                if (elementCount < 0)
                {
                    JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthConstructIncorrect);
                }

                pNew = CreateArrayFromConstructor(function, elementCount, scriptContext);
            }
            else if (JavascriptNumber::Is_NoTaggedIntCheck(firstArgument))
            {
                // Non-tagged-int number: make sure the double value is really a uint32.
                double value = JavascriptNumber::GetValue(firstArgument);
                uint32 uvalue = JavascriptConversion::ToUInt32(value);
                if (value != uvalue)
                {
                    JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthConstructIncorrect);
                }

                pNew = CreateArrayFromConstructor(function, uvalue, scriptContext);
            }
            else
            {
                //
                // First element is not int/double
                // create an array of length 1.
                // Set first element as the passed Var
                //

                pNew = CreateArrayFromConstructor(function, 1, scriptContext);

                JavascriptOperators::SetItem(pNew, pNew, 0u, firstArgument, scriptContext, PropertyOperation_ThrowIfNotExtensible);

                // If we were passed an uninitialized JavascriptArray as the this argument,
                // we need to set the length. We must do this _after_ setting the first
                // element as the array may have side effects such as a setter for property
                // named '0' which would make the previous length of the array observable.
                // If we weren't passed a JavascriptArray as the this argument, this is no-op.
                pNew->SetLength(1);
            }
        }
        else
        {
            // Called with a list of initial element values.
            // Create an array of the appropriate length and walk the list.
            pNew = CreateArrayFromConstructor(function, callInfo.Count - 1, scriptContext);
            pNew->JavascriptArray::FillFromArgs(callInfo.Count - 1, 0, args.Values);
        }

#ifdef VALIDATE_ARRAY
        pNew->ValidateArray();
#endif
        return isCtorSuperCall ?
            JavascriptOperators::OrdinaryCreateFromConstructor(RecyclableObject::FromVar(newTarget), pNew, nullptr, scriptContext) :
            pNew;
    }

    JavascriptArray* JavascriptArray::CreateArrayFromConstructor(RecyclableObject* constructor, uint32 length, ScriptContext* scriptContext)
    {
        JavascriptLibrary* library = constructor->GetLibrary();

        // Create the Array object we'll return - this is the only way to create an object which is an exotic Array object.
        // Note: We need to use the library from the ScriptContext of the constructor, not the currently executing function.
        //       This is for the case where a built-in @@create method from a different JavascriptLibrary is installed on
        //       constructor.
        return library->CreateArray(length);
    }

    JavascriptArray* JavascriptArray::CreateArrayFromConstructorNoArg(RecyclableObject* constructor, ScriptContext* scriptContext)
    {
        JavascriptLibrary* library = constructor->GetLibrary();
        return library->CreateArray();
    }

#if ENABLE_PROFILE_INFO
    Var JavascriptArray::ProfiledNewInstanceNoArg(RecyclableObject *function, ScriptContext *scriptContext, ArrayCallSiteInfo *arrayInfo, RecyclerWeakReference<FunctionBody> *weakFuncRef)
    {
        Assert(JavascriptFunction::Is(function) &&
               JavascriptFunction::FromVar(function)->GetFunctionInfo() == &JavascriptArray::EntryInfo::NewInstance);

        if (arrayInfo->IsNativeIntArray())
        {
            JavascriptNativeIntArray *arr = scriptContext->GetLibrary()->CreateNativeIntArray();
            arr->SetArrayProfileInfo(weakFuncRef, arrayInfo);
            return arr;
        }

        if (arrayInfo->IsNativeFloatArray())
        {
            JavascriptNativeFloatArray *arr = scriptContext->GetLibrary()->CreateNativeFloatArray();
            arr->SetArrayProfileInfo(weakFuncRef, arrayInfo);
            return arr;
        }

        return scriptContext->GetLibrary()->CreateArray();
    }
#endif

    Var JavascriptNativeIntArray::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);
        return NewInstance(function, args);
    }

    Var JavascriptNativeIntArray::NewInstance(RecyclableObject* function, Arguments args)
    {
        Assert(!PHASE_OFF1(NativeArrayPhase));

        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        const CallInfo &callInfo = args.Info;
        if (callInfo.Count < 2)
        {
            // No arguments passed to Array(), so create with the default size (0).
            return function->GetLibrary()->CreateNativeIntArray();
        }

        JavascriptArray* pNew = nullptr;
        if (callInfo.Count == 2)
        {
            // Exactly one argument, which is the array length if it's a uint32.
            Var firstArgument = args[1];
            int elementCount;
            if (TaggedInt::Is(firstArgument))
            {
                elementCount = TaggedInt::ToInt32(firstArgument);
                if (elementCount < 0)
                {
                    JavascriptError::ThrowRangeError(
                        function->GetScriptContext(), JSERR_ArrayLengthConstructIncorrect);
                }
                pNew = function->GetLibrary()->CreateNativeIntArray(elementCount);
            }
            else if (JavascriptNumber::Is_NoTaggedIntCheck(firstArgument))
            {
                // Non-tagged-int number: make sure the double value is really a uint32.
                double value = JavascriptNumber::GetValue(firstArgument);
                uint32 uvalue = JavascriptConversion::ToUInt32(value);
                if (value != uvalue)
                {
                    JavascriptError::ThrowRangeError(
                        function->GetScriptContext(), JSERR_ArrayLengthConstructIncorrect);
                }
                pNew = function->GetLibrary()->CreateNativeIntArray(uvalue);
            }
            else
            {
                //
                // First element is not int/double
                // create an array of length 1.
                // Set first element as the passed Var
                //

                pNew = function->GetLibrary()->CreateArray(1);
                pNew->DirectSetItemAt<Var>(0, firstArgument);
            }
        }
        else
        {
            // Called with a list of initial element values.
            // Create an array of the appropriate length and walk the list.

            JavascriptNativeIntArray *arr = function->GetLibrary()->CreateNativeIntArray(callInfo.Count - 1);
            pNew = arr->FillFromArgs(callInfo.Count - 1, 0, args.Values);
        }

#ifdef VALIDATE_ARRAY
        pNew->ValidateArray();
#endif

        return pNew;
    }

    Var JavascriptNativeFloatArray::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);
        return NewInstance(function, args);
    }

    Var JavascriptNativeFloatArray::NewInstance(RecyclableObject* function, Arguments args)
    {
        Assert(!PHASE_OFF1(NativeArrayPhase));

        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        const CallInfo &callInfo = args.Info;
        if (callInfo.Count < 2)
        {
            // No arguments passed to Array(), so create with the default size (0).
            return function->GetLibrary()->CreateNativeFloatArray();
        }

        JavascriptArray* pNew = nullptr;
        if (callInfo.Count == 2)
        {
            // Exactly one argument, which is the array length if it's a uint32.
            Var firstArgument = args[1];
            int elementCount;
            if (TaggedInt::Is(firstArgument))
            {
                elementCount = TaggedInt::ToInt32(firstArgument);
                if (elementCount < 0)
                {
                    JavascriptError::ThrowRangeError(
                        function->GetScriptContext(), JSERR_ArrayLengthConstructIncorrect);
                }
                pNew = function->GetLibrary()->CreateNativeFloatArray(elementCount);
            }
            else if (JavascriptNumber::Is_NoTaggedIntCheck(firstArgument))
            {
                // Non-tagged-int number: make sure the double value is really a uint32.
                double value = JavascriptNumber::GetValue(firstArgument);
                uint32 uvalue = JavascriptConversion::ToUInt32(value);
                if (value != uvalue)
                {
                    JavascriptError::ThrowRangeError(
                        function->GetScriptContext(), JSERR_ArrayLengthConstructIncorrect);
                }
                pNew = function->GetLibrary()->CreateNativeFloatArray(uvalue);
            }
            else
            {
                //
                // First element is not int/double
                // create an array of length 1.
                // Set first element as the passed Var
                //

                pNew = function->GetLibrary()->CreateArray(1);
                pNew->DirectSetItemAt<Var>(0, firstArgument);
            }
        }
        else
        {
            // Called with a list of initial element values.
            // Create an array of the appropriate length and walk the list.

            JavascriptNativeFloatArray *arr = function->GetLibrary()->CreateNativeFloatArray(callInfo.Count - 1);
            pNew = arr->FillFromArgs(callInfo.Count - 1, 0, args.Values);
        }

#ifdef VALIDATE_ARRAY
        pNew->ValidateArray();
#endif

        return pNew;
    }


#if ENABLE_PROFILE_INFO
    JavascriptArray * JavascriptNativeIntArray::FillFromArgs(uint length, uint start, Var *args, ArrayCallSiteInfo *arrayInfo, bool dontCreateNewArray)
#else
    JavascriptArray * JavascriptNativeIntArray::FillFromArgs(uint length, uint start, Var *args, bool dontCreateNewArray)
#endif
    {
        uint i;
        for (i = start; i < length; i++)
        {
            Var item = args[i + 1];

            bool isTaggedInt = TaggedInt::Is(item);
            bool isTaggedIntMissingValue = false;
#ifdef _M_AMD64
            if (isTaggedInt)
            {
                int32 iValue = TaggedInt::ToInt32(item);
                isTaggedIntMissingValue = Js::SparseArraySegment<int32>::IsMissingItem(&iValue);
            }
#endif
            if (isTaggedInt && !isTaggedIntMissingValue)
            {
                // This is taggedInt case and we verified that item is not missing value in AMD64.
                this->DirectSetItemAt(i, TaggedInt::ToInt32(item));
            }
            else if (!isTaggedIntMissingValue && JavascriptNumber::Is_NoTaggedIntCheck(item))
            {
                double dvalue = JavascriptNumber::GetValue(item);
                int32 ivalue;
                if (JavascriptNumber::TryGetInt32Value(dvalue, &ivalue) && !Js::SparseArraySegment<int32>::IsMissingItem(&ivalue))
                {
                    this->DirectSetItemAt(i, ivalue);
                }
                else
                {
#if ENABLE_PROFILE_INFO
                    if (arrayInfo)
                    {
                        arrayInfo->SetIsNotNativeIntArray();
                    }
#endif

                    if (HasInlineHeadSegment(length) && i < this->head->length && !dontCreateNewArray)
                    {
                        // Avoid shrinking the number of elements in the head segment. We can still create a new
                        // array here, so go ahead.
                        JavascriptNativeFloatArray *fArr =
                            this->GetScriptContext()->GetLibrary()->CreateNativeFloatArrayLiteral(length);
                        return fArr->JavascriptNativeFloatArray::FillFromArgs(length, 0, args);
                    }

                    JavascriptNativeFloatArray *fArr = JavascriptNativeIntArray::ToNativeFloatArray(this);
                    fArr->DirectSetItemAt(i, dvalue);
#if ENABLE_PROFILE_INFO
                    return fArr->JavascriptNativeFloatArray::FillFromArgs(length, i + 1, args, arrayInfo, dontCreateNewArray);
#else
                    return fArr->JavascriptNativeFloatArray::FillFromArgs(length, i + 1, args, dontCreateNewArray);
#endif
                }
            }
            else
            {
#if ENABLE_PROFILE_INFO
                if (arrayInfo)
                {
                    arrayInfo->SetIsNotNativeArray();
                }
#endif

                #pragma prefast(suppress:6237, "The right hand side condition does not have any side effects.")
                if (sizeof(int32) < sizeof(Var) && HasInlineHeadSegment(length) && i < this->head->length && !dontCreateNewArray)
                {
                    // Avoid shrinking the number of elements in the head segment. We can still create a new
                    // array here, so go ahead.
                    JavascriptArray *arr = this->GetScriptContext()->GetLibrary()->CreateArrayLiteral(length);
                    return arr->JavascriptArray::FillFromArgs(length, 0, args);
                }

                JavascriptArray *arr = JavascriptNativeIntArray::ToVarArray(this);
#if ENABLE_PROFILE_INFO
                return arr->JavascriptArray::FillFromArgs(length, i, args, nullptr, dontCreateNewArray);
#else
                return arr->JavascriptArray::FillFromArgs(length, i, args, dontCreateNewArray);
#endif
            }
        }

        return this;
    }

#if ENABLE_PROFILE_INFO
    JavascriptArray * JavascriptNativeFloatArray::FillFromArgs(uint length, uint start, Var *args, ArrayCallSiteInfo *arrayInfo, bool dontCreateNewArray)
#else
    JavascriptArray * JavascriptNativeFloatArray::FillFromArgs(uint length, uint start, Var *args, bool dontCreateNewArray)
#endif
    {
        uint i;
        for (i = start; i < length; i++)
        {
            Var item = args[i + 1];
            if (TaggedInt::Is(item))
            {
                this->DirectSetItemAt(i, TaggedInt::ToDouble(item));
            }
            else if (JavascriptNumber::Is_NoTaggedIntCheck(item))
            {
                this->DirectSetItemAt(i, JavascriptNumber::GetValue(item));
            }
            else
            {
                JavascriptArray *arr = JavascriptNativeFloatArray::ToVarArray(this);
#if ENABLE_PROFILE_INFO
                if (arrayInfo)
                {
                    arrayInfo->SetIsNotNativeArray();
                }
                return arr->JavascriptArray::FillFromArgs(length, i, args, nullptr, dontCreateNewArray);
#else
                return arr->JavascriptArray::FillFromArgs(length, i, args, dontCreateNewArray);
#endif
            }
        }

        return this;
    }

#if ENABLE_PROFILE_INFO
    JavascriptArray * JavascriptArray::FillFromArgs(uint length, uint start, Var *args, ArrayCallSiteInfo *arrayInfo, bool dontCreateNewArray)
#else
    JavascriptArray * JavascriptArray::FillFromArgs(uint length, uint start, Var *args, bool dontCreateNewArray)
#endif
    {
        uint32 i;
        for (i = start; i < length; i++)
        {
            Var item = args[i + 1];
            this->DirectSetItemAt(i, item);
        }

        return this;
    }

    DynamicType * JavascriptNativeIntArray::GetInitialType(ScriptContext * scriptContext)
    {
        return scriptContext->GetLibrary()->GetNativeIntArrayType();
    }

#if ENABLE_COPYONACCESS_ARRAY
    DynamicType * JavascriptCopyOnAccessNativeIntArray::GetInitialType(ScriptContext * scriptContext)
    {
        return scriptContext->GetLibrary()->GetCopyOnAccessNativeIntArrayType();
    }
#endif

    JavascriptNativeFloatArray *JavascriptNativeIntArray::ToNativeFloatArray(JavascriptNativeIntArray *intArray)
    {
#if ENABLE_PROFILE_INFO
        ArrayCallSiteInfo *arrayInfo = intArray->GetArrayCallSiteInfo();
        if (arrayInfo)
        {
#if DBG
            Js::JavascriptStackWalker walker(intArray->GetScriptContext());
            Js::JavascriptFunction* caller = nullptr;
            bool foundScriptCaller = false;
            while(walker.GetCaller(&caller))
            {
                if(caller != nullptr && Js::ScriptFunction::Test(caller))
                {
                    foundScriptCaller = true;
                    break;
                }
            }

            if(foundScriptCaller)
            {
                Assert(caller);
                Assert(caller->GetFunctionBody());
                if(PHASE_TRACE(Js::NativeArrayConversionPhase, caller->GetFunctionBody()))
                {
                    Output::Print(_u("Conversion: Int array to Float array    ArrayCreationFunctionNumber:%2d    CallSiteNumber:%2d \n"), arrayInfo->functionNumber, arrayInfo->callSiteNumber);
                    Output::Flush();
                }
            }
            else
            {
                if(PHASE_TRACE1(Js::NativeArrayConversionPhase))
                {
                    Output::Print(_u("Conversion: Int array to Float array across ScriptContexts"));
                    Output::Flush();
                }
            }
#else
            if(PHASE_TRACE1(Js::NativeArrayConversionPhase))
            {
                Output::Print(_u("Conversion: Int array to Float array"));
                Output::Flush();
            }
#endif

            arrayInfo->SetIsNotNativeIntArray();
        }
#endif

        // Code below has potential to throw due to OOM or SO. Just FailFast on those cases
        ScriptContext *scriptContext = intArray->GetScriptContext();
        AutoDisableInterrupt failFastError(scriptContext->GetThreadContext());

        // Grow the segments
        Recycler *recycler = scriptContext->GetRecycler();
        SparseArraySegmentBase *seg, *nextSeg, *prevSeg = nullptr;
        for (seg = intArray->head; seg; seg = nextSeg)
        {
            nextSeg = seg->next;
            uint32 size = seg->size;
            if (size == 0)
            {
                continue;
            }

            uint32 left = seg->left;
            uint32 length = seg->length;
            int i;
            int32 ival;

            // The old segment will have size/2 and length capped by the new size.
            uint32 newSegSize = seg->size >> 1;
            if (seg == intArray->head || seg->length > (newSegSize >> 1))
            {
                // Some live elements are being pushed out of this segment, so allocate a new one.
                SparseArraySegment<double> *newSeg =
                    SparseArraySegment<double>::AllocateSegment(recycler, left, length, nextSeg);
                Assert(newSeg != nullptr);
                Assert((prevSeg == nullptr) == (seg == intArray->head));
                newSeg->next = nextSeg;
                intArray->LinkSegments((SparseArraySegment<double>*)prevSeg, newSeg);
                if (intArray->GetLastUsedSegment() == seg)
                {
                    intArray->SetLastUsedSegment(newSeg);
                }
                prevSeg = newSeg;
                SegmentBTree * segmentMap = intArray->GetSegmentMap();
                if (segmentMap)
                {
                    segmentMap->SwapSegment(left, seg, newSeg);
                }

                // Fill the new segment with the overflow.
                for (i = 0; (uint)i < newSeg->length; i++)
                {
                    ival = ((SparseArraySegment<int32>*)seg)->elements[i /*+ seg->length*/];
                    if (ival == JavascriptNativeIntArray::MissingItem)
                    {
                        continue;
                    }
                    newSeg->elements[i] = (double)ival;
                }
            }
            else
            {
                seg->size = newSegSize >> 1;
                seg->CheckLengthvsSize();

                // Now convert the contents that will remain in the old segment.
                for (i = seg->length - 1; i >= 0; i--)
                {
                    ival = ((SparseArraySegment<int32>*)seg)->elements[i];
                    if (ival == JavascriptNativeIntArray::MissingItem)
                    {
                        ((SparseArraySegment<double>*)seg)->elements[i] = (double)JavascriptNativeFloatArray::MissingItem;
                    }
                    else
                    {
                        ((SparseArraySegment<double>*)seg)->elements[i] = (double)ival;
                    }
                }
                prevSeg = seg;
            }
        }

        if (intArray->GetType() == scriptContext->GetLibrary()->GetNativeIntArrayType())
        {
            intArray->type = scriptContext->GetLibrary()->GetNativeFloatArrayType();
        }
        else
        {
            if (intArray->GetDynamicType()->GetIsLocked())
            {
                DynamicTypeHandler *typeHandler = intArray->GetDynamicType()->GetTypeHandler();
                if (typeHandler->IsPathTypeHandler())
                {
                    // We can't allow a type with the new type ID to be promoted to the old type.
                    // So go to a dictionary type handler, which will orphan the new type.
                    // This should be a corner case, so the inability to share the new type is unlikely to matter.
                    // If it does matter, try building a path from the new type's built-in root.
                    static_cast<PathTypeHandlerBase*>(typeHandler)->ResetTypeHandler(intArray);
                }
                else
                {
                    intArray->ChangeType();
                }
            }
            intArray->GetType()->SetTypeId(TypeIds_NativeFloatArray);
        }

        if (CrossSite::IsCrossSiteObjectTyped(intArray))
        {
            Assert(VirtualTableInfo<CrossSiteObject<JavascriptNativeIntArray>>::HasVirtualTable(intArray));
            VirtualTableInfo<CrossSiteObject<JavascriptNativeFloatArray>>::SetVirtualTable(intArray);
        }
        else
        {
            Assert(VirtualTableInfo<JavascriptNativeIntArray>::HasVirtualTable(intArray));
            VirtualTableInfo<JavascriptNativeFloatArray>::SetVirtualTable(intArray);
        }

        failFastError.Completed();
        return (JavascriptNativeFloatArray*)intArray;
    }

    /*
    *   JavascriptArray::ChangeArrayTypeToNativeArray<double>
    *   -   Converts the Var Array's type to NativeFloat.
    *   -   Sets the VirtualTable to "JavascriptNativeFloatArray"
    */
    template<>
    void JavascriptArray::ChangeArrayTypeToNativeArray<double>(JavascriptArray * varArray, ScriptContext * scriptContext)
    {
        AssertMsg(!JavascriptNativeArray::Is(varArray), "Ensure that the incoming Array is a Var array");

        if (varArray->GetType() == scriptContext->GetLibrary()->GetArrayType())
        {
            varArray->type = scriptContext->GetLibrary()->GetNativeFloatArrayType();
        }
        else
        {
            if (varArray->GetDynamicType()->GetIsLocked())
            {
                DynamicTypeHandler *typeHandler = varArray->GetDynamicType()->GetTypeHandler();
                if (typeHandler->IsPathTypeHandler())
                {
                    // We can't allow a type with the new type ID to be promoted to the old type.
                    // So go to a dictionary type handler, which will orphan the new type.
                    // This should be a corner case, so the inability to share the new type is unlikely to matter.
                    // If it does matter, try building a path from the new type's built-in root.
                    static_cast<PathTypeHandlerBase*>(typeHandler)->ResetTypeHandler(varArray);
                }
                else
                {
                    varArray->ChangeType();
                }
            }
            varArray->GetType()->SetTypeId(TypeIds_NativeFloatArray);
        }

        if (CrossSite::IsCrossSiteObjectTyped(varArray))
        {
            Assert(VirtualTableInfo<CrossSiteObject<JavascriptArray>>::HasVirtualTable(varArray));
            VirtualTableInfo<CrossSiteObject<JavascriptNativeFloatArray>>::SetVirtualTable(varArray);
        }
        else
        {
            Assert(VirtualTableInfo<JavascriptArray>::HasVirtualTable(varArray));
            VirtualTableInfo<JavascriptNativeFloatArray>::SetVirtualTable(varArray);
        }
    }

    /*
    *   JavascriptArray::ChangeArrayTypeToNativeArray<int32>
    *   -   Converts the Var Array's type to NativeInt.
    *   -   Sets the VirtualTable to "JavascriptNativeIntArray"
    */
    template<>
    void JavascriptArray::ChangeArrayTypeToNativeArray<int32>(JavascriptArray * varArray, ScriptContext * scriptContext)
    {
        AssertMsg(!JavascriptNativeArray::Is(varArray), "Ensure that the incoming Array is a Var array");

        if (varArray->GetType() == scriptContext->GetLibrary()->GetArrayType())
        {
            varArray->type = scriptContext->GetLibrary()->GetNativeIntArrayType();
        }
        else
        {
            if (varArray->GetDynamicType()->GetIsLocked())
            {
                DynamicTypeHandler *typeHandler = varArray->GetDynamicType()->GetTypeHandler();
                if (typeHandler->IsPathTypeHandler())
                {
                    // We can't allow a type with the new type ID to be promoted to the old type.
                    // So go to a dictionary type handler, which will orphan the new type.
                    // This should be a corner case, so the inability to share the new type is unlikely to matter.
                    // If it does matter, try building a path from the new type's built-in root.
                    static_cast<PathTypeHandlerBase*>(typeHandler)->ResetTypeHandler(varArray);
                }
                else
                {
                    varArray->ChangeType();
                }
            }
            varArray->GetType()->SetTypeId(TypeIds_NativeIntArray);
        }

        if (CrossSite::IsCrossSiteObjectTyped(varArray))
        {
            Assert(VirtualTableInfo<CrossSiteObject<JavascriptArray>>::HasVirtualTable(varArray));
            VirtualTableInfo<CrossSiteObject<JavascriptNativeIntArray>>::SetVirtualTable(varArray);
        }
        else
        {
            Assert(VirtualTableInfo<JavascriptArray>::HasVirtualTable(varArray));
            VirtualTableInfo<JavascriptNativeIntArray>::SetVirtualTable(varArray);
        }
    }

    template<>
    int32 JavascriptArray::GetNativeValue<int32>(Js::Var ival, ScriptContext * scriptContext)
    {
        return JavascriptConversion::ToInt32(ival, scriptContext);
    }

    template <>
    double JavascriptArray::GetNativeValue<double>(Var ival, ScriptContext * scriptContext)
    {
        return JavascriptConversion::ToNumber(ival, scriptContext);
    }


    /*
    *   JavascriptArray::ConvertToNativeArrayInPlace
    *   In place conversion of all Var elements to Native Int/Double elements in an array.
    *   We do not update the DynamicProfileInfo of the array here.
    */
    template<typename NativeArrayType, typename T>
    NativeArrayType *JavascriptArray::ConvertToNativeArrayInPlace(JavascriptArray *varArray)
    {
        AssertMsg(!JavascriptNativeArray::Is(varArray), "Ensure that the incoming Array is a Var array");

        ScriptContext *scriptContext = varArray->GetScriptContext();
        SparseArraySegmentBase *seg, *nextSeg, *prevSeg = nullptr;
        for (seg = varArray->head; seg; seg = nextSeg)
        {
            nextSeg = seg->next;
            uint32 size = seg->size;
            if (size == 0)
            {
                continue;
            }

            int i;
            Var ival;

            uint32 growFactor = sizeof(Var) / sizeof(T);
            AssertMsg(growFactor == 1, "We support only in place conversion of Var array to Native Array");

            // Now convert the contents that will remain in the old segment.
            for (i = seg->length - 1; i >= 0; i--)
            {
                ival = ((SparseArraySegment<Var>*)seg)->elements[i];
                if (ival == JavascriptArray::MissingItem)
                {
                    ((SparseArraySegment<T>*)seg)->elements[i] = NativeArrayType::MissingItem;
                }
                else
                {
                    ((SparseArraySegment<T>*)seg)->elements[i] = GetNativeValue<T>(ival, scriptContext);
                }
            }
            prevSeg = seg;
        }

        // Update the type of the Array
        ChangeArrayTypeToNativeArray<T>(varArray, scriptContext);

        return (NativeArrayType*)varArray;
    }

    JavascriptArray *JavascriptNativeIntArray::ConvertToVarArray(JavascriptNativeIntArray *intArray)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(intArray);
#endif
        ScriptContext *scriptContext = intArray->GetScriptContext();
        Recycler *recycler = scriptContext->GetRecycler();
        SparseArraySegmentBase *seg, *nextSeg, *prevSeg = nullptr;

        // Code below has potential to throw due to OOM or SO. Just FailFast on those cases
        AutoDisableInterrupt failFastError(scriptContext->GetThreadContext());

        for (seg = intArray->head; seg; seg = nextSeg)
        {
            nextSeg = seg->next;
            uint32 size = seg->size;
            if (size == 0)
            {
                continue;
            }

            uint32 left = seg->left;
            uint32 length = seg->length;
            int i;
            int32 ival;

            // Shrink?
            uint32 growFactor = sizeof(Var) / sizeof(int32);
            if ((growFactor != 1 && (seg == intArray->head || seg->length > (seg->size / growFactor))) ||
                (seg->next == nullptr && SparseArraySegmentBase::IsLeafSegment(seg, recycler)))
            {
                // Some live elements are being pushed out of this segment, so allocate a new one.
                // And/or the old segment is not scanned by the recycler, so we need a new one to hold vars.
                SparseArraySegment<Var> *newSeg =
                    SparseArraySegment<Var>::AllocateSegment(recycler, left, length, nextSeg);

                AnalysisAssert(newSeg);
                Assert((prevSeg == nullptr) == (seg == intArray->head));
                newSeg->next = nextSeg;
                intArray->LinkSegments((SparseArraySegment<Var>*)prevSeg, newSeg);
                if (intArray->GetLastUsedSegment() == seg)
                {
                    intArray->SetLastUsedSegment(newSeg);
                }
                prevSeg = newSeg;

                SegmentBTree * segmentMap = intArray->GetSegmentMap();
                if (segmentMap)
                {
                    segmentMap->SwapSegment(left, seg, newSeg);
                }

                // Fill the new segment with the overflow.
                for (i = 0; (uint)i < newSeg->length; i++)
                {
                    ival = ((SparseArraySegment<int32>*)seg)->elements[i];
                    if (ival == JavascriptNativeIntArray::MissingItem)
                    {
                        continue;
                    }
                    newSeg->elements[i] = JavascriptNumber::ToVar(ival, scriptContext);
                }
            }
            else
            {
                seg->size = seg->size / growFactor;
                seg->CheckLengthvsSize();

                // Now convert the contents that will remain in the old segment.
                // Walk backward in case we're growing the element size.
                for (i = seg->length - 1; i >= 0; i--)
                {
                    ival = ((SparseArraySegment<int32>*)seg)->elements[i];
                    if (ival == JavascriptNativeIntArray::MissingItem)
                    {
                        ((SparseArraySegment<Var>*)seg)->elements[i] = (Var)JavascriptArray::MissingItem;
                    }
                    else
                    {
                        ((SparseArraySegment<Var>*)seg)->elements[i] = JavascriptNumber::ToVar(ival, scriptContext);
                    }
                }
                prevSeg = seg;
            }
        }

        if (intArray->GetType() == scriptContext->GetLibrary()->GetNativeIntArrayType())
        {
            intArray->type = scriptContext->GetLibrary()->GetArrayType();
        }
        else
        {
            if (intArray->GetDynamicType()->GetIsLocked())
            {
                DynamicTypeHandler *typeHandler = intArray->GetDynamicType()->GetTypeHandler();
                if (typeHandler->IsPathTypeHandler())
                {
                    // We can't allow a type with the new type ID to be promoted to the old type.
                    // So go to a dictionary type handler, which will orphan the new type.
                    // This should be a corner case, so the inability to share the new type is unlikely to matter.
                    // If it does matter, try building a path from the new type's built-in root.
                    static_cast<PathTypeHandlerBase*>(typeHandler)->ResetTypeHandler(intArray);
                }
                else
                {
                    intArray->ChangeType();
                }
            }
            intArray->GetType()->SetTypeId(TypeIds_Array);
        }

        if (CrossSite::IsCrossSiteObjectTyped(intArray))
        {
            Assert(VirtualTableInfo<CrossSiteObject<JavascriptNativeIntArray>>::HasVirtualTable(intArray));
            VirtualTableInfo<CrossSiteObject<JavascriptArray>>::SetVirtualTable(intArray);
        }
        else
        {
            Assert(VirtualTableInfo<JavascriptNativeIntArray>::HasVirtualTable(intArray));
            VirtualTableInfo<JavascriptArray>::SetVirtualTable(intArray);
        }

        failFastError.Completed();
        return intArray;
    }
    JavascriptArray *JavascriptNativeIntArray::ToVarArray(JavascriptNativeIntArray *intArray)
    {
#if ENABLE_PROFILE_INFO
        ArrayCallSiteInfo *arrayInfo = intArray->GetArrayCallSiteInfo();
        if (arrayInfo)
        {
#if DBG
            Js::JavascriptStackWalker walker(intArray->GetScriptContext());
            Js::JavascriptFunction* caller = nullptr;
            bool foundScriptCaller = false;
            while(walker.GetCaller(&caller))
            {
                if(caller != nullptr && Js::ScriptFunction::Test(caller))
                {
                    foundScriptCaller = true;
                    break;
                }
            }

            if(foundScriptCaller)
            {
                Assert(caller);
                Assert(caller->GetFunctionBody());
                if(PHASE_TRACE(Js::NativeArrayConversionPhase, caller->GetFunctionBody()))
                {
                    Output::Print(_u("Conversion: Int array to Var array    ArrayCreationFunctionNumber:%2d    CallSiteNumber:%2d \n"), arrayInfo->functionNumber, arrayInfo->callSiteNumber);
                    Output::Flush();
                }
            }
            else
            {
                if(PHASE_TRACE1(Js::NativeArrayConversionPhase))
                {
                    Output::Print(_u("Conversion: Int array to Var array across ScriptContexts"));
                    Output::Flush();
                }
            }
#else
            if(PHASE_TRACE1(Js::NativeArrayConversionPhase))
            {
                Output::Print(_u("Conversion: Int array to Var array"));
                Output::Flush();
            }
#endif

            arrayInfo->SetIsNotNativeArray();
        }
#endif

        intArray->ClearArrayCallSiteIndex();

        return ConvertToVarArray(intArray);
    }

    DynamicType * JavascriptNativeFloatArray::GetInitialType(ScriptContext * scriptContext)
    {
        return scriptContext->GetLibrary()->GetNativeFloatArrayType();
    }

    /*
    *   JavascriptNativeFloatArray::ConvertToVarArray
    *   This function only converts all Float elements to Var elements in an array.
    *   DynamicProfileInfo of the array is not updated in this function.
    */
    JavascriptArray *JavascriptNativeFloatArray::ConvertToVarArray(JavascriptNativeFloatArray *fArray)
    {
        // We can't be growing the size of the element.
        Assert(sizeof(double) >= sizeof(Var));

        uint32 shrinkFactor = sizeof(double) / sizeof(Var);
        ScriptContext *scriptContext = fArray->GetScriptContext();
        Recycler *recycler = scriptContext->GetRecycler();
        SparseArraySegmentBase *seg, *nextSeg, *prevSeg = nullptr;

        // Code below has potential to throw due to OOM or SO. Just FailFast on those cases
        AutoDisableInterrupt failFastError(scriptContext->GetThreadContext());

        for (seg = fArray->head; seg; seg = nextSeg)
        {
            nextSeg = seg->next;
            if (seg->size == 0)
            {
                continue;
            }
            uint32 left = seg->left;
            uint32 length = seg->length;
            SparseArraySegment<Var> *newSeg;
            if (seg->next == nullptr && SparseArraySegmentBase::IsLeafSegment(seg, recycler))
            {
                // The old segment is not scanned by the recycler, so we need a new one to hold vars.
                newSeg =
                    SparseArraySegment<Var>::AllocateSegment(recycler, left, length, nextSeg);
                Assert((prevSeg == nullptr) == (seg == fArray->head));
                newSeg->next = nextSeg;
                fArray->LinkSegments((SparseArraySegment<Var>*)prevSeg, newSeg);
                if (fArray->GetLastUsedSegment() == seg)
                {
                    fArray->SetLastUsedSegment(newSeg);
                }
                prevSeg = newSeg;

                SegmentBTree * segmentMap = fArray->GetSegmentMap();
                if (segmentMap)
                {
                    segmentMap->SwapSegment(left, seg, newSeg);
                }
            }
            else
            {
                newSeg = (SparseArraySegment<Var>*)seg;
                prevSeg = seg;
                if (shrinkFactor != 1)
                {
                    uint32 newSize = seg->size * shrinkFactor;
                    uint32 limit;
                    if (seg->next)
                    {
                        limit = seg->next->left;
                    }
                    else
                    {
                        limit = JavascriptArray::MaxArrayLength;
                    }
                    seg->size = min(newSize, limit - seg->left);
                    seg->CheckLengthvsSize();
                }
            }
            uint32 i;
            for (i = 0; i < seg->length; i++)
            {
                if (SparseArraySegment<double>::IsMissingItem(&((SparseArraySegment<double>*)seg)->elements[i]))
                {
                    if (seg == newSeg)
                    {
                        newSeg->elements[i] = (Var)JavascriptArray::MissingItem;
                    }
                    Assert(newSeg->elements[i] == (Var)JavascriptArray::MissingItem);
                }
                else if (*(uint64*)&(((SparseArraySegment<double>*)seg)->elements[i]) == 0ull)
                {
                    newSeg->elements[i] = TaggedInt::ToVarUnchecked(0);
                }
                else
                {
                    int32 ival;
                    double dval = ((SparseArraySegment<double>*)seg)->elements[i];
                    if (JavascriptNumber::TryGetInt32Value(dval, &ival) && !TaggedInt::IsOverflow(ival))
                    {
                        newSeg->elements[i] = TaggedInt::ToVarUnchecked(ival);
                    }
                    else
                    {
                        newSeg->elements[i] = JavascriptNumber::ToVarWithCheck(dval, scriptContext);
                    }
                }
            }
            if (seg == newSeg && shrinkFactor != 1)
            {
                // Fill the remaining slots.
                newSeg->FillSegmentBuffer(i, seg->size);
            }
        }

        if (fArray->GetType() == scriptContext->GetLibrary()->GetNativeFloatArrayType())
        {
            fArray->type = scriptContext->GetLibrary()->GetArrayType();
        }
        else
        {
            if (fArray->GetDynamicType()->GetIsLocked())
            {
                DynamicTypeHandler *typeHandler = fArray->GetDynamicType()->GetTypeHandler();
                if (typeHandler->IsPathTypeHandler())
                {
                    // We can't allow a type with the new type ID to be promoted to the old type.
                    // So go to a dictionary type handler, which will orphan the new type.
                    // This should be a corner case, so the inability to share the new type is unlikely to matter.
                    // If it does matter, try building a path from the new type's built-in root.
                    static_cast<PathTypeHandlerBase*>(typeHandler)->ResetTypeHandler(fArray);
                }
                else
                {
                    fArray->ChangeType();
                }
            }
            fArray->GetType()->SetTypeId(TypeIds_Array);
        }

        if (CrossSite::IsCrossSiteObjectTyped(fArray))
        {
            Assert(VirtualTableInfo<CrossSiteObject<JavascriptNativeFloatArray>>::HasVirtualTable(fArray));
            VirtualTableInfo<CrossSiteObject<JavascriptArray>>::SetVirtualTable(fArray);
        }
        else
        {
            Assert(VirtualTableInfo<JavascriptNativeFloatArray>::HasVirtualTable(fArray));
            VirtualTableInfo<JavascriptArray>::SetVirtualTable(fArray);
        }

        failFastError.Completed();

        return fArray;
    }

    JavascriptArray *JavascriptNativeFloatArray::ToVarArray(JavascriptNativeFloatArray *fArray)
    {
#if ENABLE_PROFILE_INFO
        ArrayCallSiteInfo *arrayInfo = fArray->GetArrayCallSiteInfo();
        if (arrayInfo)
        {
#if DBG
            Js::JavascriptStackWalker walker(fArray->GetScriptContext());
            Js::JavascriptFunction* caller = nullptr;
            bool foundScriptCaller = false;
            while(walker.GetCaller(&caller))
            {
                if(caller != nullptr && Js::ScriptFunction::Test(caller))
                {
                    foundScriptCaller = true;
                    break;
                }
            }

            if(foundScriptCaller)
            {
                Assert(caller);
                Assert(caller->GetFunctionBody());
                if(PHASE_TRACE(Js::NativeArrayConversionPhase, caller->GetFunctionBody()))
                {
                    Output::Print(_u("Conversion: Float array to Var array    ArrayCreationFunctionNumber:%2d    CallSiteNumber:%2d \n"), arrayInfo->functionNumber, arrayInfo->callSiteNumber);
                    Output::Flush();
                }
            }
            else
            {
                if(PHASE_TRACE1(Js::NativeArrayConversionPhase))
                {
                    Output::Print(_u("Conversion: Float array to Var array across ScriptContexts"));
                    Output::Flush();
                }
            }
#else
            if(PHASE_TRACE1(Js::NativeArrayConversionPhase))
            {
                Output::Print(_u("Conversion: Float array to Var array"));
                Output::Flush();
            }
#endif

            if(fArray->GetScriptContext()->IsScriptContextInNonDebugMode())
            {
                Assert(!arrayInfo->IsNativeIntArray());
            }

            arrayInfo->SetIsNotNativeArray();
        }
#endif

        fArray->ClearArrayCallSiteIndex();

        return ConvertToVarArray(fArray);

    }

    // Convert Var to index in the Array.
    // Note: Spec calls out a few rules for these parameters:
    // 1. if (arg > length) { return length; }
    // clamp to length, not length-1
    // 2. if (arg < 0) { return max(0, length + arg); }
    // treat negative arg as index from the end of the array (with -1 mapping to length-1)
    // Effectively, this function will return a value between 0 and length, inclusive.
    int64 JavascriptArray::GetIndexFromVar(Js::Var arg, int64 length, ScriptContext* scriptContext)
    {
        int64 index;

        if (TaggedInt::Is(arg))
        {
            int intValue = TaggedInt::ToInt32(arg);

            if (intValue < 0)
            {
                index = max<int64>(0, length + intValue);
            }
            else
            {
                index = intValue;
            }

            if (index > length)
            {
                index = length;
            }
        }
        else
        {
            double doubleValue = JavascriptConversion::ToInteger(arg, scriptContext);

            // Handle the Number.POSITIVE_INFINITY case
            if (doubleValue > length)
            {
                return length;
            }

            index = NumberUtilities::TryToInt64(doubleValue);

            if (index < 0)
            {
                index = max<int64>(0, index + length);
            }
        }

        return index;
    }

    TypeId JavascriptArray::OP_SetNativeIntElementC(JavascriptNativeIntArray *arr, uint32 index, Var value, ScriptContext *scriptContext)
    {
        int32 iValue;
        double dValue;

        TypeId typeId = arr->TrySetNativeIntArrayItem(value, &iValue, &dValue);
        if (typeId == TypeIds_NativeIntArray)
        {
            arr->SetArrayLiteralItem(index, iValue);
        }
        else if (typeId == TypeIds_NativeFloatArray)
        {
            arr->SetArrayLiteralItem(index, dValue);
        }
        else
        {
            arr->SetArrayLiteralItem(index, value);
        }
        return typeId;
    }

    TypeId JavascriptArray::OP_SetNativeFloatElementC(JavascriptNativeFloatArray *arr, uint32 index, Var value, ScriptContext *scriptContext)
    {
        double dValue;
        TypeId typeId = arr->TrySetNativeFloatArrayItem(value, &dValue);
        if (typeId == TypeIds_NativeFloatArray)
        {
            arr->SetArrayLiteralItem(index, dValue);
        }
        else
        {
            arr->SetArrayLiteralItem(index, value);
        }
        return typeId;
    }

    template<typename T>
    void JavascriptArray::SetArrayLiteralItem(uint32 index, T value)
    {
        SparseArraySegment<T> * segment = SparseArraySegment<T>::From(this->head);

        Assert(segment->left == 0);
        Assert(index < segment->length);

        segment->elements[index] = value;
    }

    void JavascriptNativeIntArray::SetIsPrototype()
    {
        // Force the array to be non-native to simplify inspection, filling from proto, etc.
        ToVarArray(this);
        __super::SetIsPrototype();
    }

    void JavascriptNativeFloatArray::SetIsPrototype()
    {
        // Force the array to be non-native to simplify inspection, filling from proto, etc.
        ToVarArray(this);
        __super::SetIsPrototype();
    }

#if ENABLE_PROFILE_INFO
    ArrayCallSiteInfo *JavascriptNativeArray::GetArrayCallSiteInfo()
    {
        RecyclerWeakReference<FunctionBody> *weakRef = this->weakRefToFuncBody;
        if (weakRef)
        {
            FunctionBody *functionBody = weakRef->Get();
            if (functionBody)
            {
                if (functionBody->HasDynamicProfileInfo())
                {
                    Js::ProfileId profileId = this->GetArrayCallSiteIndex();
                    if (profileId < functionBody->GetProfiledArrayCallSiteCount())
                    {
                        return functionBody->GetAnyDynamicProfileInfo()->GetArrayCallSiteInfo(functionBody, profileId);
                    }
                }
            }
            else
            {
                this->ClearArrayCallSiteIndex();
            }
        }
        return nullptr;
    }

    void JavascriptNativeArray::SetArrayProfileInfo(RecyclerWeakReference<FunctionBody> *weakRef, ArrayCallSiteInfo *arrayInfo)
    {
        Assert(weakRef);
        FunctionBody *functionBody = weakRef->Get();
        if (functionBody && functionBody->HasDynamicProfileInfo())
        {
            ArrayCallSiteInfo *baseInfo = functionBody->GetAnyDynamicProfileInfo()->GetArrayCallSiteInfo(functionBody, 0);
            Js::ProfileId index = (Js::ProfileId)(arrayInfo - baseInfo);
            Assert(index < functionBody->GetProfiledArrayCallSiteCount());
            SetArrayCallSite(index, weakRef);
        }
    }

    void JavascriptNativeArray::CopyArrayProfileInfo(Js::JavascriptNativeArray* baseArray)
    {
        if (baseArray->weakRefToFuncBody)
        {
            if (baseArray->weakRefToFuncBody->Get())
            {
                SetArrayCallSite(baseArray->GetArrayCallSiteIndex(), baseArray->weakRefToFuncBody);
            }
            else
            {
                baseArray->ClearArrayCallSiteIndex();
            }
        }
    }
#endif

    Var JavascriptNativeArray::FindMinOrMax(Js::ScriptContext * scriptContext, bool findMax)
    {
        if (JavascriptNativeIntArray::Is(this))
        {
            return this->FindMinOrMax<int32, false>(scriptContext, findMax);
        }
        else
        {
            return this->FindMinOrMax<double, true>(scriptContext, findMax);
        }
    }

    template <typename T, bool checkNaNAndNegZero>
    Var JavascriptNativeArray::FindMinOrMax(Js::ScriptContext * scriptContext, bool findMax)
    {
        AssertMsg(this->HasNoMissingValues(), "Fastpath is only for arrays with one segment and no missing values");
        uint len = this->GetLength();

        Js::SparseArraySegment<T>* headSegment = ((Js::SparseArraySegment<T>*)this->GetHead());
        uint headSegLen = headSegment->length;
        Assert(headSegLen == len);

        if (headSegment->next == nullptr)
        {
            T currentRes = headSegment->elements[0];
            for (uint i = 0; i < headSegLen; i++)
            {
                T compare = headSegment->elements[i];
                if (checkNaNAndNegZero && JavascriptNumber::IsNan(double(compare)))
                {
                    return scriptContext->GetLibrary()->GetNaN();
                }
                if (findMax ? currentRes < compare : currentRes > compare ||
                    (checkNaNAndNegZero && compare == 0 && Js::JavascriptNumber::IsNegZero(double(currentRes))))
                {
                    currentRes = compare;
                }
            }
            return Js::JavascriptNumber::ToVarNoCheck(currentRes, scriptContext);
        }
        else
        {
            AssertMsg(false, "FindMinOrMax currently supports native arrays with only one segment");
            Throw::FatalInternalError();
        }
    }

    SparseArraySegmentBase * JavascriptArray::GetLastUsedSegment() const
    {
        return HasSegmentMap() ?
            PointerValue(segmentUnion.segmentBTreeRoot->lastUsedSegment) :
            PointerValue(segmentUnion.lastUsedSegment);
    }

    void JavascriptArray::SetHeadAndLastUsedSegment(SparseArraySegmentBase * segment)
    {

        Assert(!HasSegmentMap());
        this->head = this->segmentUnion.lastUsedSegment = segment;
    }

    void JavascriptArray::SetLastUsedSegment(SparseArraySegmentBase * segment)
    {
        if (HasSegmentMap())
        {
            this->segmentUnion.segmentBTreeRoot->lastUsedSegment = segment;
        }
        else
        {
            this->segmentUnion.lastUsedSegment = segment;
        }
    }
    bool JavascriptArray::HasSegmentMap() const
    {
        return !!(GetFlags() & DynamicObjectFlags::HasSegmentMap);
    }

    SegmentBTreeRoot * JavascriptArray::GetSegmentMap() const
    {
        return (HasSegmentMap() ? segmentUnion.segmentBTreeRoot : nullptr);
    }

    void JavascriptArray::SetSegmentMap(SegmentBTreeRoot * segmentMap)
    {
        Assert(!HasSegmentMap());
        SparseArraySegmentBase * lastUsedSeg = this->segmentUnion.lastUsedSegment;
        SetFlags(GetFlags() | DynamicObjectFlags::HasSegmentMap);
        segmentUnion.segmentBTreeRoot = segmentMap;
        segmentMap->lastUsedSegment = lastUsedSeg;
    }

    void JavascriptArray::ClearSegmentMap()
    {
        if (HasSegmentMap())
        {
            SetFlags(GetFlags() & ~DynamicObjectFlags::HasSegmentMap);
            SparseArraySegmentBase * lastUsedSeg = segmentUnion.segmentBTreeRoot->lastUsedSegment;
            segmentUnion.segmentBTreeRoot = nullptr;
            segmentUnion.lastUsedSegment = lastUsedSeg;
        }
    }

    SegmentBTreeRoot * JavascriptArray::BuildSegmentMap()
    {
        Recycler* recycler = GetRecycler();
        SegmentBTreeRoot* tmpSegmentMap = AllocatorNewStruct(Recycler, recycler, SegmentBTreeRoot);
        ForEachSegment([recycler, tmpSegmentMap](SparseArraySegmentBase * current)
        {
            tmpSegmentMap->Add(recycler, current);
            return false;
        });

        // There could be OOM during building segment map. Save to array only after its successful completion.
        SetSegmentMap(tmpSegmentMap);
        return tmpSegmentMap;
    }

    void JavascriptArray::TryAddToSegmentMap(Recycler* recycler, SparseArraySegmentBase* seg)
    {
        SegmentBTreeRoot * savedSegmentMap = GetSegmentMap();
        if (savedSegmentMap)
        {
            //
            // We could OOM and throw when adding to segmentMap, resulting in a corrupted segmentMap on this
            // array. Set segmentMap to null temporarily to protect from this. It will be restored correctly
            // if adding segment succeeds.
            //
            ClearSegmentMap();
            savedSegmentMap->Add(recycler, seg);
            SetSegmentMap(savedSegmentMap);
        }
    }

    void JavascriptArray::InvalidateLastUsedSegment()
    {
        this->SetLastUsedSegment(this->head);
    }

    DescriptorFlags JavascriptArray::GetSetter(PropertyId propertyId, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        DescriptorFlags flags;
        if (GetSetterBuiltIns(propertyId, info, &flags))
        {
            return flags;
        }
        return __super::GetSetter(propertyId, setterValue, info, requestContext);
    }

    DescriptorFlags JavascriptArray::GetSetter(JavascriptString* propertyNameString, Var *setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        DescriptorFlags flags;
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetSetterBuiltIns(propertyRecord->GetPropertyId(), info, &flags))
        {
            return flags;
        }

        return __super::GetSetter(propertyNameString, setterValue, info, requestContext);
    }

    bool JavascriptArray::GetSetterBuiltIns(PropertyId propertyId, PropertyValueInfo* info, DescriptorFlags* descriptorFlags)
    {
        if (propertyId == PropertyIds::length)
        {
            PropertyValueInfo::SetNoCache(info, this);
            *descriptorFlags = WritableData;
            return true;
        }
        return false;
    }

    SparseArraySegmentBase * JavascriptArray::GetBeginLookupSegment(uint32 index, const bool useSegmentMap) const
    {
        SparseArraySegmentBase *seg = nullptr;
        SparseArraySegmentBase * lastUsedSeg = this->GetLastUsedSegment();
        if (lastUsedSeg != nullptr && lastUsedSeg->left <= index)
        {
            seg = lastUsedSeg;
            if(index - lastUsedSeg->left < lastUsedSeg->size)
            {
                return seg;
            }
        }

        SegmentBTreeRoot * segmentMap = GetSegmentMap();
        if(!useSegmentMap || !segmentMap)
        {
            return seg ? seg : PointerValue(this->head);
        }

        if(seg)
        {
            // If indexes are being accessed sequentially, check the segment after the last-used segment before checking the
            // segment map, as it is likely to hit
            SparseArraySegmentBase *const nextSeg = seg->next;
            if(nextSeg)
            {
                if(index < nextSeg->left)
                {
                    return seg;
                }
                else if(index - nextSeg->left < nextSeg->size)
                {
                    return nextSeg;
                }
            }
        }

        SparseArraySegmentBase *matchOrNextSeg;
        segmentMap->Find(index, seg, matchOrNextSeg);
        return seg ? seg : matchOrNextSeg;
    }

    uint32 JavascriptArray::GetNextIndex(uint32 index) const
    {
        if (JavascriptNativeIntArray::Is((Var)this))
        {
            return this->GetNextIndexHelper<int32>(index);
        }
        else if (JavascriptNativeFloatArray::Is((Var)this))
        {
            return this->GetNextIndexHelper<double>(index);
        }
        return this->GetNextIndexHelper<Var>(index);
    }

    template<typename T>
    uint32 JavascriptArray::GetNextIndexHelper(uint32 index) const
    {
        AssertMsg(this->head, "array head should never be null");
        uint candidateIndex;

        if (index == JavascriptArray::InvalidIndex)
        {
            candidateIndex = head->left;
        }
        else
        {
            candidateIndex = index + 1;
        }

        SparseArraySegment<T>* current = (SparseArraySegment<T>*)this->GetBeginLookupSegment(candidateIndex);

        while (current != nullptr)
        {
            if ((current->left <= candidateIndex) && ((candidateIndex - current->left) < current->length))
            {
                for (uint i = candidateIndex - current->left; i < current->length; i++)
                {
                    if (!SparseArraySegment<T>::IsMissingItem(&current->elements[i]))
                    {
                        return i + current->left;
                    }
                }
            }
            current = SparseArraySegment<T>::From(current->next);
            if (current != NULL)
            {
                if (candidateIndex < current->left)
                {
                    candidateIndex = current->left;
                }
            }
        }
        return JavascriptArray::InvalidIndex;
    }

    // If new length > length, we just reset the length
    // If new length < length, we need to remove the rest of the elements and segment
    void JavascriptArray::SetLength(uint32 newLength)
    {
        if (newLength == length)
            return;

        if (head == EmptySegment)
        {
            // Do nothing to the segment.
        }
        else if (newLength == 0)
        {
            this->ClearElements(head, 0);
            head->length = 0;
            head->next = nullptr;
            SetHasNoMissingValues();

            ClearSegmentMap();
            this->InvalidateLastUsedSegment();
        }
        else if (newLength < length)
        {
            // _ _ 2 3 _ _ 6 7 _ _

            // SetLength(0)
            // 0 <= left -> set *prev = null
            // SetLength(2)
            // 2 <= left -> set *prev = null
            // SetLength(3)
            // 3 !<= left; 3 <= right -> truncate to length - 1
            // SetLength(5)
            // 5 <=

            SparseArraySegmentBase* next = GetBeginLookupSegment(newLength - 1); // head, or next.left < newLength
            Field(SparseArraySegmentBase*)* prev = &head;

            while(next != nullptr)
            {
                if (newLength <= next->left)
                {
                    ClearSegmentMap(); // truncate segments, null out segmentMap
                    *prev = nullptr;
                    break;
                }
                else if (newLength <= (next->left + next->length))
                {
                    if (next->next)
                    {
                        ClearSegmentMap(); // Will truncate segments, null out segmentMap
                    }

                    uint32 newSegmentLength = newLength - next->left;
                    this->ClearElements(next, newSegmentLength);
                    next->next = nullptr;
                    next->length = newSegmentLength;
                    next->CheckLengthvsSize();
                    break;
                }
                else
                {
                    prev = &next->next;
                    next = next->next;
                }
            }
            this->InvalidateLastUsedSegment();
        }
        this->length = newLength;

#ifdef VALIDATE_ARRAY
        ValidateArray();
#endif
    }

    BOOL JavascriptArray::SetLength(Var newLength)
    {
        ScriptContext *scriptContext;
        if(TaggedInt::Is(newLength))
        {
            int32 lenValue = TaggedInt::ToInt32(newLength);
            if (lenValue < 0)
            {
                scriptContext = GetScriptContext();
                if (scriptContext->GetThreadContext()->RecordImplicitException())
                {
                    JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthAssignIncorrect);
                }
            }
            else
            {
                this->SetLength(lenValue);
            }
            return TRUE;
        }

        scriptContext = GetScriptContext();
        uint32 uintValue = JavascriptConversion::ToUInt32(newLength, scriptContext);
        double dblValue = JavascriptConversion::ToNumber(newLength, scriptContext);
        if (dblValue == uintValue)
        {
            this->SetLength(uintValue);
        }
        else
        {
            ThreadContext* threadContext = scriptContext->GetThreadContext();
            ImplicitCallFlags flags = threadContext->GetImplicitCallFlags();
            if (flags != ImplicitCall_None && threadContext->IsDisableImplicitCall())
            {
                // We couldn't execute the implicit call(s) needed to convert the newLength to an integer.
                // Do nothing and let the jitted code bail out.
                return TRUE;
            }

            if (threadContext->RecordImplicitException())
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthAssignIncorrect);
            }
        }

        return TRUE;
    }

    void JavascriptArray::ClearElements(SparseArraySegmentBase *seg, uint32 newSegmentLength)
    {
        SparseArraySegment<Var>::ClearElements(((SparseArraySegment<Var>*)seg)->elements + newSegmentLength, seg->length - newSegmentLength);
    }

    void JavascriptNativeIntArray::ClearElements(SparseArraySegmentBase *seg, uint32 newSegmentLength)
    {
        SparseArraySegment<int32>::ClearElements(((SparseArraySegment<int32>*)seg)->elements + newSegmentLength, seg->length - newSegmentLength);
    }

    void JavascriptNativeFloatArray::ClearElements(SparseArraySegmentBase *seg, uint32 newSegmentLength)
    {
        SparseArraySegment<double>::ClearElements(((SparseArraySegment<double>*)seg)->elements + newSegmentLength, seg->length - newSegmentLength);
    }

    Var JavascriptArray::DirectGetItem(uint32 index)
    {
        SparseArraySegment<Var> *seg = (SparseArraySegment<Var>*)this->GetLastUsedSegment();
        uint32 offset = index - seg->left;
        if (index >= seg->left && offset < seg->length)
        {
            if (!SparseArraySegment<Var>::IsMissingItem(&seg->elements[offset]))
            {
                return seg->elements[offset];
            }
        }
        Var element = nullptr;
        if (DirectGetItemAtFull(index, &element))
        {
            return element;
        }
        return GetType()->GetLibrary()->GetUndefined();
    }

    Var JavascriptNativeIntArray::DirectGetItem(uint32 index)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(this);
#endif
        SparseArraySegment<int32> *seg = (SparseArraySegment<int32>*)this->GetLastUsedSegment();
        uint32 offset = index - seg->left;
        if (index >= seg->left && offset < seg->length)
        {
            if (!SparseArraySegment<int32>::IsMissingItem(&seg->elements[offset]))
            {
                return JavascriptNumber::ToVar(seg->elements[offset], GetScriptContext());
            }
        }
        Var element = nullptr;
        if (DirectGetItemAtFull(index, &element))
        {
            return element;
        }
        return GetType()->GetLibrary()->GetUndefined();
    }

    DescriptorFlags JavascriptNativeIntArray::GetItemSetter(uint32 index, Var* setterValue, ScriptContext* requestContext)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(this);
#endif
        int32 value = 0;
        return this->DirectGetItemAt(index, &value) ? WritableData : None;
    }


    Var JavascriptNativeFloatArray::DirectGetItem(uint32 index)
    {
        SparseArraySegment<double> *seg = (SparseArraySegment<double>*)this->GetLastUsedSegment();
        uint32 offset = index - seg->left;
        if (index >= seg->left && offset < seg->length)
        {
            if (!SparseArraySegment<double>::IsMissingItem(&seg->elements[offset]))
            {
                return JavascriptNumber::ToVarWithCheck(seg->elements[offset], GetScriptContext());
            }
        }
        Var element = nullptr;
        if (DirectGetItemAtFull(index, &element))
        {
            return element;
        }
        return GetType()->GetLibrary()->GetUndefined();
    }

    Var JavascriptArray::DirectGetItem(JavascriptString *propName, ScriptContext* scriptContext)
    {
        PropertyRecord const * propertyRecord;
        scriptContext->GetOrAddPropertyRecord(propName->GetString(), propName->GetLength(), &propertyRecord);
        return JavascriptOperators::GetProperty(this, propertyRecord->GetPropertyId(), scriptContext, NULL);
    }

    BOOL JavascriptArray::DirectGetItemAtFull(uint32 index, Var* outVal)
    {
        if (this->DirectGetItemAt(index, outVal))
        {
            return TRUE;
        }

        ScriptContext* requestContext = type->GetScriptContext();
        return JavascriptOperators::GetItem(this, this->GetPrototype(), index, outVal, requestContext);
    }

    //
    // Link prev and current. If prev is NULL, make current the head segment.
    //
    void JavascriptArray::LinkSegmentsCommon(SparseArraySegmentBase* prev, SparseArraySegmentBase* current)
    {
        if (prev)
        {
            prev->next = current;
        }
        else
        {
            Assert(current);
            head = current;
        }
    }

    template<typename T>
    BOOL JavascriptArray::DirectDeleteItemAt(uint32 itemIndex)
    {
        if (itemIndex >= length)
        {
            return true;
        }
        SparseArraySegment<T>* next = (SparseArraySegment<T>*)GetBeginLookupSegment(itemIndex);
        while(next != nullptr && next->left <= itemIndex)
        {
            uint32 limit = next->left + next->length;
            if (itemIndex < limit)
            {
                next->SetElement(GetRecycler(), itemIndex, SparseArraySegment<T>::GetMissingItem());
                if(itemIndex - next->left == next->length - 1)
                {
                    --next->length;
                    next->CheckLengthvsSize();
                }
                else if(next == head)
                {
                    SetHasNoMissingValues(false);
                }
                break;
            }
            next = SparseArraySegment<T>::From(next->next);
        }
#ifdef VALIDATE_ARRAY
        ValidateArray();
#endif
        return true;
    }

    template <> Var JavascriptArray::ConvertToIndex(BigIndex idxDest, ScriptContext* scriptContext)
    {
        return idxDest.ToNumber(scriptContext);
    }

    template <> uint32 JavascriptArray::ConvertToIndex(BigIndex idxDest, ScriptContext* scriptContext)
    {
        // Note this is only for setting Array length which is a uint32
        return idxDest.IsSmallIndex() ? idxDest.GetSmallIndex() : UINT_MAX;
    }

    template <> Var JavascriptArray::ConvertToIndex(uint32 idxDest, ScriptContext* scriptContext)
    {
        return  JavascriptNumber::ToVar(idxDest, scriptContext);
    }

    void JavascriptArray::ThrowErrorOnFailure(BOOL succeeded, ScriptContext* scriptContext, uint32 index)
    {
        if (!succeeded)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_CantRedefineProp, JavascriptConversion::ToString(JavascriptNumber::ToVar(index, scriptContext), scriptContext)->GetSz());
        }
    }

    void JavascriptArray::ThrowErrorOnFailure(BOOL succeeded, ScriptContext* scriptContext, BigIndex index)
    {
        if (!succeeded)
        {
            uint64 i = (uint64)(index.IsSmallIndex() ? index.GetSmallIndex() : index.GetBigIndex());
            JavascriptError::ThrowTypeError(scriptContext, JSERR_CantRedefineProp, JavascriptConversion::ToString(JavascriptNumber::ToVar(i, scriptContext), scriptContext)->GetSz());
        }
    }

    BOOL JavascriptArray::SetArrayLikeObjects(RecyclableObject* pDestObj, uint32 idxDest, Var aItem)
    {
        return pDestObj->SetItem(idxDest, aItem, Js::PropertyOperation_ThrowIfNotExtensible);
    }

    uint64 JavascriptArray::OP_GetLength(Var obj, ScriptContext *scriptContext)
    {
        if (scriptContext->GetConfig()->IsES6ToLengthEnabled())
        {
            // Casting to uint64 is okay as ToLength will always be >= 0.
            return (uint64)JavascriptConversion::ToLength(JavascriptOperators::OP_GetLength(obj, scriptContext), scriptContext);
        }
        else
        {
            return (uint64)JavascriptConversion::ToUInt32(JavascriptOperators::OP_GetLength(obj, scriptContext), scriptContext);
        }
    }

    template<typename T>
    void JavascriptArray::TryGetArrayAndLength(Var arg,
        ScriptContext *scriptContext,
        PCWSTR methodName,
        __out JavascriptArray** array,
        __out RecyclableObject** obj,
        __out T * length)
    {
        Assert(array != nullptr);
        Assert(obj != nullptr);
        Assert(length != nullptr);

        if (JavascriptArray::Is(arg) && !JavascriptArray::FromVar(arg)->IsCrossSiteObject())
        {
#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(arg);
#endif
            *array = JavascriptArray::FromVar(arg);
            *obj = *array;
            *length = (*array)->length;
        }
        else
        {
            if (!JavascriptConversion::ToObject(arg, scriptContext, obj))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, methodName);
            }
            *length = OP_GetLength(*obj, scriptContext);
            *array = nullptr;
        }
    }

    BOOL JavascriptArray::SetArrayLikeObjects(RecyclableObject* pDestObj, BigIndex idxDest, Var aItem)
    {
        ScriptContext* scriptContext = pDestObj->GetScriptContext();

        if (idxDest.IsSmallIndex())
        {
            return pDestObj->SetItem(idxDest.GetSmallIndex(), aItem, Js::PropertyOperation_ThrowIfNotExtensible);
        }
        PropertyRecord const * propertyRecord;
        JavascriptOperators::GetPropertyIdForInt(idxDest.GetBigIndex(), scriptContext, &propertyRecord);
        return pDestObj->SetProperty(propertyRecord->GetPropertyId(), aItem, PropertyOperation_ThrowIfNotExtensible, nullptr);
    }

    template<typename T>
    void JavascriptArray::ConcatArgs(RecyclableObject* pDestObj, TypeId* remoteTypeIds,
        Js::Arguments& args, ScriptContext* scriptContext, uint start, BigIndex startIdxDest,
        BOOL FirstPromotedItemIsSpreadable, BigIndex FirstPromotedItemLength, bool spreadableCheckedAndTrue)
    {
        // This never gets called.
        Throw::InternalError();
    }
    //
    // Helper for EntryConcat. Concat args or elements of arg arrays into dest array.
    //
    template<typename T>
    void JavascriptArray::ConcatArgs(RecyclableObject* pDestObj, TypeId* remoteTypeIds,
        Js::Arguments& args, ScriptContext* scriptContext, uint start, uint startIdxDest,
        BOOL firstPromotedItemIsSpreadable, BigIndex firstPromotedItemLength, bool spreadableCheckedAndTrue)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());
        JavascriptArray* pDestArray = nullptr;

        if (JavascriptArray::Is(pDestObj))
        {
            // ConcatArgs function expects to work on the Var array so we are ensuring it.
            pDestArray = EnsureNonNativeArray(JavascriptArray::FromVar(pDestObj));
        }

        T idxDest = startIdxDest;
        for (uint idxArg = start; idxArg < args.Info.Count; idxArg++)
        {
            Var aItem = args[idxArg];
            bool spreadable = spreadableCheckedAndTrue;
            if (!spreadable && scriptContext->GetConfig()->IsES6IsConcatSpreadableEnabled())
            {
                // firstPromotedItemIsSpreadable is ONLY used to resume after a type promotion from uint32 to uint64
                // we do this because calls to IsConcatSpreadable are observable (a big deal for proxies) and we don't
                // want to do the work a second time as soon as we record the length we clear the flag.
                JS_REENTRANT(jsReentLock, spreadable = firstPromotedItemIsSpreadable || JavascriptOperators::IsConcatSpreadable(aItem));

                if (!spreadable)
                {
                    JS_REENTRANT(jsReentLock, JavascriptArray::SetConcatItem<T>(aItem, idxArg, pDestArray, pDestObj, idxDest, scriptContext));
                    ++idxDest;
                    continue;
                }
            }
            else
            {
                spreadableCheckedAndTrue = false; // if it was `true`, reset after the first use
            }

            if (pDestArray && JavascriptArray::IsDirectAccessArray(aItem) && JavascriptArray::IsDirectAccessArray(pDestArray)
                && BigIndex(idxDest + JavascriptArray::FromVar(aItem)->length).IsSmallIndex() && !JavascriptArray::FromVar(aItem)->IsFillFromPrototypes()) // Fast path
            {
                if (JavascriptNativeIntArray::Is(aItem))
                {
                    JavascriptNativeIntArray *pItemArray = JavascriptNativeIntArray::FromVar(aItem);
                    JS_REENTRANT(jsReentLock, CopyNativeIntArrayElementsToVar(pDestArray, BigIndex(idxDest).GetSmallIndex(), pItemArray));
                    idxDest = idxDest + pItemArray->length;
                }
                else if (JavascriptNativeFloatArray::Is(aItem))
                {
                    JavascriptNativeFloatArray *pItemArray = JavascriptNativeFloatArray::FromVar(aItem);
                    JS_REENTRANT(jsReentLock, CopyNativeFloatArrayElementsToVar(pDestArray, BigIndex(idxDest).GetSmallIndex(), pItemArray));
                    idxDest = idxDest + pItemArray->length;
                }
                else
                {
                    JavascriptArray* pItemArray = JavascriptArray::FromVar(aItem);
                    JS_REENTRANT(jsReentLock, CopyArrayElements(pDestArray, BigIndex(idxDest).GetSmallIndex(), pItemArray));
                    idxDest = idxDest + pItemArray->length;
                }
            }
            else
            {
                // Flatten if other array or remote array (marked with TypeIds_Array)
                if (DynamicObject::IsAnyArray(aItem) || remoteTypeIds[idxArg] == TypeIds_Array || spreadable)
                {
                    //CONSIDER: enumerating remote array instead of walking all indices
                    BigIndex length;
                    if (firstPromotedItemIsSpreadable)
                    {
                        firstPromotedItemIsSpreadable = false;
                        length = firstPromotedItemLength;
                    }
                    else
                    {
                        JS_REENTRANT(jsReentLock, length = OP_GetLength(aItem, scriptContext));
                    }

                    if (PromoteToBigIndex(length,idxDest))
                    {
                        // This is a special case for spreadable objects. We do not pre-calculate the length
                        // in EntryConcat like we do with Arrays because a getProperty on an object Length
                        // is observable. The result is we have to check for overflows separately for
                        // spreadable objects and promote to a bigger index type when we find them.
                        ConcatArgs<BigIndex>(pDestArray, remoteTypeIds, args, scriptContext, idxArg, idxDest, /*firstPromotedItemIsSpreadable*/true, length);
                        return;
                    }

                    if (length + idxDest > FiftyThirdPowerOfTwoMinusOne) // 2^53-1: from ECMA 22.1.3.1 Array.prototype.concat(...arguments)
                    {
                        JavascriptError::ThrowTypeError(scriptContext, JSERR_IllegalArraySizeAndLength);
                    }

                    RecyclableObject* itemObject = RecyclableObject::FromVar(aItem);
                    Var subItem;
                    uint32 lengthToUin32Max = length.IsSmallIndex() ? length.GetSmallIndex() : MaxArrayLength;
                    for (uint32 idxSubItem = 0u; idxSubItem < lengthToUin32Max; ++idxSubItem)
                    {
                        JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(itemObject, idxSubItem));
                        if (hasItem)
                        {
                            JS_REENTRANT(jsReentLock, subItem = JavascriptOperators::GetItem(itemObject, idxSubItem, scriptContext));

                            if (pDestArray)
                            {
                                pDestArray->GenericDirectSetItemAt(idxDest, subItem);
                            }
                            else
                            {
                                JS_REENTRANT(jsReentLock, ThrowErrorOnFailure(SetArrayLikeObjects(pDestObj, idxDest, subItem), scriptContext, idxDest));
                            }
                        }
                        ++idxDest;
                    }

                    for (BigIndex idxSubItem = MaxArrayLength; idxSubItem < length; ++idxSubItem)
                    {
                        PropertyRecord const * propertyRecord;
                        JavascriptOperators::GetPropertyIdForInt(idxSubItem.GetBigIndex(), scriptContext, &propertyRecord);
                        JS_REENTRANT(jsReentLock, BOOL hasProp = JavascriptOperators::HasProperty(itemObject, propertyRecord->GetPropertyId()));
                        if (hasProp)
                        {
                            JS_REENTRANT(jsReentLock, subItem = JavascriptOperators::GetProperty(itemObject, propertyRecord->GetPropertyId(), scriptContext));
                            if (pDestArray)
                            {
                                pDestArray->GenericDirectSetItemAt(idxDest, subItem);
                            }
                            else
                            {
                                JS_REENTRANT(jsReentLock, ThrowErrorOnFailure(SetArrayLikeObjects(pDestObj, idxDest, subItem), scriptContext, idxSubItem));
                            }
                        }
                        ++idxDest;
                    }
                }
                else // concat 1 item
                {
                    JS_REENTRANT(jsReentLock, JavascriptArray::SetConcatItem<T>(aItem, idxArg, pDestArray, pDestObj, idxDest, scriptContext));
                    ++idxDest;
                }
            }

            firstPromotedItemIsSpreadable = false;
        }
        if (!pDestArray)
        {
            JS_REENTRANT(jsReentLock, pDestObj->SetProperty(PropertyIds::length, ConvertToIndex<T, Var>(idxDest, scriptContext), Js::PropertyOperation_None, nullptr));
       }
        else if (pDestArray->GetLength() != ConvertToIndex<T, uint32>(idxDest, scriptContext))
        {
            pDestArray->SetLength(ConvertToIndex<T, uint32>(idxDest, scriptContext));
        }
    }

    bool JavascriptArray::PromoteToBigIndex(BigIndex lhs, BigIndex rhs)
    {
        return false; // already a big index
    }

    bool JavascriptArray::PromoteToBigIndex(BigIndex lhs, uint32 rhs)
    {
        ::Math::RecordOverflowPolicy destLengthOverflow;
        if (lhs.IsSmallIndex())
        {
            UInt32Math::Add(lhs.GetSmallIndex(), rhs, destLengthOverflow);
            return destLengthOverflow.HasOverflowed();
        }
        return true;
    }

    JavascriptArray* JavascriptArray::ConcatIntArgs(JavascriptNativeIntArray* pDestArray, TypeId *remoteTypeIds, Js::Arguments& args, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());
        uint idxDest = 0u;
        for (uint idxArg = 0; idxArg < args.Info.Count; idxArg++)
        {
            Var aItem = args[idxArg];
            bool spreadableCheckedAndTrue = false;

            if (scriptContext->GetConfig()->IsES6IsConcatSpreadableEnabled())
            {
                JS_REENTRANT(jsReentLock, spreadableCheckedAndTrue = !scriptContext->GetConfig()->IsES6IsConcatSpreadableEnabled() || JavascriptOperators::IsConcatSpreadable(aItem));
                if (!JavascriptNativeIntArray::Is(pDestArray))
                {
                    ConcatArgs<uint>(pDestArray, remoteTypeIds, args, scriptContext, idxArg, idxDest, spreadableCheckedAndTrue);
                    return pDestArray;
                }

                if(!spreadableCheckedAndTrue)
                {
                    JS_REENTRANT(jsReentLock, pDestArray->SetItem(idxDest, aItem, PropertyOperation_ThrowIfNotExtensible));
                    idxDest = idxDest + 1;
                    if (!JavascriptNativeIntArray::Is(pDestArray)) // SetItem could convert pDestArray to a var array if aItem is not an integer if so fall back
                    {
                        JS_REENTRANT(jsReentLock, ConcatArgs<uint>(pDestArray, remoteTypeIds, args, scriptContext, idxArg + 1, idxDest));
                        return pDestArray;
                    }
                    continue;
                }
            }

            if (JavascriptNativeIntArray::Is(aItem) && !JavascriptNativeIntArray::FromVar(aItem)->IsFillFromPrototypes()) // Fast path
            {
                JavascriptNativeIntArray* pItemArray = JavascriptNativeIntArray::FromVar(aItem);

                JS_REENTRANT(jsReentLock, bool converted = CopyNativeIntArrayElements(pDestArray, idxDest, pItemArray));

                idxDest = idxDest + pItemArray->length;
                if (converted)
                {
                    // Copying the last array forced a conversion, so switch over to the var version
                    // to finish.
                    JS_REENTRANT(jsReentLock, ConcatArgs<uint>(pDestArray, remoteTypeIds, args, scriptContext, idxArg + 1, idxDest));
                    return pDestArray;
                }
            }
            else if (!JavascriptArray::IsAnyArray(aItem) && remoteTypeIds[idxArg] != TypeIds_Array)
            {
                if (TaggedInt::Is(aItem))
                {
                    pDestArray->DirectSetItemAt(idxDest, TaggedInt::ToInt32(aItem));
                }
                else
                {
#if DBG
                    int32 int32Value;
                    Assert(
                        JavascriptNumber::TryGetInt32Value(JavascriptNumber::GetValue(aItem), &int32Value) &&
                        !SparseArraySegment<int32>::IsMissingItem(&int32Value));
#endif
                    pDestArray->DirectSetItemAt(idxDest, static_cast<int32>(JavascriptNumber::GetValue(aItem)));
                }
                ++idxDest;
            }
            else
            {
                JavascriptArray *pVarDestArray = JavascriptNativeIntArray::ConvertToVarArray(pDestArray);
                BigIndex length;
                JS_REENTRANT(jsReentLock, length = OP_GetLength(aItem, scriptContext),
                    ConcatArgs<uint>(pVarDestArray, remoteTypeIds, args, scriptContext, idxArg, idxDest, spreadableCheckedAndTrue, length));
                return pVarDestArray;
            }
        }
        if (pDestArray->GetLength() != idxDest)
        {
            pDestArray->SetLength(idxDest);
        }
        return pDestArray;
    }

    JavascriptArray* JavascriptArray::ConcatFloatArgs(JavascriptNativeFloatArray* pDestArray, TypeId *remoteTypeIds, Js::Arguments& args, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());
        uint idxDest = 0u;
        for (uint idxArg = 0; idxArg < args.Info.Count; idxArg++)
        {
            Var aItem = args[idxArg];

            bool spreadableCheckedAndTrue = false;

            if (scriptContext->GetConfig()->IsES6IsConcatSpreadableEnabled())
            {
                JS_REENTRANT(jsReentLock, spreadableCheckedAndTrue = !scriptContext->GetConfig()->IsES6IsConcatSpreadableEnabled() || JavascriptOperators::IsConcatSpreadable(aItem));
                if (!JavascriptNativeFloatArray::Is(pDestArray))
                {
                    ConcatArgs<uint>(pDestArray, remoteTypeIds, args, scriptContext, idxArg, idxDest, spreadableCheckedAndTrue);
                    return pDestArray;
                }

                if (!spreadableCheckedAndTrue)
                {
                    JS_REENTRANT(jsReentLock, pDestArray->SetItem(idxDest, aItem, PropertyOperation_ThrowIfNotExtensible));

                    idxDest = idxDest + 1;
                    if (!JavascriptNativeFloatArray::Is(pDestArray)) // SetItem could convert pDestArray to a var array if aItem is not an integer if so fall back
                    {
                        JS_REENTRANT(jsReentLock, ConcatArgs<uint>(pDestArray, remoteTypeIds, args, scriptContext, idxArg + 1, idxDest));
                        return pDestArray;
                    }
                    continue;
                }
            }

            bool converted = false;
            if (JavascriptArray::IsAnyArray(aItem) || remoteTypeIds[idxArg] == TypeIds_Array)
            {
                if (JavascriptNativeIntArray::Is(aItem) && !JavascriptArray::FromVar(aItem)->IsFillFromPrototypes()) // Fast path
                {
                    JavascriptNativeIntArray *pIntArray = JavascriptNativeIntArray::FromVar(aItem);

                    JS_REENTRANT(jsReentLock, converted = CopyNativeIntArrayElementsToFloat(pDestArray, idxDest, pIntArray));

                    idxDest = idxDest + pIntArray->length;
                }
                else if (JavascriptNativeFloatArray::Is(aItem) && !JavascriptArray::FromVar(aItem)->IsFillFromPrototypes())
                {
                    JavascriptNativeFloatArray* pItemArray = JavascriptNativeFloatArray::FromVar(aItem);

                    JS_REENTRANT(jsReentLock, converted = CopyNativeFloatArrayElements(pDestArray, idxDest, pItemArray));

                    idxDest = idxDest + pItemArray->length;
                }
                else
                {
                    JavascriptArray *pVarDestArray = JavascriptNativeFloatArray::ConvertToVarArray(pDestArray);

                    BigIndex length;
                    JS_REENTRANT(jsReentLock, length = OP_GetLength(aItem, scriptContext),
                        ConcatArgs<uint>(pVarDestArray, remoteTypeIds, args, scriptContext, idxArg, idxDest, spreadableCheckedAndTrue, length));

                    return pVarDestArray;
                }

                if (converted)
                {
                    // Copying the last array forced a conversion, so switch over to the var version
                    // to finish.

                    JS_REENTRANT(jsReentLock, ConcatArgs<uint>(pDestArray, remoteTypeIds, args, scriptContext, idxArg + 1, idxDest));

                    return pDestArray;
                }
            }
            else
            {
                if (TaggedInt::Is(aItem))
                {
                    pDestArray->DirectSetItemAt(idxDest, (double)TaggedInt::ToInt32(aItem));
                }
                else
                {
                    Assert(JavascriptNumber::Is(aItem));
                    pDestArray->DirectSetItemAt(idxDest, JavascriptNumber::GetValue(aItem));
                }
                ++idxDest;
            }
        }
        if (pDestArray->GetLength() != idxDest)
        {
            pDestArray->SetLength(idxDest);
        }

        return pDestArray;
    }

    bool JavascriptArray::BoxConcatItem(Var aItem, uint idxArg, ScriptContext *scriptContext)
    {
        return idxArg == 0 && !JavascriptOperators::IsObject(aItem);
    }

    Var JavascriptArray::EntryConcat(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.concat"));
        }

        //
        // Compute the destination ScriptArray size:
        // - Each item, flattening only one level if a ScriptArray.
        //

        uint32 cDestLength = 0;
        JavascriptArray * pDestArray = NULL;

        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault + (args.Info.Count * sizeof(TypeId*)));
        TypeId* remoteTypeIds = (TypeId*)_alloca(args.Info.Count * sizeof(TypeId*));

        bool isInt = true;
        bool isFloat = true;
        ::Math::RecordOverflowPolicy destLengthOverflow;
        for (uint idxArg = 0; idxArg < args.Info.Count; idxArg++)
        {
            Var aItem = args[idxArg];
#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(aItem);
#endif
            if (DynamicObject::IsAnyArray(aItem)) // Get JavascriptArray or ES5Array length
            {
                JavascriptArray * pItemArray = JavascriptArray::FromAnyArray(aItem);
                if (isFloat)
                {
                    if (!JavascriptNativeIntArray::Is(pItemArray))
                    {
                        isInt = false;
                        if (!JavascriptNativeFloatArray::Is(pItemArray))
                        {
                            isFloat = false;
                        }
                    }
                }
                cDestLength = UInt32Math::Add(cDestLength, pItemArray->GetLength(), destLengthOverflow);
            }
            else // Get remote array or object length
            {
                // We already checked for types derived from JavascriptArray. These are types that should behave like array
                // i.e. proxy to array and remote array.
                if (JavascriptOperators::IsArray(aItem))
                {
                    // Don't try to preserve nativeness of remote arrays. The extra complexity is probably not
                    // worth it.
                    isInt = false;
                    isFloat = false;
                    if (!JavascriptProxy::Is(aItem))
                    {
                        if (scriptContext->GetConfig()->IsES6ToLengthEnabled())
                        {
                            JS_REENTRANT(jsReentLock,
                                int64 len = JavascriptConversion::ToLength(JavascriptOperators::OP_GetLength(aItem, scriptContext), scriptContext));
                            // clipping to MaxArrayLength will overflow when added to cDestLength which we catch below
                            cDestLength = UInt32Math::Add(cDestLength, len < MaxArrayLength ? (uint32)len : MaxArrayLength, destLengthOverflow);
                        }
                        else
                        {
                            JS_REENTRANT(jsReentLock,
                                uint len = JavascriptConversion::ToUInt32(JavascriptOperators::OP_GetLength(aItem, scriptContext), scriptContext));
                            cDestLength = UInt32Math::Add(cDestLength, len, destLengthOverflow);
                        }
                    }
                    remoteTypeIds[idxArg] = TypeIds_Array; // Mark remote array, no matter remote JavascriptArray or ES5Array.
                }
                else
                {
                    if (isFloat)
                    {
                        if (BoxConcatItem(aItem, idxArg, scriptContext))
                        {
                            // A primitive will be boxed, so we have to create a var array for the result.
                            isInt = false;
                            isFloat = false;
                        }
                        else if (!TaggedInt::Is(aItem))
                        {
                            if (!JavascriptNumber::Is(aItem))
                            {
                                isInt = false;
                                isFloat = false;
                            }
                            else if (isInt)
                            {
                                int32 int32Value;
                                if(!JavascriptNumber::TryGetInt32Value(JavascriptNumber::GetValue(aItem), &int32Value) ||
                                    SparseArraySegment<int32>::IsMissingItem(&int32Value))
                                {
                                    isInt = false;
                                }
                            }
                        }
                        else if(isInt)
                        {
                            int32 int32Value = TaggedInt::ToInt32(aItem);
                            if(SparseArraySegment<int32>::IsMissingItem(&int32Value))
                            {
                                isInt = false;
                            }
                        }
                    }

                    remoteTypeIds[idxArg] = TypeIds_Limit;
                    cDestLength = UInt32Math::Add(cDestLength, 1, destLengthOverflow);
                }
            }
        }
        if (destLengthOverflow.HasOverflowed())
        {
            cDestLength = MaxArrayLength;
            isInt = false;
            isFloat = false;
        }

        //
        // Create the destination array
        //
        RecyclableObject* pDestObj = nullptr;
        bool isArray = false;

        JS_REENTRANT(jsReentLock, pDestObj = ArraySpeciesCreate(args[0], 0, scriptContext));

        if (pDestObj)
        {
#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(pDestObj);
#endif
            // Check the thing that species create made. If it's a native array that can't handle the source
            // data, convert it. If it's a more conservative kind of array than the source data, indicate that
            // so that the data will be converted on copy.
            if (isInt)
            {
                if (JavascriptNativeIntArray::Is(pDestObj))
                {
                    isArray = true;
                }
                else
                {
                    isInt = false;
                    isFloat = JavascriptNativeFloatArray::Is(pDestObj);
                    isArray = JavascriptArray::Is(pDestObj);
                }
            }
            else if (isFloat)
            {
                if (JavascriptNativeIntArray::Is(pDestObj))
                {
                    JavascriptNativeIntArray::ToNativeFloatArray(JavascriptNativeIntArray::FromVar(pDestObj));
                    isArray = true;
                }
                else
                {
                    isFloat = JavascriptNativeFloatArray::Is(pDestObj);
                    isArray = JavascriptArray::Is(pDestObj);
                }
            }
            else
            {
                if (JavascriptNativeIntArray::Is(pDestObj))
                {
                    JavascriptNativeIntArray::ToVarArray(JavascriptNativeIntArray::FromVar(pDestObj));
                    isArray = true;
                }
                else if (JavascriptNativeFloatArray::Is(pDestObj))
                {
                    JavascriptNativeFloatArray::ToVarArray(JavascriptNativeFloatArray::FromVar(pDestObj));
                    isArray = true;
                }
                else
                {
                    isArray = JavascriptArray::Is(pDestObj);
                }
            }

        }

        if (pDestObj == nullptr || isArray)
        {
            if (isInt)
            {
                JavascriptNativeIntArray *pIntArray = isArray ? JavascriptNativeIntArray::FromVar(pDestObj) : scriptContext->GetLibrary()->CreateNativeIntArray(cDestLength);
                pIntArray->EnsureHead<int32>();

                JS_REENTRANT(jsReentLock, pDestArray = ConcatIntArgs(pIntArray, remoteTypeIds, args, scriptContext));
            }
            else if (isFloat)
            {
                JavascriptNativeFloatArray *pFArray = isArray ? JavascriptNativeFloatArray::FromVar(pDestObj) : scriptContext->GetLibrary()->CreateNativeFloatArray(cDestLength);
                pFArray->EnsureHead<double>();

                JS_REENTRANT(jsReentLock, pDestArray = ConcatFloatArgs(pFArray, remoteTypeIds, args, scriptContext));
            }
            else
            {

                pDestArray = isArray ?  JavascriptArray::FromVar(pDestObj) : scriptContext->GetLibrary()->CreateArray(cDestLength);
                // if the constructor has changed then we no longer specialize for ints and floats
                pDestArray->EnsureHead<Var>();

                JS_REENTRANT(jsReentLock, ConcatArgsCallingHelper(pDestArray, remoteTypeIds, args, scriptContext, destLengthOverflow));
            }

            //
            // Return the new array instance.
            //

#ifdef VALIDATE_ARRAY
            pDestArray->ValidateArray();
#endif

            return pDestArray;
        }

        Assert(pDestObj);
        JS_REENTRANT(jsReentLock, ConcatArgsCallingHelper(pDestObj, remoteTypeIds, args, scriptContext, destLengthOverflow));

        return pDestObj;
    }

    void JavascriptArray::ConcatArgsCallingHelper(RecyclableObject* pDestObj, TypeId* remoteTypeIds, Js::Arguments& args, ScriptContext* scriptContext, ::Math::RecordOverflowPolicy &destLengthOverflow)
    {
        if (destLengthOverflow.HasOverflowed())
        {
            ConcatArgs<BigIndex>(pDestObj, remoteTypeIds, args, scriptContext);
        }
        else
        {
            // Use faster uint32 version if no overflow
            ConcatArgs<uint32>(pDestObj, remoteTypeIds, args, scriptContext);
        }
    }

    template<typename T>
    /* static */ void JavascriptArray::SetConcatItem(Var aItem, uint idxArg, JavascriptArray* pDestArray, RecyclableObject* pDestObj, T idxDest, ScriptContext *scriptContext)
    {
        if (BoxConcatItem(aItem, idxArg, scriptContext))
        {
            // bug# 725784: ES5: not calling ToObject in Step 1 of 15.4.4.4
            RecyclableObject* pObj = nullptr;
            if (FALSE == JavascriptConversion::ToObject(aItem, scriptContext, &pObj))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.concat"));
            }
            if (pDestArray)
            {
                pDestArray->GenericDirectSetItemAt(idxDest, pObj);
            }
            else
            {
                SetArrayLikeObjects(pDestObj, idxDest, pObj);
            }
        }
        else
        {
            if (pDestArray)
            {
                pDestArray->GenericDirectSetItemAt(idxDest, aItem);
            }
            else
            {
                SetArrayLikeObjects(pDestObj, idxDest, aItem);
            }
        }
    }

    int64 JavascriptArray::GetFromLastIndex(Var arg, int64 length, ScriptContext *scriptContext)
    {
        int64 fromIndex;

        if (TaggedInt::Is(arg))
        {
            int intValue = TaggedInt::ToInt32(arg);

            if (intValue >= 0)
            {
                fromIndex = min<int64>(intValue, length - 1);
            }
            else if ((uint32)-intValue > length)
            {
                return length;
            }
            else
            {
                fromIndex = intValue + length;
            }
        }
        else
        {
            double value = JavascriptConversion::ToInteger(arg, scriptContext);

            if (value >= 0)
            {
                fromIndex = (int64)min(value, (double)(length - 1));
            }
            else if (value + length < 0)
            {
                return length;
            }
            else
            {
                fromIndex = (int64)(value + length);
            }
        }

        return fromIndex;
    }

    // includesAlgorithm specifies to follow ES7 Array.prototype.includes semantics instead of Array.prototype.indexOf
    // Differences
    //    1. Returns boolean true or false value instead of the search hit index
    //    2. Follows SameValueZero algorithm instead of StrictEquals
    //    3. Missing values are scanned if the search value is undefined

    template <bool includesAlgorithm>
    Var JavascriptArray::IndexOfHelper(Arguments const & args, ScriptContext *scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        RecyclableObject* obj = nullptr;
        JavascriptArray* pArr = nullptr;
        BigIndex length;
        Var trueValue = scriptContext->GetLibrary()->GetTrue();
        Var falseValue = scriptContext->GetLibrary()->GetFalse();

        JS_REENTRANT(jsReentLock, TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.indexOf"), &pArr, &obj, &length));

        Var search = nullptr;
        uint32 fromIndex = 0;
        uint64 fromIndex64 = 0;

        // The evaluation of method arguments may change the type of the array. Hence, we do that prior to the actual helper method calls.
        // The if clause of the conditional statement below applies to an JavascriptArray or TypedArray instances. The rest of the conditional
        // clauses apply to an ES5Array or other valid Javascript objects.
        if ((pArr || TypedArrayBase::Is(obj)) && (length.IsSmallIndex() || length.IsUint32Max()))
        {
            uint32 len = length.IsUint32Max() ? MaxArrayLength : length.GetSmallIndex();
            JS_REENTRANT(jsReentLock, BOOL gotParam = GetParamForIndexOf(len, args, search, fromIndex, scriptContext));
            if (!gotParam)
            {
                return includesAlgorithm ? falseValue : TaggedInt::ToVarUnchecked(-1);
            }
        }
        else if (length.IsSmallIndex())
        {
            JS_REENTRANT(jsReentLock, BOOL gotParam = GetParamForIndexOf(length.GetSmallIndex(), args, search, fromIndex, scriptContext));
            if (!gotParam)
            {
                return includesAlgorithm ? falseValue : TaggedInt::ToVarUnchecked(-1);
            }
        }
        else
        {
            JS_REENTRANT(jsReentLock, BOOL gotParam = GetParamForIndexOf(length.GetBigIndex(), args, search, fromIndex64, scriptContext));
            if (!gotParam)
            {
                return includesAlgorithm ? falseValue : TaggedInt::ToVarUnchecked(-1);
            }
        }

        // Side effects (such as defining a property in a ToPrimitive call) during evaluation of fromIndex argument may convert the array to an ES5 array.
        if (pArr && !JavascriptArray::Is(obj))
        {
            AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
            pArr = nullptr;
        }

        if (pArr)
        {
            if (length.IsSmallIndex() || length.IsUint32Max())
            {
                uint32 len = length.IsUint32Max() ? MaxArrayLength : length.GetSmallIndex();
                JS_REENTRANT(jsReentLock, int32 index = pArr->HeadSegmentIndexOfHelper(search, fromIndex, len, includesAlgorithm, scriptContext));

                // If we found the search value in the head segment, or if we determined there is no need to search other segments,
                // we stop right here.
                if (index != -1 || fromIndex == -1)
                {
                    if (includesAlgorithm)
                    {
                        //Array.prototype.includes
                        return (index == -1) ? falseValue : trueValue;
                    }
                    else
                    {
                        //Array.prototype.indexOf
                        return JavascriptNumber::ToVar(index, scriptContext);
                    }
                }

                //  If we really must search other segments, let's do it now. We'll have to search the slow way (dealing with holes, etc.).
                switch (pArr->GetTypeId())
                {
                case Js::TypeIds_Array:
                JS_REENTRANT_UNLOCK(jsReentLock, return TemplatedIndexOfHelper<includesAlgorithm>(pArr, search, fromIndex, len, scriptContext));
                case Js::TypeIds_NativeIntArray:
                JS_REENTRANT_UNLOCK(jsReentLock, return TemplatedIndexOfHelper<includesAlgorithm>(JavascriptNativeIntArray::FromVar(pArr), search, fromIndex, len, scriptContext));
                case Js::TypeIds_NativeFloatArray:
                JS_REENTRANT_UNLOCK(jsReentLock, return TemplatedIndexOfHelper<includesAlgorithm>(JavascriptNativeFloatArray::FromVar(pArr), search, fromIndex, len, scriptContext));
                default:
                    AssertMsg(FALSE, "invalid array typeid");
                JS_REENTRANT_UNLOCK(jsReentLock, return TemplatedIndexOfHelper<includesAlgorithm>(pArr, search, fromIndex, len, scriptContext));
                }
            }
        }

        // source object is not a JavascriptArray but source could be a TypedArray
        if (TypedArrayBase::Is(obj))
        {
            if (length.IsSmallIndex() || length.IsUint32Max())
            {
                JS_REENTRANT_UNLOCK(jsReentLock, return TemplatedIndexOfHelper<includesAlgorithm>(TypedArrayBase::FromVar(obj), search, fromIndex, length.GetSmallIndex(), scriptContext));
            }
        }
        if (length.IsSmallIndex())
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return TemplatedIndexOfHelper<includesAlgorithm>(obj, search, fromIndex, length.GetSmallIndex(), scriptContext));
        }
        else
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return TemplatedIndexOfHelper<includesAlgorithm>(obj, search, fromIndex64, length.GetBigIndex(), scriptContext));
        }
    }

    // Array.prototype.indexOf as defined in ES6.0 (final) Section 22.1.3.11
    Var JavascriptArray::EntryIndexOf(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Array_Prototype_indexOf);

        JS_REENTRANT_UNLOCK(jsReentLock, Var returnValue =  IndexOfHelper<false>(args, scriptContext));

        //IndexOfHelper code is reused for array.prototype.includes as well. Let us assert here we didn't get a true or false instead of index
        Assert(returnValue != scriptContext->GetLibrary()->GetTrue() && returnValue != scriptContext->GetLibrary()->GetFalse());

        return returnValue;
    }

    Var JavascriptArray::EntryIncludes(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Array_Prototype_includes);

        JS_REENTRANT(jsReentLock, Var returnValue = IndexOfHelper<true>(args, scriptContext));
        Assert(returnValue == scriptContext->GetLibrary()->GetTrue() || returnValue == scriptContext->GetLibrary()->GetFalse());

        return returnValue;
    }


    template<typename T>
    BOOL JavascriptArray::GetParamForIndexOf(T length, Arguments const& args, Var& search, T& fromIndex, ScriptContext * scriptContext)
    {
        if (length == 0)
        {
            return false;
        }

        if (args.Info.Count > 2)
        {
            fromIndex = GetFromIndex(args[2], length, scriptContext);
            if (fromIndex >= length)
            {
                return false;
            }
            search = args[1];
        }
        else
        {
            fromIndex = 0;
            search = args.Info.Count > 1 ? args[1] : scriptContext->GetLibrary()->GetUndefined();
        }
        return true;
    }

    template <>
    BOOL JavascriptArray::TemplatedGetItem(RecyclableObject * obj, uint32 index, Var * element, ScriptContext * scriptContext, bool checkHasItem)
    {
        // Note: Sometime cross site array go down this path to get the marshalling
        Assert(!VirtualTableInfo<JavascriptArray>::HasVirtualTable(obj)
            && !VirtualTableInfo<JavascriptNativeIntArray>::HasVirtualTable(obj)
            && !VirtualTableInfo<JavascriptNativeFloatArray>::HasVirtualTable(obj));
        if (checkHasItem && !JavascriptOperators::HasItem(obj, index))
        {
            return FALSE;
        }
        return JavascriptOperators::GetItem(obj, index, element, scriptContext);
    }

    template <>
    BOOL JavascriptArray::TemplatedGetItem(RecyclableObject * obj, uint64 index, Var * element, ScriptContext * scriptContext, bool checkHasItem)
    {
        // Note: Sometime cross site array go down this path to get the marshalling
        Assert(!VirtualTableInfo<JavascriptArray>::HasVirtualTable(obj)
            && !VirtualTableInfo<JavascriptNativeIntArray>::HasVirtualTable(obj)
            && !VirtualTableInfo<JavascriptNativeFloatArray>::HasVirtualTable(obj));
        PropertyRecord const * propertyRecord;
        JavascriptOperators::GetPropertyIdForInt(index, scriptContext, &propertyRecord);

        if (checkHasItem && !JavascriptOperators::HasProperty(obj, propertyRecord->GetPropertyId()))
        {
            return FALSE;
        }

        *element = JavascriptOperators::GetProperty(obj, propertyRecord->GetPropertyId(), scriptContext);
        return *element != scriptContext->GetLibrary()->GetUndefined();
    }

    template <>
    BOOL JavascriptArray::TemplatedGetItem(JavascriptArray *pArr, uint32 index, Var * element, ScriptContext * scriptContext, bool checkHasItem)
    {
        Assert(VirtualTableInfo<JavascriptArray>::HasVirtualTable(pArr)
            || VirtualTableInfo<CrossSiteObject<JavascriptArray>>::HasVirtualTable(pArr));
        return pArr->JavascriptArray::DirectGetItemAtFull(index, element);
    }
    template <>
    BOOL JavascriptArray::TemplatedGetItem(JavascriptArray *pArr, uint64 index, Var * element, ScriptContext * scriptContext, bool checkHasItem)
    {
        // This should never get called.
        Assert(false);
        Throw::InternalError();
    }

    template <>
    BOOL JavascriptArray::TemplatedGetItem(JavascriptNativeIntArray *pArr, uint32 index, Var * element, ScriptContext * scriptContext, bool checkHasItem)
    {
        Assert(VirtualTableInfo<JavascriptNativeIntArray>::HasVirtualTable(pArr)
            || VirtualTableInfo<CrossSiteObject<JavascriptNativeIntArray>>::HasVirtualTable(pArr));
        return pArr->JavascriptNativeIntArray::DirectGetItemAtFull(index, element);
    }

    template <>
    BOOL JavascriptArray::TemplatedGetItem(JavascriptNativeIntArray *pArr, uint64 index, Var * element, ScriptContext * scriptContext, bool checkHasItem)
    {
        // This should never get called.
        Assert(false);
        Throw::InternalError();
    }

    template <>
    BOOL JavascriptArray::TemplatedGetItem(JavascriptNativeFloatArray *pArr, uint32 index, Var * element, ScriptContext * scriptContext, bool checkHasItem)
    {
        Assert(VirtualTableInfo<JavascriptNativeFloatArray>::HasVirtualTable(pArr)
            || VirtualTableInfo<CrossSiteObject<JavascriptNativeFloatArray>>::HasVirtualTable(pArr));
        return pArr->JavascriptNativeFloatArray::DirectGetItemAtFull(index, element);
    }

    template <>
    BOOL JavascriptArray::TemplatedGetItem(JavascriptNativeFloatArray *pArr, uint64 index, Var * element, ScriptContext * scriptContext, bool checkHasItem)
    {
        // This should never get called.
        Assert(false);
        Throw::InternalError();
    }

    template <>
    BOOL JavascriptArray::TemplatedGetItem(TypedArrayBase * typedArrayBase, uint32 index, Var * element, ScriptContext * scriptContext, bool checkHasItem)
    {
        // We need to do explicit check for items since length value may not actually match the actual TypedArray length.
        // User could add a length property to a TypedArray instance which lies and returns a different value from the underlying length.
        // Since this method can be called via Array.prototype.indexOf with .apply or .call passing a TypedArray as this parameter
        // we don't know whether or not length == typedArrayBase->GetLength().
        if (checkHasItem && !typedArrayBase->HasItem(index))
        {
            return false;
        }

        *element = typedArrayBase->DirectGetItem(index);
        return true;
    }

    template <>
    BOOL JavascriptArray::TemplatedGetItem(TypedArrayBase * typedArrayBase, uint64 index, Var * element, ScriptContext * scriptContext, bool checkHasItem)
    {
        // This should never get called.
        Assert(false);
        Throw::InternalError();
    }

    template <bool includesAlgorithm, typename T, typename P>
    Var JavascriptArray::TemplatedIndexOfHelper(T * pArr, Var search, P fromIndex, P toIndex, ScriptContext * scriptContext)
    {
        Var element = nullptr;
        bool isSearchTaggedInt = TaggedInt::Is(search);
        bool doUndefinedSearch = includesAlgorithm && JavascriptOperators::GetTypeId(search) == TypeIds_Undefined;

        Var trueValue = scriptContext->GetLibrary()->GetTrue();
        Var falseValue = scriptContext->GetLibrary()->GetFalse();

        //Consider: enumerating instead of walking all indices
        for (P i = fromIndex; i < toIndex; i++)
        {
            if (!TryTemplatedGetItem(pArr, i, &element, scriptContext, !includesAlgorithm))
            {
                if (doUndefinedSearch)
                {
                    return trueValue;
                }
                continue;
            }

            if (isSearchTaggedInt && TaggedInt::Is(element))
            {
                if (element == search)
                {
                    return includesAlgorithm? trueValue : JavascriptNumber::ToVar(i, scriptContext);
                }
                continue;
            }

            if (includesAlgorithm)
            {
                //Array.prototype.includes
                if (JavascriptConversion::SameValueZero(element, search))
                {
                    return trueValue;
                }
            }
            else
            {
                //Array.prototype.indexOf
                if (JavascriptOperators::StrictEqual(element, search, scriptContext))
                {
                    return JavascriptNumber::ToVar(i, scriptContext);
                }
            }
        }

        return includesAlgorithm ? falseValue :  TaggedInt::ToVarUnchecked(-1);
    }

    int32 JavascriptArray::HeadSegmentIndexOfHelper(Var search, uint32 &fromIndex, uint32 toIndex, bool includesAlgorithm, ScriptContext * scriptContext)
    {
        Assert(Is(GetTypeId()) && !JavascriptNativeArray::Is(GetTypeId()));

        if (!HasNoMissingValues() || fromIndex >= GetHead()->length)
        {
            return -1;
        }

        bool isSearchTaggedInt = TaggedInt::Is(search);
        // We need to cast head segment to SparseArraySegment<Var> to have access to GetElement (onSparseArraySegment<T>). Because there are separate overloads of this
        // virtual method on JavascriptNativeIntArray and JavascriptNativeFloatArray, we know this version of this method will only be called for true JavascriptArray, and not for
        // either of the derived native arrays, so the elements of each segment used here must be Vars. Hence, the cast is safe.
        SparseArraySegment<Var>* head = static_cast<SparseArraySegment<Var>*>(GetHead());
        uint32 toIndexTrimmed = toIndex <= head->length ? toIndex : head->length;
        for (uint32 i = fromIndex; i < toIndexTrimmed; i++)
        {
            Var element = head->GetElement(i);
            if (isSearchTaggedInt && TaggedInt::Is(element))
            {
                if (search == element)
                {
                    return i;
                }
            }
            else if (includesAlgorithm && JavascriptConversion::SameValueZero(element, search))
            {
                //Array.prototype.includes
                return i;
            }
            else if (JavascriptOperators::StrictEqual(element, search, scriptContext))
            {
                //Array.prototype.indexOf
                return i;
            }
        }

        // Element not found in the head segment. Keep looking only if the range of indices extends past
        // the head segment.
        fromIndex = toIndex > GetHead()->length ? GetHead()->length : -1;
        return -1;
    }

    template<typename T>
    bool AreAllBytesEqual(T value)
    {
        byte* bValue = (byte*)&value;
        byte firstByte = *bValue++;
        for (int i = 1; i < sizeof(T); ++i)
        {
            if (*bValue++ != firstByte)
            {
                return false;
            }
        }
        return true;
    }

    template<>
    void JavascriptArray::CopyValueToSegmentBuferNoCheck(Field(double)* buffer, uint32 length, double value)
    {
        if (JavascriptNumber::IsZero(value) && !JavascriptNumber::IsNegZero(value))
        {
            memset(buffer, 0, sizeof(double) * length);
        }
        else
        {
            for (uint32 i = 0; i < length; i++)
            {
                buffer[i] = value;
            }
        }
    }

    template<>
    void JavascriptArray::CopyValueToSegmentBuferNoCheck(Field(int32)* buffer, uint32 length, int32 value)
    {
        if (value == 0 || AreAllBytesEqual(value))
        {
            memset(buffer, *(byte*)&value, sizeof(int32)* length);
        }
        else
        {
            for (uint32 i = 0; i < length; i++)
            {
                buffer[i] = value;
            }
        }
    }

    template<>
    void JavascriptArray::CopyValueToSegmentBuferNoCheck(Field(Js::Var)* buffer, uint32 length, Js::Var value)
    {
        for (uint32 i = 0; i < length; i++)
        {
            buffer[i] = value;
        }
    }

    int32 JavascriptNativeIntArray::HeadSegmentIndexOfHelper(Var search, uint32 &fromIndex, uint32 toIndex, bool includesAlgorithm,  ScriptContext * scriptContext)
    {
        // We proceed largely in the same manner as in JavascriptArray's version of this method (see comments there for more information),
        // except when we can further optimize thanks to the knowledge that all elements in the array are int32's. This allows for two additional optimizations:
        // 1. Only tagged ints or JavascriptNumbers that can be represented as int32 can be strict equal to some element in the array (all int32). Thus, if
        // the search value is some other kind of Var, we can return -1 without ever iterating over the elements.
        // 2. If the search value is a number that can be represented as int32, then we inspect the elements, but we don't need to perform the full strict equality algorithm.
        // Instead we can use simple C++ equality (which in case of such values is equivalent to strict equality in JavaScript).

        if (!HasNoMissingValues() || fromIndex >= GetHead()->length)
        {
            return -1;
        }

        bool isSearchTaggedInt = TaggedInt::Is(search);
        if (!isSearchTaggedInt && !JavascriptNumber::Is_NoTaggedIntCheck(search))
        {
            // The value can't be in the array, but it could be in a prototype, and we can only guarantee that
            // the head segment has no gaps.
            fromIndex = toIndex > GetHead()->length ? GetHead()->length : -1;
            return -1;
        }
        int32 searchAsInt32;
        if (isSearchTaggedInt)
        {
            searchAsInt32 = TaggedInt::ToInt32(search);
        }
        else if (!JavascriptNumber::TryGetInt32Value<true>(JavascriptNumber::GetValue(search), &searchAsInt32))
        {
            // The value can't be in the array, but it could be in a prototype, and we can only guarantee that
            // the head segment has no gaps.
            fromIndex = toIndex > GetHead()->length ? GetHead()->length : -1;
            return -1;
        }

        // We need to cast head segment to SparseArraySegment<int32> to have access to GetElement (onSparseArraySegment<T>). Because there are separate overloads of this
        // virtual method on JavascriptNativeIntArray and JavascriptNativeFloatArray, we know this version of this method will only be called for true JavascriptNativeIntArray, and not for
        // the other two, so the elements of each segment used here must be int32's. Hence, the cast is safe.

        SparseArraySegment<int32> * head = static_cast<SparseArraySegment<int32>*>(GetHead());
        uint32 toIndexTrimmed = toIndex <= head->length ? toIndex : head->length;
        for (uint32 i = fromIndex; i < toIndexTrimmed; i++)
        {
            int32 element = head->GetElement(i);
            if (searchAsInt32 == element)
            {
                return i;
            }
        }

        // Element not found in the head segment. Keep looking only if the range of indices extends past
        // the head segment.
        fromIndex = toIndex > GetHead()->length ? GetHead()->length : -1;
        return -1;
    }

    int32 JavascriptNativeFloatArray::HeadSegmentIndexOfHelper(Var search, uint32 &fromIndex, uint32 toIndex, bool includesAlgorithm, ScriptContext * scriptContext)
    {
        // We proceed largely in the same manner as in JavascriptArray's version of this method (see comments there for more information),
        // except when we can further optimize thanks to the knowledge that all elements in the array are doubles. This allows for two additional optimizations:
        // 1. Only tagged ints or JavascriptNumbers can be strict equal to some element in the array (all doubles). Thus, if
        // the search value is some other kind of Var, we can return -1 without ever iterating over the elements.
        // 2. If the search value is a number, then we inspect the elements, but we don't need to perform the full strict equality algorithm.
        // Instead we can use simple C++ equality (which in case of such values is equivalent to strict equality in JavaScript).

        if (!HasNoMissingValues() || fromIndex >= GetHead()->length)
        {
            return -1;
        }

        bool isSearchTaggedInt = TaggedInt::Is(search);
        if (!isSearchTaggedInt && !JavascriptNumber::Is_NoTaggedIntCheck(search))
        {
            // The value can't be in the array, but it could be in a prototype, and we can only guarantee that
            // the head segment has no gaps.
            fromIndex = toIndex > GetHead()->length ? GetHead()->length : -1;
            return -1;
        }

        double searchAsDouble = isSearchTaggedInt ? TaggedInt::ToDouble(search) : JavascriptNumber::GetValue(search);

        // We need to cast head segment to SparseArraySegment<double> to have access to GetElement (SparseArraySegment). We know the
        // segment's elements are all Vars so the cast is safe. It would have been more convenient here if JavascriptArray
        // used SparseArraySegment<Var>, instead of SparseArraySegmentBase.


        SparseArraySegment<double> * head = static_cast<SparseArraySegment<double>*>(GetHead());
        uint32 toIndexTrimmed = toIndex <= head->length ? toIndex : head->length;

        bool matchNaN = includesAlgorithm && JavascriptNumber::IsNan(searchAsDouble);

        for (uint32 i = fromIndex; i < toIndexTrimmed; i++)
        {
            double element = head->GetElement(i);

            if (element == searchAsDouble)
            {
                return i;
            }

            //NaN != NaN we expect to match for NaN in Array.prototype.includes algorithm
            if (matchNaN && JavascriptNumber::IsNan(element))
            {
                return i;
            }

        }

        fromIndex = toIndex > GetHead()->length ? GetHead()->length : -1;
        return -1;
    }

    Var JavascriptArray::EntryJoin(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.join"));
        }

        JavascriptString* separator;
        if (args.Info.Count >= 2)
        {
            TypeId typeId = JavascriptOperators::GetTypeId(args[1]);
            //ES5 15.4.4.5 If separator is undefined, let separator be the single-character String ",".
            if (TypeIds_Undefined != typeId)
            {
                JS_REENTRANT(jsReentLock, separator = JavascriptConversion::ToString(args[1], scriptContext));
            }
            else
            {
                separator = scriptContext->GetLibrary()->GetCommaDisplayString();
            }
        }
        else
        {
            separator = scriptContext->GetLibrary()->GetCommaDisplayString();
        }

        JS_REENTRANT_UNLOCK(jsReentLock, return JoinHelper(args[0], separator, scriptContext));
    }

    JavascriptString* JavascriptArray::JoinToString(Var value, ScriptContext* scriptContext)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(value);
        if (typeId == TypeIds_Null || typeId == TypeIds_Undefined)
        {
            return scriptContext->GetLibrary()->GetEmptyString();
        }
        else
        {
            return JavascriptConversion::ToString(value, scriptContext);
        }
    }

    JavascriptString* JavascriptArray::JoinHelper(Var thisArg, JavascriptString* separator, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        bool isArray = JavascriptArray::Is(thisArg) && (scriptContext == JavascriptArray::FromVar(thisArg)->GetScriptContext());
        bool isProxy = JavascriptProxy::Is(thisArg) && (scriptContext == JavascriptProxy::FromVar(thisArg)->GetScriptContext());
        Var target = NULL;
        bool isTargetObjectPushed = false;
        // if we are visiting a proxy object, track that we have visited the target object as well so the next time w
        // call the join helper for the target of this proxy, we will return above.
        if (isProxy)
        {
            JavascriptProxy* proxy = JavascriptProxy::FromVar(thisArg);
            Assert(proxy);
            target = proxy->GetTarget();
            if (target != nullptr)
            {
                // If we end up joining same array, instead of going in infinite loop, return the empty string
                if (scriptContext->CheckObject(target))
                {
                    return scriptContext->GetLibrary()->GetEmptyString();
                }
                else
                {
                    scriptContext->PushObject(target);
                    isTargetObjectPushed = true;
                }
            }
        }
        // If we end up joining same array, instead of going in infinite loop, return the empty string
        else if (scriptContext->CheckObject(thisArg))
        {
            return scriptContext->GetLibrary()->GetEmptyString();
        }

        if (!isTargetObjectPushed)
        {
            scriptContext->PushObject(thisArg);
        }

        JavascriptString* res = nullptr;

        TryFinally([&]()
        {
            if (isArray)
            {
#if ENABLE_COPYONACCESS_ARRAY
                JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray(thisArg);
#endif
                JavascriptArray * arr = JavascriptArray::FromVar(thisArg);
                switch (arr->GetTypeId())
                {
                case Js::TypeIds_Array:
                    JS_REENTRANT(jsReentLock, res = JoinArrayHelper(arr, separator, scriptContext));
                    break;
                case Js::TypeIds_NativeIntArray:
                    JS_REENTRANT(jsReentLock, res = JoinArrayHelper(JavascriptNativeIntArray::FromVar(arr), separator, scriptContext));
                    break;
                case Js::TypeIds_NativeFloatArray:
                    JS_REENTRANT(jsReentLock, res = JoinArrayHelper(JavascriptNativeFloatArray::FromVar(arr), separator, scriptContext));
                    break;
                }

            }
            else if (RecyclableObject::Is(thisArg))
            {
                JS_REENTRANT(jsReentLock, res = JoinOtherHelper(RecyclableObject::FromVar(thisArg), separator, scriptContext));
            }
            else
            {
                JS_REENTRANT(jsReentLock, res = JoinOtherHelper(scriptContext->GetLibrary()->CreateNumberObject(thisArg), separator, scriptContext));
            }
        },
        [&](bool/*hasException*/)
        {
            Var top = scriptContext->PopObject();
            if (isProxy)
            {
                AssertMsg(top == target, "Unmatched operation stack");
            }
            else
            {
                AssertMsg(top == thisArg, "Unmatched operation stack");
            }
        });

        if (res == nullptr)
        {
            res = scriptContext->GetLibrary()->GetEmptyString();
        }

        return res;
    }

    static const charcount_t Join_MaxEstimatedAppendCount = static_cast<charcount_t>((64 << 20) / sizeof(void *)); // 64 MB worth of pointers

    template <typename T>
    JavascriptString* JavascriptArray::JoinArrayHelper(T * arr, JavascriptString* separator, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(VirtualTableInfo<T>::HasVirtualTable(arr) || VirtualTableInfo<CrossSiteObject<T>>::HasVirtualTable(arr));
        const uint32 arrLength = arr->length;
        switch(arrLength)
        {
            default:
            {
CaseDefault:
                bool hasSeparator = (separator->GetLength() != 0);
                const charcount_t estimatedAppendCount =
                    min(
                        Join_MaxEstimatedAppendCount,
                        static_cast<charcount_t>(arrLength + (hasSeparator ? arrLength - 1 : 0)));
                CompoundString *const cs =
                    CompoundString::NewWithPointerCapacity(estimatedAppendCount, scriptContext->GetLibrary());
                Var item = nullptr;
                BOOL gotItem;
                JS_REENTRANT(jsReentLock, gotItem = TemplatedGetItem(arr, 0u, &item, scriptContext));
                if (gotItem)
                {
                    JS_REENTRANT(jsReentLock, cs->Append(JavascriptArray::JoinToString(item, scriptContext)));
                }

                for (uint32 i = 1; i < arrLength; i++)
                {
                    if (hasSeparator)
                    {
                        cs->Append(separator);
                    }

                    JS_REENTRANT(jsReentLock, gotItem = TryTemplatedGetItem(arr, i, &item, scriptContext));
                    if (gotItem)
                    {
                        JS_REENTRANT(jsReentLock, cs->Append(JavascriptArray::JoinToString(item, scriptContext)));
                    }
                }
                return cs;
            }

            case 2:
            {
                bool hasSeparator = (separator->GetLength() != 0);
                BOOL gotItem;
                if(hasSeparator)
                {
                    goto CaseDefault;
                }

                JavascriptString *res = nullptr;
                Var item = nullptr;

                JS_REENTRANT(jsReentLock, gotItem = TemplatedGetItem(arr, 0u, &item, scriptContext));
                if (gotItem)
                {
                    JS_REENTRANT(jsReentLock, res = JavascriptArray::JoinToString(item, scriptContext));
                }
                JS_REENTRANT(jsReentLock, gotItem = TryTemplatedGetItem(arr, 1u, &item, scriptContext));
                if (gotItem)
                {
                    JS_REENTRANT(jsReentLock, JavascriptString *const itemString = JavascriptArray::JoinToString(item, scriptContext));
                    return res ? ConcatString::New(res, itemString) : itemString;
                }

                if(res)
                {
                    return res;
                }

                goto Case0;
            }

            case 1:
            {
                Var item = nullptr;
                BOOL gotItem;
                JS_REENTRANT(jsReentLock, gotItem = TemplatedGetItem(arr, 0u, &item, scriptContext));
                if (gotItem)
                {
                    JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::JoinToString(item, scriptContext));
                }
                // fall through
            }

            case 0:
Case0:
                return scriptContext->GetLibrary()->GetEmptyString();
        }
    }

    JavascriptString* JavascriptArray::JoinOtherHelper(RecyclableObject* object, JavascriptString* separator, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        // In ES6-mode, we always load the length property from the object instead of using the internal slot.
        // Even for arrays, this is now observable via proxies.
        // If source object is not an array, we fall back to this behavior anyway.
        JS_REENTRANT(jsReentLock,
            int64 cSrcLength = (int64)OP_GetLength(object, scriptContext));

        switch (cSrcLength)
        {
            default:
            {
CaseDefault:
                bool hasSeparator = (separator->GetLength() != 0);
                BOOL gotItem;
                const charcount_t estimatedAppendCount =
                    min(
                        Join_MaxEstimatedAppendCount,
                        static_cast<charcount_t>(cSrcLength + (hasSeparator ? cSrcLength - 1 : 0)));
                CompoundString *const cs =
                    CompoundString::NewWithPointerCapacity(estimatedAppendCount, scriptContext->GetLibrary());
                Var value = nullptr;
                JS_REENTRANT(jsReentLock, gotItem = JavascriptOperators::GetItem(object, 0u, &value, scriptContext));
                if (gotItem)
                {
                    JS_REENTRANT(jsReentLock, cs->Append(JavascriptArray::JoinToString(value, scriptContext)));
                }
                for (uint32 i = 1; i < cSrcLength; i++)
                {
                    if (hasSeparator)
                    {
                        cs->Append(separator);
                    }
                    JS_REENTRANT(jsReentLock, gotItem = JavascriptOperators::GetItem(object, i, &value, scriptContext));
                    if (gotItem)
                    {
                        JS_REENTRANT(jsReentLock, cs->Append(JavascriptArray::JoinToString(value, scriptContext)));
                    }
                }
                return cs;
            }

            case 2:
            {
                bool hasSeparator = (separator->GetLength() != 0);
                BOOL gotItem;
                if(hasSeparator)
                {
                    goto CaseDefault;
                }

                JavascriptString *res = nullptr;
                Var value = nullptr;
                JS_REENTRANT(jsReentLock, gotItem = JavascriptOperators::GetItem(object, 0u, &value, scriptContext));
                if (gotItem)
                {
                    JS_REENTRANT(jsReentLock, res = JavascriptArray::JoinToString(value, scriptContext));
                }
                JS_REENTRANT(jsReentLock, gotItem = JavascriptOperators::GetItem(object, 1u, &value, scriptContext));
                if (gotItem)
                {
                    JS_REENTRANT(jsReentLock, JavascriptString *const valueString = JavascriptArray::JoinToString(value, scriptContext));
                    return res ? ConcatString::New(res, valueString) : valueString;
                }
                if(res)
                {
                    return res;
                }
                goto Case0;
            }

            case 1:
            {
                Var value = nullptr;
                JS_REENTRANT(jsReentLock, BOOL gotItem = JavascriptOperators::GetItem(object, 0u, &value, scriptContext));
                if (gotItem)
                {
                    JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::JoinToString(value, scriptContext));
                }
                // fall through
            }

            case 0:
Case0:
                return scriptContext->GetLibrary()->GetEmptyString();
        }
    }

    Var JavascriptArray::EntryLastIndexOf(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Array_Prototype_lastIndexOf);

        Assert(!(callInfo.Flags & CallFlags_New));

        int64 length;
        JavascriptArray * pArr = nullptr;
        RecyclableObject* obj = nullptr;

        JS_REENTRANT(jsReentLock, TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.lastIndexOf"), &pArr, &obj, &length));

        Var search = nullptr;
        int64 fromIndex;
        JS_REENTRANT(jsReentLock,
            BOOL gotParam = GetParamForLastIndexOf(length, args, search, fromIndex, scriptContext));
        if (!gotParam)
        {
            return TaggedInt::ToVarUnchecked(-1);
        }

        // Side effects (such as defining a property in a ToPrimitive call) during evaluation of fromIndex argument may convert the array to an ES5 array.
        if (pArr && !JavascriptArray::Is(obj))
        {
            AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
            pArr = nullptr;
        }

        if (pArr)
        {
            switch (pArr->GetTypeId())
            {
            case Js::TypeIds_Array:
                JS_REENTRANT_UNLOCK(jsReentLock, return LastIndexOfHelper(pArr, search, fromIndex, scriptContext));
            case Js::TypeIds_NativeIntArray:
                JS_REENTRANT_UNLOCK(jsReentLock, return LastIndexOfHelper(JavascriptNativeIntArray::FromVar(pArr), search, fromIndex, scriptContext));
            case Js::TypeIds_NativeFloatArray:
                JS_REENTRANT_UNLOCK(jsReentLock, return LastIndexOfHelper(JavascriptNativeFloatArray::FromVar(pArr), search, fromIndex, scriptContext));
            default:
                AssertMsg(FALSE, "invalid array typeid");
                JS_REENTRANT_UNLOCK(jsReentLock, return LastIndexOfHelper(pArr, search, fromIndex, scriptContext));
            }
        }

        // source object is not a JavascriptArray but source could be a TypedArray
        if (TypedArrayBase::Is(obj))
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return LastIndexOfHelper(TypedArrayBase::FromVar(obj), search, fromIndex, scriptContext));
        }

        JS_REENTRANT_UNLOCK(jsReentLock, return LastIndexOfHelper(obj, search, fromIndex, scriptContext));
    }

    // Array.prototype.lastIndexOf as described in ES6.0 (draft 22) Section 22.1.3.14
    BOOL JavascriptArray::GetParamForLastIndexOf(int64 length, Arguments const & args, Var& search, int64& fromIndex, ScriptContext * scriptContext)
    {
        if (length == 0)
        {
            return false;
        }

        if (args.Info.Count > 2)
        {
            fromIndex = GetFromLastIndex(args[2], length, scriptContext);

            if (fromIndex >= length)
            {
                return false;
            }
            search = args[1];
        }
        else
        {
            search = args.Info.Count > 1 ? args[1] : scriptContext->GetLibrary()->GetUndefined();
            fromIndex = length - 1;
        }
        return true;
    }

    template <typename T>
    Var JavascriptArray::LastIndexOfHelper(T* pArr, Var search, int64 fromIndex, ScriptContext * scriptContext)
    {
        Var element = nullptr;
        bool isSearchTaggedInt = TaggedInt::Is(search);

        // First handle the indices > 2^32
        while (fromIndex >= MaxArrayLength)
        {
            Var index = JavascriptNumber::ToVar(fromIndex, scriptContext);

            if (JavascriptOperators::OP_HasItem(pArr, index, scriptContext))
            {
                element = JavascriptOperators::OP_GetElementI(pArr, index, scriptContext);

                if (isSearchTaggedInt && TaggedInt::Is(element))
                {
                    if (element == search)
                    {
                        return index;
                    }
                    fromIndex--;
                    continue;
                }

                if (JavascriptOperators::StrictEqual(element, search, scriptContext))
                {
                    return index;
                }
            }

            fromIndex--;
        }

        Assert(fromIndex < MaxArrayLength);

        // fromIndex now has to be < MaxArrayLength so casting to uint32 is safe
        uint32 end = static_cast<uint32>(fromIndex);

        for (uint32 i = 0; i <= end; i++)
        {
            uint32 index = end - i;

            if (!TryTemplatedGetItem(pArr, index, &element, scriptContext))
            {
                continue;
            }

            if (isSearchTaggedInt && TaggedInt::Is(element))
            {
                if (element == search)
                {
                    return JavascriptNumber::ToVar(index, scriptContext);
                }
                continue;
            }

            if (JavascriptOperators::StrictEqual(element, search, scriptContext))
            {
                return JavascriptNumber::ToVar(index, scriptContext);
            }
        }

        return TaggedInt::ToVarUnchecked(-1);
    }

    /*
    *   PopWithNoDst
    *   - For pop calls that do not return a value, we only need to decrement the length of the array.
    */
    void JavascriptNativeArray::PopWithNoDst(Var nativeArray)
    {
        Assert(JavascriptNativeArray::Is(nativeArray));
        JavascriptArray * arr = JavascriptArray::FromVar(nativeArray);

        // we will bailout on length 0
        Assert(arr->GetLength() != 0);

        uint32 index = arr->GetLength() - 1;
        arr->SetLength(index);
    }

    /*
    *   JavascriptNativeIntArray::Pop
    *   -   Returns int32 value from the array.
    *   -   Returns missing item when the element is not available in the array object.
    *   -   It doesn't walk up the prototype chain.
    *   -   Length is decremented only if it pops an int32 element, in all other cases - we bail out from the jitted code.
    *   -   This api cannot cause any implicit call and hence do not need implicit call bailout test around this api
    */
    int32 JavascriptNativeIntArray::Pop(ScriptContext * scriptContext, Var object)
    {
        Assert(JavascriptNativeIntArray::Is(object));
        JavascriptNativeIntArray * arr = JavascriptNativeIntArray::FromVar(object);

        Assert(arr->GetLength() != 0);

        uint32 index = arr->length - 1;

        int32 element = Js::JavascriptOperators::OP_GetNativeIntElementI_UInt32(object, index, scriptContext);

        //If it is a missing item, then don't update the length - Pre-op Bail out will happen.
        if(!SparseArraySegment<int32>::IsMissingItem(&element))
        {
            arr->SetLength(index);
        }
        return element;
    }

    /*
    *   JavascriptNativeFloatArray::Pop
    *   -   Returns double value from the array.
    *   -   Returns missing item when the element is not available in the array object.
    *   -   It doesn't walk up the prototype chain.
    *   -   Length is decremented only if it pops a double element, in all other cases - we bail out from the jitted code.
    *   -   This api cannot cause any implicit call and hence do not need implicit call bailout test around this api
    */
    double JavascriptNativeFloatArray::Pop(ScriptContext * scriptContext, Var object)
    {
        Assert(JavascriptNativeFloatArray::Is(object));
        JavascriptNativeFloatArray * arr = JavascriptNativeFloatArray::FromVar(object);

        Assert(arr->GetLength() != 0);

        uint32 index = arr->length - 1;

        double element = Js::JavascriptOperators::OP_GetNativeFloatElementI_UInt32(object, index, scriptContext);

        // If it is a missing item then don't update the length - Pre-op Bail out will happen.
        if(!SparseArraySegment<double>::IsMissingItem(&element))
        {
            arr->SetLength(index);
        }
        return element;
    }

    /*
    *   JavascriptArray::Pop
    *   -   Calls the generic Pop API, which can find elements from the prototype chain, when it is not available in the array object.
    *   -   This API may cause implicit calls. Handles Array and non-array objects
    */
    Var JavascriptArray::Pop(ScriptContext * scriptContext, Var object)
    {
        if (JavascriptArray::Is(object))
        {
            return EntryPopJavascriptArray(scriptContext, JavascriptArray::FromVar(object));
        }
        else
        {
            return EntryPopNonJavascriptArray(scriptContext, object);
        }
    }

    Var JavascriptArray::EntryPopJavascriptArray(ScriptContext * scriptContext, JavascriptArray* arr)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());
        uint32 length = arr->length;

        if (length == 0)
        {
            // If length is 0, return 'undefined'
            return scriptContext->GetLibrary()->GetUndefined();
        }

        uint32 index = length - 1;
        Var element = nullptr;
        JS_REENTRANT(jsReentLock, BOOL gotItem = arr->DirectGetItemAtFull(index, &element));

        if (!gotItem)
        {
            element = scriptContext->GetLibrary()->GetUndefined();
        }
        else
        {
            element = CrossSite::MarshalVar(scriptContext, element);
        }
        arr->SetLength(index); // SetLength will clear element at index

#ifdef VALIDATE_ARRAY
        arr->ValidateArray();
#endif
        return element;
    }

    Var JavascriptArray::EntryPopNonJavascriptArray(ScriptContext * scriptContext, Var object)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());
        RecyclableObject* dynamicObject = nullptr;
        if (FALSE == JavascriptConversion::ToObject(object, scriptContext, &dynamicObject))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.pop"));
        }
        JS_REENTRANT(jsReentLock, BigIndex length = OP_GetLength(dynamicObject, scriptContext));

        ThrowTypeErrorOnFailureHelper h(scriptContext, _u("Array.prototype.pop"));
        if (length == 0u)
        {
            // Set length = 0
            JS_REENTRANT(jsReentLock,
                h.ThrowTypeErrorOnFailure(JavascriptOperators::SetProperty(dynamicObject, dynamicObject, PropertyIds::length, TaggedInt::ToVarUnchecked(0), scriptContext, PropertyOperation_ThrowIfNotExtensible)));
            return scriptContext->GetLibrary()->GetUndefined();
        }
        BigIndex index = length;
        --index;
        Var element = nullptr;
        if (index.IsSmallIndex())
        {
            JS_REENTRANT(jsReentLock, BOOL gotItem = JavascriptOperators::GetItem(dynamicObject, index.GetSmallIndex(), &element, scriptContext));
            if (!gotItem)
            {
                element = scriptContext->GetLibrary()->GetUndefined();
            }
            JS_REENTRANT(jsReentLock,
                h.ThrowTypeErrorOnFailure(JavascriptOperators::DeleteItem(dynamicObject, index.GetSmallIndex(), PropertyOperation_ThrowOnDeleteIfNotConfig)),
                // Set the new length
                h.ThrowTypeErrorOnFailure(JavascriptOperators::SetProperty(dynamicObject, dynamicObject, PropertyIds::length, JavascriptNumber::ToVar(index.GetSmallIndex(), scriptContext), scriptContext, PropertyOperation_ThrowIfNotExtensible)));
        }
        else
        {
            JS_REENTRANT(jsReentLock, BOOL gotItem = JavascriptOperators::GetItem(dynamicObject, index.GetBigIndex(), &element, scriptContext));
            if (!gotItem)
            {
                element = scriptContext->GetLibrary()->GetUndefined();
            }
            JS_REENTRANT(jsReentLock, h.ThrowTypeErrorOnFailure(JavascriptOperators::DeleteItem(dynamicObject, index.GetBigIndex(), PropertyOperation_ThrowOnDeleteIfNotConfig)),
                // Set the new length
                h.ThrowTypeErrorOnFailure(JavascriptOperators::SetProperty(dynamicObject, dynamicObject, PropertyIds::length, JavascriptNumber::ToVar(index.GetBigIndex(), scriptContext), scriptContext, PropertyOperation_ThrowIfNotExtensible)));
        }
        return element;
    }

    Var JavascriptArray::EntryPop(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.pop"));
        }

        if (JavascriptArray::Is(args[0]))
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return EntryPopJavascriptArray(scriptContext, JavascriptArray::FromVar(args.Values[0])));
        }
        else
        {

            JS_REENTRANT_UNLOCK(jsReentLock, return EntryPopNonJavascriptArray(scriptContext, args.Values[0]));
        }
    }

    /*
    *   JavascriptNativeIntArray::Push
    *   Pushes Int element in a native Int Array.
    *   We call the generic Push, if the array is not native Int or we have a really big array.
    */
    Var JavascriptNativeIntArray::Push(ScriptContext * scriptContext, Var array, int value)
    {
        // Handle non crossSite native int arrays here length within MaxArrayLength.
        // JavascriptArray::Push will handle other cases.
        if (JavascriptNativeIntArray::IsNonCrossSite(array))
        {
            JavascriptNativeIntArray * nativeIntArray = JavascriptNativeIntArray::FromVar(array);
            Assert(!nativeIntArray->IsCrossSiteObject());
            uint32 n = nativeIntArray->length;

            if(n < JavascriptArray::MaxArrayLength)
            {
                nativeIntArray->SetItem(n, value);

                n++;

                AssertMsg(n == nativeIntArray->length, "Wrong update to the length of the native Int array");

                return JavascriptNumber::ToVar(n, scriptContext);
            }
        }
        return JavascriptArray::Push(scriptContext, array, JavascriptNumber::ToVar(value, scriptContext));
    }

    /*
    *   JavascriptNativeFloatArray::Push
    *   Pushes Float element in a native Int Array.
    *   We call the generic Push, if the array is not native Float or we have a really big array.
    */
    Var JavascriptNativeFloatArray::Push(ScriptContext * scriptContext, Var * array, double value)
    {
        // Handle non crossSite native int arrays here length within MaxArrayLength.
        // JavascriptArray::Push will handle other cases.
        if(JavascriptNativeFloatArray::IsNonCrossSite(array))
        {
            JavascriptNativeFloatArray * nativeFloatArray = JavascriptNativeFloatArray::FromVar(array);
            Assert(!nativeFloatArray->IsCrossSiteObject());
            uint32 n = nativeFloatArray->length;

            if(n < JavascriptArray::MaxArrayLength)
            {
                nativeFloatArray->SetItem(n, value);

                n++;

                AssertMsg(n == nativeFloatArray->length, "Wrong update to the length of the native Float array");
                return JavascriptNumber::ToVar(n, scriptContext);
            }
        }

        return JavascriptArray::Push(scriptContext, array, JavascriptNumber::ToVarNoCheck(value, scriptContext));
    }

    /*
    *   JavascriptArray::Push
    *   Pushes Var element in a Var Array.
    */
    Var JavascriptArray::Push(ScriptContext * scriptContext, Var object, Var value)
    {
        Var args[2];
        args[0] = object;
        args[1] = value;

        if (JavascriptArray::Is(object))
        {
            return EntryPushJavascriptArray(scriptContext, args, 2);
        }
        else
        {
            return EntryPushNonJavascriptArray(scriptContext, args, 2);
        }

    }

    /*
    *   EntryPushNonJavascriptArray
    *   - Handles Entry push calls, when Objects are not javascript arrays
    */
    Var JavascriptArray::EntryPushNonJavascriptArray(ScriptContext * scriptContext, Var * args, uint argCount)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        RecyclableObject* obj = nullptr;
        if (FALSE == JavascriptConversion::ToObject(args[0], scriptContext, &obj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.push"));
        }

        JS_REENTRANT_UNLOCK(jsReentLock, Var length = JavascriptOperators::OP_GetLength(obj, scriptContext));
        if(JavascriptOperators::GetTypeId(length) == TypeIds_Undefined && scriptContext->GetThreadContext()->IsDisableImplicitCall() &&
            scriptContext->GetThreadContext()->GetImplicitCallFlags() != Js::ImplicitCall_None)
        {
            return length;
        }

        ThrowTypeErrorOnFailureHelper h(scriptContext, _u("Array.prototype.push"));
        BigIndex n;
        if (scriptContext->GetConfig()->IsES6ToLengthEnabled())
        {
            n = (uint64) JavascriptConversion::ToLength(length, scriptContext);
        }
        else
        {
            n = JavascriptConversion::ToUInt32(length, scriptContext);
        }
        // First handle "small" indices.
        uint index;
        for (index=1; index < argCount && n < JavascriptArray::MaxArrayLength; ++index, ++n)
        {
            JS_REENTRANT(jsReentLock,
                BOOL setItem = JavascriptOperators::SetItem(obj, obj, n.GetSmallIndex(), args[index], scriptContext, PropertyOperation_ThrowIfNotExtensible));
            if (h.IsThrowTypeError(setItem))
            {
                if (scriptContext->GetThreadContext()->RecordImplicitException())
                {
                    h.ThrowTypeErrorOnFailure();
                }
                else
                {
                    return nullptr;
                }
            }
        }

        // Use BigIndex if we need to push indices >= MaxArrayLength
        if (index < argCount)
        {
            BigIndex big = n;

            for (; index < argCount; ++index, ++big)
            {
                JS_REENTRANT(jsReentLock, BOOL setItem = big.SetItem(obj, args[index], PropertyOperation_ThrowIfNotExtensible));
                if (h.IsThrowTypeError(setItem))
                {
                    if(scriptContext->GetThreadContext()->RecordImplicitException())
                    {
                        h.ThrowTypeErrorOnFailure();
                    }
                    else
                    {
                        return nullptr;
                    }
                }

            }

            // Set the new length; for objects it is all right for this to be >= MaxArrayLength
            JS_REENTRANT(jsReentLock,
                BOOL setLength = JavascriptOperators::SetProperty(obj, obj, PropertyIds::length, big.ToNumber(scriptContext), scriptContext, PropertyOperation_ThrowIfNotExtensible));
            if (h.IsThrowTypeError(setLength))
            {
                if(scriptContext->GetThreadContext()->RecordImplicitException())
                {
                    h.ThrowTypeErrorOnFailure();
                }
                else
                {
                    return nullptr;
                }
            }

            return big.ToNumber(scriptContext);
        }
        else
        {
            // Set the new length
            Var lengthAsNUmberVar = JavascriptNumber::ToVar(n.IsSmallIndex() ? n.GetSmallIndex() : n.GetBigIndex(), scriptContext);
            JS_REENTRANT(jsReentLock,
                BOOL setLength = JavascriptOperators::SetProperty(obj, obj, PropertyIds::length, lengthAsNUmberVar, scriptContext, PropertyOperation_ThrowIfNotExtensible));
            if (h.IsThrowTypeError(setLength))
            {
                if(scriptContext->GetThreadContext()->RecordImplicitException())
                {
                    h.ThrowTypeErrorOnFailure();
                }
                else
                {
                    return nullptr;
                }
            }

            return lengthAsNUmberVar;
        }
    }

    /*
    *   JavascriptArray::EntryPushJavascriptArray
    *   Pushes Var element in a Var Array.
    *   Returns the length of the array.
    */
    Var JavascriptArray::EntryPushJavascriptArray(ScriptContext * scriptContext, Var * args, uint argCount)
    {
        JavascriptArray * arr = JavascriptArray::FromAnyArray(args[0]);
        uint n = arr->length;
        ThrowTypeErrorOnFailureHelper h(scriptContext, _u("Array.prototype.push"));

        // Fast Path for one push for small indexes
        if (argCount == 2 && n < JavascriptArray::MaxArrayLength)
        {
            // Set Item is overridden by CrossSiteObject, so no need to check for IsCrossSiteObject()
            h.ThrowTypeErrorOnFailure(arr->SetItem(n, args[1], PropertyOperation_None));
            return JavascriptNumber::ToVar(n + 1, scriptContext);
        }

        // Fast Path for multiple push for small indexes
        if (JavascriptArray::MaxArrayLength - argCount + 1 > n && JavascriptArray::IsVarArray(arr) && scriptContext == arr->GetScriptContext())
        {
            uint index;
            for (index = 1; index < argCount; ++index, ++n)
            {
                Assert(n != JavascriptArray::MaxArrayLength);
                // Set Item is overridden by CrossSiteObject, so no need to check for IsCrossSiteObject()
                arr->JavascriptArray::DirectSetItemAt(n, args[index]);
            }
            return JavascriptNumber::ToVar(n, scriptContext);
        }

        return EntryPushJavascriptArrayNoFastPath(scriptContext, args, argCount);
    }

    Var JavascriptArray::EntryPushJavascriptArrayNoFastPath(ScriptContext * scriptContext, Var * args, uint argCount)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());
        JavascriptArray * arr = JavascriptArray::FromAnyArray(args[0]);
        uint n = arr->length;
        ThrowTypeErrorOnFailureHelper h(scriptContext, _u("Array.prototype.push"));

        // First handle "small" indices.
        uint index;
        for (index = 1; index < argCount && n < JavascriptArray::MaxArrayLength; ++index, ++n)
        {
            // Set Item is overridden by CrossSiteObject, so no need to check for IsCrossSiteObject()
            h.ThrowTypeErrorOnFailure(arr->SetItem(n, args[index], PropertyOperation_None));
        }

        // Use BigIndex if we need to push indices >= MaxArrayLength
        if (index < argCount)
        {
            // Not supporting native array with BigIndex.
            arr = EnsureNonNativeArray(arr);
            Assert(n == JavascriptArray::MaxArrayLength);
            for (BigIndex big = n; index < argCount; ++index, ++big)
            {
                JS_REENTRANT(jsReentLock, h.ThrowTypeErrorOnFailure(big.SetItem(arr, args[index])));
            }

#ifdef VALIDATE_ARRAY
            arr->ValidateArray();
#endif
            // This is where we should set the length, but for arrays it cannot be >= MaxArrayLength
            JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthAssignIncorrect);
        }

#ifdef VALIDATE_ARRAY
        arr->ValidateArray();
#endif
        return JavascriptNumber::ToVar(n, scriptContext);
    }

    /*
    *   JavascriptArray::EntryPush
    *   Handles Push calls(Script Function)
    */
    Var JavascriptArray::EntryPush(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.push"));
        }

        if (JavascriptArray::Is(args[0]))
        {
            return EntryPushJavascriptArray(scriptContext, args.Values, args.Info.Count);
        }
        else
        {
            return EntryPushNonJavascriptArray(scriptContext, args.Values, args.Info.Count);
        }
    }

    template <typename T>
    void JavascriptArray::CopyHeadIfInlinedHeadSegment(JavascriptArray *array, Recycler *recycler)
    {
        if (JavascriptArray::IsInlineSegment(array->head, array))
        {
            AnalysisAssert(array->head);
            SparseArraySegment<T>* newHeadSeg = array->ReallocNonLeafSegment((SparseArraySegment<T>*)PointerValue(array->head), array->head->next);
            array->head = newHeadSeg;
        }
    }

    template <typename T>
    void JavascriptArray::ReallocateNonLeafLastSegmentIfLeaf(JavascriptArray * arr, Recycler * recycler)
    {
        Assert(arr->head && arr->head->next); // Doesn't make sense to reallocate a leaf last segment as a non-leaf if its not going to point to any other segments.

        // TODO: Consider utilizing lastUsedSegment once we fix CopyHeadIfInlinedHeadSegment in that respect.
        SparseArraySegmentBase *lastSeg = nullptr;
        SparseArraySegmentBase *seg = arr->head;
        while (seg)
        {
            lastSeg = seg;
            seg = seg->next;
        }

        if (SparseArraySegmentBase::IsLeafSegment(lastSeg, recycler))
        {
            AnalysisAssert(lastSeg);
            arr->ReallocNonLeafSegment((SparseArraySegment<T>*)lastSeg, lastSeg->next, true /*forceNonLeaf*/);
        }
    }

    Var JavascriptArray::EntryReverse(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.reverse"));
        }

        BigIndex length = 0u;
        JavascriptArray* pArr = nullptr;
        RecyclableObject* obj = nullptr;

        JS_REENTRANT(jsReentLock, TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.reverse"), &pArr, &obj, &length));

        if (length.IsSmallIndex())
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::ReverseHelper(pArr, nullptr, obj, length.GetSmallIndex(), scriptContext));
        }
        Assert(pArr == nullptr || length.IsUint32Max()); // if pArr is not null lets make sure length is safe to cast, which will only happen if length is a uint32max

        JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::ReverseHelper(pArr, nullptr, obj, length.GetBigIndex(), scriptContext));
    }

    bool JavascriptArray::HasAnyES5ArrayInPrototypeChain(JavascriptArray *arr, bool forceCheckProtoChain)
    {
        Assert(arr != nullptr);

#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(arr);
#endif

        bool hasAnyES5Array = false;

        // If there is no gap (unless forced) we are not filling from the prototype - so no point checking for ES5Array.
        if (forceCheckProtoChain || arr->IsFillFromPrototypes())
        {
            RecyclableObject* prototype = arr->GetPrototype();

            while (JavascriptOperators::GetTypeId(prototype) != TypeIds_Null)
            {
                RecyclableObject* protoObj = prototype;

                if (!(DynamicObject::IsAnyArray(protoObj) || JavascriptOperators::IsObject(protoObj)) 
                    || JavascriptProxy::Is(protoObj)
                    || protoObj->IsExternal())
                {
                    hasAnyES5Array = true;
                    break;
                }

                if (DynamicObject::IsAnyArray(protoObj))
                {
                    if (ES5Array::Is(protoObj))
                    {
                        hasAnyES5Array = true;
                        break;
                    }
                }
                else if (DynamicType::Is(protoObj->GetTypeId()))
                {
                    DynamicObject* dynobj = DynamicObject::FromVar(protoObj);
                    ArrayObject* objectArray = dynobj->GetObjectArray();
                    if (objectArray != nullptr && ES5Array::Is(objectArray))
                    {
                        hasAnyES5Array = true;
                        break;
                    }
                }

                prototype = prototype->GetPrototype();
            }
        }
        return hasAnyES5Array;
    }

    // Array.prototype.reverse as described in ES6.0 (draft 22) Section 22.1.3.20
    template <typename T>
    Var JavascriptArray::ReverseHelper(JavascriptArray* pArr, Js::TypedArrayBase* typedArrayBase, RecyclableObject* obj, T length, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        T middle = length / 2;
        Var lowerValue = nullptr, upperValue = nullptr;
        T lowerExists, upperExists;
        const char16* methodName;
        bool isTypedArrayEntryPoint = typedArrayBase != nullptr;

        if (isTypedArrayEntryPoint)
        {
            methodName = _u("[TypedArray].prototype.reverse");
        }
        else
        {
            methodName = _u("Array.prototype.reverse");
        }

        // If we came from Array.prototype.map and source object is not a JavascriptArray, source could be a TypedArray
        if (!isTypedArrayEntryPoint && pArr == nullptr && TypedArrayBase::Is(obj))
        {
            typedArrayBase = TypedArrayBase::FromVar(obj);
        }

        ThrowTypeErrorOnFailureHelper h(scriptContext, methodName);

        bool useNoSideEffectReverse = pArr != nullptr && !HasAnyES5ArrayInPrototypeChain(pArr);

        if (useNoSideEffectReverse)
        {
            Recycler * recycler = scriptContext->GetRecycler();

            if (length <= 1)
            {
                return pArr;
            }

            if (pArr->IsFillFromPrototypes())
            {
                // For odd-length arrays, the middle element is unchanged,
                // so we cannot fill it from the prototypes.
                if (length % 2 == 0)
                {
                    JS_REENTRANT(jsReentLock, pArr->FillFromPrototypes(0, (uint32)length));
                }
                else
                {
                    middle = length / 2;
                    JS_REENTRANT(jsReentLock,
                        pArr->FillFromPrototypes(0, (uint32)middle),
                        pArr->FillFromPrototypes(1 + (uint32)middle, (uint32)length));
                }
            }

            // As we have already established that the FillFromPrototype should not change the bound of the array.
            if (length != (T)pArr->length)
            {
                Js::Throw::FatalInternalError();
            }

            if (pArr->HasNoMissingValues() && pArr->head && pArr->head->next)
            {
                // This function currently does not track missing values in the head segment if there are multiple segments
                pArr->SetHasNoMissingValues(false);
            }

            // Above FillFromPrototypes call can change the length of the array. Our segment calculation below will
            // not work with the stale length. Update the length.
            // Note : since we are reversing the whole segment below - the functionality is not spec compliant already.
            length = pArr->length;

            SparseArraySegmentBase *prevSeg = nullptr;
            SparseArraySegmentBase *nextSeg = nullptr;
            SparseArraySegmentBase *pinPrevSeg = nullptr;

            bool isIntArray = false;
            bool isFloatArray = false;

            pArr->ClearSegmentMap(); // Just dump the segment map on reverse

            if (JavascriptNativeIntArray::Is(pArr))
            {
                isIntArray = true;
            }
            else if (JavascriptNativeFloatArray::Is(pArr))
            {
                isFloatArray = true;
            }

            // During the loop below we are going to reverse the segments list. The head segment will become the last segment.
            // We have to verify that the current head segment is not the inilined segement, otherwise due to shuffling below (of EnsureHeadStartsFromZero call below), the inlined segment will no longer
            // be the head and that can create issue down the line. Create new segment if it is an inilined segment.
            if (pArr->head && (pArr->head->next || (pArr->head->left + pArr->head->length) < length))
            {
                if (isIntArray)
                {
                    CopyHeadIfInlinedHeadSegment<int32>(pArr, recycler);
                    if (pArr->head->next)
                    {
                    ReallocateNonLeafLastSegmentIfLeaf<int32>(pArr, recycler);
                }
                }
                else if (isFloatArray)
                {
                    CopyHeadIfInlinedHeadSegment<double>(pArr, recycler);
                    if (pArr->head->next)
                    {
                    ReallocateNonLeafLastSegmentIfLeaf<double>(pArr, recycler);
                }
                }
                else
                {
                    CopyHeadIfInlinedHeadSegment<Var>(pArr, recycler);
                }
            }

            SparseArraySegmentBase* seg = pArr->head;

            while (seg)
            {
                nextSeg = seg->next;

                // If seg.length == 0, it is possible that (seg.left + seg.length == prev.left + prev.length),
                // resulting in 2 segments sharing the same "left".
                if (seg->length > 0)
                {
                    if (isIntArray)
                    {
                        ((SparseArraySegment<int32>*)seg)->ReverseSegment(recycler);
                    }
                    else if (isFloatArray)
                    {
                        ((SparseArraySegment<double>*)seg)->ReverseSegment(recycler);
                    }
                    else
                    {
                        ((SparseArraySegment<Var>*)seg)->ReverseSegment(recycler);
                    }

                    seg->left = ((uint32)length) > (seg->left + seg->length) ? ((uint32)length) - (seg->left + seg->length) : 0;

                    seg->next = prevSeg;
                    // Make sure size doesn't overlap with next segment.
                    // An easy fix is to just truncate the size...
                    seg->EnsureSizeInBound();

                    // If the last segment is a leaf, then we may be losing our last scanned pointer to its previous
                    // segment. Hold onto it with pinPrevSeg until we reallocate below.
                    pinPrevSeg = prevSeg;
                    prevSeg = seg;
                }

                seg = nextSeg;
            }

            pArr->head = prevSeg;

            if (isIntArray)
            {
                pArr->EnsureHeadStartsFromZero<int32>(recycler);
            }
            else if (isFloatArray)
            {
                pArr->EnsureHeadStartsFromZero<double>(recycler);
            }
            else
            {
                pArr->EnsureHeadStartsFromZero<Var>(recycler);
            }

            pArr->InvalidateLastUsedSegment(); // lastUsedSegment might be 0-length and discarded above
#ifdef VALIDATE_ARRAY
            pArr->ValidateArray();
#endif
        }
        else if (typedArrayBase)
        {
            Assert(length <= JavascriptArray::MaxArrayLength);
            if (typedArrayBase->GetLength() == length)
            {
                // If typedArrayBase->length == length then we know that the TypedArray will have all items < length
                // and we won't have to check that the elements exist or not.
                for (uint32 lower = 0; lower < (uint32)middle; lower++)
                {
                    uint32 upper = (uint32)length - lower - 1;

                    lowerValue = typedArrayBase->DirectGetItem(lower);
                    upperValue = typedArrayBase->DirectGetItem(upper);

                    // We still have to call HasItem even though we know the TypedArray has both lower and upper because
                    // there may be a proxy handler trapping HasProperty.
                    lowerExists = typedArrayBase->HasItem(lower);
                    upperExists = typedArrayBase->HasItem(upper);

                    JS_REENTRANT(jsReentLock,
                        h.ThrowTypeErrorOnFailure(typedArrayBase->DirectSetItem(lower, upperValue)),
                        h.ThrowTypeErrorOnFailure(typedArrayBase->DirectSetItem(upper, lowerValue)));
                }
            }
            else
            {
                Assert(middle <= UINT_MAX);
                for (uint32 lower = 0; lower < (uint32)middle; lower++)
                {
                    uint32 upper = (uint32)length - lower - 1;

                    lowerValue = typedArrayBase->DirectGetItem(lower);
                    upperValue = typedArrayBase->DirectGetItem(upper);

                    lowerExists = typedArrayBase->HasItem(lower);
                    upperExists = typedArrayBase->HasItem(upper);

                    if (lowerExists)
                    {
                        if (upperExists)
                        {
                            JS_REENTRANT(jsReentLock,
                                h.ThrowTypeErrorOnFailure(typedArrayBase->DirectSetItem(lower, upperValue)),
                                h.ThrowTypeErrorOnFailure(typedArrayBase->DirectSetItem(upper, lowerValue)));
                        }
                        else
                        {
                            // This will always fail for a TypedArray if lower < length
                            h.ThrowTypeErrorOnFailure(typedArrayBase->DeleteItem(lower, PropertyOperation_ThrowOnDeleteIfNotConfig));
                            JS_REENTRANT(jsReentLock, h.ThrowTypeErrorOnFailure(typedArrayBase->DirectSetItem(upper, lowerValue)));
                        }
                    }
                    else
                    {
                        if (upperExists)
                        {
                            JS_REENTRANT(jsReentLock, h.ThrowTypeErrorOnFailure(typedArrayBase->DirectSetItem(lower, upperValue)));
                            // This will always fail for a TypedArray if upper < length
                            h.ThrowTypeErrorOnFailure(typedArrayBase->DeleteItem(upper, PropertyOperation_ThrowOnDeleteIfNotConfig));
                        }
                    }
                }
            }
        }
        else
        {
            for (T lower = 0; lower < middle; lower++)
            {
                T upper = length - lower - 1;

                JS_REENTRANT(jsReentLock,
                    lowerExists = JavascriptOperators::HasItem(obj, lower) && JavascriptOperators::GetItem(obj, lower, &lowerValue, scriptContext),
                    upperExists = JavascriptOperators::HasItem(obj, upper) && JavascriptOperators::GetItem(obj, upper, &upperValue, scriptContext));

                if (lowerExists)
                {
                    if (upperExists)
                    {
                        JS_REENTRANT(jsReentLock,
                            h.ThrowTypeErrorOnFailure(JavascriptOperators::SetItem(obj, obj, lower, upperValue, scriptContext, PropertyOperation_ThrowIfNotExtensible)),
                            h.ThrowTypeErrorOnFailure(JavascriptOperators::SetItem(obj, obj, upper, lowerValue, scriptContext, PropertyOperation_ThrowIfNotExtensible)));
                    }
                    else
                    {
                        JS_REENTRANT(jsReentLock,
                            h.ThrowTypeErrorOnFailure(JavascriptOperators::DeleteItem(obj, lower, PropertyOperation_ThrowOnDeleteIfNotConfig)),
                            h.ThrowTypeErrorOnFailure(JavascriptOperators::SetItem(obj, obj, upper, lowerValue, scriptContext, PropertyOperation_ThrowIfNotExtensible)));
                    }
                }
                else
                {
                    if (upperExists)
                    {
                        JS_REENTRANT(jsReentLock,
                            h.ThrowTypeErrorOnFailure(JavascriptOperators::SetItem(obj, obj, lower, upperValue, scriptContext, PropertyOperation_ThrowIfNotExtensible)),
                            h.ThrowTypeErrorOnFailure(JavascriptOperators::DeleteItem(obj, upper, PropertyOperation_ThrowOnDeleteIfNotConfig)));
                    }
                }
            }
        }

        return obj;
    }

    template<typename T>
    void JavascriptArray::ShiftHelper(JavascriptArray* pArr, ScriptContext * scriptContext)
    {
        Recycler * recycler = scriptContext->GetRecycler();

        SparseArraySegment<T>* next = SparseArraySegment<T>::From(pArr->head->next);
        while (next)
        {
            next->left--;
            next = SparseArraySegment<T>::From(next->next);
        }

        // head and next might overlap as the next segment left is decremented
        next = SparseArraySegment<T>::From(pArr->head->next);
        if (next && (pArr->head->size > next->left))
        {
            AssertMsg(pArr->head->left == 0, "Array always points to a head starting at index 0");
            AssertMsg(pArr->head->size == next->left + 1, "Shift next->left overlaps current segment by more than 1 element");

            SparseArraySegment<T> *head = SparseArraySegment<T>::From(pArr->head);
            SparseArraySegment<T> *oldHead = head;
            bool isInlineSegment = JavascriptArray::IsInlineSegment(head, pArr);
            bool nextIsInlineSegment = JavascriptArray::IsInlineSegment(next, pArr);
            // Merge the two adjacent segments
            if (next->length != 0)
            {
                uint32 offset = head->size - 1;
                // There is room for one unshifted element in head segment.
                // Hence it's enough if we grow the head segment by next->length - 1

                if (next->next)
                {
                    // If we have a next->next, we can't grow pass the left of that

                    // If the array had a segment map before, the next->next might just be right after next as well.
                    // So we just need to grow to the end of the next segment
                    // TODO: merge that segment too?
                    Assert(next->next->left >= head->size);
                    uint32 maxGrowSize = next->next->left - head->size;
                    if (maxGrowSize != 0)
                    {
                        head = head->GrowByMinMax(recycler, next->length - 1, maxGrowSize); //-1 is to account for unshift
                    }
                    else
                    {
                        // The next segment is only of length one, so we already have space in the header to copy that
                        Assert(next->length == 1);
                    }
                }
                else
                {
                    head = head->GrowByMin(recycler, next->length - 1); //-1 is to account for unshift
                }
                MoveArray(head->elements + offset, next->elements, next->length);
                head->length = offset + next->length;
                head->CheckLengthvsSize();
                pArr->head = head;

                if (isInlineSegment && oldHead != head)
                {
                    pArr->ClearElements(oldHead, 0);
                }
            }
            head->next = next->next;
            if (nextIsInlineSegment)
            {
                pArr->ClearElements(next, 0);
            }

            pArr->InvalidateLastUsedSegment();
        }

#ifdef VALIDATE_ARRAY
            pArr->ValidateArray();
#endif
    }

    Var JavascriptArray::EntryShift(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        Var res = scriptContext->GetLibrary()->GetUndefined();

        if (args.Info.Count == 0)
        {
            return res;
        }

        bool useNoSideEffectShift = JavascriptArray::Is(args[0])
            && !JavascriptArray::FromVar(args[0])->IsCrossSiteObject()
            && !HasAnyES5ArrayInPrototypeChain(JavascriptArray::FromVar(args[0]));

        if (useNoSideEffectShift)
        {
            JavascriptArray * pArr = JavascriptArray::FromVar(args[0]);

            if (pArr->length == 0)
            {
                return res;
            }

            uint32 length = pArr->length;

            if(pArr->IsFillFromPrototypes())
            {
                JS_REENTRANT(jsReentLock, pArr->FillFromPrototypes(0, pArr->length)); // We need find all missing value from [[proto]] object
            }

            // As we have already established that the FillFromPrototype should not change the bound of the array.
            if (length != pArr->length)
            {
                Js::Throw::FatalInternalError();
            }

            if(pArr->HasNoMissingValues() && pArr->head && pArr->head->next)
            {
                // This function currently does not track missing values in the head segment if there are multiple segments
                pArr->SetHasNoMissingValues(false);
            }

            pArr->length--;

            pArr->ClearSegmentMap(); // Dump segmentMap on shift (before any allocation)

            Recycler * recycler = scriptContext->GetRecycler();

            bool isIntArray = false;
            bool isFloatArray = false;

            if(JavascriptNativeIntArray::Is(pArr))
            {
                isIntArray = true;
            }
            else if(JavascriptNativeFloatArray::Is(pArr))
            {
                isFloatArray = true;
            }

            if (pArr->head->length != 0)
            {
                if(isIntArray)
                {
                    int32 nativeResult = SparseArraySegment<int32>::From(pArr->head)->GetElement(0);

                    if(SparseArraySegment<int32>::IsMissingItem(&nativeResult))
                    {
                        res = scriptContext->GetLibrary()->GetUndefined();
                    }
                    else
                    {
                        res = Js::JavascriptNumber::ToVar(nativeResult, scriptContext);
                    }
                    SparseArraySegment<int32>::From(pArr->head)->RemoveElement(recycler, 0);
                }
                else if (isFloatArray)
                {
                    double nativeResult = SparseArraySegment<double>::From(pArr->head)->GetElement(0);

                    if(SparseArraySegment<double>::IsMissingItem(&nativeResult))
                    {
                        res = scriptContext->GetLibrary()->GetUndefined();
                    }
                    else
                    {
                        res = Js::JavascriptNumber::ToVarNoCheck(nativeResult, scriptContext);
                    }
                    SparseArraySegment<double>::From(pArr->head)->RemoveElement(recycler, 0);
                }
                else
                {
                    res = SparseArraySegment<Var>::From(pArr->head)->GetElement(0);

                    if(SparseArraySegment<Var>::IsMissingItem(&res))
                    {
                        res = scriptContext->GetLibrary()->GetUndefined();
                    }
                    else
                    {
                        res = CrossSite::MarshalVar(scriptContext, res);
                    }
                    SparseArraySegment<Var>::From(pArr->head)->RemoveElement(recycler, 0);
                }
            }

            if(isIntArray)
            {
                ShiftHelper<int32>(pArr, scriptContext);
            }
            else if (isFloatArray)
            {
                ShiftHelper<double>(pArr, scriptContext);
            }
            else
            {
                ShiftHelper<Var>(pArr, scriptContext);
            }
        }
        else
        {
            RecyclableObject* dynamicObject = nullptr;
            if (FALSE == JavascriptConversion::ToObject(args[0], scriptContext, &dynamicObject))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.shift"));
            }

            ThrowTypeErrorOnFailureHelper h(scriptContext, _u("Array.prototype.shift"));

            JS_REENTRANT(jsReentLock, BigIndex length = OP_GetLength(dynamicObject, scriptContext));

            if (length == 0u)
            {
                // If length is 0, return 'undefined'
                JS_REENTRANT(jsReentLock,
                    h.ThrowTypeErrorOnFailure(JavascriptOperators::SetProperty(dynamicObject, dynamicObject, PropertyIds::length, TaggedInt::ToVarUnchecked(0), scriptContext, PropertyOperation_ThrowIfNotExtensible)));
                return scriptContext->GetLibrary()->GetUndefined();
            }

            JS_REENTRANT(jsReentLock,
                BOOL gotItem = JavascriptOperators::GetItem(dynamicObject, 0u, &res, scriptContext));
            if (!gotItem)
            {
                res = scriptContext->GetLibrary()->GetUndefined();
            }
            --length;
            uint32 lengthToUin32Max = length.IsSmallIndex() ? length.GetSmallIndex() : MaxArrayLength;
            for (uint32 i = 0u; i < lengthToUin32Max; i++)
            {
                JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(dynamicObject, i + 1));
                if (hasItem)
                {
                    Var element = nullptr;
                    JS_REENTRANT(jsReentLock,
                        element = JavascriptOperators::GetItem(dynamicObject, i + 1, scriptContext),
                        h.ThrowTypeErrorOnFailure(JavascriptOperators::SetItem(dynamicObject, dynamicObject, i, element, scriptContext, PropertyOperation_ThrowIfNotExtensible, /*skipPrototypeCheck*/ true)));
                }
                else
                {
                    JS_REENTRANT(jsReentLock, h.ThrowTypeErrorOnFailure(JavascriptOperators::DeleteItem(dynamicObject, i, PropertyOperation_ThrowOnDeleteIfNotConfig)));
                }
            }

            for (uint64 i = MaxArrayLength; length > i; i++)
            {
                JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(dynamicObject, i + 1));
                if (hasItem)
                {
                    Var element = nullptr;
                    JS_REENTRANT(jsReentLock,
                        element = JavascriptOperators::GetItem(dynamicObject, i + 1, scriptContext),
                        h.ThrowTypeErrorOnFailure(JavascriptOperators::SetItem(dynamicObject, dynamicObject, i, element, scriptContext, PropertyOperation_ThrowIfNotExtensible)));
                }
                else
                {
                    JS_REENTRANT(jsReentLock, h.ThrowTypeErrorOnFailure(JavascriptOperators::DeleteItem(dynamicObject, i, PropertyOperation_ThrowOnDeleteIfNotConfig)));
                }
            }

            if (length.IsSmallIndex())
            {
                JS_REENTRANT(jsReentLock,
                    h.ThrowTypeErrorOnFailure(JavascriptOperators::DeleteItem(dynamicObject, length.GetSmallIndex(), PropertyOperation_ThrowOnDeleteIfNotConfig)),
                    h.ThrowTypeErrorOnFailure(JavascriptOperators::SetProperty(dynamicObject, dynamicObject, PropertyIds::length, JavascriptNumber::ToVar(length.GetSmallIndex(), scriptContext), scriptContext, PropertyOperation_ThrowIfNotExtensible)));
            }
            else
            {
                JS_REENTRANT(jsReentLock,
                    h.ThrowTypeErrorOnFailure(JavascriptOperators::DeleteItem(dynamicObject, length.GetBigIndex(), PropertyOperation_ThrowOnDeleteIfNotConfig)),
                    h.ThrowTypeErrorOnFailure(JavascriptOperators::SetProperty(dynamicObject, dynamicObject, PropertyIds::length, JavascriptNumber::ToVar(length.GetBigIndex(), scriptContext), scriptContext, PropertyOperation_ThrowIfNotExtensible)));
            }
        }
        return res;
    }

    Js::JavascriptArray* JavascriptArray::CreateNewArrayHelper(uint32 len, bool isIntArray, bool isFloatArray,  Js::JavascriptArray* baseArray, ScriptContext* scriptContext)
    {
        if (isIntArray)
        {
            Js::JavascriptNativeIntArray *pnewArr = scriptContext->GetLibrary()->CreateNativeIntArray(len);
            pnewArr->EnsureHead<int32>();
#if ENABLE_PROFILE_INFO
            pnewArr->CopyArrayProfileInfo(Js::JavascriptNativeIntArray::FromVar(baseArray));
#endif

            return pnewArr;
        }
        else if (isFloatArray)
        {
            Js::JavascriptNativeFloatArray *pnewArr  = scriptContext->GetLibrary()->CreateNativeFloatArray(len);
            pnewArr->EnsureHead<double>();
#if ENABLE_PROFILE_INFO
            pnewArr->CopyArrayProfileInfo(Js::JavascriptNativeFloatArray::FromVar(baseArray));
#endif

            return pnewArr;
        }
        else
        {
            JavascriptArray *pnewArr = pnewArr = scriptContext->GetLibrary()->CreateArray(len);
            pnewArr->EnsureHead<Var>();
            return pnewArr;
        }
   }

    template<typename T>
    void JavascriptArray::SliceHelper(JavascriptArray* pArr,  JavascriptArray* pnewArr, uint32 start, uint32 newLen)
    {
        JS_REENTRANCY_LOCK(jsReentLock, pArr->GetScriptContext()->GetThreadContext());

        SparseArraySegment<T>* headSeg = SparseArraySegment<T>::From(pArr->head);
        SparseArraySegment<T>* pnewHeadSeg = SparseArraySegment<T>::From(pnewArr->head);

        // Fill the newly created sliced array
        CopyArray(pnewHeadSeg->elements, newLen, headSeg->elements + start, newLen);
        pnewHeadSeg->length = newLen;
        pnewHeadSeg->CheckLengthvsSize();

        Assert(pnewHeadSeg->length <= pnewHeadSeg->size);
        // Prototype lookup for missing elements
        if (!pArr->HasNoMissingValues())
        {
            for (uint32 i = 0; i < newLen && (i + start) < pArr->length; i++)
            {
                // array type might be changed in the below call to DirectGetItemAtFull
                // need recheck array type before checking array item [i + start]
                if (pArr->IsMissingItem(i + start))
                {
                    Var element = nullptr;
                    pnewArr->SetHasNoMissingValues(false);
                    JS_REENTRANT(jsReentLock, BOOL gotItem = pArr->DirectGetItemAtFull(i + start, &element));
                    if (gotItem)
                    {
                        JS_REENTRANT(jsReentLock, pnewArr->SetItem(i, element, PropertyOperation_None));
                    }
                }
            }
        }
#ifdef DBG
        else
        {
            for (uint32 i = 0; i < newLen; i++)
            {
                AssertMsg(!SparseArraySegment<T>::IsMissingItem(&headSeg->elements[i+start]), "Array marked incorrectly as having missing value");
            }
        }
#endif
    }

    // If the creating profile data has changed, convert it to the type of array indicated
    // in the profile
    void JavascriptArray::GetArrayTypeAndConvert(bool* isIntArray, bool* isFloatArray)
    {
        if (JavascriptNativeIntArray::Is(this))
        {
#if ENABLE_PROFILE_INFO
            JavascriptNativeIntArray* nativeIntArray = JavascriptNativeIntArray::FromVar(this);
            ArrayCallSiteInfo* info = nativeIntArray->GetArrayCallSiteInfo();
            if(!info || info->IsNativeIntArray())
            {
                *isIntArray = true;
            }
            else if(info->IsNativeFloatArray())
            {
                JavascriptNativeIntArray::ToNativeFloatArray(nativeIntArray);
                *isFloatArray = true;
            }
            else
            {
                JavascriptNativeIntArray::ToVarArray(nativeIntArray);
            }
#else
            *isIntArray = true;
#endif
        }
        else if (JavascriptNativeFloatArray::Is(this))
        {
#if ENABLE_PROFILE_INFO
            JavascriptNativeFloatArray* nativeFloatArray = JavascriptNativeFloatArray::FromVar(this);
            ArrayCallSiteInfo* info = nativeFloatArray->GetArrayCallSiteInfo();

            if(info && !info->IsNativeArray())
            {
                JavascriptNativeFloatArray::ToVarArray(nativeFloatArray);
            }
            else
            {
                *isFloatArray = true;
            }
#else
            *isFloatArray = true;
#endif
        }
    }

    Var JavascriptArray::EntrySlice(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        Var res = scriptContext->GetLibrary()->GetUndefined();

        if (args.Info.Count == 0)
        {
            return res;
        }

        BigIndex length;
        JavascriptArray* pArr = nullptr;
        RecyclableObject* obj = nullptr;

        JS_REENTRANT(jsReentLock, TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.slice"), &pArr, &obj, &length));

        if (length.IsSmallIndex())
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::SliceHelper(pArr, nullptr, obj, length.GetSmallIndex(), args, scriptContext));
        }

        Assert(pArr == nullptr || length.IsUint32Max());
        JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::SliceHelper(pArr, nullptr, obj, length.GetBigIndex(), args, scriptContext));
    }

    // Array.prototype.slice as described in ES6.0 (draft 22) Section 22.1.3.22
    template <typename T>
    Var JavascriptArray::SliceHelper(JavascriptArray* pArr, Js::TypedArrayBase* typedArrayBase, RecyclableObject* obj, T length, Arguments& args, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        JavascriptArray* newArr = nullptr;
        RecyclableObject* newObj = nullptr;
        bool isIntArray = false;
        bool isFloatArray = false;
        bool isTypedArrayEntryPoint = typedArrayBase != nullptr;
        bool isBuiltinArrayCtor = true;
        T startT = 0;
        T newLenT = length;
        T endT = length;

#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(pArr);
#endif
        if (args.Info.Count > 1)
        {
            JS_REENTRANT(jsReentLock, startT = GetFromIndex(args[1], length, scriptContext));

            if (args.Info.Count > 2 && JavascriptOperators::GetTypeId(args[2]) != TypeIds_Undefined)
            {
                JS_REENTRANT(jsReentLock, endT = GetFromIndex(args[2], length, scriptContext));
            }

            newLenT = endT > startT ? endT - startT : 0;
        }

        // Side effects (such as defining a property in a ToPrimitive call) during evaluation of arguments start or end may convert the array to an ES5 array.
        if (pArr && !JavascriptArray::Is(obj))
        {
            AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
            pArr = nullptr;
        }

        if (TypedArrayBase::IsDetachedTypedArray(obj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("Array.prototype.slice"));
        }

        // If we came from Array.prototype.slice and source object is not a JavascriptArray, source could be a TypedArray
        if (!isTypedArrayEntryPoint && pArr == nullptr && TypedArrayBase::Is(obj))
        {
            typedArrayBase = TypedArrayBase::FromVar(obj);
        }

        // If the entry point is %TypedArray%.prototype.slice or the source object is an Array exotic object we should try to load the constructor property
        // and use it to construct the return object.
        if (isTypedArrayEntryPoint)
        {
            JS_REENTRANT(jsReentLock,
                Var constructor = JavascriptOperators::SpeciesConstructor(typedArrayBase, TypedArrayBase::GetDefaultConstructor(args[0], scriptContext), scriptContext));
            isBuiltinArrayCtor = false;

            AssertAndFailFast(pArr == nullptr);
            Assert(JavascriptOperators::IsConstructor(constructor));

            Js::Var constructorArgs[] = { constructor, JavascriptNumber::ToVar(newLenT, scriptContext) };
            Js::CallInfo constructorCallInfo(Js::CallFlags_New, _countof(constructorArgs));
            JS_REENTRANT(jsReentLock, newObj = RecyclableObject::FromVar(TypedArrayBase::TypedArrayCreate(constructor, &Js::Arguments(constructorCallInfo, constructorArgs), (uint32)newLenT, scriptContext)));
        }

        else if (pArr != nullptr)
        {
            JS_REENTRANT(jsReentLock, newObj = ArraySpeciesCreate(pArr, newLenT, scriptContext, &isIntArray, &isFloatArray, &isBuiltinArrayCtor));
        }

        // skip the typed array and "pure" array case, we still need to handle special arrays like es5array, remote array, and proxy of array.
        else
        {
            JS_REENTRANT(jsReentLock, newObj = ArraySpeciesCreate(obj, newLenT, scriptContext, nullptr, nullptr, &isBuiltinArrayCtor));
        }

        // If we didn't create a new object above we will create a new array here.
        // This is the pre-ES6 behavior or the case of calling Array.prototype.slice with a constructor argument that is not a constructor function.
        if (newObj == nullptr)
        {
            if (pArr)
            {
                pArr->GetArrayTypeAndConvert(&isIntArray, &isFloatArray);
            }

            if (newLenT > JavascriptArray::MaxArrayLength)
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthConstructIncorrect);
            }

            newArr = CreateNewArrayHelper(static_cast<uint32>(newLenT), isIntArray, isFloatArray, pArr, scriptContext);
#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(newArr);
#endif
            newObj = newArr;
        }
        else
        {
            // If the new object we created is an array, remember that as it will save us time setting properties in the object below
            if (JavascriptArray::Is(newObj))
            {
#if ENABLE_COPYONACCESS_ARRAY
                JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(newObj);
#endif
                newArr = JavascriptArray::FromVar(newObj);
            }
        }

        uint32 start  = (uint32) startT;
        uint32 newLen = (uint32) newLenT;

        // We at least have to have newObj as a valid object
        Assert(newObj);

        // Bail out early if the new object will have zero length.
        if (newLen == 0)
        {
            return newObj;
        }

        // The ArraySpeciesCreate call above could have converted the source array into an ES5Array. If this happens
        // we will process the array elements like an ES5Array.
        if (pArr && !JavascriptArray::Is(obj))
        {
            AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
            pArr = nullptr;
        }

        if (pArr)
        {
            if (HasAnyES5ArrayInPrototypeChain(pArr))
            {
                JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::SliceObjectHelper(obj, start, 0u, newArr, newObj, newLen, scriptContext));
            }

            // If we constructed a new Array object, we have some nice helpers here
            if (newArr && isBuiltinArrayCtor)
            {
                if (JavascriptArray::IsDirectAccessArray(newArr))
                {
                    if (((start + newLen) <= pArr->head->length) && newLen <= newArr->head->size) //Fast Path
                    {
                        if (isIntArray)
                        {
                            JS_REENTRANT(jsReentLock, SliceHelper<int32>(pArr, newArr, start, newLen));
                        }
                        else if (isFloatArray)
                        {
                            JS_REENTRANT(jsReentLock, SliceHelper<double>(pArr, newArr, start, newLen));
                        }
                        else
                        {
                            JS_REENTRANT(jsReentLock, SliceHelper<Var>(pArr, newArr, start, newLen));
                        }
                    }
                    else
                    {
                        if (isIntArray)
                        {
                            JS_REENTRANT(jsReentLock, CopyNativeIntArrayElements(JavascriptNativeIntArray::FromVar(newArr), 0, JavascriptNativeIntArray::FromVar(pArr), start, start + newLen));
                        }
                        else if (isFloatArray)
                        {
                            JS_REENTRANT(jsReentLock, CopyNativeFloatArrayElements(JavascriptNativeFloatArray::FromVar(newArr), 0, JavascriptNativeFloatArray::FromVar(pArr), start, start + newLen));
                        }
                        else
                        {
                            JS_REENTRANT(jsReentLock, CopyArrayElements(newArr, 0u, pArr, start, start + newLen));
                        }
                    }
                }
                else
                {
                    AssertMsg(CONFIG_FLAG(ForceES5Array), "newArr can only be ES5Array when it is forced");
                    Var element = nullptr;
                    for (uint32 i = 0; i < newLen; i++)
                    {
                        JS_REENTRANT(jsReentLock, BOOL gotItem = pArr->DirectGetItemAtFull(i + start, &element));
                        if (!gotItem)
                        {
                            continue;
                        }

                        newArr->SetItem(i, element, PropertyOperation_None);

                        // Side-effects in the prototype lookup may have changed the source array into an ES5Array. If this happens
                        // we will process the rest of the array elements like an ES5Array.
                        if (!JavascriptArray::Is(obj))
                        {
                            AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
                            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::SliceObjectHelper(obj, start, i + 1, newArr, newObj, newLen, scriptContext));
                        }
                    }
                }
            }
            else
            {
                // The constructed object isn't an array, we'll need to use normal object manipulation
                Var element = nullptr;

                for (uint32 i = 0; i < newLen; i++)
                {
                    JS_REENTRANT(jsReentLock, BOOL gotItem = pArr->DirectGetItemAtFull(i + start, &element));
                    if (!gotItem)
                    {
                        continue;
                    }

                    JS_REENTRANT(jsReentLock, ThrowErrorOnFailure(JavascriptArray::SetArrayLikeObjects(newObj, i, element), scriptContext, i));

                    // Side-effects in the prototype lookup may have changed the source array into an ES5Array. If this happens
                    // we will process the rest of the array elements like an ES5Array.
                    if (!JavascriptArray::Is(obj))
                    {
                        AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
                        JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::SliceObjectHelper(obj, start, i + 1, newArr, newObj, newLen, scriptContext));
                    }
                }
            }
        }
        else if (typedArrayBase)
        {
            AssertAndFailFast(TypedArrayBase::Is(typedArrayBase));

            // Source is a TypedArray, we must have created the return object via a call to constructor, but newObj may not be a TypedArray (or an array either)
            TypedArrayBase* newTypedArray = nullptr;

            if (TypedArrayBase::Is(newObj))
            {
                newTypedArray = TypedArrayBase::FromVar(newObj);
            }
            else
            {
                AssertAndFailFast(newArr != nullptr);
            }

            Var element;

            for (uint32 i = 0; i < newLen; i++)
            {
                // We only need to call HasItem in the case that we are called from Array.prototype.slice
                if (!isTypedArrayEntryPoint && !typedArrayBase->HasItem(i + start))
                {
                    continue;
                }

                element = typedArrayBase->DirectGetItem(i + start);

                // The object we got back from the constructor might not be a TypedArray. In fact, it could be any object.
                if (newTypedArray)
                {
                    JS_REENTRANT(jsReentLock, newTypedArray->DirectSetItem(i, element));
                }
                else
                {
                    newArr->SetItem(i, element, PropertyOperation_None);
                }
            }
        }
        else
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::SliceObjectHelper(obj, start, 0u, newArr, newObj, newLen, scriptContext));;
        }

        if (!isTypedArrayEntryPoint)
        {
            JavascriptOperators::SetProperty(newObj, newObj, Js::PropertyIds::length, JavascriptNumber::ToVar(newLen, scriptContext), scriptContext, PropertyOperation_ThrowIfNotExtensible);
        }

#ifdef VALIDATE_ARRAY
        if (JavascriptArray::Is(newObj))
        {
            JavascriptArray::FromVar(newObj)->ValidateArray();
        }
#endif

        return newObj;
    }

    Var JavascriptArray::SliceObjectHelper(RecyclableObject* obj, uint32 sliceStart, uint32 start, JavascriptArray* newArr, RecyclableObject* newObj, uint32 newLen, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        for (uint32 i = start; i < newLen; i++)
        {
            JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(obj, i + sliceStart));
                if (hasItem)
            {
                JS_REENTRANT(jsReentLock, Var element = JavascriptOperators::GetItem(obj, i + sliceStart, scriptContext));
                if (newArr != nullptr)
                {
                    newArr->SetItem(i, element, PropertyOperation_None);
                }
                else
                {
                        JS_REENTRANT(jsReentLock, ThrowErrorOnFailure(JavascriptArray::SetArrayLikeObjects(newObj, i, element), scriptContext, i));
                }
            }
        }

            JS_REENTRANT(jsReentLock,
                JavascriptOperators::SetProperty(newObj, newObj, Js::PropertyIds::length, JavascriptNumber::ToVar(newLen, scriptContext), scriptContext, PropertyOperation_ThrowIfNotExtensible));

#ifdef VALIDATE_ARRAY
        if (JavascriptArray::Is(newObj))
        {
            JavascriptArray::FromVar(newObj)->ValidateArray();
        }
#endif

        return newObj;
    }

    struct CompareVarsInfo
    {
        ScriptContext* scriptContext;
        RecyclableObject* compFn;
    };

    int __cdecl compareVars(void* cvInfoV, const void* aRef, const void* bRef)
    {
        CompareVarsInfo* cvInfo=(CompareVarsInfo*)cvInfoV;
        ScriptContext* requestContext=cvInfo->scriptContext;
        RecyclableObject* compFn=cvInfo->compFn;

        AssertMsg(*(Var*)aRef, "No null expected in sort");
        AssertMsg(*(Var*)bRef, "No null expected in sort");

        if (compFn != nullptr)
        {
            ScriptContext* scriptContext = compFn->GetScriptContext();

            // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
            CallFlags flags = CallFlags_Value;
            Var undefined = scriptContext->GetLibrary()->GetUndefined();
            Var retVal;
            if (requestContext != scriptContext)
            {
                Var leftVar = CrossSite::MarshalVar(scriptContext, *(Var*)aRef);
                Var rightVar = CrossSite::MarshalVar(scriptContext, *(Var*)bRef);
                retVal = CALL_FUNCTION(scriptContext->GetThreadContext(), compFn, CallInfo(flags, 3), undefined, leftVar, rightVar);
            }
            else
            {
                retVal = CALL_FUNCTION(scriptContext->GetThreadContext(), compFn, CallInfo(flags, 3), undefined, *(Var*)aRef, *(Var*)bRef);
            }

            if (TaggedInt::Is(retVal))
            {
                return TaggedInt::ToInt32(retVal);
            }
            double dblResult;
            if (JavascriptNumber::Is_NoTaggedIntCheck(retVal))
            {
                dblResult = JavascriptNumber::GetValue(retVal);
            }
            else
            {
                dblResult = JavascriptConversion::ToNumber_Full(retVal, scriptContext);
            }
            if (dblResult < 0)
            {
                return -1;
            }
            return (dblResult > 0) ? 1 : 0;
        }
        else
        {
            JavascriptString* pStr1 = JavascriptConversion::ToString(*(Var*)aRef, requestContext);
            JavascriptString* pStr2 = JavascriptConversion::ToString(*(Var*)bRef, requestContext);

            return JavascriptString::strcmp(pStr1, pStr2);
        }
    }

    static void hybridSort(__inout_ecount(length) Field(Var) *elements, uint32 length, CompareVarsInfo* compareInfo)
    {
        // The cost of memory moves starts to be more expensive than additional comparer calls (given a simple comparer)
        // for arrays of more than 512 elements.
        if (length > 512)
        {
            qsort_s(elements, length, compareVars, compareInfo);
            return;
        }

        for (int i = 1; i < (int)length; i++)
        {
            if (compareVars(compareInfo, elements + i, elements + i - 1) < 0) {
                // binary search for the left-most element greater than value:
                int first = 0;
                int last = i - 1;
                while (first <= last)
                {
                    int middle = (first + last) / 2;
                    if (compareVars(compareInfo, elements + i, elements + middle) < 0)
                    {
                        last = middle - 1;
                    }
                    else
                    {
                        first = middle + 1;
                    }
                }

                // insert value right before first:
                Var value = elements[i];
                MoveArray(elements + first + 1, elements + first, (i - first));
                elements[first] = value;
            }
        }
    }

    void JavascriptArray::Sort(RecyclableObject* compFn)
    {
        JS_REENTRANCY_LOCK(jsReentLock, this->GetScriptContext()->GetThreadContext());

        if (length <= 1)
        {
            return;
        }

        this->EnsureHead<Var>();
        ScriptContext* scriptContext = this->GetScriptContext();
        Recycler* recycler = scriptContext->GetRecycler();

        CompareVarsInfo cvInfo;
        cvInfo.scriptContext = scriptContext;
        cvInfo.compFn = compFn;

        Assert(head != nullptr);

        // Just dump the segment map on sort
        ClearSegmentMap();

        uint32 countUndefined = 0;
        SparseArraySegment<Var>* startSeg = SparseArraySegment<Var>::From(head);

        // Sort may have side effects on the array. Setting a dummy head so that original array is not affected
        uint32 saveLength = length;
        // that if compare function tries to modify the array it won't AV.
        head = const_cast<SparseArraySegmentBase*>(EmptySegment);
        SetFlags(DynamicObjectFlags::None);
        this->InvalidateLastUsedSegment();
        length = 0;

        TryFinally([&]()
        {
            //The array is a continuous array if there is only one segment
            if (startSeg->next == nullptr
                // If this flag is specified, we want to improve the consistency of our array sorts
                // by removing missing values from all kinds of arrays before sorting (done here by
                // using the copy-to-one-segment path for array sorts) and by using a stronger sort
                // comparer than the spec requires (done in CompareElements).
                && !CONFIG_FLAG(StrongArraySort)
                ) // Single segment fast path
            {
                if (compFn != nullptr)
                {
                    countUndefined = startSeg->RemoveUndefined(scriptContext);

#ifdef VALIDATE_ARRAY
                    ValidateSegment(startSeg);
#endif
                    JS_REENTRANT(jsReentLock, hybridSort(startSeg->elements, startSeg->length, &cvInfo));
                    startSeg->CheckLengthvsSize();
                }
                else
                {
                    JS_REENTRANT(jsReentLock, countUndefined = sort(startSeg->elements, &startSeg->length, scriptContext));
                }
                head = startSeg;
            }
            else
            {
                SparseArraySegment<Var>* allElements = SparseArraySegment<Var>::AllocateSegment(recycler, 0, 0, nullptr);
                SparseArraySegment<Var>* next = startSeg;

                uint32 nextIndex = 0;
                // copy all the elements to single segment
                while (next)
                {
                    countUndefined += next->RemoveUndefined(scriptContext);
                    if (next->length != 0)
                    {
                        allElements = SparseArraySegment<Var>::CopySegment(recycler, allElements, nextIndex, next, next->left, next->length);
                    }
                    next = SparseArraySegment<Var>::From(next->next);
                    nextIndex = allElements->length;

#ifdef VALIDATE_ARRAY
                    ValidateSegment(allElements);
#endif
                }

                if (compFn != nullptr)
                {
                    JS_REENTRANT(jsReentLock, hybridSort(allElements->elements, allElements->length, &cvInfo));
                }
                else
                {
                    JS_REENTRANT(jsReentLock, sort(allElements->elements, &allElements->length, scriptContext));
                    allElements->CheckLengthvsSize();
                }

                head = allElements;
                head->next = nullptr;
            }
        },
        [&](bool hasException)
        {
            length = saveLength;
            ClearSegmentMap(); // Dump the segmentMap again in case user compare function rebuilds it
            if (hasException)
            {
                head = startSeg;
                this->InvalidateLastUsedSegment();
            }
        });

#if DEBUG
        {
            uint32 countNull = 0;
            uint32 index = head->length - 1;
            while (countNull < head->length)
            {
                if (SparseArraySegment<Var>::From(head)->elements[index] != NULL)
                {
                    break;
                }
                index--;
                countNull++;
            }
            AssertMsg(countNull == 0, "No null expected at the end");
        }
#endif

        if (countUndefined != 0)
        {
            // fill undefined at the end
            uint32 newLength = head->length + countUndefined;
            if (newLength > head->size)
            {
                SparseArraySegmentBase *oldHead = head;
                bool isInlineSegment = JavascriptArray::IsInlineSegment(head, this);
                head = SparseArraySegment<Var>::From(head)->GrowByMin(recycler, newLength - head->size);
                if (isInlineSegment)
                {
                    this->ClearElements(oldHead, 0);
                }
            }

            Var undefined = scriptContext->GetLibrary()->GetUndefined();
            for (uint32 i = head->length; i < newLength; i++)
            {
                SparseArraySegment<Var>::From(head)->elements[i] = undefined;
            }
            head->length = newLength;
            head->CheckLengthvsSize();
        }
        SetHasNoMissingValues();
        this->InvalidateLastUsedSegment();

#ifdef VALIDATE_ARRAY
        ValidateArray();
#endif
        return;
    }

    uint32 JavascriptArray::sort(__inout_ecount(*len) Field(Var) *orig, uint32 *len, ScriptContext *scriptContext)
    {
        uint32 count = 0, countUndefined = 0;
        Element *elements = RecyclerNewArrayZ(scriptContext->GetRecycler(), Element, *len);
        RecyclableObject *undefined = scriptContext->GetLibrary()->GetUndefined();

        //
        // Create the Elements array
        //

        for (uint32 i = 0; i < *len; ++i)
        {
            if (!SparseArraySegment<Var>::IsMissingItem(&orig[i]))
            {
                if (!JavascriptOperators::IsUndefinedObject(orig[i], undefined))
                {
                    elements[count].Value = orig[i];
                    elements[count].StringValue =  JavascriptConversion::ToString(orig[i], scriptContext);

                    count++;
                }
                else
                {
                    countUndefined++;
                }
            }
        }

        if (count > 0)
        {
            SortElements(elements, 0, count - 1);

            for (uint32 i = 0; i < count; ++i)
            {
                orig[i] = elements[i].Value;
            }
        }

        for (uint32 i = count + countUndefined; i < *len; ++i)
        {
            orig[i] = SparseArraySegment<Var>::GetMissingItem();
        }

        *len = count; // set the correct length
        return countUndefined;
    }

    int __cdecl JavascriptArray::CompareElements(void* context, const void* elem1, const void* elem2)
    {
        const Element* element1 = static_cast<const Element*>(elem1);
        const Element* element2 = static_cast<const Element*>(elem2);

        Assert(element1 != NULL);
        Assert(element2 != NULL);

        if (!CONFIG_FLAG(StrongArraySort))
        {
            return JavascriptString::strcmp(element1->StringValue, element2->StringValue);
        }
        else
        {
            int str_cmp = JavascriptString::strcmp(element1->StringValue, element2->StringValue);
            if (str_cmp != 0)
            {
                return str_cmp;
            }
            // If they are equal, we get to a slightly more complex problem. We want to make a very
            // predictable sort here, regardless of the structure of the array. To achieve this, we
            // need to get an order for every pair of non-identical elements, else there will be an
            // identifiable difference between sparse and dense array sorts in some cases.

            // Handle a common set of equivalent nodes first for speed/convenience
            if (element1->Value == element2->Value)
            {
                return 0;
            }

            // Easy way to do most remaining cases is to just compare the type ids if they differ.
            if (JavascriptOperators::GetTypeId(element1->Value) != JavascriptOperators::GetTypeId(element2->Value))
            {
                return JavascriptOperators::GetTypeId(element1->Value) - JavascriptOperators::GetTypeId(element2->Value);
            }

            // Further comparisons are possible, but get increasingly complex, and aren't necessary
            // for the cases on hand.
            return 0;
        }
    }

    void JavascriptArray::SortElements(Element* elements, uint32 left, uint32 right)
    {
        // Note: use write barrier policy of Field(Var)
        qsort_s<Element, Field(Var)>(elements, right - left + 1, CompareElements, this);
    }

    Var JavascriptArray::EntrySort(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Array.prototype.sort"));

        Assert(!(callInfo.Flags & CallFlags_New));

        AssertMsg(args.Info.Count >= 1, "Should have at least one argument");

        RecyclableObject* compFn = NULL;
        if (args.Info.Count > 1)
        {
            if (JavascriptConversion::IsCallable(args[1]))
            {
                compFn = RecyclableObject::FromVar(args[1]);
            }
            else
            {
                TypeId typeId = JavascriptOperators::GetTypeId(args[1]);

                // Use default comparer:
                // - In ES5 mode if the argument is undefined.
                bool useDefaultComparer = typeId == TypeIds_Undefined;
                if (!useDefaultComparer)
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Array.prototype.sort"));
                }
            }
        }

        bool useNoSideEffectSort = JavascriptArray::Is(args[0])
            && !JavascriptArray::FromVar(args[0])->IsCrossSiteObject()
            && !HasAnyES5ArrayInPrototypeChain(JavascriptArray::FromVar(args[0]));

        if (useNoSideEffectSort)
        {
            JavascriptArray *arr = JavascriptArray::FromVar(args[0]);

            if (arr->length <= 1)
            {
                return args[0];
            }

            uint32 length = arr->length;

            if(arr->IsFillFromPrototypes())
            {
                JS_REENTRANT(jsReentLock, arr->FillFromPrototypes(0, arr->length)); // We need find all missing value from [[proto]] object
            }

            // As we have already established that the FillFromPrototype should not change the bound of the array.
            if (length != arr->length)
            {
                Js::Throw::FatalInternalError();
            }

            // Maintain nativity of the array only for the following cases (To favor inplace conversions - keeps the conversion cost less):
            // -    int cases for X86 and
            // -    FloatArray for AMD64
            // We convert the entire array back and forth once here O(n), rather than doing the costly conversion down the call stack which is O(nlogn)

#if defined(_M_X64_OR_ARM64)
            if(compFn && JavascriptNativeFloatArray::Is(arr))
            {
                arr = JavascriptNativeFloatArray::ConvertToVarArray((JavascriptNativeFloatArray*)arr);
                JS_REENTRANT(jsReentLock, arr->Sort(compFn));
                arr = arr->ConvertToNativeArrayInPlace<JavascriptNativeFloatArray, double>(arr);
            }
            else
            {
                EnsureNonNativeArray(arr);
                JS_REENTRANT(jsReentLock, arr->Sort(compFn));
            }
#else
            if(compFn && JavascriptNativeIntArray::Is(arr))
            {
                //EnsureNonNativeArray(arr);
                arr = JavascriptNativeIntArray::ConvertToVarArray((JavascriptNativeIntArray*)arr);
                JS_REENTRANT(jsReentLock, arr->Sort(compFn));
                arr = arr->ConvertToNativeArrayInPlace<JavascriptNativeIntArray, int32>(arr);
            }
            else
            {
                EnsureNonNativeArray(arr);
                JS_REENTRANT(jsReentLock, arr->Sort(compFn));
            }
#endif

        }
        else
        {
            RecyclableObject* pObj = nullptr;
            if (FALSE == JavascriptConversion::ToObject(args[0], scriptContext, &pObj))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.sort"));
            }
            JS_REENTRANT(jsReentLock,
                uint32 len = JavascriptConversion::ToUInt32(JavascriptOperators::OP_GetLength(pObj, scriptContext), scriptContext));
            JavascriptArray* sortArray = scriptContext->GetLibrary()->CreateArray(len);
            sortArray->EnsureHead<Var>();
            ThrowTypeErrorOnFailureHelper h(scriptContext, _u("Array.prototype.sort"));

            BEGIN_TEMP_ALLOCATOR(tempAlloc, scriptContext, _u("Runtime"))
            {
                JsUtil::List<uint32, ArenaAllocator>* indexList = JsUtil::List<uint32, ArenaAllocator>::New(tempAlloc);

                for (uint32 i = 0; i < len; i++)
                {
                    Var item = nullptr;
                    JS_REENTRANT(jsReentLock, BOOL gotItem = JavascriptOperators::GetItem(pObj, i, &item, scriptContext));
                    if (gotItem)
                    {
                        indexList->Add(i);
                        sortArray->DirectSetItemAt(i, item);
                    }
                }
                if (indexList->Count() > 0)
                {
                    if (sortArray->length > 1)
                    {
                        JS_REENTRANT(jsReentLock, sortArray->FillFromPrototypes(0, sortArray->length)); // We need find all missing value from [[proto]] object
                    }
                    JS_REENTRANT(jsReentLock, sortArray->Sort(compFn));

                    uint32 removeIndex = sortArray->head->length;
                    for (uint32 i = 0; i < removeIndex; i++)
                    {
                        AssertMsg(!SparseArraySegment<Var>::IsMissingItem(&SparseArraySegment<Var>::From(sortArray->head)->elements[i]), "No gaps expected in sorted array");
                        JS_REENTRANT(jsReentLock, h.ThrowTypeErrorOnFailure(JavascriptOperators::SetItem(pObj, pObj, i, SparseArraySegment<Var>::From(sortArray->head)->elements[i], scriptContext)));
                    }
                    for (int i = 0; i < indexList->Count(); i++)
                    {
                        uint32 value = indexList->Item(i);
                        if (value >= removeIndex)
                        {
                            JS_REENTRANT(jsReentLock, h.ThrowTypeErrorOnFailure((JavascriptOperators::DeleteItem(pObj, value))));
                        }
                    }
                }

            }
            END_TEMP_ALLOCATOR(tempAlloc, scriptContext);
        }
        return args[0];
    }

    Var JavascriptArray::EntrySplice(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        AssertMsg(args.Info.Count >= 1, "Should have at least one argument");

        JavascriptArray* pArr = 0;
        RecyclableObject* pObj = 0;
        uint64 start = 0u;
        uint64 deleteLen = 0u;
        uint64 length = 0u;

        JS_REENTRANT(jsReentLock, TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.splice"), &pArr, &pObj, &length));

        switch (args.Info.Count)
        {
        case 1:
            start = length;
            deleteLen = 0u;
            break;

        case 2:
            JS_REENTRANT(jsReentLock, start = GetFromIndex(args[1], length, scriptContext));
            deleteLen = length - start;
            break;

        default:
            JS_REENTRANT(jsReentLock, start = GetFromIndex(args[1], length, scriptContext),
                deleteLen = GetFromIndex(args[2], (length - start), scriptContext, false));
            break;
        }

        // Side effects (such as defining a property in a ToPrimitive call) during evaluation of arguments start or deleteCount may convert the array to an ES5 array.
        if (pArr && !JavascriptArray::Is(pObj))
        {
            AssertOrFailFastMsg(ES5Array::Is(pObj), "The array should have been converted to an ES5Array");
            pArr = nullptr;
        }

        Var* insertArgs = args.Info.Count > 3 ? &args.Values[3] : nullptr;
        uint32 insertLen = args.Info.Count > 3 ? args.Info.Count - 3 : 0;

        // Force check the prototype as we may insert values more than current elements
        if (pArr != nullptr && !HasAnyES5ArrayInPrototypeChain(pArr, true /*forceCheckProtoChain*/))
        {
            // Since we get the length from an array and that cannot be more than uint32.
            _Analysis_assume_(length <= UINT_MAX);
            JS_REENTRANT_UNLOCK(jsReentLock,
                return TryArraySplice(pArr, (uint32)start, (uint32)length, (uint32)deleteLen, insertArgs, insertLen, scriptContext));
        }

        uint64 newLen = (length - deleteLen) + insertLen;
        if (newLen > UINT_MAX || length > UINT_MAX || (length + insertLen) > UINT_MAX)
        {
            JS_REENTRANT_UNLOCK(jsReentLock,
                return ObjectSpliceHelper<uint64>(pObj, length, start, deleteLen, (Var*)insertArgs, insertLen, scriptContext, nullptr));
        }
        else
        {
            JS_REENTRANT_UNLOCK(jsReentLock,
                return ObjectSpliceHelper<uint32>(pObj, (uint32)length, (uint32)start, (uint32)deleteLen, (Var*)insertArgs, insertLen, scriptContext, nullptr));
        }
    }

    inline BOOL JavascriptArray::IsSingleSegmentArray() const
    {
        return nullptr == head->next;
    }

    template<typename T>
    void JavascriptArray::ArraySegmentSpliceHelper(JavascriptArray *pnewArr, SparseArraySegment<T> *seg, SparseArraySegment<T> **prev,
                                                    uint32 start, uint32 deleteLen, Var* insertArgs, uint32 insertLen, Recycler *recycler)
    {
        // book keeping variables
        uint32 relativeStart    = start - seg->left;  // This will be different from start when head->left is non zero -
                                                      //(Missing elements at the beginning)

        uint32 headDeleteLen    = min(start + deleteLen , seg->left + seg->length) - start;   // actual number of elements to delete in
                                                                                              // head if deleteLen overflows the length of head

        uint32 newHeadLen       = seg->length - headDeleteLen + insertLen;     // new length of the head after splice

        // Save the deleted elements
        if (headDeleteLen != 0)
        {
            SparseArraySegmentBase *oldHead = pnewArr->head;
            bool isInlineSegment = JavascriptArray::IsInlineSegment(oldHead, pnewArr);
            pnewArr->InvalidateLastUsedSegment();
            pnewArr->head = SparseArraySegment<T>::CopySegment(recycler, SparseArraySegment<T>::From(pnewArr->head), 0, seg, start, headDeleteLen);
            if (isInlineSegment && oldHead != pnewArr->head)
            {
                pnewArr->ClearElements(oldHead, 0);
            }
        }

        if (newHeadLen != 0)
        {
            if (seg->size < newHeadLen)
            {
                if (seg->next)
                {
                    // If we have "next", require that we haven't adjusted next segments left yet.
                    seg = seg->GrowByMinMax(recycler, newHeadLen - seg->size, seg->next->left - deleteLen + insertLen - seg->left - seg->size);
                }
                else
                {
                    seg = seg->GrowByMin(recycler, newHeadLen - seg->size);
                }
#ifdef VALIDATE_ARRAY
                ValidateSegment(seg);
#endif
            }

            // Move the elements if necessary
            if (headDeleteLen != insertLen)
            {
                uint32 noElementsToMove = seg->length - (relativeStart + headDeleteLen);
                MoveArray(seg->elements + relativeStart + insertLen,
                            seg->elements + relativeStart + headDeleteLen,
                            noElementsToMove);
                if (newHeadLen < seg->length) // truncate if necessary
                {
                    seg->Truncate(seg->left + newHeadLen); // set end elements to null so that when we introduce null elements we are safe
                }
                seg->length = newHeadLen;
                seg->CheckLengthvsSize();
            }
            // Copy the new elements
            if (insertLen > 0)
            {
                Assert(!VirtualTableInfo<JavascriptNativeIntArray>::HasVirtualTable(pnewArr) &&
                   !VirtualTableInfo<JavascriptNativeFloatArray>::HasVirtualTable(pnewArr));

                // inserted elements starts at argument 3 of splice(start, deleteNumber, insertelem1, insertelem2, insertelem3, ...);
                CopyArray(seg->elements + relativeStart, insertLen,
                          reinterpret_cast<const T*>(insertArgs), insertLen);
            }
            *prev = seg;
        }
        else
        {
            *prev = SparseArraySegment<T>::From(seg->next);
        }
    }

    template<typename T>
    void JavascriptArray::ArraySpliceHelper(JavascriptArray* pnewArr, JavascriptArray* pArr, uint32 start, uint32 deleteLen, Var* insertArgs, uint32 insertLen, ScriptContext *scriptContext)
    {
        // Skip pnewArr->EnsureHead(): we don't use existing segment at all.
        Recycler *recycler  = scriptContext->GetRecycler();

        Field(SparseArraySegmentBase*)* prevSeg  = &pArr->head;        // holds the next pointer of previous
        Field(SparseArraySegmentBase*)* prevPrevSeg  = &pArr->head;    // this holds the previous pointer to prevSeg dirty trick.
        SparseArraySegmentBase* savePrev = nullptr;

        Assert(pArr->head); // We should never have a null head.
        pArr->EnsureHead<T>();
        SparseArraySegment<T>* startSeg = SparseArraySegment<T>::From(pArr->head);

        const uint32 limit = start + deleteLen;
        uint32 rightLimit;
        if (UInt32Math::Add(startSeg->left, startSeg->size, &rightLimit))
        {
            rightLimit = JavascriptArray::MaxArrayLength;
        }

        // Find out the segment to start delete
        while (startSeg && (rightLimit <= start))
        {
            savePrev = startSeg;
            prevPrevSeg = prevSeg;
            prevSeg = &startSeg->next;
            startSeg = SparseArraySegment<T>::From(startSeg->next);

            if (startSeg)
            {
                if (UInt32Math::Add(startSeg->left, startSeg->size, &rightLimit))
                {
                    rightLimit = JavascriptArray::MaxArrayLength;
                }
            }
        }

        bool hasInlineSegment = JavascriptArray::IsInlineSegment(startSeg, pArr);
        if (startSeg)
        {
            // Delete Phase
            if (startSeg->left <= start && (startSeg->left + startSeg->length) >= limit)
            {
                // All splice happens in one segment.
                SparseArraySegmentBase *nextSeg = startSeg->next;
                // Splice the segment first, which might OOM throw but the array would be intact.
                JavascriptArray::ArraySegmentSpliceHelper(pnewArr, (SparseArraySegment<T>*)startSeg, (SparseArraySegment<T>**)prevSeg, start, deleteLen, insertArgs, insertLen, recycler);
                while (nextSeg)
                {
                    // adjust next segments left
                    nextSeg->left = nextSeg->left - deleteLen + insertLen;
                    if (nextSeg->next == nullptr)
                    {
                        nextSeg->EnsureSizeInBound();
                    }
                    nextSeg = nextSeg->next;
                }
                if (*prevSeg)
                {
                    (*prevSeg)->EnsureSizeInBound();
                }
                return;
            }
            else
            {
                SparseArraySegment<T>* newHeadSeg = nullptr; // pnewArr->head is null
                Field(SparseArraySegmentBase*)* prevNewHeadSeg = &pnewArr->head;

                // delete till deleteLen and reuse segments for new array if it is possible.
                // 3 steps -
                //1. delete 1st segment (which may be partial delete)
                // 2. delete next n complete segments
                // 3. delete last segment (which again may be partial delete)

                // Step (1)  -- WOOB 1116297: When left >= start, step (1) is skipped, resulting in pNewArr->head->left != 0. We need to touch up pNewArr.
                if (startSeg->left < start)
                {
                    if (start < startSeg->left + startSeg->length)
                    {
                        uint32 headDeleteLen = startSeg->left + startSeg->length - start;

                        if (startSeg->next)
                        {
                            // We know the new segment will have a next segment, so allocate it as non-leaf.
                            newHeadSeg = SparseArraySegment<T>::template AllocateSegmentImpl<false>(recycler, 0, headDeleteLen, headDeleteLen, nullptr);
                        }
                        else
                        {
                            newHeadSeg = SparseArraySegment<T>::AllocateSegment(recycler, 0, headDeleteLen, headDeleteLen, nullptr);
                        }
                        newHeadSeg = SparseArraySegment<T>::CopySegment(recycler, newHeadSeg, 0, startSeg, start, headDeleteLen);
                        newHeadSeg->next = nullptr;
                        *prevNewHeadSeg = newHeadSeg;
                        prevNewHeadSeg = &newHeadSeg->next;
                        startSeg->Truncate(start);
                    }
                    savePrev = startSeg;
                    prevPrevSeg = prevSeg;
                    prevSeg = &startSeg->next;
                    startSeg = SparseArraySegment<T>::From(startSeg->next);
                }

                // Step (2) first we should do a hard copy if we have an inline head Segment
                else if (hasInlineSegment && nullptr != startSeg)
                {
                    // start should be in between left and left + length
                    if (startSeg->left  <= start && start < startSeg->left + startSeg->length)
                    {
                        uint32 headDeleteLen = startSeg->left + startSeg->length - start;
                        if (startSeg->next)
                        {
                            // We know the new segment will have a next segment, so allocate it as non-leaf.
                            newHeadSeg = SparseArraySegment<T>::template AllocateSegmentImpl<false>(recycler, 0, headDeleteLen, headDeleteLen, nullptr);
                        }
                        else
                        {
                            newHeadSeg = SparseArraySegment<T>::AllocateSegment(recycler, 0, headDeleteLen, headDeleteLen, nullptr);
                        }
                        newHeadSeg = SparseArraySegment<T>::CopySegment(recycler, newHeadSeg, 0, startSeg, start, headDeleteLen);
                        *prevNewHeadSeg = newHeadSeg;
                        prevNewHeadSeg = &newHeadSeg->next;

                        // Remove the entire segment from the original array
                        *prevSeg = startSeg->next;
                        pArr->ClearElements(startSeg, 0);
                        startSeg = SparseArraySegment<T>::From(startSeg->next);
                    }
                    // if we have an inline head segment with 0 elements, remove it
                    else if (startSeg->left == 0 && startSeg->length == 0)
                    {
                        Assert(startSeg->size != 0);
                        *prevSeg = startSeg->next;
                        startSeg = SparseArraySegment<T>::From(startSeg->next);
                    }
                }
                // Step (2) proper
                SparseArraySegmentBase *temp = nullptr;
                while (startSeg && (startSeg->left + startSeg->length) <= limit)
                {
                    temp = startSeg->next;

                    // move that entire segment to new array
                    startSeg->left = startSeg->left - start;
                    startSeg->next = nullptr;
                    *prevNewHeadSeg = startSeg;
                    prevNewHeadSeg = &startSeg->next;

                    // Remove the entire segment from the original array
                    *prevSeg = temp;
                    startSeg = (SparseArraySegment<T>*)temp;
                }

                // Step(2) above could delete the original head segment entirely, causing current head not
                // starting from 0. Then if any of the following throw, we have a corrupted array. Need
                // protection here.
                bool dummyHeadNodeInserted = false;
                if (!savePrev && (!startSeg || startSeg->left != 0))
                {
                    Assert(pArr->head == startSeg);
                    pArr->EnsureHeadStartsFromZero<T>(recycler);
                    Assert(pArr->head && pArr->head->next == startSeg);

                    savePrev = pArr->head;
                    prevPrevSeg = prevSeg;
                    prevSeg = &pArr->head->next;
                    dummyHeadNodeInserted = true;
                }

                // Step (3)
                if (startSeg && (startSeg->left < limit))
                {
                    // copy the first part of the last segment to be deleted to new array
                    uint32 headDeleteLen = start + deleteLen - startSeg->left ;

                    newHeadSeg = SparseArraySegment<T>::AllocateSegment(recycler, startSeg->left -  start, headDeleteLen, (SparseArraySegmentBase *)nullptr);
                    newHeadSeg = SparseArraySegment<T>::CopySegment(recycler, newHeadSeg, startSeg->left -  start, startSeg, startSeg->left, headDeleteLen);
                    newHeadSeg->next = nullptr;
                    *prevNewHeadSeg = newHeadSeg;
                    prevNewHeadSeg = &newHeadSeg->next;

                    // move the last segment
                    MoveArray(startSeg->elements, startSeg->elements + headDeleteLen, startSeg->length - headDeleteLen);
                    startSeg->left = startSeg->left + headDeleteLen; // We are moving the left ahead to point to the right index
                    startSeg->length = startSeg->length - headDeleteLen;
                    startSeg->CheckLengthvsSize();
                    startSeg->Truncate(startSeg->left + startSeg->length);
                    startSeg->EnsureSizeInBound(); // Just truncated, size might exceed next.left
                }

                if (startSeg && ((startSeg->left - deleteLen + insertLen) == 0) && dummyHeadNodeInserted)
                {
                    Assert(start + insertLen == 0);
                    // Remove the dummy head node to preserve array consistency.
                    pArr->head = startSeg;
                    savePrev = nullptr;
                    prevSeg = &pArr->head;
                }

                while (startSeg)
                {
                    startSeg->left = startSeg->left - deleteLen + insertLen ;
                    if (startSeg->next == nullptr)
                    {
                        startSeg->EnsureSizeInBound();
                    }
                    startSeg = SparseArraySegment<T>::From(startSeg->next);
                }
            }
        }

        // The size of pnewArr head allocated in above step 1 might exceed next.left concatenated in step 2/3.
        pnewArr->head->EnsureSizeInBound();
        if (savePrev)
        {
            savePrev->EnsureSizeInBound();
        }

        // insert elements
        if (insertLen > 0)
        {
            Assert(!JavascriptNativeIntArray::Is(pArr) && !JavascriptNativeFloatArray::Is(pArr));

            // InsertPhase
            SparseArraySegment<T> *segInsert = nullptr;

            // see if we are just about the right of the previous segment
            Assert(!savePrev || savePrev->left <= start);
            if (savePrev && (start - savePrev->left < savePrev->size))
            {
                segInsert = (SparseArraySegment<T>*)savePrev;
                uint32 spaceLeft = segInsert->size - (start - segInsert->left);
                if(spaceLeft < insertLen)
                {
                    SparseArraySegment<T> *oldSegInsert = segInsert;
                    bool isInlineSegment = JavascriptArray::IsInlineSegment(segInsert, pArr);
                    if (!segInsert->next)
                    {
                        segInsert = segInsert->GrowByMin(recycler, insertLen - spaceLeft);
                    }
                    else
                    {
                        segInsert = segInsert->GrowByMinMax(recycler, insertLen - spaceLeft, segInsert->next->left - segInsert->left - segInsert->size);
                    }

                    if (isInlineSegment)
                    {
                        pArr->ClearElements(oldSegInsert, 0);
                    }
                }
                *prevPrevSeg = segInsert;
                segInsert->length = start + insertLen - segInsert->left;
                segInsert->CheckLengthvsSize();
            }
            else
            {
                segInsert = SparseArraySegment<T>::AllocateSegment(recycler, start, insertLen, *prevSeg);
                segInsert->next = *prevSeg;
                *prevSeg = segInsert;
                savePrev = segInsert;
            }

            uint32 relativeStart = start - segInsert->left;
            // inserted elements starts at argument 3 of splice(start, deleteNumber, insertelem1, insertelem2, insertelem3, ...);
            CopyArray(segInsert->elements + relativeStart, insertLen,
                      reinterpret_cast<const T*>(insertArgs), insertLen);
        }
    }

    Var JavascriptArray::TryArraySplice(JavascriptArray* pArr, uint32 start, uint32 len, uint32 deleteLen,
        Var* insertArgs, uint32 insertLen, ScriptContext *scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(pArr != nullptr);

        RecyclableObject* newObj = nullptr;
        Recycler *recycler = scriptContext->GetRecycler();

        ::Math::RecordOverflowPolicy newLenOverflow;
        uint32 newLen = UInt32Math::Add(len - deleteLen, insertLen, newLenOverflow); // new length of the array after splice

        // If we have missing values then convert to not native array for now
        // In future, we could support this scenario.
        if (deleteLen == insertLen)
        {
            JS_REENTRANT(jsReentLock, pArr->FillFromPrototypes(start, start + deleteLen));
        }
        else if (len)
        {
            JS_REENTRANT(jsReentLock, pArr->FillFromPrototypes(start, len));
        }

        //
        // If newLen overflowed, pre-process to prevent pushing sparse array segments or elements out of
        // max array length, which would result in tons of index overflow and difficult to fix.
        //
        if (newLenOverflow.HasOverflowed())
        {
            pArr = EnsureNonNativeArray(pArr);
            BigIndex dstIndex = MaxArrayLength;

            uint32 maxInsertLen = MaxArrayLength - start;
            if (insertLen > maxInsertLen)
            {
                // Copy overflowing insertArgs to properties
                for (uint32 i = maxInsertLen; i < insertLen; i++)
                {
                    pArr->GenericDirectSetItemAt(dstIndex, insertArgs[i]);
                    ++dstIndex;
                }

                insertLen = maxInsertLen; // update

                                          // Truncate elements on the right to properties
                if (start + deleteLen < len)
                {
                    pArr->TruncateToProperties(dstIndex, start + deleteLen);
                }
            }
            else
            {
                // Truncate would-overflow elements to properties
                pArr->TruncateToProperties(dstIndex, MaxArrayLength - insertLen + deleteLen);
            }

            len = pArr->length; // update
            newLen = len - deleteLen + insertLen;
            Assert(newLen == MaxArrayLength);
        }

        if (insertArgs)
        {
            pArr = EnsureNonNativeArray(pArr);
        }

        bool isIntArray = false;
        bool isFloatArray = false;
        bool isBuiltinArrayCtor = true;
        JavascriptArray *newArr = nullptr;

        // Just dump the segment map on splice (before any possible allocation and throw)
        pArr->ClearSegmentMap();

        // If the source object is an Array exotic object (Array.isArray) we should try to load the constructor property
        // and use it to construct the return object.
        JS_REENTRANT(jsReentLock, newObj = ArraySpeciesCreate(pArr, deleteLen, scriptContext, nullptr, nullptr, &isBuiltinArrayCtor));
        if (newObj != nullptr)
        {
            pArr = EnsureNonNativeArray(pArr);
            // If the new object we created is an array, remember that as it will save us time setting properties in the object below
            if (JavascriptArray::Is(newObj))
            {
#if ENABLE_COPYONACCESS_ARRAY
                JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(newObj);
#endif
                newArr = JavascriptArray::FromVar(newObj);
            }
        }
        else
            // This is the ES5 case, pArr['constructor'] doesn't exist, or pArr['constructor'] is the builtin Array constructor
        {
            pArr->GetArrayTypeAndConvert(&isIntArray, &isFloatArray);
            newArr = CreateNewArrayHelper(deleteLen, isIntArray, isFloatArray, pArr, scriptContext);
#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(newArr);
#endif
        }

        // If return object is a JavascriptArray, we can use all the array splice helpers
        if (newArr && isBuiltinArrayCtor && len == pArr->length)
        {
            // Code below has potential to throw due to OOM or SO. Just FailFast on those cases
            AutoDisableInterrupt failFastOnError(scriptContext->GetThreadContext());

            // Array has a single segment (need not start at 0) and splice start lies in the range
            // of that segment we optimize splice - Fast path.
            if (pArr->IsSingleSegmentArray() && pArr->head->HasIndex(start))
            {
                SparseArraySegmentBase *oldHead = pArr->head;
                bool isInlineSegment = JavascriptArray::IsInlineSegment(oldHead, pArr);
                if (isIntArray)
                {
                    ArraySegmentSpliceHelper<int32>(newArr, SparseArraySegment<int32>::From(pArr->head), (SparseArraySegment<int32>**)&pArr->head, start, deleteLen, insertArgs, insertLen, recycler);
                }
                else if (isFloatArray)
                {
                    ArraySegmentSpliceHelper<double>(newArr, SparseArraySegment<double>::From(pArr->head), (SparseArraySegment<double>**)&pArr->head, start, deleteLen, insertArgs, insertLen, recycler);
                }
                else
                {
                    ArraySegmentSpliceHelper<Var>(newArr, SparseArraySegment<Var>::From(pArr->head), (SparseArraySegment<Var>**)&pArr->head, start, deleteLen, insertArgs, insertLen, recycler);
                }

                if (isInlineSegment && oldHead != pArr->head)
                {
                    pArr->ClearElements(oldHead, 0);
                }

                // Since the start index is within the bounds of the original array's head segment, it will not acquire any new
                // missing values. If the original array had missing values in the head segment, some of them may have been
                // copied into the array that will be returned; otherwise, the array that is returned will also not have any
                // missing values.
                newArr->SetHasNoMissingValues(pArr->HasNoMissingValues());
            }
            else
            {
                if (isIntArray)
                {
                    ArraySpliceHelper<int32>(newArr, pArr, start, deleteLen, insertArgs, insertLen, scriptContext);
                }
                else if (isFloatArray)
                {
                    ArraySpliceHelper<double>(newArr, pArr, start, deleteLen, insertArgs, insertLen, scriptContext);
                }
                else
                {
                    ArraySpliceHelper<Var>(newArr, pArr, start, deleteLen, insertArgs, insertLen, scriptContext);
                }

                // This function currently does not track missing values in the head segment if there are multiple segments
                pArr->SetHasNoMissingValues(false);
                newArr->SetHasNoMissingValues(false);
            }

            if (isIntArray)
            {
                pArr->EnsureHeadStartsFromZero<int32>(recycler);
                newArr->EnsureHeadStartsFromZero<int32>(recycler);
            }
            else if (isFloatArray)
            {
                pArr->EnsureHeadStartsFromZero<double>(recycler);
                newArr->EnsureHeadStartsFromZero<double>(recycler);
            }
            else
            {
                pArr->EnsureHeadStartsFromZero<Var>(recycler);
                newArr->EnsureHeadStartsFromZero<Var>(recycler);
            }

            pArr->InvalidateLastUsedSegment();

            // it is possible for valueOf accessors for the start or deleteLen
            // arguments to modify the size of the array. Since the resulting size of the array
            // is based on the cached value of length, this might lead to us having to trim
            // excess array segments at the end of the splice operation, which SetLength() will do.
            // However, this is also slower than performing the simple length assignment, so we only
            // do it if we can detect the array length changing.
            if (pArr->length != len)
            {
                pArr->SetLength(newLen);
            }
            else
            {
                pArr->length = newLen;
            }

            if (newArr->length != deleteLen)
            {
                newArr->SetLength(deleteLen);
            }
            else
            {
                newArr->length = deleteLen;
            }

            failFastOnError.Completed();

            newArr->InvalidateLastUsedSegment();

#ifdef VALIDATE_ARRAY
            newArr->ValidateArray();
            pArr->ValidateArray();
#endif
            if (newLenOverflow.HasOverflowed())
            {
                // ES5 15.4.4.12 16: If new len overflowed, SetLength throws
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthAssignIncorrect);
            }

            return newArr;
        }

        if (newLenOverflow.HasOverflowed())
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return ObjectSpliceHelper<uint64>(pArr, len, start, deleteLen, insertArgs, insertLen, scriptContext, newObj));
        }
        else // Use uint32 version if no overflow
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return ObjectSpliceHelper<uint32>(pArr, len, start, deleteLen, insertArgs, insertLen, scriptContext, newObj));
        }

    }

    template<typename T>
    RecyclableObject* JavascriptArray::ObjectSpliceHelper(RecyclableObject* pObj, T len, T start,
        T deleteLen, Var* insertArgs, uint32 insertLen, ScriptContext *scriptContext, RecyclableObject* pNewObj)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());
        JavascriptArray *pnewArr = nullptr;

        if (pNewObj == nullptr)
        {
            JS_REENTRANT(jsReentLock, pNewObj = ArraySpeciesCreate(pObj, deleteLen, scriptContext));
            if (pNewObj == nullptr)
            {
                if (deleteLen > UINT_MAX)
                {
                    JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthConstructIncorrect);
                }

                pnewArr = scriptContext->GetLibrary()->CreateArray(static_cast<uint32>(deleteLen));
                pnewArr->EnsureHead<Var>();

                pNewObj = pnewArr;
            }
        }

        if (JavascriptArray::Is(pNewObj))
        {
#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(pNewObj);
#endif
            pnewArr = JavascriptArray::FromVar(pNewObj);
        }

        // copy elements to delete to new array
        if (pnewArr != nullptr)
        {
            for (uint32 i = 0; i < deleteLen; i++)
            {
                JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(pObj, start + i));
                if (hasItem)
                {
                    JS_REENTRANT(jsReentLock, Var element = JavascriptOperators::GetItem(pObj, start + i, scriptContext));
                    pnewArr->SetItem(i, element, PropertyOperation_None);
                }
            }
        }
        else
        {
            BigIndex k = 0u;
            for (T i = 0u; i < deleteLen; i++)
            {
                JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(pObj, start + i));
                if (hasItem)
                {
                    Var element = nullptr;
                    JS_REENTRANT(jsReentLock, element = JavascriptOperators::GetItem(pObj, start + i, scriptContext),
                        ThrowErrorOnFailure(JavascriptArray::SetArrayLikeObjects(pNewObj, k, element), scriptContext, k));
                }
                ++k;
            }
        }

        ThrowTypeErrorOnFailureHelper h(scriptContext, _u("Array.prototype.splice"));

        // Now we need reserve room if it is necessary
        if (insertLen > deleteLen) // Might overflow max array length
        {
            // Unshift [start + deleteLen, len) to start + insertLen
            JS_REENTRANT(jsReentLock, Unshift<BigIndex, T>(pObj, start + insertLen, start + deleteLen, len, scriptContext));
        }
        else if (insertLen < deleteLen) // Won't overflow max array length
        {
            T j = 0;
            for (T i = start + deleteLen; i < len; i++)
            {
                JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(pObj, i));
                if (hasItem)
                {
                    Var element = nullptr;
                    JS_REENTRANT(jsReentLock, element = JavascriptOperators::GetItem(pObj, i, scriptContext),
                        h.ThrowTypeErrorOnFailure(JavascriptOperators::SetItem(pObj, pObj, start + insertLen + j, element, scriptContext, PropertyOperation_ThrowIfNotExtensible)));
                }
                else
                {
                    JS_REENTRANT(jsReentLock,
                        h.ThrowTypeErrorOnFailure(JavascriptOperators::DeleteItem(pObj, start + insertLen + j, PropertyOperation_ThrowOnDeleteIfNotConfig)));
                }
                j++;
            }

            // Clean up the rest
            for (T i = len; i > len - deleteLen + insertLen; i--)
            {
                JS_REENTRANT(jsReentLock, h.ThrowTypeErrorOnFailure(JavascriptOperators::DeleteItem(pObj, i - 1, PropertyOperation_ThrowOnDeleteIfNotConfig)));
            }
        }

        if (insertLen > 0)
        {
            T dstIndex = start; // insert index might overflow max array length
            for (uint32 i = 0; i < insertLen; i++)
            {
                JS_REENTRANT(jsReentLock,
                    h.ThrowTypeErrorOnFailure(IndexTrace<BigIndex>::SetItem(pObj, dstIndex, insertArgs[i], PropertyOperation_ThrowIfNotExtensible)));
                ++dstIndex;
            }
        }

        // Set up new length
        T newLen = T(len - deleteLen) + insertLen;
        JS_REENTRANT(jsReentLock,
            h.ThrowTypeErrorOnFailure(JavascriptOperators::SetProperty(pObj, pObj, PropertyIds::length, IndexTrace<BigIndex>::ToNumber(newLen, scriptContext), scriptContext, PropertyOperation_ThrowIfNotExtensible)),
            h.ThrowTypeErrorOnFailure(JavascriptOperators::SetProperty(pNewObj, pNewObj, PropertyIds::length, IndexTrace<BigIndex>::ToNumber(deleteLen, scriptContext), scriptContext, PropertyOperation_ThrowIfNotExtensible)));
#ifdef VALIDATE_ARRAY
        if (pnewArr)
        {
            pnewArr->ValidateArray();
        }
#endif
        return pNewObj;
    }

    Var JavascriptArray::EntryToLocaleString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedObject, _u("Array.prototype.toLocaleString"));
        }

        if (JavascriptArray::IsDirectAccessArray(args[0]))
        {
            JavascriptArray* arr = JavascriptArray::FromVar(args[0]);
            JS_REENTRANT_UNLOCK(jsReentLock, return ToLocaleString(arr, scriptContext));
        }
        else
        {
            if (TypedArrayBase::IsDetachedTypedArray(args[0]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray, _u("Array.prototype.toLocalString"));
            }

            RecyclableObject* obj = nullptr;
            if (FALSE == JavascriptConversion::ToObject(args[0], scriptContext, &obj))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.toLocaleString"));
            }
            JS_REENTRANT_UNLOCK(jsReentLock, return ToLocaleString(obj, scriptContext));
        }
    }

    //
    // Unshift object elements [start, end) to toIndex, asserting toIndex > start.
    //
    template<typename T, typename P>
    void JavascriptArray::Unshift(RecyclableObject* obj, const T& toIndex, P start, P end, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        typedef IndexTrace<T> index_trace;

        ThrowTypeErrorOnFailureHelper h(scriptContext, _u("Array.prototype.unshift"));
        if (start < end)
        {
            T newEnd = (end - start - 1);// newEnd - 1
            T dst = toIndex + newEnd;
            for (P i = end; i > start; --i)
            {
                JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(obj, i - 1));
                if (hasItem)
                {
                    Var element = nullptr;
                    JS_REENTRANT(jsReentLock, element = JavascriptOperators::GetItem(obj, i - 1, scriptContext),
                        h.ThrowTypeErrorOnFailure(index_trace::SetItem(obj, dst, element, PropertyOperation_ThrowIfNotExtensible)));
                }
                else
                {
                    JS_REENTRANT(jsReentLock, h.ThrowTypeErrorOnFailure(index_trace::DeleteItem(obj, dst, PropertyOperation_ThrowOnDeleteIfNotConfig)));
                }

                --dst;
            }
        }
    }

    template<typename T>
    void JavascriptArray::GrowArrayHeadHelperForUnshift(JavascriptArray* pArr, uint32 unshiftElements, ScriptContext * scriptContext)
    {
        SparseArraySegmentBase* nextToHeadSeg = pArr->head->next;
        Recycler* recycler = scriptContext->GetRecycler();

        SparseArraySegmentBase *oldHead = pArr->head;
        bool isInlineSegment = JavascriptArray::IsInlineSegment(oldHead, pArr);
        if (nextToHeadSeg == nullptr)
        {
            pArr->EnsureHead<T>();
            pArr->head = SparseArraySegment<T>::From(pArr->head)->GrowByMin(recycler, unshiftElements);
        }
        else
        {
            pArr->head = SparseArraySegment<T>::From(pArr->head)->GrowByMinMax(recycler, unshiftElements, ((nextToHeadSeg->left + unshiftElements) - pArr->head->left - pArr->head->size));
        }

        if (isInlineSegment)
        {
            pArr->ClearElements(oldHead, 0);
        }
    }

    template<typename T>
    void JavascriptArray::UnshiftHelper(JavascriptArray* pArr, uint32 unshiftElements, Js::Var * elements)
    {
        SparseArraySegment<T>* head = SparseArraySegment<T>::From(pArr->head);
        // Make enough room in the head segment to insert new elements at the front
        MoveArray(head->elements + unshiftElements, head->elements, pArr->head->length);
        uint32 oldHeadLength = head->length;
        head->length += unshiftElements;
        head->CheckLengthvsSize();

        /* Set head segment as the last used segment */
        pArr->InvalidateLastUsedSegment();

        bool hasNoMissingValues = pArr->HasNoMissingValues();

        /* Set HasNoMissingValues to false -> Since we shifted elements right, we might have missing values after the memmove */
        if(unshiftElements > oldHeadLength)
        {
            pArr->SetHasNoMissingValues(false);
        }

#if ENABLE_PROFILE_INFO
        pArr->FillFromArgs(unshiftElements, 0, elements, nullptr, true/*dontCreateNewArray*/);
#else
        pArr->FillFromArgs(unshiftElements, 0, elements, true/*dontCreateNewArray*/);
#endif

        // Setting back to the old value
        pArr->SetHasNoMissingValues(hasNoMissingValues);
    }

    Var JavascriptArray::EntryUnshift(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        Var res = scriptContext->GetLibrary()->GetUndefined();

        if (args.Info.Count == 0)
        {
           return res;
        }

        JavascriptArray * pArr = nullptr;
        if (JavascriptArray::Is(args[0])
            && !JavascriptArray::FromVar(args[0])->IsCrossSiteObject())
        {
#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(args[0]);
#endif
            pArr = JavascriptArray::FromVar(args[0]);
        }

            uint32 unshiftElements = args.Info.Count - 1;

        // forceCheckProtoChain - since the array expand to accommodate new items thus we have to check if we have accessor on the proto chain.
        bool useNoSideEffectUnshift = pArr != nullptr && (unshiftElements == 0 || !HasAnyES5ArrayInPrototypeChain(pArr, true /*forceCheckProtoChain*/));

        if (useNoSideEffectUnshift)
        {
            if (unshiftElements > 0)
            {
                uint32 length = pArr->length;
                if (pArr->IsFillFromPrototypes())
                {
                    JS_REENTRANT(jsReentLock, pArr->FillFromPrototypes(0, pArr->length)); // We need find all missing value from [[proto]] object
                }

                // As we have already established that the FillFromPrototype should not change the bound of the array.
                if (length != pArr->length)
                {
                    Js::Throw::FatalInternalError();
                }

                // Pre-process: truncate overflowing elements to properties
                bool newLenOverflowed = false;
                uint32 maxLen = MaxArrayLength - unshiftElements;
                if (pArr->length > maxLen)
                {
                    newLenOverflowed = true;
                    // Ensure the array is non-native when overflow happens
                    EnsureNonNativeArray(pArr);
                    pArr->TruncateToProperties(MaxArrayLength, maxLen);
                    Assert(pArr->length + unshiftElements == MaxArrayLength);
                }

                pArr->ClearSegmentMap(); // Dump segmentMap on unshift (before any possible allocation and throw)

                Assert(pArr->length <= MaxArrayLength - unshiftElements);

                bool isIntArray = false;
                bool isFloatArray = false;

                if (JavascriptNativeIntArray::Is(pArr))
                {
                    isIntArray = true;
                }
                else if (JavascriptNativeFloatArray::Is(pArr))
                {
                    isFloatArray = true;
                }

                // If we need to grow head segment and there is already a next segment, then allocate the new head segment upfront
                // If there is OOM in array allocation, then array consistency is maintained.
                if (pArr->head->size < pArr->head->length + unshiftElements)
                {
                    if (isIntArray)
                    {
                        GrowArrayHeadHelperForUnshift<int32>(pArr, unshiftElements, scriptContext);
                    }
                    else if (isFloatArray)
                    {
                        GrowArrayHeadHelperForUnshift<double>(pArr, unshiftElements, scriptContext);
                    }
                    else
                    {
                        GrowArrayHeadHelperForUnshift<Var>(pArr, unshiftElements, scriptContext);
                    }
                }

                SparseArraySegmentBase* renumberSeg = pArr->head->next;

                while (renumberSeg)
                {
                    renumberSeg->left += unshiftElements;
                    if (renumberSeg->next == nullptr)
                    {
                        // last segment can shift its left + size beyond MaxArrayLength, so truncate if so
                        renumberSeg->EnsureSizeInBound();
                    }
                    renumberSeg = renumberSeg->next;
                }

                try
                {
                    if (isIntArray)
                    {
                        UnshiftHelper<int32>(pArr, unshiftElements, args.Values);
                    }
                    else if (isFloatArray)
                    {
                        UnshiftHelper<double>(pArr, unshiftElements, args.Values);
                    }
                    else
                    {
                        UnshiftHelper<Var>(pArr, unshiftElements, args.Values);
                    }
                }
                catch (...)
                {
                    Js::Throw::FatalInternalError();
                }

                pArr->InvalidateLastUsedSegment();
                pArr->length += unshiftElements;

#ifdef VALIDATE_ARRAY
                pArr->ValidateArray();
#endif

                if (newLenOverflowed) // ES5: throw if new "length" exceeds max array length
                {
                    JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthAssignIncorrect);
                }
            }
            res = JavascriptNumber::ToVar(pArr->length, scriptContext);
        }
        else
        {
            RecyclableObject* dynamicObject = nullptr;
            if (FALSE == JavascriptConversion::ToObject(args[0], scriptContext, &dynamicObject))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.unshift"));
            }

            JS_REENTRANT(jsReentLock, BigIndex length = OP_GetLength(dynamicObject, scriptContext));
            if (unshiftElements > 0)
            {
                uint32 MaxSpaceUint32 = MaxArrayLength - unshiftElements;
                // Note: end will always be a smallIndex either it is less than length in which case it is MaxSpaceUint32
                // or MaxSpaceUint32 is greater than length meaning length is a uint32 number
                BigIndex end = length > MaxSpaceUint32 ? MaxSpaceUint32 : length;
                if (end < length)
                {
                    // Unshift [end, length) to MaxArrayLength
                    // MaxArrayLength + (length - MaxSpaceUint32 - 1) = length + unshiftElements -1
                    if (length.IsSmallIndex())
                    {
                        JS_REENTRANT(jsReentLock, Unshift<BigIndex>(dynamicObject, MaxArrayLength, end.GetSmallIndex(), length.GetSmallIndex(), scriptContext));
                    }
                    else
                    {
                        JS_REENTRANT(jsReentLock, Unshift<BigIndex, uint64>(dynamicObject, MaxArrayLength, (uint64)end.GetSmallIndex(), length.GetBigIndex(), scriptContext));
                    }
                }

                // Unshift [0, end) to unshiftElements
                // unshiftElements + (MaxSpaceUint32 - 0 - 1) = MaxArrayLength -1 therefore this unshift covers up to MaxArrayLength - 1
                JS_REENTRANT(jsReentLock, Unshift<uint32>(dynamicObject, unshiftElements, (uint32)0, end.GetSmallIndex(), scriptContext));

                for (uint32 i = 0; i < unshiftElements; i++)
                {
                    JS_REENTRANT(jsReentLock,
                        JavascriptOperators::SetItem(dynamicObject, dynamicObject, i, args[i + 1], scriptContext, PropertyOperation_ThrowIfNotExtensible, true));
                }
            }

            ThrowTypeErrorOnFailureHelper h(scriptContext, _u("Array.prototype.unshift"));

            //ES6 - update 'length' even if unshiftElements == 0;
            BigIndex newLen = length + unshiftElements;
            res = JavascriptNumber::ToVar(newLen.IsSmallIndex() ? newLen.GetSmallIndex() : newLen.GetBigIndex(), scriptContext);
            JS_REENTRANT(jsReentLock,
                BOOL setLength = JavascriptOperators::SetProperty(dynamicObject, dynamicObject, PropertyIds::length, res, scriptContext, PropertyOperation_ThrowIfNotExtensible));
            h.ThrowTypeErrorOnFailure(setLength);
        }
        return res;

    }

    Var JavascriptArray::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NeedObject);
        }

         // ES5 15.4.4.2: call join, or built-in Object.prototype.toString

        RecyclableObject* obj = nullptr;
        if (FALSE == JavascriptConversion::ToObject(args[0], scriptContext, &obj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.toString"));
        }

        // In ES5 we could be calling a user defined join, even on array. We must [[Get]] join at runtime.
        JS_REENTRANT(jsReentLock, Var join = JavascriptOperators::GetProperty(obj, PropertyIds::join, scriptContext));
        if (JavascriptConversion::IsCallable(join))
        {
            RecyclableObject* func = RecyclableObject::FromVar(join);
            // We need to record implicit call here, because marked the Array.toString as no side effect,
            // but if we call user code here which may have side effect
            ThreadContext * threadContext = scriptContext->GetThreadContext();
            JS_REENTRANT(jsReentLock,
                Var result = threadContext->ExecuteImplicitCall(func, ImplicitCall_ToPrimitive, [=]() -> Js::Var
            {
                // Stack object should have a pre-op bail on implicit call. We shouldn't see them here.
                Assert(!ThreadContext::IsOnStack(obj));

                // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
                CallFlags flags = CallFlags_Value;
                return CALL_FUNCTION(threadContext, func, CallInfo(flags, 1), obj);
            }));

            if(!result)
            {
                // There was an implicit call and implicit calls are disabled. This would typically cause a bailout.
                Assert(threadContext->IsDisableImplicitCall());
                result = scriptContext->GetLibrary()->GetNull();
            }

            return result;
        }
        else
        {
            // call built-in Object.prototype.toString
            JS_REENTRANT_UNLOCK(jsReentLock,
                return CALL_ENTRYPOINT(scriptContext->GetThreadContext(), JavascriptObject::EntryToString, function, CallInfo(1), obj));
        }
    }

#if DEBUG
    BOOL JavascriptArray::GetIndex(const char16* propName, uint32 *pIndex)
    {
        uint32 lu, luDig;

        int32 cch = (int32)wcslen(propName);
        char16* pch = const_cast<char16 *>(propName);

        lu = *pch - '0';
        if (lu > 9)
            return FALSE;

        if (0 == lu)
        {
            *pIndex = 0;
            return 1 == cch;
        }

        while ((luDig = *++pch - '0') < 10)
        {
            // If we overflow 32 bits, ignore the item
            if (lu > 0x19999999)
                return FALSE;
            lu *= 10;
            if(lu > (ULONG_MAX - luDig))
                return FALSE;
            lu += luDig;
        }

        if (pch - propName != cch)
            return FALSE;

        if (lu == JavascriptArray::InvalidIndex)
        {
            // 0xFFFFFFFF is not treated as an array index so that the length can be
            // capped at 32 bits.
            return FALSE;
        }

        *pIndex = lu;
        return TRUE;
    }
#endif

    JavascriptString* JavascriptArray::GetLocaleSeparator(ScriptContext* scriptContext)
    {
#ifdef ENABLE_GLOBALIZATION
        LCID lcid = GetUserDefaultLCID();
        int count = 0;
        char16 szSeparator[6];

        // According to the document for GetLocaleInfo this is a sufficient buffer size.
        count = GetLocaleInfoW(lcid, LOCALE_SLIST, szSeparator, 5);
        if( !count)
        {
            AssertMsg(FALSE, "GetLocaleInfo failed");
            return scriptContext->GetLibrary()->GetCommaSpaceDisplayString();
        }
        else
        {
            // Append ' '  if necessary
            if( count < 2 || szSeparator[count-2] != ' ')
            {
                szSeparator[count-1] = ' ';
                szSeparator[count] = '\0';
            }

            return JavascriptString::NewCopyBuffer(szSeparator, count, scriptContext);
        }
#else
        // xplat-todo: Support locale-specific seperator
        return scriptContext->GetLibrary()->GetCommaSpaceDisplayString();
#endif
    }

    template <typename T>
    JavascriptString* JavascriptArray::ToLocaleString(T* arr, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        uint32 length = 0;
        if (TypedArrayBase::Is(arr))
        {
            // For a TypedArray use the actual length of the array.
            length = TypedArrayBase::FromVar(arr)->GetLength();
        }
        else
        {
            //For anything else, use the "length" property if present.
            JS_REENTRANT(jsReentLock, length = ItemTrace<T>::GetLength(arr, scriptContext));
        }

        if (length == 0 || scriptContext->CheckObject(arr))
        {
            return scriptContext->GetLibrary()->GetEmptyString();
        }

        JavascriptString* res = scriptContext->GetLibrary()->GetEmptyString();
        bool pushedObject = false;

        TryFinally([&]()
        {
            scriptContext->PushObject(arr);
            pushedObject = true;

            Var element = nullptr;
            JS_REENTRANT(jsReentLock, BOOL gotItem = ItemTrace<T>::GetItem(arr, 0, &element, scriptContext));
            if (gotItem)
            {
                JS_REENTRANT(jsReentLock, res = JavascriptArray::ToLocaleStringHelper(element, scriptContext));
            }

            if (length > 1)
            {
                JavascriptString* separator = GetLocaleSeparator(scriptContext);

                for (uint32 i = 1; i < length; i++)
                {
                    res = JavascriptString::Concat(res, separator);
                    JS_REENTRANT(jsReentLock, gotItem = ItemTrace<T>::GetItem(arr, i, &element, scriptContext));
                    if (gotItem)
                    {
                        JS_REENTRANT(jsReentLock, res = JavascriptString::Concat(res, JavascriptArray::ToLocaleStringHelper(element, scriptContext)));
                    }
                }
            }
        },
        [&](bool/*hasException*/)
        {
            if (pushedObject)
            {
                Var top = scriptContext->PopObject();
                AssertMsg(top == arr, "Unmatched operation stack");
            }
        });

        if (res == nullptr)
        {
            res = scriptContext->GetLibrary()->GetEmptyString();
        }

        return res;
    }

    Var JavascriptArray::EntryIsArray(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Array_Constructor_isArray);

        if (args.Info.Count < 2)
        {
            return scriptContext->GetLibrary()->GetFalse();
        }

#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(args[1]);
#endif
        if (JavascriptOperators::IsArray(args[1]))
        {
            return scriptContext->GetLibrary()->GetTrue();
        }
        return scriptContext->GetLibrary()->GetFalse();
    }

    ///----------------------------------------------------------------------------
    /// Find() calls the given predicate callback on each element of the array, in
    /// order, and returns the first element that makes the predicate return true,
    /// as described in (ES6.0: S22.1.3.8).
    ///----------------------------------------------------------------------------
    Var JavascriptArray::EntryFind(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.find"));
        }

        int64 length;
        JavascriptArray * pArr = nullptr;
        RecyclableObject* obj = nullptr;

        JS_REENTRANT_UNLOCK(jsReentLock, TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.find"), &pArr, &obj, &length));

            return JavascriptArray::FindHelper<false>(pArr, nullptr, obj, length, args, scriptContext);
    }

    template <bool findIndex>
    Var JavascriptArray::FindHelper(JavascriptArray* pArr, Js::TypedArrayBase* typedArrayBase, RecyclableObject* obj, int64 length, Arguments& args, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        if (args.Info.Count < 2 || !JavascriptConversion::IsCallable(args[1]))
        {
            // typedArrayBase is only non-null if and only if we came here via the TypedArray entrypoint
            if (typedArrayBase != nullptr)
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, findIndex ? _u("[TypedArray].prototype.findIndex") : _u("[TypedArray].prototype.find"));
            }
            else
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, findIndex ? _u("Array.prototype.findIndex") : _u("Array.prototype.find"));
            }
        }

        RecyclableObject* callBackFn = RecyclableObject::FromVar(args[1]);
        Var thisArg;

        if (args.Info.Count > 2)
        {
            thisArg = args[2];
        }
        else
        {
            thisArg = scriptContext->GetLibrary()->GetUndefined();
        }

        // If we came from Array.prototype.find/findIndex and source object is not a JavascriptArray, source could be a TypedArray
        if (typedArrayBase == nullptr && pArr == nullptr && TypedArrayBase::Is(obj))
        {
            typedArrayBase = TypedArrayBase::FromVar(obj);
        }

        // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
        CallFlags flags = CallFlags_Value;
        Var element = nullptr;
        Var testResult = nullptr;

        if (pArr)
        {
            Var undefined = scriptContext->GetLibrary()->GetUndefined();

            Assert(length <= UINT_MAX);
            for (uint32 k = 0; k < (uint32)length; k++)
            {
                element = undefined;
                JS_REENTRANT(jsReentLock, pArr->DirectGetItemAtFull(k, &element));

                Var index = JavascriptNumber::ToVar(k, scriptContext);

                JS_REENTRANT(jsReentLock,
                    testResult = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 4), thisArg,
                        element,
                        index,
                        pArr));

                if (JavascriptConversion::ToBoolean(testResult, scriptContext))
                {
                    return findIndex ? index : element;
                }

                // Side-effects in the callback function may have changed the source array into an ES5Array. If this happens
                // we will process the rest of the array elements like an ES5Array.
                if (!JavascriptArray::Is(obj))
                {
                    AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
                    JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::FindObjectHelper<findIndex>(obj, length, k + 1, callBackFn, thisArg, scriptContext));
                }
            }
        }
        else if (typedArrayBase)
        {
            Assert(length <= UINT_MAX);
            for (uint32 k = 0; k < (uint32)length; k++)
            {
                // Spec does not ask to call HasItem, so we need to go to visit the whole length

                element = typedArrayBase->DirectGetItem(k);

                Var index = JavascriptNumber::ToVar(k, scriptContext);

                JS_REENTRANT(jsReentLock,
                    testResult = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 4), thisArg,
                        element,
                        index,
                        typedArrayBase));

                if (JavascriptConversion::ToBoolean(testResult, scriptContext))
                {
                    return findIndex ? index : element;
                }
            }
        }
        else
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::FindObjectHelper<findIndex>(obj, length, 0u, callBackFn, thisArg, scriptContext));
        }

        return findIndex ? JavascriptNumber::ToVar(-1, scriptContext) : scriptContext->GetLibrary()->GetUndefined();
    }

    template <bool findIndex>
    Var JavascriptArray::FindObjectHelper(RecyclableObject* obj, int64 length, int64 start, RecyclableObject* callBackFn, Var thisArg, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
        CallFlags flags = CallFlags_Value;
        Var element = nullptr;
        Var testResult = nullptr;

        for (int64 k = start; k < length; k++)
        {
            JS_REENTRANT(jsReentLock, element = JavascriptOperators::GetItem(obj, (uint64)k, scriptContext));
            Var index = JavascriptNumber::ToVar(k, scriptContext);

                JS_REENTRANT(jsReentLock,
                    testResult = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 4), thisArg,
                    element,
                    index,
                        obj));

            if (JavascriptConversion::ToBoolean(testResult, scriptContext))
            {
                return findIndex ? index : element;
            }
        }

        return findIndex ? JavascriptNumber::ToVar(-1, scriptContext) : scriptContext->GetLibrary()->GetUndefined();
    }

    ///----------------------------------------------------------------------------
    /// FindIndex() calls the given predicate callback on each element of the
    /// array, in order, and returns the index of the first element that makes the
    /// predicate return true, as described in (ES6.0: S22.1.3.9).
    ///----------------------------------------------------------------------------
    Var JavascriptArray::EntryFindIndex(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.findIndex"));
        }

        int64 length;
        JavascriptArray * pArr = nullptr;
        RecyclableObject* obj = nullptr;

        JS_REENTRANT_UNLOCK(jsReentLock,
            TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.findIndex"), &pArr, &obj, &length));
            return JavascriptArray::FindHelper<true>(pArr, nullptr, obj, length, args, scriptContext);
    }

    ///----------------------------------------------------------------------------
    /// Entries() returns a new ArrayIterator object configured to return key-
    /// value pairs matching the elements of the this array/array-like object,
    /// as described in (ES6.0: S22.1.3.4).
    ///----------------------------------------------------------------------------
    Var JavascriptArray::EntryEntries(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.entries"));
        }

        RecyclableObject* thisObj = nullptr;
        if (FALSE == JavascriptConversion::ToObject(args[0], scriptContext, &thisObj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.entries"));
        }

#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(thisObj);
#endif
        JS_REENTRANT_UNLOCK(jsReentLock,
            return scriptContext->GetLibrary()->CreateArrayIterator(thisObj, JavascriptArrayIteratorKind::KeyAndValue));
    }

    ///----------------------------------------------------------------------------
    /// Keys() returns a new ArrayIterator object configured to return the keys
    /// of the this array/array-like object, as described in (ES6.0: S22.1.3.13).
    ///----------------------------------------------------------------------------
    Var JavascriptArray::EntryKeys(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.keys"));
        }

        RecyclableObject* thisObj = nullptr;
        if (FALSE == JavascriptConversion::ToObject(args[0], scriptContext, &thisObj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.keys"));
        }

#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(thisObj);
#endif
        JS_REENTRANT_UNLOCK(jsReentLock,
            return scriptContext->GetLibrary()->CreateArrayIterator(thisObj, JavascriptArrayIteratorKind::Key));
    }

    ///----------------------------------------------------------------------------
    /// Values() returns a new ArrayIterator object configured to return the values
    /// of the this array/array-like object, as described in (ES6.0: S22.1.3.29).
    ///----------------------------------------------------------------------------
    Var JavascriptArray::EntryValues(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.values"));
        }

        RecyclableObject* thisObj = nullptr;
        if (FALSE == JavascriptConversion::ToObject(args[0], scriptContext, &thisObj))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.values"));
        }

#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(thisObj);
#endif
        JS_REENTRANT_UNLOCK(jsReentLock,
            return scriptContext->GetLibrary()->CreateArrayIterator(thisObj, JavascriptArrayIteratorKind::Value));
    }

    Var JavascriptArray::EntryEvery(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Array.prototype.every"));

        Assert(!(callInfo.Flags & CallFlags_New));

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Array_Prototype_every);

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.every"));
        }

        BigIndex length;
        JavascriptArray* pArr = nullptr;
        RecyclableObject* obj = nullptr;

        JS_REENTRANT(jsReentLock, TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.every"), &pArr, &obj, &length));

        if (length.IsSmallIndex())
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::EveryHelper(pArr, nullptr, obj, length.GetSmallIndex(), args, scriptContext));
        }
        Assert(pArr == nullptr || length.IsUint32Max()); // if pArr is not null lets make sure length is safe to cast, which will only happen if length is a uint32max
        JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::EveryHelper(pArr, nullptr, obj, length.GetBigIndex(), args, scriptContext));
    }

    // Array.prototype.every as described by ES6.0 (draft 22) Section 22.1.3.5
    template <typename T>
    Var JavascriptArray::EveryHelper(JavascriptArray* pArr, Js::TypedArrayBase* typedArrayBase, RecyclableObject* obj, T length, Arguments& args, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        if (args.Info.Count < 2 || !JavascriptConversion::IsCallable(args[1]))
        {
            // typedArrayBase is only non-null if and only if we came here via the TypedArray entrypoint
            if (typedArrayBase != nullptr)
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("[TypedArray].prototype.every"));
            }
            else
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Array.prototype.every"));
            }
        }

        RecyclableObject* callBackFn = RecyclableObject::FromVar(args[1]);
        Var thisArg = nullptr;


        if (args.Info.Count > 2)
        {
            thisArg = args[2];
        }
        else
        {
            thisArg = scriptContext->GetLibrary()->GetUndefined();
        }

        // If we came from Array.prototype.map and source object is not a JavascriptArray, source could be a TypedArray
        if (typedArrayBase == nullptr && pArr == nullptr && TypedArrayBase::Is(obj))
        {
            typedArrayBase = TypedArrayBase::FromVar(obj);
        }

        Var element = nullptr;
        Var testResult = nullptr;
        // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
        CallFlags flags = CallFlags_Value;

        if (pArr)
        {
            Assert(length <= UINT_MAX);
            for (uint32 k = 0; k < (uint32)length; k++)
            {
                JS_REENTRANT(jsReentLock, BOOL gotItem = pArr->DirectGetItemAtFull(k, &element));
                if (!gotItem)
                {
                    continue;
                }

                JS_REENTRANT(jsReentLock,
                    testResult = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 4), thisArg,
                        element,
                        JavascriptNumber::ToVar(k, scriptContext),
                        pArr));

                if (!JavascriptConversion::ToBoolean(testResult, scriptContext))
                {
                    return scriptContext->GetLibrary()->GetFalse();
                }

                // Side-effects in the callback function may have changed the source array into an ES5Array. If this happens
                // we will process the rest of the array elements like an ES5Array.
                if (!JavascriptArray::Is(obj))
                {
                    AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
                    JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::EveryObjectHelper<T>(obj, length, k + 1, callBackFn, thisArg, scriptContext));
                }
            }
        }
        else if (typedArrayBase)
        {
            AssertAndFailFast(TypedArrayBase::Is(typedArrayBase));
            uint32 end = (uint32)min(length, (T)typedArrayBase->GetLength());

            for (uint32 k = 0; k < end; k++)
            {
                // No need to do HasItem, as it cannot be observable unless 'typedArrayBase' is proxy. And we have established that it is indeed typedarray.

                element = typedArrayBase->DirectGetItem(k);

                JS_REENTRANT(jsReentLock,
                    testResult = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 4), thisArg,
                        element,
                        JavascriptNumber::ToVar(k, scriptContext),
                        typedArrayBase));

                if (!JavascriptConversion::ToBoolean(testResult, scriptContext))
                {
                    return scriptContext->GetLibrary()->GetFalse();
                }
            }
        }
        else
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::EveryObjectHelper<T>(obj, length, 0u, callBackFn, thisArg, scriptContext));
        }

        return scriptContext->GetLibrary()->GetTrue();
    }

    template <typename T>
    Var JavascriptArray::EveryObjectHelper(RecyclableObject* obj, T length, T start, RecyclableObject* callBackFn, Var thisArg, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
        CallFlags flags = CallFlags_Value;
        Var element = nullptr;
        Var testResult = nullptr;

        for (T k = start; k < length; k++)
        {
            // According to es6 spec, we need to call Has first before calling Get
                JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(obj, k));
                if (hasItem)
            {
                    JS_REENTRANT(jsReentLock,
                        element = JavascriptOperators::GetItem(obj, k, scriptContext),
                        testResult = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 4), thisArg,
                        element,
                        JavascriptNumber::ToVar(k, scriptContext),
                            obj));

                if (!JavascriptConversion::ToBoolean(testResult, scriptContext))
                {
                    return scriptContext->GetLibrary()->GetFalse();
                }
            }
        }

        return scriptContext->GetLibrary()->GetTrue();
    }


    Var JavascriptArray::EntrySome(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Array.prototype.some"));

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Array_Prototype_some);

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.some"));
        }

        BigIndex length;
        JavascriptArray* pArr = nullptr;
        RecyclableObject* obj = nullptr;

        JS_REENTRANT(jsReentLock, TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.some"), &pArr, &obj, &length));

        if (length.IsSmallIndex())
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::SomeHelper(pArr, nullptr, obj, length.GetSmallIndex(), args, scriptContext));
        }
        Assert(pArr == nullptr || length.IsUint32Max()); // if pArr is not null lets make sure length is safe to cast, which will only happen if length is a uint32max
        JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::SomeHelper(pArr, nullptr, obj, length.GetBigIndex(), args, scriptContext));
    }

    // Array.prototype.some as described in ES6.0 (draft 22) Section 22.1.3.23
    template <typename T>
    Var JavascriptArray::SomeHelper(JavascriptArray* pArr, Js::TypedArrayBase* typedArrayBase, RecyclableObject* obj, T length, Arguments& args, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        if (args.Info.Count < 2 || !JavascriptConversion::IsCallable(args[1]))
        {
            // We are in the TypedArray version of this API if and only if typedArrayBase != nullptr
            if (typedArrayBase != nullptr)
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("[TypedArray].prototype.some"));
            }
            else
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Array.prototype.some"));
            }
        }

        RecyclableObject* callBackFn = RecyclableObject::FromVar(args[1]);
        Var thisArg = nullptr;

        if (args.Info.Count > 2)
        {
            thisArg = args[2];
        }
        else
        {
            thisArg = scriptContext->GetLibrary()->GetUndefined();
        }

        // If we came from Array.prototype.some and source object is not a JavascriptArray, source could be a TypedArray
        if (typedArrayBase == nullptr && pArr == nullptr && TypedArrayBase::Is(obj))
        {
            typedArrayBase = TypedArrayBase::FromVar(obj);
        }

        // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
        CallFlags flags = CallFlags_Value;
        Var element = nullptr;
        Var testResult = nullptr;

        if (pArr)
        {
            Assert(length <= UINT_MAX);
            for (uint32 k = 0; k < (uint32)length; k++)
            {
                JS_REENTRANT(jsReentLock, BOOL gotItem = pArr->DirectGetItemAtFull(k, &element));
                if (!gotItem)
                {
                    continue;
                }

                JS_REENTRANT_UNLOCK(jsReentLock,
                    testResult = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 4), thisArg,
                        element,
                        JavascriptNumber::ToVar(k, scriptContext),
                        pArr));

                if (JavascriptConversion::ToBoolean(testResult, scriptContext))
                {
                    return scriptContext->GetLibrary()->GetTrue();
                }

                // Side-effects in the callback function may have changed the source array into an ES5Array. If this happens
                // we will process the rest of the array elements like an ES5Array.
                if (!JavascriptArray::Is(obj))
                {
                    AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
                    JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::SomeObjectHelper<T>(obj, length, k + 1, callBackFn, thisArg, scriptContext));
                }
            }
        }
        else if (typedArrayBase)
        {
            AssertAndFailFast(TypedArrayBase::Is(typedArrayBase));
            uint32 end = (uint32)min(length, (T)typedArrayBase->GetLength());

            for (uint32 k = 0; k < end; k++)
            {
                // No need to do HasItem, as it cannot be observable unless 'typedArrayBase' is proxy. And we have established that it is indeed typedarray.

                element = typedArrayBase->DirectGetItem(k);

                JS_REENTRANT_UNLOCK(jsReentLock,
                    testResult = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 4), thisArg,
                        element,
                        JavascriptNumber::ToVar(k, scriptContext),
                        typedArrayBase));

                if (JavascriptConversion::ToBoolean(testResult, scriptContext))
                {
                    return scriptContext->GetLibrary()->GetTrue();
                }
            }
        }
        else
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::SomeObjectHelper<T>(obj, length, 0u, callBackFn, thisArg, scriptContext));
        }

        return scriptContext->GetLibrary()->GetFalse();
    }

    template <typename T>
    Var JavascriptArray::SomeObjectHelper(RecyclableObject* obj, T length, T start, RecyclableObject* callBackFn, Var thisArg, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
        CallFlags flags = CallFlags_Value;
        Var element = nullptr;
        Var testResult = nullptr;

        for (T k = start; k < length; k++)
        {
            JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(obj, k));
            if (hasItem)
            {
                JS_REENTRANT_UNLOCK(jsReentLock,
                    element = JavascriptOperators::GetItem(obj, k, scriptContext),
                    testResult = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 4), thisArg,
                        element,
                        JavascriptNumber::ToVar(k, scriptContext),
                        obj));

                if (JavascriptConversion::ToBoolean(testResult, scriptContext))
                {
                    return scriptContext->GetLibrary()->GetTrue();
                }
            }
        }

        return scriptContext->GetLibrary()->GetFalse();
    }

    Var JavascriptArray::EntryForEach(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Array.prototype.forEach"));

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Array_Prototype_forEach)

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.forEach"));
        }

        BigIndex length;
        JavascriptArray* pArr = nullptr;
        RecyclableObject* dynamicObject = nullptr;
        RecyclableObject* callBackFn = nullptr;
        Var thisArg = nullptr;

        JS_REENTRANT(jsReentLock, TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.forEach"), &pArr, &dynamicObject, &length));

        if (args.Info.Count < 2 || !JavascriptConversion::IsCallable(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Array.prototype.forEach"));
        }
        callBackFn = RecyclableObject::FromVar(args[1]);

        if (args.Info.Count > 2)
        {
            thisArg = args[2];
        }
        else
        {
            thisArg = scriptContext->GetLibrary()->GetUndefined();
        }

        // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
        CallFlags flags = CallFlags_Value;

        auto fn32 = [dynamicObject, callBackFn, flags, thisArg,
            scriptContext](uint32 k, Var element)
        {
            CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 4), thisArg,
                element,
                JavascriptNumber::ToVar(k, scriptContext),
                dynamicObject);
        };

        auto fn64 = [dynamicObject, callBackFn, flags, thisArg,
            scriptContext](uint64 k, Var element)
        {
            CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 4), thisArg,
                element,
                JavascriptNumber::ToVar(k, scriptContext),
                dynamicObject);
        };

        if (pArr)
        {
            Assert(pArr == dynamicObject);
            JS_REENTRANT(jsReentLock, pArr->ForEachItemInRange<true>(0, length.IsUint32Max() ? MaxArrayLength : length.GetSmallIndex(), scriptContext, fn32));
        }
        else
        {
            if (length.IsSmallIndex())
            {
                JS_REENTRANT(jsReentLock, TemplatedForEachItemInRange<true>(dynamicObject, 0u, length.GetSmallIndex(), scriptContext, fn32));
            }
            else
            {
                JS_REENTRANT(jsReentLock, TemplatedForEachItemInRange<true>(dynamicObject, 0ui64, length.GetBigIndex(), scriptContext, fn64));
            }
        }
        return scriptContext->GetLibrary()->GetUndefined();
    }

    Var JavascriptArray::EntryCopyWithin(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        RecyclableObject* obj = nullptr;
        JavascriptArray* pArr = nullptr;
        int64 length;

        JS_REENTRANT_UNLOCK(jsReentLock, TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.copyWithin"), &pArr, &obj, &length));
            return JavascriptArray::CopyWithinHelper(pArr, nullptr, obj, length, args, scriptContext);
    }

    // Array.prototype.copyWithin as defined in ES6.0 (draft 22) Section 22.1.3.3
    Var JavascriptArray::CopyWithinHelper(JavascriptArray* pArr, Js::TypedArrayBase* typedArrayBase, RecyclableObject* obj, int64 length, Arguments& args, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(args.Info.Count > 0);

        JavascriptLibrary* library = scriptContext->GetLibrary();
        int64 fromVal = 0;
        int64 toVal = 0;
        int64 finalVal = length;

        // If we came from Array.prototype.copyWithin and source object is not a JavascriptArray, source could be a TypedArray
        if (typedArrayBase == nullptr && pArr == nullptr && TypedArrayBase::Is(obj))
        {
            typedArrayBase = TypedArrayBase::FromVar(obj);
        }

        if (args.Info.Count > 1)
        {
            JS_REENTRANT(jsReentLock, toVal = JavascriptArray::GetIndexFromVar(args[1], length, scriptContext));

            if (args.Info.Count > 2)
            {
                JS_REENTRANT(jsReentLock, fromVal = JavascriptArray::GetIndexFromVar(args[2], length, scriptContext));

                if (args.Info.Count > 3 && args[3] != library->GetUndefined())
                {
                    JS_REENTRANT(jsReentLock, finalVal = JavascriptArray::GetIndexFromVar(args[3], length, scriptContext));
                }
            }
        }

        // If count would be negative or zero, we won't do anything so go ahead and return early.
        if (finalVal <= fromVal || length <= toVal)
        {
            return obj;
        }

        // Make sure we won't underflow during the count calculation
        Assert(finalVal > fromVal && length > toVal);

        int64 count = min(finalVal - fromVal, length - toVal);

        // We shouldn't have made it here if the count was going to be zero
        Assert(count > 0);

        int direction;

        if (fromVal < toVal && toVal < (fromVal + count))
        {
            direction = -1;
            fromVal += count - 1;
            toVal += count - 1;
        }
        else
        {
            direction = 1;
        }

        // Side effects (such as defining a property in a ToPrimitive call) during evaluation of arguments may convert the array to an ES5 array.
        if (pArr && !JavascriptArray::Is(obj))
        {
            AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
            pArr = nullptr;
        }

        // If we are going to copy elements from or to indices > 2^32-1 we'll execute this (slightly slower path)
        // It's possible to optimize here so that we use the normal code below except for the > 2^32-1 indices
        if ((direction == -1 && (fromVal >= MaxArrayLength || toVal >= MaxArrayLength))
            || (((fromVal + count) > MaxArrayLength) || ((toVal + count) > MaxArrayLength)))
        {
            while (count > 0)
            {
                Var index = JavascriptNumber::ToVar(fromVal, scriptContext);

                JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::OP_HasItem(obj, index, scriptContext));
                if (hasItem)
                {
                    Var val = nullptr;
                    JS_REENTRANT(jsReentLock,
                        val = JavascriptOperators::OP_GetElementI(obj, index, scriptContext),
                        JavascriptOperators::OP_SetElementI(obj, JavascriptNumber::ToVar(toVal, scriptContext), val, scriptContext, PropertyOperation_ThrowIfNotExtensible));
                }
                else
                {
                    JavascriptOperators::OP_DeleteElementI(obj, JavascriptNumber::ToVar(toVal, scriptContext), scriptContext, PropertyOperation_ThrowOnDeleteIfNotConfig);
                }

                fromVal += direction;
                toVal += direction;
                count--;
            }
        }
        else
        {
            Assert(fromVal < MaxArrayLength);
            Assert(toVal < MaxArrayLength);
            Assert(direction == -1 || (fromVal + count < MaxArrayLength && toVal + count < MaxArrayLength));

            uint32 fromIndex = static_cast<uint32>(fromVal);
            uint32 toIndex = static_cast<uint32>(toVal);

            while (count > 0)
            {
                JS_REENTRANT(jsReentLock, BOOL hasItem = obj->HasItem(fromIndex));
                if (hasItem)
                {
                    if (typedArrayBase)
                    {
                        Var val = typedArrayBase->DirectGetItem(fromIndex);

                        JS_REENTRANT(jsReentLock, typedArrayBase->DirectSetItem(toIndex, val));
                    }
                    else if (pArr)
                    {
                        JS_REENTRANT(jsReentLock, Var val = pArr->DirectGetItem(fromIndex));
                        pArr->SetItem(toIndex, val, Js::PropertyOperation_ThrowIfNotExtensible);

                        if (!JavascriptArray::Is(obj))
                        {
                            AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
                            pArr = nullptr;
                        }
                    }
                    else
                    {
                        Var val = nullptr;
                        JS_REENTRANT(jsReentLock,
                            val = JavascriptOperators::OP_GetElementI_UInt32(obj, fromIndex, scriptContext),
                            JavascriptOperators::OP_SetElementI_UInt32(obj, toIndex, val, scriptContext, PropertyOperation_ThrowIfNotExtensible));
                    }
                }
                else
                {
                    obj->DeleteItem(toIndex, PropertyOperation_ThrowOnDeleteIfNotConfig);
                }

                fromIndex += direction;
                toIndex += direction;
                count--;
            }
        }

        return obj;
    }

    Var JavascriptArray::EntryFill(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        RecyclableObject* obj = nullptr;
        JavascriptArray* pArr = nullptr;
        int64 length;

        JS_REENTRANT_UNLOCK(jsReentLock,
            TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.fill"), &pArr, &obj, &length));
            return JavascriptArray::FillHelper(pArr, nullptr, obj, length, args, scriptContext);
    }

    // Array.prototype.fill as defined in ES6.0 (draft 22) Section 22.1.3.6
    Var JavascriptArray::FillHelper(JavascriptArray* pArr, Js::TypedArrayBase* typedArrayBase, RecyclableObject* obj, int64 length, Arguments& args, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(args.Info.Count > 0);

        JavascriptLibrary* library = scriptContext->GetLibrary();

        // If we came from Array.prototype.fill and source object is not a JavascriptArray, source could be a TypedArray
        if (typedArrayBase == nullptr && pArr == nullptr && TypedArrayBase::Is(obj))
        {
            typedArrayBase = TypedArrayBase::FromVar(obj);
        }

        Var fillValue;

        if (args.Info.Count > 1)
        {
            fillValue = args[1];
        }
        else
        {
            fillValue = library->GetUndefined();
        }

        int64 k = 0;
        int64 finalVal = length;

        if (args.Info.Count > 2)
        {
            JS_REENTRANT_UNLOCK(jsReentLock, k = JavascriptArray::GetIndexFromVar(args[2], length, scriptContext));

            if (args.Info.Count > 3 && !JavascriptOperators::IsUndefinedObject(args[3]))
            {
                JS_REENTRANT_UNLOCK(jsReentLock, finalVal = JavascriptArray::GetIndexFromVar(args[3], length, scriptContext));
            }

            // Side-effects in the callback function may have changed the source array into an ES5Array. If this happens
            // we will process the array elements like an ES5Array.
            if (pArr && !JavascriptArray::Is(obj))
            {
                AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
                pArr = nullptr;
            }
        }

        if (k < MaxArrayLength)
        {
            int64 end = min<int64>(finalVal, MaxArrayLength);
            uint32 u32k = static_cast<uint32>(k);

            while (u32k < end)
            {
                if (typedArrayBase)
                {
                    JS_REENTRANT(jsReentLock, typedArrayBase->DirectSetItem(u32k, fillValue));
                }
                else if (pArr)
                {
                    pArr->SetItem(u32k, fillValue, PropertyOperation_ThrowIfNotExtensible);
                }
                else
                {
                    JS_REENTRANT(jsReentLock,
                        JavascriptOperators::OP_SetElementI_UInt32(obj, u32k, fillValue, scriptContext, Js::PropertyOperation_ThrowIfNotExtensible));
                }

                u32k++;
            }

            BigIndex dstIndex = MaxArrayLength;

            for (int64 i = end; i < finalVal; ++i)
            {
                if (pArr)
                {
                    pArr->GenericDirectSetItemAt(dstIndex, fillValue);
                    ++dstIndex;
                }
                else
                {
                    JS_REENTRANT(jsReentLock,
                        JavascriptOperators::OP_SetElementI(obj, JavascriptNumber::ToVar(i, scriptContext), fillValue, scriptContext, Js::PropertyOperation_ThrowIfNotExtensible));
                }
            }
        }
        else
        {
            BigIndex dstIndex = static_cast<uint64>(k);

            for (int64 i = k; i < finalVal; i++)
            {
                if (pArr)
                {
                    pArr->GenericDirectSetItemAt(dstIndex, fillValue);
                    ++dstIndex;
                }
                else
                {
                    JS_REENTRANT(jsReentLock,
                        JavascriptOperators::OP_SetElementI(obj, JavascriptNumber::ToVar(i, scriptContext), fillValue, scriptContext, Js::PropertyOperation_ThrowIfNotExtensible));
                }
            }
        }

        return obj;
    }

    // Array.prototype.map as defined by ES6.0 (Final) 22.1.3.15
    Var JavascriptArray::EntryMap(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());
        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Array.prototype.map"));

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Array_Prototype_map);

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.map"));
        }

        BigIndex length;
        JavascriptArray* pArr = nullptr;
        RecyclableObject* obj = nullptr;

        JS_REENTRANT(jsReentLock, TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.map"), &pArr, &obj, &length));

        if (length.IsSmallIndex())
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::MapHelper(pArr, nullptr, obj, length.GetSmallIndex(), args, scriptContext));
        }
        Assert(pArr == nullptr || length.IsUint32Max()); // if pArr is not null lets make sure length is safe to cast, which will only happen if length is a uint32max
        JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::MapHelper(pArr, nullptr, obj, length.GetBigIndex(), args, scriptContext));
    }


    template<typename T>
    Var JavascriptArray::MapHelper(JavascriptArray* pArr, Js::TypedArrayBase* typedArrayBase, RecyclableObject* obj, T length, Arguments& args, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        RecyclableObject* newObj = nullptr;
        JavascriptArray* newArr = nullptr;
        bool isTypedArrayEntryPoint = typedArrayBase != nullptr;
        bool isBuiltinArrayCtor = true;

        if (args.Info.Count < 2 || !JavascriptConversion::IsCallable(args[1]))
        {
            if (isTypedArrayEntryPoint)
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("[TypedArray].prototype.map"));
            }
            else
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Array.prototype.map"));
            }
        }

        RecyclableObject* callBackFn = RecyclableObject::FromVar(args[1]);
        Var thisArg;

        if (args.Info.Count > 2)
        {
            thisArg = args[2];
        }
        else
        {
            thisArg = scriptContext->GetLibrary()->GetUndefined();
        }

        // If we came from Array.prototype.map and source object is not a JavascriptArray, source could be a TypedArray
        if (!isTypedArrayEntryPoint && pArr == nullptr && TypedArrayBase::Is(obj))
        {
            typedArrayBase = TypedArrayBase::FromVar(obj);
        }

        // If the entry point is %TypedArray%.prototype.map or the source object is an Array exotic object we should try to load the constructor property
        // and use it to construct the return object.
        if (isTypedArrayEntryPoint)
        {
            JS_REENTRANT(jsReentLock,
                Var constructor = JavascriptOperators::SpeciesConstructor(
                    typedArrayBase, TypedArrayBase::GetDefaultConstructor(args[0], scriptContext), scriptContext));

            isBuiltinArrayCtor = false;

            Assert(JavascriptOperators::IsConstructor(constructor));

            Js::Var constructorArgs[] = { constructor, JavascriptNumber::ToVar(length, scriptContext) };
            Js::CallInfo constructorCallInfo(Js::CallFlags_New, _countof(constructorArgs));
            JS_REENTRANT(jsReentLock, newObj = RecyclableObject::FromVar(TypedArrayBase::TypedArrayCreate(constructor, &Js::Arguments(constructorCallInfo, constructorArgs), (uint32)length, scriptContext)));
        }
        // skip the typed array and "pure" array case, we still need to handle special arrays like es5array, remote array, and proxy of array.
        else if (pArr == nullptr || scriptContext->GetConfig()->IsES6SpeciesEnabled())
        {
            JS_REENTRANT(jsReentLock, newObj = ArraySpeciesCreate(obj, length, scriptContext, nullptr, nullptr, &isBuiltinArrayCtor));
        }

        if (newObj == nullptr)
        {
            if (length > UINT_MAX)
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthConstructIncorrect);
            }
            newArr = scriptContext->GetLibrary()->CreateArray(static_cast<uint32>(length));
            newArr->EnsureHead<Var>();
            newObj = newArr;
        }
        else
        {
            // If the new object we created is an array, remember that as it will save us time setting properties in the object below
            if (JavascriptArray::Is(newObj))
            {
#if ENABLE_COPYONACCESS_ARRAY
                JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(newObj);
#endif
                newArr = JavascriptArray::FromVar(newObj);
            }
        }

        Var element = nullptr;
        Var mappedValue = nullptr;
        // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
        CallFlags callBackFnflags = CallFlags_Value;
        CallInfo callBackFnInfo = CallInfo(callBackFnflags, 4);

        // We at least have to have newObj as a valid object
        Assert(newObj);

        // The ArraySpeciesCreate call above could have converted the source array into an ES5Array. If this happens
        // we will process the array elements like an ES5Array.
        if (pArr && !JavascriptArray::Is(obj))
        {
            AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
            pArr = nullptr;
        }

        if (pArr != nullptr)
        {
            // If source is a JavascriptArray, newObj may or may not be an array based on what was in source's constructor property
            Assert(length <= UINT_MAX);
            for (uint32 k = 0; k < (uint32)length; k++)
            {
                JS_REENTRANT(jsReentLock, BOOL gotItem = pArr->DirectGetItemAtFull(k, &element));
                if (!gotItem)
                {
                    continue;
                }

                JS_REENTRANT(jsReentLock,
                    mappedValue = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, callBackFnInfo, thisArg,
                        element,
                        JavascriptNumber::ToVar(k, scriptContext),
                        pArr));

                // If newArr is a valid pointer, then we constructed an array to return. Otherwise we need to do generic object operations
                if (newArr && isBuiltinArrayCtor)
                {
                    newArr->DirectSetItemAt(k, mappedValue);
                }
                else
                {
                    JS_REENTRANT(jsReentLock, ThrowErrorOnFailure(JavascriptArray::SetArrayLikeObjects(newObj, k, mappedValue), scriptContext, k));
                }

                // Side-effects in the callback function may have changed the source array into an ES5Array. If this happens
                // we will process the rest of the array elements like an ES5Array.
                if (!JavascriptArray::Is(obj))
                {
                    AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
                    JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::MapObjectHelper<T>(obj, length, k + 1, newObj, newArr, isBuiltinArrayCtor, callBackFn, thisArg, scriptContext));
                }
            }
        }
        else if (typedArrayBase != nullptr)
        {
            AssertAndFailFast(TypedArrayBase::Is(typedArrayBase));

            // Source is a TypedArray, we may have tried to call a constructor, but newObj may not be a TypedArray (or an array either)
            TypedArrayBase* newTypedArray = nullptr;

            if (TypedArrayBase::Is(newObj))
            {
                newTypedArray = TypedArrayBase::FromVar(newObj);
            }
            else
            {
                AssertAndFailFast(newArr != nullptr);
            }

            Assert(length <= UINT_MAX);
            uint32 end = (uint32)min(length, (T)typedArrayBase->GetLength());

            for (uint32 k = 0; k < end; k++)
            {
                // No need to do HasItem, as it cannot be observable unless 'typedArrayBase' is proxy. And we have established that it is indeed typedarray.

                element = typedArrayBase->DirectGetItem(k);
                JS_REENTRANT(jsReentLock,
                    mappedValue = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, callBackFnInfo, thisArg,
                        element,
                        JavascriptNumber::ToVar(k, scriptContext),
                        obj));

                // If newObj is a TypedArray, set the mappedValue directly, otherwise it should be an array, set that item to that array
                if (newTypedArray)
                {
                    JS_REENTRANT(jsReentLock, newTypedArray->DirectSetItem(k, mappedValue));
                }
                else
                {
                    newArr->SetItem(k, mappedValue, PropertyOperation_None);
                }
            }
        }
        else
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::MapObjectHelper<T>(obj, length, 0u, newObj, newArr, isBuiltinArrayCtor, callBackFn, thisArg, scriptContext));
        }

#ifdef VALIDATE_ARRAY
        if (JavascriptArray::Is(newObj))
        {
            newArr->ValidateArray();
        }
#endif

        return newObj;
    }

    template<typename T>
    Var JavascriptArray::MapObjectHelper(RecyclableObject* obj, T length, T start, RecyclableObject* newObj, JavascriptArray* newArr,
        bool isBuiltinArrayCtor, RecyclableObject* callBackFn, Var thisArg, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
        CallFlags callBackFnflags = CallFlags_Value;
        CallInfo callBackFnInfo = CallInfo(callBackFnflags, 4);
        Var element = nullptr;
        Var mappedValue = nullptr;

        for (T k = start; k < length; k++)
        {
                JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(obj, k));
                if (hasItem)
            {
                    JS_REENTRANT(jsReentLock,
                        element = JavascriptOperators::GetItem(obj, k, scriptContext),
                        mappedValue = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, callBackFnInfo, thisArg,
                        element,
                        JavascriptNumber::ToVar(k, scriptContext),
                            obj));

                if (newArr && isBuiltinArrayCtor)
                {
                    newArr->SetItem((uint32)k, mappedValue, PropertyOperation_None);
                }
                else
                {
                    JS_REENTRANT(jsReentLock, ThrowErrorOnFailure(JavascriptArray::SetArrayLikeObjects(newObj, BigIndex(k), mappedValue), scriptContext, BigIndex(k)));
                }
            }
        }

#ifdef VALIDATE_ARRAY
        if (JavascriptArray::Is(newObj))
        {
            newArr->ValidateArray();
        }
#endif

        return newObj;
    }

    Var JavascriptArray::EntryFilter(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Array.prototype.filter"));

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Array_Prototype_filter);

        Assert(!(callInfo.Flags & CallFlags_New));
        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.filter"));
        }

        BigIndex length;
        JavascriptArray* pArr = nullptr;
        RecyclableObject* obj = nullptr;

        JS_REENTRANT(jsReentLock, TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.filter"), &pArr, &obj, &length));

        if (length.IsSmallIndex())
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::FilterHelper(pArr, obj, length.GetSmallIndex(), args, scriptContext));
        }

        JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::FilterHelper(pArr, obj, length.GetBigIndex(), args, scriptContext));
    }

    template <typename T>
    Var JavascriptArray::FilterHelper(JavascriptArray* pArr, RecyclableObject* obj, T length, Arguments& args, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        if (args.Info.Count < 2 || !JavascriptConversion::IsCallable(args[1]))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Array.prototype.filter"));
        }

        RecyclableObject* callBackFn = RecyclableObject::FromVar(args[1]);
        Var thisArg = nullptr;

        if (args.Info.Count > 2)
        {
            thisArg = args[2];
        }
        else
        {
            thisArg = scriptContext->GetLibrary()->GetUndefined();
        }

        // If the source object is an Array exotic object we should try to load the constructor property and use it to construct the return object.
        bool isBuiltinArrayCtor = true;
        JS_REENTRANT(jsReentLock, RecyclableObject* newObj = ArraySpeciesCreate(obj, 0, scriptContext, nullptr, nullptr, &isBuiltinArrayCtor));
        JavascriptArray* newArr = nullptr;

        if (newObj == nullptr)
        {
            newArr = scriptContext->GetLibrary()->CreateArray(0);
            newArr->EnsureHead<Var>();
            newObj = newArr;
        }
        else
        {
            // If the new object we created is an array, remember that as it will save us time setting properties in the object below
            if (JavascriptArray::Is(newObj))
            {
#if ENABLE_COPYONACCESS_ARRAY
                JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(newObj);
#endif
                newArr = JavascriptArray::FromVar(newObj);
            }
        }

        // The ArraySpeciesCreate call above could have converted the source array into an ES5Array. If this happens
        // we will process the array elements like an ES5Array.
        if (pArr && !JavascriptArray::Is(obj))
        {
            AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
            pArr = nullptr;
        }

        Var element = nullptr;
        Var selected = nullptr;

        if (pArr)
        {
            Assert(length <= MaxArrayLength);
            uint32 i = 0;

            Assert(length <= UINT_MAX);
            for (uint32 k = 0; k < (uint32)length; k++)
            {
                JS_REENTRANT(jsReentLock, BOOL gotItem = pArr->DirectGetItemAtFull(k, &element));
                if (!gotItem)
                {
                    continue;
                }

                JS_REENTRANT(jsReentLock,
                    selected = CALL_ENTRYPOINT(scriptContext->GetThreadContext(),
                        callBackFn->GetEntryPoint(), callBackFn, CallInfo(CallFlags_Value, 4),
                        thisArg,
                        element,
                        JavascriptNumber::ToVar(k, scriptContext),
                        pArr));

                if (JavascriptConversion::ToBoolean(selected, scriptContext))
                {
                    // Try to fast path if the return object is an array
                    if (newArr && isBuiltinArrayCtor)
                    {
                        newArr->DirectSetItemAt(i, element);
                    }
                    else
                    {
                        JS_REENTRANT(jsReentLock, ThrowErrorOnFailure(JavascriptArray::SetArrayLikeObjects(newObj, i, element), scriptContext, i));
                    }
                    ++i;
                }

                // Side-effects in the callback function may have changed the source array into an ES5Array. If this happens
                // we will process the rest of the array elements like an ES5Array.
                if (!JavascriptArray::Is(obj))
                {
                    AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
                    JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::FilterObjectHelper<T>(obj, length, k + 1, newArr, newObj, i, callBackFn, thisArg, scriptContext));
                }
            }
        }
        else
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::FilterObjectHelper<T>(obj, length, 0u, newArr, newObj, 0u, callBackFn, thisArg, scriptContext));
        }

#ifdef VALIDATE_ARRAY
        if (newArr)
        {
            newArr->ValidateArray();
        }
#endif

        return newObj;
    }

    template <typename T>
    Var JavascriptArray::FilterObjectHelper(RecyclableObject* obj, T length, T start, JavascriptArray* newArr, RecyclableObject* newObj, T newStart,
        RecyclableObject* callBackFn, Var thisArg, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Var element = nullptr;
        Var selected = nullptr;
        BigIndex i = BigIndex(newStart);

        for (T k = start; k < length; k++)
        {
                JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(obj, k));
                if (hasItem)
            {
                    JS_REENTRANT(jsReentLock,
                        element = JavascriptOperators::GetItem(obj, k, scriptContext),
                        selected = CALL_ENTRYPOINT(scriptContext->GetThreadContext(),
                            callBackFn->GetEntryPoint(), callBackFn, CallInfo(CallFlags_Value, 4),
                        thisArg,
                        element,
                        JavascriptNumber::ToVar(k, scriptContext),
                            obj));

                if (JavascriptConversion::ToBoolean(selected, scriptContext))
                {
                    if (newArr)
                    {
                        newArr->GenericDirectSetItemAt(i, element);
                    }
                    else
                    {
                            JS_REENTRANT(jsReentLock, ThrowErrorOnFailure(JavascriptArray::SetArrayLikeObjects(newObj, i, element), scriptContext, i));
                    }

                    ++i;
                }
            }
        }

#ifdef VALIDATE_ARRAY
        if (newArr)
        {
            newArr->ValidateArray();
        }
#endif

        return newObj;
    }

    Var JavascriptArray::EntryReduce(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Array.prototype.reduce"));

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Array_Prototype_reduce);

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.reduce"));
        }

        BigIndex length;
        JavascriptArray * pArr = nullptr;
        RecyclableObject* obj = nullptr;
        JS_REENTRANT(jsReentLock, TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.reduce"), &pArr, &obj, &length));

        if (length.IsSmallIndex())
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::ReduceHelper(pArr, nullptr, obj, length.GetSmallIndex(), args, scriptContext));
        }

        JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::ReduceHelper(pArr, nullptr, obj, length.GetBigIndex(), args, scriptContext));
    }

    // Array.prototype.reduce as described in ES6.0 (draft 22) Section 22.1.3.18
    template <typename T>
    Var JavascriptArray::ReduceHelper(JavascriptArray* pArr, Js::TypedArrayBase* typedArrayBase, RecyclableObject* obj, T length, Arguments& args, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        if (args.Info.Count < 2 || !JavascriptConversion::IsCallable(args[1]))
        {
            if (typedArrayBase != nullptr)
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("[TypedArray].prototype.reduce"));
            }
            else
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Array.prototype.reduce"));
            }
        }

        // If we came from Array.prototype.reduce and source object is not a JavascriptArray, source could be a TypedArray
        if (typedArrayBase == nullptr && pArr == nullptr && TypedArrayBase::Is(obj))
        {
            typedArrayBase = TypedArrayBase::FromVar(obj);
        }

        T k = 0;
        RecyclableObject* callBackFn = RecyclableObject::FromVar(args[1]);
        Var accumulator = nullptr;
        Var element = nullptr;

        if (args.Info.Count > 2)
        {
            accumulator = args[2];
        }
        else
        {
            if (length == 0)
            {
                JavascriptError::ThrowTypeError(scriptContext, VBSERR_ActionNotSupported);
            }

            bool bPresent = false;

            if (pArr)
            {
                for (; k < length && bPresent == false; k++)
                {
                    JS_REENTRANT(jsReentLock, BOOL gotItem = pArr->DirectGetItemAtFull((uint32)k, &element));
                    if (!gotItem)
                    {
                        continue;
                    }

                    bPresent = true;
                    accumulator = element;
                }

                // Side-effects in the callback function may have changed the source array into an ES5Array. If this happens
                // we will process the array elements like an ES5Array.
                if (!JavascriptArray::Is(obj))
                {
                    AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
                    pArr = nullptr;
                }
            }
            else if (typedArrayBase)
            {
                AssertAndFailFast(TypedArrayBase::Is(typedArrayBase));
                uint32 end = (uint32)min(length, (T)typedArrayBase->GetLength());

                for (; k < end && bPresent == false; k++)
                {
                    // No need to do HasItem, as it cannot be observable unless 'typedArrayBase' is proxy. And we have established that it is indeed typedarray.

                    element = typedArrayBase->DirectGetItem((uint32)k);

                    bPresent = true;
                    accumulator = element;
                }
            }
            else
            {
                for (; k < length && bPresent == false; k++)
                {
                    JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(obj, k));
                    if (hasItem)
                    {
                        JS_REENTRANT(jsReentLock, accumulator = JavascriptOperators::GetItem(obj, k, scriptContext));
                        bPresent = true;
                    }
                }
            }

            if (bPresent == false)
            {
                JavascriptError::ThrowTypeError(scriptContext, VBSERR_ActionNotSupported);
            }
        }

        Assert(accumulator);

        Var undefinedValue = scriptContext->GetLibrary()->GetUndefined();
        // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
        CallFlags flags = CallFlags_Value;

        if (pArr)
        {
            for (; k < length; k++)
            {
                JS_REENTRANT(jsReentLock, BOOL gotItem = pArr->DirectGetItemAtFull((uint32)k, &element));
                if (!gotItem)
                {
                    continue;
                }

                JS_REENTRANT(jsReentLock,
                    accumulator = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 5),
                        undefinedValue,
                        accumulator,
                        element,
                        JavascriptNumber::ToVar(k, scriptContext),
                        pArr
                ));

                // Side-effects in the callback function may have changed the source array into an ES5Array. If this happens
                // we will process the rest of the array elements like an ES5Array.
                if (!JavascriptArray::Is(obj))
                {
                    AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
                    JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::ReduceObjectHelper<T>(obj, length, k + 1, callBackFn, accumulator, scriptContext));
                }
            }
        }
        else if (typedArrayBase)
        {
            AssertAndFailFast(TypedArrayBase::Is(typedArrayBase));
            uint32 end = (uint32)min(length, (T)typedArrayBase->GetLength());

            for (; k < end; k++)
            {
                // No need to do HasItem, as it cannot be observable unless 'typedArrayBase' is proxy. And we have established that it is indeed typedarray.

                element = typedArrayBase->DirectGetItem((uint32)k);

                JS_REENTRANT(jsReentLock,
                    accumulator = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 5), undefinedValue,
                        accumulator,
                        element,
                        JavascriptNumber::ToVar(k, scriptContext),
                        typedArrayBase));
            }
        }
        else
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::ReduceObjectHelper<T>(obj, length, k, callBackFn, accumulator, scriptContext));
        }

        return accumulator;
    }

    template <typename T>
    Var JavascriptArray::ReduceObjectHelper(RecyclableObject* obj, T length, T start, RecyclableObject* callBackFn, Var accumulator, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
        CallFlags flags = CallFlags_Value;
        Var element = nullptr;

        for (T k = start; k < length; k++)
        {
                JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(obj, k));
                if (hasItem)
            {
                    JS_REENTRANT(jsReentLock,
                        element = JavascriptOperators::GetItem(obj, k, scriptContext),
                        accumulator = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 5), scriptContext->GetLibrary()->GetUndefined(),
                            accumulator,
                            element,
                            JavascriptNumber::ToVar(k, scriptContext),
                            obj));
            }
        }

        return accumulator;
    }

    Var JavascriptArray::EntryReduceRight(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Array.prototype.reduceRight"));

        CHAKRATEL_LANGSTATS_INC_BUILTINCOUNT(Array_Prototype_reduceRight);

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.prototype.reduceRight"));
        }

        BigIndex length;
        JavascriptArray * pArr = nullptr;
        RecyclableObject* obj = nullptr;
        JS_REENTRANT(jsReentLock, TryGetArrayAndLength(args[0], scriptContext, _u("Array.prototype.reduceRight"), &pArr, &obj, &length));

        if (length.IsSmallIndex())
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::ReduceRightHelper(pArr, nullptr, obj, length.GetSmallIndex(), args, scriptContext));
        }

        JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::ReduceRightHelper(pArr, nullptr, obj, length.GetBigIndex(), args, scriptContext));
    }

    // Array.prototype.reduceRight as described in ES6.0 (draft 22) Section 22.1.3.19
    template <typename T>
    Var JavascriptArray::ReduceRightHelper(JavascriptArray* pArr, Js::TypedArrayBase* typedArrayBase, RecyclableObject* obj, T length, Arguments& args, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        if (args.Info.Count < 2 || !JavascriptConversion::IsCallable(args[1]))
        {
            if (typedArrayBase != nullptr)
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("[TypedArray].prototype.reduceRight"));
            }
            else
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Array.prototype.reduceRight"));
            }
        }

        // If we came from Array.prototype.reduceRight and source object is not a JavascriptArray, source could be a TypedArray
        if (typedArrayBase == nullptr && pArr == nullptr && TypedArrayBase::Is(obj))
        {
            typedArrayBase = TypedArrayBase::FromVar(obj);
        }

        RecyclableObject* callBackFn = RecyclableObject::FromVar(args[1]);
        Var accumulator = nullptr;
        Var element = nullptr;
        T k = 0;
        T index = 0;

        if (args.Info.Count > 2)
        {
            accumulator = args[2];
        }
        else
        {
            if (length == 0)
            {
                JavascriptError::ThrowTypeError(scriptContext, VBSERR_ActionNotSupported);
            }

            bool bPresent = false;
            if (pArr)
            {
                for (; k < length && bPresent == false; k++)
                {
                    index = length - k - 1;
                    JS_REENTRANT(jsReentLock, BOOL gotItem = pArr->DirectGetItemAtFull((uint32)index, &element));
                    if (!gotItem)
                    {
                        continue;
                    }
                    bPresent = true;
                    accumulator = element;
                }

                // Side-effects in the callback function may have changed the source array into an ES5Array. If this happens
                // we will process the array elements like an ES5Array.
                if (!JavascriptArray::Is(obj))
                {
                    AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
                    pArr = nullptr;
                }
            }
            else if (typedArrayBase)
            {
                AssertAndFailFast(TypedArrayBase::Is(typedArrayBase));
                uint32 end = (uint32)min(length, (T)typedArrayBase->GetLength());

                for (; k < end && bPresent == false; k++)
                {
                    // No need to do HasItem, as it cannot be observable unless 'typedArrayBase' is proxy. And we have established that it is indeed typedarray.

                    index = length - k - 1;
                    element = typedArrayBase->DirectGetItem((uint32)index);
                    bPresent = true;
                    accumulator = element;
                }
            }
            else
            {
                for (; k < length && bPresent == false; k++)
                {
                    index = length - k - 1;
                    JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(obj, index));
                    if (hasItem)
                    {
                        JS_REENTRANT(jsReentLock, accumulator = JavascriptOperators::GetItem(obj, index, scriptContext));
                        bPresent = true;
                    }
                }
            }
            if (bPresent == false)
            {
                JavascriptError::ThrowTypeError(scriptContext, VBSERR_ActionNotSupported);
            }
        }

        // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
        CallFlags flags = CallFlags_Value;
        Var undefinedValue = scriptContext->GetLibrary()->GetUndefined();

        if (pArr)
        {
            for (; k < length; k++)
            {
                index = length - k - 1;
                JS_REENTRANT(jsReentLock, BOOL gotItem = pArr->DirectGetItemAtFull((uint32)index, &element));
                if (!gotItem)
                {
                    continue;
                }

                JS_REENTRANT(jsReentLock,
                    accumulator = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 5), undefinedValue,
                        accumulator,
                        element,
                        JavascriptNumber::ToVar(index, scriptContext),
                        pArr));

                // Side-effects in the callback function may have changed the source array into an ES5Array. If this happens
                // we will process the rest of the array elements like an ES5Array.
                if (!JavascriptArray::Is(obj))
                {
                    AssertOrFailFastMsg(ES5Array::Is(obj), "The array should have been converted to an ES5Array");
                    JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::ReduceRightObjectHelper<T>(obj, length, k + 1, callBackFn, accumulator, scriptContext));
                }
            }
        }
        else if (typedArrayBase)
        {
            AssertAndFailFast(TypedArrayBase::Is(typedArrayBase));
            uint32 end = (uint32)min(length, (T)typedArrayBase->GetLength());

            for (; k < end; k++)
            {
                // No need to do HasItem, as it cannot be observable unless 'typedArrayBase' is proxy. And we have established that it is indeed typedarray.

                index = length - k - 1;
                element = typedArrayBase->DirectGetItem((uint32)index);

                JS_REENTRANT(jsReentLock,
                    accumulator = CALL_FUNCTION(scriptContext->GetThreadContext(), callBackFn, CallInfo(flags, 5), undefinedValue,
                        accumulator,
                        element,
                        JavascriptNumber::ToVar(index, scriptContext),
                        typedArrayBase));
            }
        }
        else
        {
            JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::ReduceRightObjectHelper<T>(obj, length, k, callBackFn, accumulator, scriptContext));
        }

        return accumulator;
    }

    template <typename T>
    Var JavascriptArray::ReduceRightObjectHelper(RecyclableObject* obj, T length, T start, RecyclableObject* callBackFn, Var accumulator, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        // The correct flag value is CallFlags_Value but we pass CallFlags_None in compat modes
        CallFlags flags = CallFlags_Value;
        Var element = nullptr;
        T index = 0;

        for (T k = start; k < length; k++)
        {
            index = length - k - 1;
            JS_REENTRANT(jsReentLock, BOOL hasItem = JavascriptOperators::HasItem(obj, index));
            if (hasItem)
            {
                JS_REENTRANT(jsReentLock,
                    element = JavascriptOperators::GetItem(obj, index, scriptContext),
                    accumulator = CALL_FUNCTION(scriptContext->GetThreadContext(),
                        callBackFn, CallInfo(flags, 5), scriptContext->GetLibrary()->GetUndefined(),
                        accumulator,
                        element,
                        JavascriptNumber::ToVar(index, scriptContext),
                        obj));
            }
        }

        return accumulator;
    }

    Var JavascriptArray::EntryFrom(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        AUTO_TAG_NATIVE_LIBRARY_ENTRY(function, callInfo, _u("Array.from"));

        Assert(!(callInfo.Flags & CallFlags_New));

        JavascriptLibrary* library = scriptContext->GetLibrary();
        RecyclableObject* constructor = nullptr;

        if (JavascriptOperators::IsConstructor(args[0]))
        {
            constructor = RecyclableObject::FromVar(args[0]);
        }

        RecyclableObject* items = nullptr;

        if (args.Info.Count < 2 || !JavascriptConversion::ToObject(args[1], scriptContext, &items))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedObject, _u("Array.from"));
        }

        JavascriptArray* itemsArr = nullptr;

        if (JavascriptArray::Is(items))
        {
#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(items);
#endif
            itemsArr = JavascriptArray::FromVar(items);
        }

        bool mapping = false;
        JavascriptFunction* mapFn = nullptr;
        Var mapFnThisArg = nullptr;

        if (args.Info.Count >= 3 && !JavascriptOperators::IsUndefinedObject(args[2]))
        {
            if (!JavascriptFunction::Is(args[2]))
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_FunctionArgument_NeedFunction, _u("Array.from"));
            }

            mapFn = JavascriptFunction::FromVar(args[2]);

            if (args.Info.Count >= 4)
            {
                mapFnThisArg = args[3];
            }
            else
            {
                mapFnThisArg = library->GetUndefined();
            }

            mapping = true;
        }

        RecyclableObject* newObj = nullptr;
        JavascriptArray* newArr = nullptr;

        JS_REENTRANT(jsReentLock, RecyclableObject* iterator = JavascriptOperators::GetIterator(items, scriptContext, true /* optional */));

        if (iterator != nullptr)
        {
            if (constructor)
            {
                Js::Var constructorArgs[] = { constructor };
                Js::CallInfo constructorCallInfo(Js::CallFlags_New, _countof(constructorArgs));
                JS_REENTRANT(jsReentLock, newObj = RecyclableObject::FromVar(JavascriptOperators::NewScObject(constructor, Js::Arguments(constructorCallInfo, constructorArgs), scriptContext)));

                if (JavascriptArray::Is(newObj))
                {
#if ENABLE_COPYONACCESS_ARRAY
                    JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(newObj);
#endif
                    newArr = JavascriptArray::FromVar(newObj);
                }
            }
            else
            {
                newArr = scriptContext->GetLibrary()->CreateArray(0);
                newArr->EnsureHead<Var>();
                newObj = newArr;
            }

            uint32 k = 0;

            JS_REENTRANT(jsReentLock, JavascriptOperators::DoIteratorStepAndValue(iterator, scriptContext, [&](Var nextValue) {
                if (mapping)
                {
                    Assert(mapFn != nullptr);
                    Assert(mapFnThisArg != nullptr);

                    Js::Var mapFnArgs[] = { mapFnThisArg, nextValue, JavascriptNumber::ToVar(k, scriptContext) };
                    Js::CallInfo mapFnCallInfo(Js::CallFlags_Value, _countof(mapFnArgs));
                    nextValue = mapFn->CallFunction(Js::Arguments(mapFnCallInfo, mapFnArgs));
                }

                if (newArr)
                {
                    newArr->SetItem(k, nextValue, PropertyOperation_None);
                }
                else
                {
                    ThrowErrorOnFailure(JavascriptArray::SetArrayLikeObjects(newObj, k, nextValue), scriptContext, k);
                }

                k++;
            }));

            JS_REENTRANT(jsReentLock, JavascriptOperators::SetProperty(newObj, newObj, Js::PropertyIds::length, JavascriptNumber::ToVar(k, scriptContext), scriptContext, PropertyOperation_ThrowIfNotExtensible));
        }
        else
        {
            JS_REENTRANT(jsReentLock, int64 len = (int64)OP_GetLength(items, scriptContext));

            if (constructor)
            {
                Js::Var constructorArgs[] = { constructor, JavascriptNumber::ToVar(len, scriptContext) };
                Js::CallInfo constructorCallInfo(Js::CallFlags_New, _countof(constructorArgs));
                JS_REENTRANT(jsReentLock, newObj = RecyclableObject::FromVar(JavascriptOperators::NewScObject(constructor, Js::Arguments(constructorCallInfo, constructorArgs), scriptContext)));

                if (JavascriptArray::Is(newObj))
                {
#if ENABLE_COPYONACCESS_ARRAY
                    JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(newObj);
#endif
                    newArr = JavascriptArray::FromVar(newObj);
                }
            }
            else
            {
                // Abstract operation ArrayCreate throws RangeError if length argument is > 2^32 -1
                if (len > MaxArrayLength)
                {
                    JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthAssignIncorrect, _u("Array.from"));
                }

                // Static cast len should be valid (len < 2^32) or we would throw above
                newArr = scriptContext->GetLibrary()->CreateArray(static_cast<uint32>(len));
                newArr->EnsureHead<Var>();
                newObj = newArr;
            }

            uint32 k = 0;

            for ( ; k < len; k++)
            {
                Var kValue;

                if (itemsArr)
                {
                    JS_REENTRANT(jsReentLock, kValue = itemsArr->DirectGetItem(k));
                }
                else
                {
                    JS_REENTRANT(jsReentLock, kValue = JavascriptOperators::OP_GetElementI_UInt32(items, k, scriptContext));
                }

                if (mapping)
                {
                    Assert(mapFn != nullptr);
                    Assert(mapFnThisArg != nullptr);

                    Js::Var mapFnArgs[] = { mapFnThisArg, kValue, JavascriptNumber::ToVar(k, scriptContext) };
                    Js::CallInfo mapFnCallInfo(Js::CallFlags_Value, _countof(mapFnArgs));
                    JS_REENTRANT(jsReentLock, kValue = mapFn->CallFunction(Js::Arguments(mapFnCallInfo, mapFnArgs)));
                }

                if (newArr)
                {
                    newArr->SetItem(k, kValue, PropertyOperation_None);
                }
                else
                {
                    JS_REENTRANT(jsReentLock, ThrowErrorOnFailure(JavascriptArray::SetArrayLikeObjects(newObj, k, kValue), scriptContext, k));
                }
            }

            JS_REENTRANT(jsReentLock, JavascriptOperators::SetProperty(newObj, newObj, Js::PropertyIds::length, JavascriptNumber::ToVar(len, scriptContext), scriptContext, PropertyOperation_ThrowIfNotExtensible));
        }

        return newObj;
    }

    Var JavascriptArray::EntryOf(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(!(callInfo.Flags & CallFlags_New));

        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NullOrUndefined, _u("Array.of"));
        }

        JS_REENTRANT_UNLOCK(jsReentLock, return JavascriptArray::OfHelper(false, args, scriptContext));
    }

    Var JavascriptArray::EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);

        Assert(args.Info.Count > 0);

        return args[0];
    }

    // Array.of and %TypedArray%.of as described in ES6.0 (draft 22) Section 22.1.2.2 and 22.2.2.2
    Var JavascriptArray::OfHelper(bool isTypedArrayEntryPoint, Arguments& args, ScriptContext* scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        Assert(args.Info.Count > 0);

        // args.Info.Count cannot equal zero or we would have thrown above so no chance of underflowing
        uint32 len = args.Info.Count - 1;
        Var newObj = nullptr;
        JavascriptArray* newArr = nullptr;
        TypedArrayBase* newTypedArray = nullptr;
        bool isBuiltinArrayCtor = true;

        if (JavascriptOperators::IsConstructor(args[0]))
        {
            RecyclableObject* constructor = RecyclableObject::FromVar(args[0]);
            isBuiltinArrayCtor = (constructor == scriptContext->GetLibrary()->GetArrayConstructor());

            Js::Var constructorArgs[] = { constructor, JavascriptNumber::ToVar(len, scriptContext) };
            Js::CallInfo constructorCallInfo(Js::CallFlags_New, _countof(constructorArgs));
            if (isTypedArrayEntryPoint)
            {
                JS_REENTRANT(jsReentLock, newObj = TypedArrayBase::TypedArrayCreate(constructor, &Js::Arguments(constructorCallInfo, constructorArgs), len, scriptContext));
            }
            else
            {
                JS_REENTRANT(jsReentLock, newObj = JavascriptOperators::NewScObject(constructor, Js::Arguments(constructorCallInfo, constructorArgs), scriptContext));
            }

            // If the new object we created is an array, remember that as it will save us time setting properties in the object below
            if (JavascriptArray::Is(newObj))
            {
#if ENABLE_COPYONACCESS_ARRAY
                JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(newObj);
#endif
                newArr = JavascriptArray::FromVar(newObj);
            }
            else if (TypedArrayBase::Is(newObj))
            {
                newTypedArray = TypedArrayBase::FromVar(newObj);
            }
        }
        else
        {
            // We only throw when the constructor property is not a constructor function in the TypedArray version
            if (isTypedArrayEntryPoint)
            {
                JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedFunction, _u("[TypedArray].of"));
            }

            newArr = scriptContext->GetLibrary()->CreateArray(len);
            newArr->EnsureHead<Var>();
            newObj = newArr;
        }

        // At least we have a new object of some kind
        Assert(newObj);

        if (isBuiltinArrayCtor)
        {
            for (uint32 k = 0; k < len; k++)
            {
                Var kValue = args[k + 1];

                newArr->DirectSetItemAt(k, kValue);
            }
        }
        else if (newTypedArray)
        {
            for (uint32 k = 0; k < len; k++)
            {
                Var kValue = args[k + 1];

                JS_REENTRANT(jsReentLock, newTypedArray->DirectSetItem(k, kValue));
            }
        }
        else
        {
            for (uint32 k = 0; k < len; k++)
            {
                Var kValue = args[k + 1];
                JS_REENTRANT(jsReentLock, ThrowErrorOnFailure(JavascriptArray::SetArrayLikeObjects(RecyclableObject::FromVar(newObj), k, kValue), scriptContext, k));
            }
        }

        if (!isTypedArrayEntryPoint)
        {
            // Set length if we are in the Array version of the function
            JS_REENTRANT(jsReentLock, JavascriptOperators::OP_SetProperty(newObj, Js::PropertyIds::length, JavascriptNumber::ToVar(len, scriptContext), scriptContext, nullptr, PropertyOperation_ThrowIfNotExtensible));
        }

        return newObj;
    }

    JavascriptString* JavascriptArray::ToLocaleStringHelper(Var value, ScriptContext* scriptContext)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(value);
        if (typeId == TypeIds_Null || typeId == TypeIds_Undefined)
        {
            return scriptContext->GetLibrary()->GetEmptyString();
        }
        else
        {
            return JavascriptConversion::ToLocaleString(value, scriptContext);
        }
    }

    inline BOOL JavascriptArray::IsFullArray() const
    {
        if (head && head->length == length)
        {
            AssertMsg(head->next == 0 && head->left == 0, "Invalid Array");
            return true;
        }
        return (0 == length);
    }

    /*
    *   IsFillFromPrototypes
    *   -   Check the array has no missing values and only head segment.
    *   -   Also ensure if the lengths match.
    */
    bool JavascriptArray::IsFillFromPrototypes()
    {
        return !(this->head->next == nullptr && this->HasNoMissingValues() && this->length == this->head->length);
    }

    // Fill all missing value in the array and fill it from prototype between startIndex and limitIndex
    // typically startIndex = 0 and limitIndex = length. From start of the array till end of the array.
    void JavascriptArray::FillFromPrototypes(uint32 startIndex, uint32 limitIndex)
    {
        if (startIndex >= limitIndex)
        {
            return;
        }

        RecyclableObject* prototype = this->GetPrototype();

        // Fill all missing values by walking through prototype
        while (JavascriptOperators::GetTypeId(prototype) != TypeIds_Null)
        {
            ForEachOwnMissingArrayIndexOfObject(this, nullptr, prototype, startIndex, limitIndex,0, [this](uint32 index, Var value) {
                this->SetItem(index, value, PropertyOperation_None);
            });

            prototype = prototype->GetPrototype();
        }
#ifdef VALIDATE_ARRAY
        ValidateArray();
#endif
    }

    //
    // JavascriptArray requires head->left == 0 for fast path Get.
    //
    template<typename T>
    void JavascriptArray::EnsureHeadStartsFromZero(Recycler * recycler)
    {
        if (head == nullptr || head->left != 0)
        {
            // This is used to fix up altered arrays.
            // any SegmentMap would be invalid at this point.
            ClearSegmentMap();

            //
            // We could OOM and throw when allocating new empty head, resulting in a corrupted array. Need
            // some protection here. Save the head and switch this array to EmptySegment. Will be restored
            // correctly if allocating new segment succeeds.
            //
            SparseArraySegment<T>* savedHead = SparseArraySegment<T>::From(this->head);
            SparseArraySegment<T>* savedLastUsedSegment = (SparseArraySegment<T>*)this->GetLastUsedSegment();
            SetHeadAndLastUsedSegment(const_cast<SparseArraySegmentBase*>(EmptySegment));

            SparseArraySegment<T> *newSeg = SparseArraySegment<T>::AllocateSegment(recycler, 0, 0, savedHead);
            newSeg->next = savedHead;
            this->head = newSeg;
            SetHasNoMissingValues();
            this->SetLastUsedSegment(savedLastUsedSegment);
        }
    }

#ifdef ENABLE_DEBUG_CONFIG_OPTIONS
    void JavascriptArray::CheckForceES5Array()
    {
        if (Configuration::Global.flags.ForceES5Array)
        {
            // There's a bad interaction with the jitted code for native array creation here.
            // ForceES5Array doesn't interact well with native arrays
            if (PHASE_OFF1(NativeArrayPhase))
            {
                GetTypeHandler()->ConvertToTypeWithItemAttributes(this);
            }
        }
    }
#endif

    template <typename Fn>
    void JavascriptArray::ForEachOwnMissingArrayIndexOfObject(JavascriptArray *baseArray, JavascriptArray *destArray, RecyclableObject* obj, uint32 startIndex, uint32 limitIndex, uint32 destIndex, Fn fn)
    {
        JS_REENTRANCY_LOCK(jsReentLock, baseArray->GetScriptContext()->GetThreadContext());
        Assert(DynamicObject::IsAnyArray(obj) || JavascriptOperators::IsObject(obj));

        Var oldValue;
        JavascriptArray* arr = nullptr;
        if (DynamicObject::IsAnyArray(obj))
        {
            arr = JavascriptArray::FromAnyArray(obj);
        }
        else if (DynamicType::Is(obj->GetTypeId()))
        {
            DynamicObject* dynobj = DynamicObject::FromVar(obj);
            ArrayObject* objectArray = dynobj->GetObjectArray();
            arr = (objectArray && JavascriptArray::IsAnyArray(objectArray)) ? JavascriptArray::FromAnyArray(objectArray) : nullptr;
        }

        if (arr != nullptr)
        {
            if (JavascriptArray::Is(arr))
            {
                arr = EnsureNonNativeArray(arr);
                ArrayElementEnumerator e(arr, startIndex, limitIndex);

                while(e.MoveNext<Var>())
                {
                    uint32 index = e.GetIndex();
                    if (!baseArray->DirectGetVarItemAt(index, &oldValue, baseArray->GetScriptContext()))
                    {
                        uint32 n = destIndex + (index - startIndex);
                        if (destArray == nullptr || !destArray->DirectGetItemAt(n, &oldValue))
                        {
                            JS_REENTRANT(jsReentLock, fn(index, e.GetItem<Var>()));
                        }
                    }
                }
            }
            else
            {
                ScriptContext* scriptContext = obj->GetScriptContext();

                Assert(ES5Array::Is(arr));

                ES5Array* es5Array = ES5Array::FromVar(arr);
                ES5ArrayIndexStaticEnumerator<true> e(es5Array);

                while (e.MoveNext())
                {
                    uint32 index = e.GetIndex();
                    if (index < startIndex) continue;
                    else if (index >= limitIndex) break;

                    if (!baseArray->DirectGetVarItemAt(index, &oldValue, baseArray->GetScriptContext()))
                    {
                        uint32 n = destIndex + (index - startIndex);
                        if (destArray == nullptr || !destArray->DirectGetItemAt(n, &oldValue))
                        {
                            Var value = nullptr;
                            JS_REENTRANT(jsReentLock, BOOL gotItem = JavascriptOperators::GetOwnItem(obj, index, &value, scriptContext));
                            if (gotItem)
                            {
                                JS_REENTRANT(jsReentLock, fn(index, value));
                            }
                        }
                    }
                }
            }
        }
    }

    //
    // ArrayElementEnumerator to enumerate array elements (not including elements from prototypes).
    //
    JavascriptArray::ArrayElementEnumerator::ArrayElementEnumerator(JavascriptArray* arr, uint32 start, uint32 end)
        : start(start), end(min(end, arr->length))
    {
        Init(arr);
    }

    //
    // Initialize this enumerator and prepare for the first MoveNext.
    //
    void JavascriptArray::ArrayElementEnumerator::Init(JavascriptArray* arr)
    {
        // Find start segment
        seg = (arr ? arr->GetBeginLookupSegment(start) : nullptr);
        while (seg && (seg->left + seg->length <= start))
        {
            seg = seg->next;
        }

        // Set start index and endIndex
        if (seg)
        {
            if (seg->left >= end)
            {
                seg = nullptr;
            }
            else
            {
                // set index to be at target index - 1, so MoveNext will move to target
                index = max(seg->left, start) - seg->left - 1;
                endIndex = min(end - seg->left, seg->length);
            }
        }
    }

    //
    // Move to the next element if available.
    //
    template<typename T>
    inline bool JavascriptArray::ArrayElementEnumerator::MoveNext()
    {
        while (seg)
        {
            // Look for next non-null item in current segment
            while (++index < endIndex)
            {
                if (!SparseArraySegment<T>::IsMissingItem(&((SparseArraySegment<T>*)seg)->elements[index]))
                {
                    return true;
                }
            }

            // Move to next segment
            seg = seg->next;
            if (seg)
            {
                if (seg->left >= end)
                {
                    seg = nullptr;
                    break;
                }
                else
                {
                    index = static_cast<uint32>(-1);
                    endIndex = min(end - seg->left, seg->length);
                }
            }
        }

        return false;
    }

    //
    // Get current array element index.
    //
    uint32 JavascriptArray::ArrayElementEnumerator::GetIndex() const
    {
        Assert(seg && index < seg->length && index < endIndex);
        return seg->left + index;
    }

    //
    // Get current array element value.
    //
    template<typename T>
    T JavascriptArray::ArrayElementEnumerator::GetItem() const
    {
        Assert(seg && index < seg->length && index < endIndex &&
               !SparseArraySegment<T>::IsMissingItem(&((SparseArraySegment<T>*)seg)->elements[index]));
        return ((SparseArraySegment<T>*)seg)->elements[index];
    }

    //
    // Construct a BigIndex initialized to a given uint32 (small index).
    //
    JavascriptArray::BigIndex::BigIndex(uint32 initIndex)
        : index(initIndex), bigIndex(InvalidIndex)
    {
        //ok if initIndex == InvalidIndex
    }

    //
    // Construct a BigIndex initialized to a given uint64 (large or small index).
    //
    JavascriptArray::BigIndex::BigIndex(uint64 initIndex)
        : index(InvalidIndex), bigIndex(initIndex)
    {
        if (bigIndex < InvalidIndex) // if it's actually small index
        {
            index = static_cast<uint32>(bigIndex);
            bigIndex = InvalidIndex;
        }
    }

    bool JavascriptArray::BigIndex::IsUint32Max() const
    {
        return index == InvalidIndex && bigIndex == InvalidIndex;
    }
    bool JavascriptArray::BigIndex::IsSmallIndex() const
    {
        return index < InvalidIndex;
    }

    uint32 JavascriptArray::BigIndex::GetSmallIndex() const
    {
        Assert(IsSmallIndex());
        return index;
    }

    uint64 JavascriptArray::BigIndex::GetBigIndex() const
    {
        Assert(!IsSmallIndex());
        return bigIndex;
    }
    //
    // Convert this index value to a JS number
    //
    Var JavascriptArray::BigIndex::ToNumber(ScriptContext* scriptContext) const
    {
        if (IsSmallIndex())
        {
            return small_index::ToNumber(index, scriptContext);
        }
        else
        {
            return JavascriptNumber::ToVar(bigIndex, scriptContext);
        }
    }

    //
    // Increment this index by 1.
    //
    const JavascriptArray::BigIndex& JavascriptArray::BigIndex::operator++()
    {
        if (IsSmallIndex())
        {
            ++index;
            // If index reaches InvalidIndex, we will start to use bigIndex which is initially InvalidIndex.
        }
        else
        {
            bigIndex = bigIndex + 1;
        }

        return *this;
    }

    //
    // Decrement this index by 1.
    //
    const JavascriptArray::BigIndex& JavascriptArray::BigIndex::operator--()
    {
        if (IsSmallIndex())
        {
            --index;
        }
        else
        {
            Assert(index == InvalidIndex && bigIndex >= InvalidIndex);

            --bigIndex;
            if (bigIndex < InvalidIndex)
            {
                index = InvalidIndex - 1;
                bigIndex = InvalidIndex;
            }
        }

        return *this;
    }

    JavascriptArray::BigIndex JavascriptArray::BigIndex::operator+(const BigIndex& delta) const
    {
        if (delta.IsSmallIndex())
        {
            return operator+(delta.GetSmallIndex());
        }
        if (IsSmallIndex())
        {
            return index + delta.GetBigIndex();
        }

        return bigIndex + delta.GetBigIndex();
    }

    //
    // Get a new BigIndex representing this + delta.
    //
    JavascriptArray::BigIndex JavascriptArray::BigIndex::operator+(uint32 delta) const
    {
        if (IsSmallIndex())
        {
            uint32 newIndex;
            if (UInt32Math::Add(index, delta, &newIndex))
            {
                return static_cast<uint64>(index) + static_cast<uint64>(delta);
            }
            else
            {
                return newIndex; // ok if newIndex == InvalidIndex
            }
        }
        else
        {
            return bigIndex + static_cast<uint64>(delta);
        }
    }

    bool JavascriptArray::BigIndex::operator==(const BigIndex& rhs) const
    {
        if (rhs.IsSmallIndex() && this->IsSmallIndex())
        {
            return this->GetSmallIndex() == rhs.GetSmallIndex();
        }
        else if (rhs.IsSmallIndex() && !this->IsSmallIndex())
        {
            // if lhs is big promote rhs
            return this->GetBigIndex() == (uint64) rhs.GetSmallIndex();
        }
        else if (!rhs.IsSmallIndex() && this->IsSmallIndex())
        {
            // if rhs is big promote lhs
            return ((uint64)this->GetSmallIndex()) == rhs.GetBigIndex();
        }
        return this->GetBigIndex() == rhs.GetBigIndex();
    }

    bool JavascriptArray::BigIndex::operator> (const BigIndex& rhs) const
    {
        if (rhs.IsSmallIndex() && this->IsSmallIndex())
        {
            return this->GetSmallIndex() > rhs.GetSmallIndex();
        }
        else if (rhs.IsSmallIndex() && !this->IsSmallIndex())
        {
            // if lhs is big promote rhs
            return this->GetBigIndex() > (uint64)rhs.GetSmallIndex();
        }
        else if (!rhs.IsSmallIndex() && this->IsSmallIndex())
        {
            // if rhs is big promote lhs
            return ((uint64)this->GetSmallIndex()) > rhs.GetBigIndex();
        }
        return this->GetBigIndex() > rhs.GetBigIndex();
    }

    bool JavascriptArray::BigIndex::operator< (const BigIndex& rhs) const
    {
        if (rhs.IsSmallIndex() && this->IsSmallIndex())
        {
            return this->GetSmallIndex() < rhs.GetSmallIndex();
        }
        else if (rhs.IsSmallIndex() && !this->IsSmallIndex())
        {
            // if lhs is big promote rhs
            return this->GetBigIndex() < (uint64)rhs.GetSmallIndex();
        }
        else if (!rhs.IsSmallIndex() && this->IsSmallIndex())
        {
            // if rhs is big promote lhs
            return ((uint64)this->GetSmallIndex()) < rhs.GetBigIndex();
        }
        return this->GetBigIndex() < rhs.GetBigIndex();
    }

    bool JavascriptArray::BigIndex::operator<=(const BigIndex& rhs) const
    {
        if (rhs.IsSmallIndex() && this->IsSmallIndex())
        {
            return this->GetSmallIndex() <= rhs.GetSmallIndex();
        }
        else if (rhs.IsSmallIndex() && !this->IsSmallIndex())
        {
            // if lhs is big promote rhs
            return this->GetBigIndex() <= (uint64)rhs.GetSmallIndex();
        }
        else if (!rhs.IsSmallIndex() && !this->IsSmallIndex())
        {
            // if rhs is big promote lhs
            return ((uint64)this->GetSmallIndex()) <= rhs.GetBigIndex();
        }
        return this->GetBigIndex() <= rhs.GetBigIndex();
    }

    bool JavascriptArray::BigIndex::operator>=(const BigIndex& rhs) const
    {
        if (rhs.IsSmallIndex() && this->IsSmallIndex())
        {
            return this->GetSmallIndex() >= rhs.GetSmallIndex();
        }
        else if (rhs.IsSmallIndex() && !this->IsSmallIndex())
        {
            // if lhs is big promote rhs
            return this->GetBigIndex() >= (uint64)rhs.GetSmallIndex();
        }
        else if (!rhs.IsSmallIndex() && this->IsSmallIndex())
        {
            // if rhs is big promote lhs
            return ((uint64)this->GetSmallIndex()) >= rhs.GetBigIndex();
        }
        return this->GetBigIndex() >= rhs.GetBigIndex();
    }

    BOOL JavascriptArray::BigIndex::GetItem(JavascriptArray* arr, Var* outVal) const
    {
        if (IsSmallIndex())
        {
            return small_index::GetItem(arr, index, outVal);
        }
        else
        {
            ScriptContext* scriptContext = arr->GetScriptContext();
            PropertyRecord const * propertyRecord;
            JavascriptOperators::GetPropertyIdForInt(bigIndex, scriptContext, &propertyRecord);
            return arr->GetProperty(arr, propertyRecord->GetPropertyId(), outVal, NULL, scriptContext);
        }
    }

    BOOL JavascriptArray::BigIndex::SetItem(JavascriptArray* arr, Var newValue) const
    {
        if (IsSmallIndex())
        {
            return small_index::SetItem(arr, index, newValue);
        }
        else
        {
            ScriptContext* scriptContext = arr->GetScriptContext();
            PropertyRecord const * propertyRecord;
            JavascriptOperators::GetPropertyIdForInt(bigIndex, scriptContext, &propertyRecord);
            return arr->SetProperty(propertyRecord->GetPropertyId(), newValue, PropertyOperation_None, NULL);
        }
    }

    void JavascriptArray::BigIndex::SetItemIfNotExist(JavascriptArray* arr, Var newValue) const
    {
        if (IsSmallIndex())
        {
            small_index::SetItemIfNotExist(arr, index, newValue);
        }
        else
        {
            ScriptContext* scriptContext = arr->GetScriptContext();
            PropertyRecord const * propertyRecord;
            JavascriptOperators::GetPropertyIdForInt(bigIndex, scriptContext, &propertyRecord);
            Var oldValue;
            PropertyId propertyId = propertyRecord->GetPropertyId();
            if (!arr->GetProperty(arr, propertyId, &oldValue, NULL, scriptContext))
            {
                arr->SetProperty(propertyId, newValue, PropertyOperation_None, NULL);
            }
        }
    }

    BOOL JavascriptArray::BigIndex::DeleteItem(JavascriptArray* arr) const
    {
        if (IsSmallIndex())
        {
            return small_index::DeleteItem(arr, index);
        }
        else
        {
            ScriptContext* scriptContext = arr->GetScriptContext();
            PropertyRecord const * propertyRecord;
            JavascriptOperators::GetPropertyIdForInt(bigIndex, scriptContext, &propertyRecord);
            return arr->DeleteProperty(propertyRecord->GetPropertyId(), PropertyOperation_None);
        }
    }

    BOOL JavascriptArray::BigIndex::SetItem(RecyclableObject* obj, Var newValue, PropertyOperationFlags flags) const
    {
        if (IsSmallIndex())
        {
            return small_index::SetItem(obj, index, newValue, flags);
        }
        else
        {
            ScriptContext* scriptContext = obj->GetScriptContext();
            PropertyRecord const * propertyRecord;
            JavascriptOperators::GetPropertyIdForInt(bigIndex, scriptContext, &propertyRecord);
            return JavascriptOperators::SetProperty(obj, obj, propertyRecord->GetPropertyId(), newValue, scriptContext, flags);
        }
    }

    BOOL JavascriptArray::BigIndex::DeleteItem(RecyclableObject* obj, PropertyOperationFlags flags) const
    {
        if (IsSmallIndex())
        {
            return small_index::DeleteItem(obj, index, flags);
        }
        else
        {
            PropertyRecord const * propertyRecord;
            JavascriptOperators::GetPropertyIdForInt(bigIndex, obj->GetScriptContext(), &propertyRecord);
            return JavascriptOperators::DeleteProperty(obj, propertyRecord->GetPropertyId(), flags);
        }
    }

    //
    // Truncate the array at start and clone the truncated span as properties starting at dstIndex (asserting dstIndex >= MaxArrayLength).
    //
    void JavascriptArray::TruncateToProperties(const BigIndex& dstIndex, uint32 start)
    {
        Assert(!dstIndex.IsSmallIndex());
        typedef IndexTrace<BigIndex> index_trace;

        BigIndex dst = dstIndex;
        uint32 i = start;

        ArrayElementEnumerator e(this, start);
        while(e.MoveNext<Var>())
        {
            // delete all items not enumerated
            while (i < e.GetIndex())
            {
                index_trace::DeleteItem(this, dst);
                ++i;
                ++dst;
            }

            // Copy over the item
            index_trace::SetItem(this, dst, e.GetItem<Var>());
            ++i;
            ++dst;
        }

        // Delete the rest till length
        while (i < this->length)
        {
            index_trace::DeleteItem(this, dst);
            ++i;
            ++dst;
        }

        // Elements moved, truncate the array at start
        SetLength(start);
    }

    //
    // Copy a srcArray elements (including elements from prototypes) to a dstArray starting from an index.
    //
    void JavascriptArray::InternalCopyArrayElements(JavascriptArray* dstArray, const uint32 dstIndex, JavascriptArray* srcArray, uint32 start, uint32 end)
    {
        Assert(start < end && end <= srcArray->length);

        uint32 count = 0;

        // iterate on the array itself
        ArrayElementEnumerator e(srcArray, start, end);
        while(e.MoveNext<Var>())
        {
            uint32 n = dstIndex + (e.GetIndex() - start);
            dstArray->DirectSetItemAt(n, e.GetItem<Var>());
            count++;
        }

        // iterate on the array's prototypes only if not all elements found
        if (start + count != end)
        {
            InternalFillFromPrototype(dstArray, dstIndex, srcArray, start, end, count);
        }
    }

    //
    // Faster small_index overload of CopyArrayElements, asserting the uint32 dstIndex won't overflow.
    //
    void JavascriptArray::CopyArrayElements(JavascriptArray* dstArray, uint32 dstIndex, JavascriptArray* srcArray, uint32 start, uint32 end)
    {
        end = min(end, srcArray->length);
        if (start < end)
        {
            Assert(end - start <= MaxArrayLength - dstIndex);
            InternalCopyArrayElements(dstArray, dstIndex, srcArray, start, end);
        }
    }

    template <typename T>
    void JavascriptArray::CopyAnyArrayElementsToVar(JavascriptArray* dstArray, T dstIndex, JavascriptArray* srcArray, uint32 start, uint32 end)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(srcArray);
#endif
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(dstArray);
#endif
        if (JavascriptNativeIntArray::Is(srcArray))
        {
            CopyNativeIntArrayElementsToVar(dstArray, dstIndex, JavascriptNativeIntArray::FromVar(srcArray), start, end);
        }
        else if (JavascriptNativeFloatArray::Is(srcArray))
        {
            CopyNativeFloatArrayElementsToVar(dstArray, dstIndex, JavascriptNativeFloatArray::FromVar(srcArray), start, end);
        }
        else
        {
            CopyArrayElements(dstArray, dstIndex, srcArray, start, end);
        }
    }

    //
    // Faster small_index overload of CopyArrayElements, asserting the uint32 dstIndex won't overflow.
    //
    void JavascriptArray::CopyNativeIntArrayElementsToVar(JavascriptArray* dstArray, uint32 dstIndex, JavascriptNativeIntArray* srcArray, uint32 start, uint32 end)
    {
        end = min(end, srcArray->length);
        if (start < end)
        {
            Assert(end - start <= MaxArrayLength - dstIndex);
            InternalCopyNativeIntArrayElements(dstArray, dstIndex, srcArray, start, end);
        }
    }

    bool JavascriptArray::CopyNativeIntArrayElements(JavascriptNativeIntArray* dstArray, uint32 dstIndex, JavascriptNativeIntArray* srcArray, uint32 start, uint32 end)
    {
        end = min(end, srcArray->length);
        if (start >= end)
        {
            return false;
        }

        Assert(end - start <= MaxArrayLength - dstIndex);
        Assert(start < end && end <= srcArray->length);

        uint32 count = 0;

        // iterate on the array itself
        ArrayElementEnumerator e(srcArray, start, end);
        while(e.MoveNext<int32>())
        {
            uint n = dstIndex + (e.GetIndex() - start);
            dstArray->DirectSetItemAt(n, e.GetItem<int32>());
            count++;
        }

        // iterate on the array's prototypes only if not all elements found
        if (start + count != end)
        {
            JavascriptArray *varArray = JavascriptNativeIntArray::ToVarArray(dstArray);
            InternalFillFromPrototype(varArray, dstIndex, srcArray, start, end, count);
            return true;
        }

        return false;
    }

    bool JavascriptArray::CopyNativeIntArrayElementsToFloat(JavascriptNativeFloatArray* dstArray, uint32 dstIndex, JavascriptNativeIntArray* srcArray, uint32 start, uint32 end)
    {
        end = min(end, srcArray->length);
        if (start >= end)
        {
            return false;
        }

        Assert(end - start <= MaxArrayLength - dstIndex);
        Assert(start < end && end <= srcArray->length);

        uint32 count = 0;

        // iterate on the array itself
        ArrayElementEnumerator e(srcArray, start, end);
        while(e.MoveNext<int32>())
        {
            uint n = dstIndex + (e.GetIndex() - start);
            dstArray->DirectSetItemAt(n, (double)e.GetItem<int32>());
            count++;
        }

        // iterate on the array's prototypes only if not all elements found
        if (start + count != end)
        {
            JavascriptArray *varArray = JavascriptNativeFloatArray::ToVarArray(dstArray);
            InternalFillFromPrototype(varArray, dstIndex, srcArray, start, end, count);
            return true;
        }

        return false;
    }

    //
    // Faster small_index overload of CopyArrayElements, asserting the uint32 dstIndex won't overflow.
    //
    void JavascriptArray::CopyNativeFloatArrayElementsToVar(JavascriptArray* dstArray, uint32 dstIndex, JavascriptNativeFloatArray* srcArray, uint32 start, uint32 end)
    {
        end = min(end, srcArray->length);
        if (start < end)
        {
            Assert(end - start <= MaxArrayLength - dstIndex);
            InternalCopyNativeFloatArrayElements(dstArray, dstIndex, srcArray, start, end);
        }
    }

    bool JavascriptArray::CopyNativeFloatArrayElements(JavascriptNativeFloatArray* dstArray, uint32 dstIndex, JavascriptNativeFloatArray* srcArray, uint32 start, uint32 end)
    {
        end = min(end, srcArray->length);
        if (start >= end)
        {
            return false;
        }

        Assert(end - start <= MaxArrayLength - dstIndex);
        Assert(start < end && end <= srcArray->length);

        uint32 count = 0;

        // iterate on the array itself
        ArrayElementEnumerator e(srcArray, start, end);
        while(e.MoveNext<double>())
        {
            uint n = dstIndex + (e.GetIndex() - start);
            dstArray->DirectSetItemAt(n, e.GetItem<double>());
            count++;
        }

        // iterate on the array's prototypes only if not all elements found
        if (start + count != end)
        {
            JavascriptArray *varArray = JavascriptNativeFloatArray::ToVarArray(dstArray);
            InternalFillFromPrototype(varArray, dstIndex, srcArray, start, end, count);
            return true;
        }

        return false;
    }

    JavascriptArray *JavascriptArray::EnsureNonNativeArray(JavascriptArray *arr)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(arr);
#endif
        if (JavascriptNativeIntArray::Is(arr))
        {
            arr = JavascriptNativeIntArray::ToVarArray((JavascriptNativeIntArray*)arr);
        }
        else if (JavascriptNativeFloatArray::Is(arr))
        {
            arr = JavascriptNativeFloatArray::ToVarArray((JavascriptNativeFloatArray*)arr);
        }

        return arr;
    }

    BOOL JavascriptNativeIntArray::DirectGetItemAtFull(uint32 index, Var* outVal)
    {
        ScriptContext* requestContext = type->GetScriptContext();
        if (JavascriptConversion::PropertyQueryFlagsToBoolean(JavascriptNativeIntArray::GetItemQuery(this, index, outVal, requestContext)))
        {
            return TRUE;
        }

        return JavascriptOperators::GetItem(this, this->GetPrototype(), index, outVal, requestContext);
    }

    BOOL JavascriptNativeFloatArray::DirectGetItemAtFull(uint32 index, Var* outVal)
    {
        ScriptContext* requestContext = type->GetScriptContext();
        if (JavascriptConversion::PropertyQueryFlagsToBoolean(JavascriptNativeFloatArray::GetItemQuery(this, index, outVal, requestContext)))
        {
            return TRUE;
        }

        return JavascriptOperators::GetItem(this, this->GetPrototype(), index, outVal, requestContext);
    }

    void JavascriptArray::InternalCopyNativeIntArrayElements(JavascriptArray* dstArray, uint32 dstIndex, JavascriptNativeIntArray* srcArray, uint32 start, uint32 end)
    {
        Assert(start < end && end <= srcArray->length);

        uint32 count = 0;

        // iterate on the array itself
        ScriptContext *scriptContext = dstArray->GetScriptContext();

        ArrayElementEnumerator e(srcArray, start, end);
        while(e.MoveNext<int32>())
        {
            uint32 n = dstIndex + (e.GetIndex() - start);
            dstArray->DirectSetItemAt(n, JavascriptNumber::ToVar(e.GetItem<int32>(), scriptContext));
            count++;
        }

        // iterate on the array's prototypes only if not all elements found
        if (start + count != end)
        {
            InternalFillFromPrototype(dstArray, dstIndex, srcArray, start, end, count);
        }
    }

    void JavascriptArray::InternalCopyNativeFloatArrayElements(JavascriptArray* dstArray, uint32 dstIndex, JavascriptNativeFloatArray* srcArray, uint32 start, uint32 end)
    {
        Assert(start < end && end <= srcArray->length);

        uint32 count = 0;

        // iterate on the array itself
        ScriptContext *scriptContext = dstArray->GetScriptContext();
        ArrayElementEnumerator e(srcArray, start, end);
        while(e.MoveNext<double>())
        {
            uint32 n = dstIndex + (e.GetIndex() - start);
            dstArray->DirectSetItemAt(n, JavascriptNumber::ToVarWithCheck(e.GetItem<double>(), scriptContext));
            count++;
        }

        // iterate on the array's prototypes only if not all elements found
        if (start + count != end)
        {
            InternalFillFromPrototype(dstArray, dstIndex, srcArray, start, end, count);
        }
    }

    Var JavascriptArray::SpreadArrayArgs(Var arrayToSpread, const Js::AuxArray<uint32> *spreadIndices, ScriptContext *scriptContext)
    {
        JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

        // At this stage we have an array literal with some arguments to be spread.
        // First we need to calculate the real size of the final literal.
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(arrayToSpread);
#endif
        JavascriptArray *array = FromVar(arrayToSpread);
        uint32 arrayLength = array->GetLength();
        uint32 actualLength = arrayLength;

        for (unsigned i = 0; i < spreadIndices->count; ++i)
        {
            JS_REENTRANT(jsReentLock,
                actualLength = UInt32Math::Add(actualLength - 1, GetSpreadArgLen(array->DirectGetItem(spreadIndices->elements[i]), scriptContext)));
        }

        JavascriptArray *result = FromVar(OP_NewScArrayWithMissingValues(actualLength, scriptContext));

        // Now we copy each element and expand the spread parameters inline.
        for (unsigned i = 0, spreadArrIndex = 0, resultIndex = 0; i < arrayLength && resultIndex < actualLength; ++i)
        {
            uint32 spreadIndex = spreadIndices->elements[spreadArrIndex]; // The index of the next element to be spread.

            // An array needs a slow copy if it is a cross-site object or we have missing values that need to be set to undefined.
            auto needArraySlowCopy = [&](Var instance) {
                if (JavascriptArray::Is(instance))
                {
                    JavascriptArray *arr = JavascriptArray::FromVar(instance);
                    JS_REENTRANT_UNLOCK(jsReentLock, return arr->IsCrossSiteObject() || arr->IsFillFromPrototypes());
                }
                return false;
            };

            // Designed to have interchangeable arguments with CopyAnyArrayElementsToVar.
            auto slowCopy = [&scriptContext, &needArraySlowCopy
                ](JavascriptArray *dstArray, unsigned dstIndex, Var srcArray, uint32 start, uint32 end) {
                Assert(needArraySlowCopy(srcArray) || ArgumentsObject::Is(srcArray) || TypedArrayBase::Is(srcArray) || JavascriptString::Is(srcArray));
                JS_REENTRANCY_LOCK(jsReentLock, scriptContext->GetThreadContext());

                RecyclableObject *propertyObject;
                if (!JavascriptOperators::GetPropertyObject(srcArray, scriptContext, &propertyObject))
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_InvalidSpreadArgument);
                }

                for (uint32 j = start; j < end; j++)
                {
                    Var element = nullptr;
                    JS_REENTRANT(jsReentLock, BOOL gotItem = JavascriptOperators::GetItem(srcArray, propertyObject, j, &element, scriptContext));
                    if (!gotItem)
                    {
                        // Copy across missing values as undefined as per 12.2.5.2 SpreadElement : ... AssignmentExpression 5f.
                        element = scriptContext->GetLibrary()->GetUndefined();
                    }
                    dstArray->DirectSetItemAt(dstIndex++, element);
                }
            };

            if (i < spreadIndex)
            {
                // Any non-spread elements can be copied in bulk.

                if (needArraySlowCopy(array))
                {
                    slowCopy(result, resultIndex, (Var)array, i, spreadIndex);
                }
                else
                {
                    JS_REENTRANT(jsReentLock, CopyAnyArrayElementsToVar(result, resultIndex, array, i, spreadIndex));
                }
                resultIndex += spreadIndex - i;
                i = spreadIndex - 1;
                continue;
            }
            else if (i > spreadIndex)
            {
                // Any non-spread elements terminating the array can also be copied in bulk.
                Assert(spreadArrIndex == spreadIndices->count - 1);
                if (needArraySlowCopy(array))
                {
                    slowCopy(result, resultIndex, array, i, arrayLength);
                }
                else
                {
                    JS_REENTRANT(jsReentLock, CopyAnyArrayElementsToVar(result, resultIndex, array, i, arrayLength));
                }
                break;
            }
            else
            {
                JS_REENTRANT(jsReentLock, Var instance = array->DirectGetItem(i));

                if (SpreadArgument::Is(instance))
                {
                    SpreadArgument* spreadArgument = SpreadArgument::FromVar(instance);
                    uint32 len = spreadArgument->GetArgumentSpreadCount();
                    const Var*  spreadItems = spreadArgument->GetArgumentSpread();
                    for (uint32 j = 0; j < len; j++)
                    {
                        result->DirectSetItemAt(resultIndex++, spreadItems[j]);
                    }

                }
                else
                {
                    Assert(JavascriptOperators::IsUndefinedObject(instance));
                    result->DirectSetItemAt(resultIndex++, instance);
                }

                if (spreadArrIndex < spreadIndices->count - 1)
                {
                    spreadArrIndex++;
                }
            }
        }
        AssertMsg(arrayLength == array->GetLength(), "Array's length should not have changed");
        return result;
    }

    uint32 JavascriptArray::GetSpreadArgLen(Var spreadArg, ScriptContext *scriptContext)
    {
        // A spread argument can be anything that returns a 'length' property, even if that
        // property is null or undefined.
        spreadArg = CrossSite::MarshalVar(scriptContext, spreadArg);
        if (JavascriptArray::Is(spreadArg))
        {
            JavascriptArray *arr = JavascriptArray::FromVar(spreadArg);
            return arr->GetLength();
        }

        if (TypedArrayBase::Is(spreadArg))
        {
            TypedArrayBase *tarr = TypedArrayBase::FromVar(spreadArg);
            return tarr->GetLength();
        }

        if (SpreadArgument::Is(spreadArg))
        {
            SpreadArgument *spreadFunctionArgs = SpreadArgument::FromVar(spreadArg);
            return spreadFunctionArgs->GetArgumentSpreadCount();
        }

        AssertMsg(false, "LdCustomSpreadIteratorList should have converted the arg to one of the above types");
        Throw::FatalInternalError();
    }

#ifdef VALIDATE_ARRAY
    class ArraySegmentsVisitor
    {
    private:
        SparseArraySegmentBase* seg;

    public:
        ArraySegmentsVisitor(SparseArraySegmentBase* head)
            : seg(head)
        {
        }

        void operator()(SparseArraySegmentBase* s)
        {
            Assert(seg == s);
            if (seg)
            {
                seg = seg->next;
            }
        }
    };

    void JavascriptArray::ValidateArrayCommon()
    {
        SparseArraySegmentBase * lastUsedSegment = this->GetLastUsedSegment();
        AssertMsg(this != nullptr && head && lastUsedSegment, "Array should not be null");
        AssertMsg(head->left == 0, "Array always should have a segment starting at zero");

        // Simple segments validation
        bool foundLastUsedSegment = false;
        SparseArraySegmentBase *seg = head;
        while(seg != nullptr)
        {
            if (seg == lastUsedSegment)
            {
                foundLastUsedSegment = true;
            }

            AssertMsg(seg->length <= seg->size , "Length greater than size not possible");

            SparseArraySegmentBase* next = seg->next;
            if (next != nullptr)
            {
                AssertMsg(seg->left < next->left, "Segment is adjacent to or overlaps with next segment");
                AssertMsg(seg->size <= (next->left - seg->left), "Segment is adjacent to or overlaps with next segment");
                AssertMsg(!SparseArraySegmentBase::IsLeafSegment(seg, this->GetScriptContext()->GetRecycler()), "Leaf segment with a next pointer");
            }
            else
            {
                AssertMsg(seg->length <= MaxArrayLength - seg->left, "Segment index range overflow");
                AssertMsg(seg->left + seg->length <= this->length, "Segment index range exceeds array length");
            }

            seg = next;
        }
        AssertMsg(foundLastUsedSegment || HasSegmentMap(), "Corrupt lastUsedSegment in array header");

        // Validate segmentMap if present
        if (HasSegmentMap())
        {
            ArraySegmentsVisitor visitor(head);
            GetSegmentMap()->Walk(visitor);
        }
    }

    void JavascriptArray::ValidateArray()
    {
        if (!Js::Configuration::Global.flags.ArrayValidate)
        {
            return;
        }
        ValidateArrayCommon();
        // Detailed segments validation
        JavascriptArray::ValidateVarSegment(SparseArraySegment<Var>::From(head));
    }

    void JavascriptNativeIntArray::ValidateArray()
    {
        if (!Js::Configuration::Global.flags.ArrayValidate)
        {
#if DBG
            SparseArraySegmentBase *seg = head;
            while (seg)
            {
                if (seg->next != nullptr)
                {
                    AssertMsg(!SparseArraySegmentBase::IsLeafSegment(seg, this->GetScriptContext()->GetRecycler()), "Leaf segment with a next pointer");
                }
                seg = seg->next;
            }
#endif
            return;
        }
        ValidateArrayCommon();
        // Detailed segments validation
        JavascriptArray::ValidateSegment<int32>(SparseArraySegment<int32>::From(head));
    }

    void JavascriptNativeFloatArray::ValidateArray()
    {
        if (!Js::Configuration::Global.flags.ArrayValidate)
        {
#if DBG
            SparseArraySegmentBase *seg = head;
            while (seg)
            {
                if (seg->next != nullptr)
                {
                    AssertMsg(!SparseArraySegmentBase::IsLeafSegment(seg, this->GetScriptContext()->GetRecycler()), "Leaf segment with a next pointer");
                }
                seg = seg->next;
            }
#endif
            return;
        }
        ValidateArrayCommon();
        // Detailed segments validation
        JavascriptArray::ValidateSegment<double>(SparseArraySegment<double>::From(head));
    }


    void JavascriptArray::ValidateVarSegment(SparseArraySegment<Var>* seg)
    {
        if (!Js::Configuration::Global.flags.ArrayValidate)
        {
            return;
        }
        int32 inspect;
        double inspectDouble;
        while (seg)
        {
            uint32 i = 0;
            for (i = 0; i < seg->length; i++)
            {
                if (SparseArraySegment<Var>::IsMissingItem(&seg->elements[i]))
                {
                    continue;
                }
                if (TaggedInt::Is(seg->elements[i]))
                {
                    inspect = TaggedInt::ToInt32(seg->elements[i]);

                }
                else if (JavascriptNumber::Is_NoTaggedIntCheck(seg->elements[i]))
                {
                    inspectDouble = JavascriptNumber::GetValue(seg->elements[i]);
                }
                else
                {
                    AssertMsg(RecyclableObject::Is(seg->elements[i]), "Invalid entry in segment");
                }
            }
            ValidateSegment(seg);

            seg = SparseArraySegment<Var>::From(seg->next);
        }
    }

    template<typename T>
    void JavascriptArray::ValidateSegment(SparseArraySegment<T>* seg)
    {
        if (!Js::Configuration::Global.flags.ArrayValidate)
        {
            return;
        }

        while (seg)
        {
            uint32 i = seg->length;
            while (i < seg->size)
            {
                AssertMsg(SparseArraySegment<T>::IsMissingItem(&seg->elements[i]), "Non missing value the end of the segment");
                i++;
            }

            seg = SparseArraySegment<T>::From(seg->next);
        }
    }
#endif

    template <typename T>
    void JavascriptArray::InitBoxedInlineHeadSegment(SparseArraySegment<T> * dst, SparseArraySegment<T> * src)
    {
        // Don't copy the segment map, we will build it again
        SetFlags(GetFlags() & ~DynamicObjectFlags::HasSegmentMap);

        SetHeadAndLastUsedSegment(dst);

        dst->left = src->left;
        dst->length = src->length;
        dst->size = src->size;
        dst->CheckLengthvsSize();
        dst->next = src->next;

        CopyArray(dst->elements, dst->size, src->elements, src->size);
    }

    JavascriptArray::JavascriptArray(JavascriptArray * instance, bool boxHead)
        : ArrayObject(instance)
    {
        if (boxHead)
        {
            InitBoxedInlineHeadSegment(DetermineInlineHeadSegmentPointer<JavascriptArray, 0, true>(this), SparseArraySegment<Var>::From(instance->head));
        }
        else
        {

            SetFlags(GetFlags() & ~DynamicObjectFlags::HasSegmentMap);
            head = instance->head;
            SetLastUsedSegment(instance->GetLastUsedSegment());
        }
    }

    template <typename T>
    T * JavascriptArray::BoxStackInstance(T * instance)
    {
        Assert(ThreadContext::IsOnStack(instance));
        // On the stack, the we reserved a pointer before the object as to store the boxed value
        T ** boxedInstanceRef = ((T **)instance) - 1;
        T * boxedInstance = *boxedInstanceRef;
        if (boxedInstance)
        {
            return boxedInstance;
        }

        const size_t inlineSlotsSize = instance->GetTypeHandler()->GetInlineSlotsSize();
        if (ThreadContext::IsOnStack(instance->head))
        {
            boxedInstance = RecyclerNewPlusZ(instance->GetRecycler(),
                inlineSlotsSize + sizeof(Js::SparseArraySegmentBase) + instance->head->size * sizeof(typename T::TElement),
                T, instance, true);
        }
        else if(inlineSlotsSize)
        {
            boxedInstance = RecyclerNewPlusZ(instance->GetRecycler(), inlineSlotsSize, T, instance, false);
        }
        else
        {
            boxedInstance = RecyclerNew(instance->GetRecycler(), T, instance, false);
        }

        *boxedInstanceRef = boxedInstance;
        return boxedInstance;
    }

    JavascriptArray *
    JavascriptArray::BoxStackInstance(JavascriptArray * instance)
    {
        return BoxStackInstance<JavascriptArray>(instance);
    }

#if ENABLE_TTD
    void JavascriptArray::MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor)
    {
        TTDAssert(this->GetTypeId() == Js::TypeIds_Array || this->GetTypeId() == Js::TypeIds_ES5Array, "Should only be used on basic arrays (or called as super from ES5Array.");

        ScriptContext* ctx = this->GetScriptContext();

        uint32 index = Js::JavascriptArray::InvalidIndex;
        while(true)
        {
            index = this->GetNextIndex(index);
            if(index == Js::JavascriptArray::InvalidIndex) // End of array
            {
                break;
            }

            Js::Var aval = nullptr;
            if(this->DirectGetVarItemAt(index, &aval, ctx))
            {
                extractor->MarkVisitVar(aval);
            }
        }
    }

    void JavascriptArray::ProcessCorePaths()
    {
        TTDAssert(this->GetTypeId() == Js::TypeIds_Array, "Should only be used on basic arrays.");

        ScriptContext* ctx = this->GetScriptContext();

        uint32 index = Js::JavascriptArray::InvalidIndex;
        while(true)
        {
            index = this->GetNextIndex(index);
            if(index == Js::JavascriptArray::InvalidIndex) // End of array
            {
                break;
            }

            Js::Var aval = nullptr;
            if(this->DirectGetVarItemAt(index, &aval, ctx))
            {
                TTD::UtilSupport::TTAutoString pathExt;
                ctx->TTDWellKnownInfo->BuildArrayIndexBuffer(index, pathExt);

                ctx->TTDWellKnownInfo->EnqueueNewPathVarAsNeeded(this, aval, pathExt.GetStrValue());
            }
        }
    }

    TTD::NSSnapObjects::SnapObjectType JavascriptArray::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapArrayObject;
    }

    void JavascriptArray::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTDAssert(this->GetTypeId() == Js::TypeIds_Array, "Should only be used on basic Js arrays.");

        TTD::NSSnapObjects::SnapArrayInfo<TTD::TTDVar>* sai = TTD::NSSnapObjects::ExtractArrayValues<TTD::TTDVar>(this, alloc);
        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapArrayInfo<TTD::TTDVar>*, TTD::NSSnapObjects::SnapObjectType::SnapArrayObject>(objData, sai);
    }
#endif

    JavascriptNativeArray::JavascriptNativeArray(JavascriptNativeArray * instance) :
        JavascriptArray(instance, false),
        weakRefToFuncBody(instance->weakRefToFuncBody)
    {
    }

    JavascriptNativeIntArray::JavascriptNativeIntArray(JavascriptNativeIntArray * instance, bool boxHead) :
        JavascriptNativeArray(instance)
    {
        if (boxHead)
        {
            InitBoxedInlineHeadSegment(DetermineInlineHeadSegmentPointer<JavascriptNativeIntArray, 0, true>(this), SparseArraySegment<int>::From(instance->head));
        }
        else
        {
            // Base class ctor should have copied these
            Assert(head == instance->head);
            Assert(segmentUnion.lastUsedSegment == instance->GetLastUsedSegment());
        }
    }

    JavascriptNativeIntArray *
    JavascriptNativeIntArray::BoxStackInstance(JavascriptNativeIntArray * instance)
    {
        return JavascriptArray::BoxStackInstance<JavascriptNativeIntArray>(instance);
    }

#if ENABLE_TTD
    TTD::NSSnapObjects::SnapObjectType JavascriptNativeIntArray::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapNativeIntArrayObject;
    }

    void JavascriptNativeIntArray::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTD::NSSnapObjects::SnapArrayInfo<int32>* sai = TTD::NSSnapObjects::ExtractArrayValues<int32>(this, alloc);
        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapArrayInfo<int32>*, TTD::NSSnapObjects::SnapObjectType::SnapNativeIntArrayObject>(objData, sai);
    }

#if ENABLE_COPYONACCESS_ARRAY
    TTD::NSSnapObjects::SnapObjectType JavascriptCopyOnAccessNativeIntArray::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::Invalid;
    }

    void JavascriptCopyOnAccessNativeIntArray::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTDAssert(false, "Not implemented yet!!!");
    }
#endif
#endif

    JavascriptNativeFloatArray::JavascriptNativeFloatArray(JavascriptNativeFloatArray * instance, bool boxHead) :
        JavascriptNativeArray(instance)
    {
        if (boxHead)
        {
            InitBoxedInlineHeadSegment(DetermineInlineHeadSegmentPointer<JavascriptNativeFloatArray, 0, true>(this), SparseArraySegment<double>::From(instance->head));
        }
        else
        {
            // Base class ctor should have copied these
            Assert(head == instance->head);
            Assert(segmentUnion.lastUsedSegment == instance->GetLastUsedSegment());
        }
    }

    JavascriptNativeFloatArray *
    JavascriptNativeFloatArray::BoxStackInstance(JavascriptNativeFloatArray * instance)
    {
        return JavascriptArray::BoxStackInstance<JavascriptNativeFloatArray>(instance);
    }

#if ENABLE_TTD
    TTD::NSSnapObjects::SnapObjectType JavascriptNativeFloatArray::GetSnapTag_TTD() const
    {
        return TTD::NSSnapObjects::SnapObjectType::SnapNativeFloatArrayObject;
    }

    void JavascriptNativeFloatArray::ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc)
    {
        TTDAssert(this->GetTypeId() == Js::TypeIds_NativeFloatArray, "Should only be used on native float arrays.");

        TTD::NSSnapObjects::SnapArrayInfo<double>* sai = TTD::NSSnapObjects::ExtractArrayValues<double>(this, alloc);
        TTD::NSSnapObjects::StdExtractSetKindSpecificInfo<TTD::NSSnapObjects::SnapArrayInfo<double>*, TTD::NSSnapObjects::SnapObjectType::SnapNativeFloatArrayObject>(objData, sai);
    }
#endif

    template<typename T>
    RecyclableObject*
    JavascriptArray::ArraySpeciesCreate(Var originalArray, T length, ScriptContext* scriptContext, bool *pIsIntArray, bool *pIsFloatArray, bool *pIsBuiltinArrayCtor)
    {
        if (originalArray == nullptr || !scriptContext->GetConfig()->IsES6SpeciesEnabled())
        {
            return nullptr;
        }

        if (JavascriptArray::Is(originalArray)
            && !DynamicObject::FromVar(originalArray)->GetDynamicType()->GetTypeHandler()->GetIsNotPathTypeHandlerOrHasUserDefinedCtor()
            && DynamicObject::FromVar(originalArray)->GetPrototype() == scriptContext->GetLibrary()->GetArrayPrototype()
            && !scriptContext->GetLibrary()->GetArrayObjectHasUserDefinedSpecies())
        {
            return nullptr;
        }

        Var constructor = scriptContext->GetLibrary()->GetUndefined();

        if (JavascriptOperators::IsArray(originalArray))
        {
            if (!JavascriptOperators::GetProperty(RecyclableObject::FromVar(originalArray), PropertyIds::constructor, &constructor, scriptContext))
            {
                return nullptr;
            }

            if (JavascriptOperators::IsConstructor(constructor))
            {
                ScriptContext* constructorScriptContext = RecyclableObject::FromVar(constructor)->GetScriptContext();
                if (constructorScriptContext != scriptContext)
                {
                    if (constructorScriptContext->GetLibrary()->GetArrayConstructor() == constructor)
                    {
                        constructor = scriptContext->GetLibrary()->GetUndefined();
                    }
                }
            }

            if (JavascriptOperators::IsObject(constructor))
            {
                if (!JavascriptOperators::GetProperty((RecyclableObject*)constructor, PropertyIds::_symbolSpecies, &constructor, scriptContext))
                {
                    if (pIsBuiltinArrayCtor != nullptr)
                    {
                        *pIsBuiltinArrayCtor = false;
                    }
                    return nullptr;
                }
                if (constructor == scriptContext->GetLibrary()->GetNull())
                {
                    constructor = scriptContext->GetLibrary()->GetUndefined();
                }
            }
        }

        if (constructor == scriptContext->GetLibrary()->GetUndefined() || constructor == scriptContext->GetLibrary()->GetArrayConstructor())
        {
            if (length > UINT_MAX)
            {
                JavascriptError::ThrowRangeError(scriptContext, JSERR_ArrayLengthConstructIncorrect);
            }

            if (nullptr == pIsIntArray)
            {
                return scriptContext->GetLibrary()->CreateArray(static_cast<uint32>(length));
            }
            else
            {
                // If the constructor function is the built-in Array constructor, we can be smart and create the right type of native array.
                JavascriptArray* pArr = JavascriptArray::FromVar(originalArray);
                pArr->GetArrayTypeAndConvert(pIsIntArray, pIsFloatArray);
                return CreateNewArrayHelper(static_cast<uint32>(length), *pIsIntArray, *pIsFloatArray, pArr, scriptContext);
            }
        }

        if (!JavascriptOperators::IsConstructor(constructor))
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_NotAConstructor, _u("constructor[Symbol.species]"));
        }

        if (pIsBuiltinArrayCtor != nullptr)
        {
            *pIsBuiltinArrayCtor = false;
        }

        Js::Var constructorArgs[] = { constructor, JavascriptNumber::ToVar(length, scriptContext) };
        Js::CallInfo constructorCallInfo(Js::CallFlags_New, _countof(constructorArgs));

        return RecyclableObject::FromVar(JavascriptOperators::NewScObject(constructor, Js::Arguments(constructorCallInfo, constructorArgs), scriptContext));
    }
    /*static*/
    PropertyId const JavascriptArray::specialPropertyIds[] =
    {
        PropertyIds::length
    };

    BOOL JavascriptArray::DeleteProperty(PropertyId propertyId, PropertyOperationFlags flags)
    {
        if (propertyId == PropertyIds::length)
        {
            return false;
        }
        return DynamicObject::DeleteProperty(propertyId, flags);
    }

    BOOL JavascriptArray::DeleteProperty(JavascriptString *propertyNameString, PropertyOperationFlags flags)
    {
        JsUtil::CharacterBuffer<WCHAR> propertyName(propertyNameString->GetString(), propertyNameString->GetLength());
        if (BuiltInPropertyRecords::length.Equals(propertyName))
        {
            return false;
        }
        return DynamicObject::DeleteProperty(propertyNameString, flags);
    }

    PropertyQueryFlags JavascriptArray::HasPropertyQuery(PropertyId propertyId)
    {
        if (propertyId == PropertyIds::length)
        {
            return PropertyQueryFlags::Property_Found;
        }

        ScriptContext* scriptContext = GetScriptContext();
        uint32 index;
        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            return JavascriptConversion::BooleanToPropertyQueryFlags(this->HasItem(index));
        }

        return DynamicObject::HasPropertyQuery(propertyId);
    }

    BOOL JavascriptArray::IsEnumerable(PropertyId propertyId)
    {
        if (propertyId == PropertyIds::length)
        {
            return false;
        }
        return DynamicObject::IsEnumerable(propertyId);
    }

    BOOL JavascriptArray::IsConfigurable(PropertyId propertyId)
    {
        if (propertyId == PropertyIds::length)
        {
            return false;
        }
        return DynamicObject::IsConfigurable(propertyId);
    }

    //
    // Evolve typeHandlers explicitly so that simple typeHandlers can skip array
    // handling and only check instance objectArray for numeric propertyIds.
    //
    BOOL JavascriptArray::SetEnumerable(PropertyId propertyId, BOOL value)
    {
        if (propertyId == PropertyIds::length)
        {
            Assert(!value); // Can't change array length enumerable
            return true;
        }

        ScriptContext* scriptContext = this->GetScriptContext();

        uint32 index;
        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            return GetTypeHandler()->ConvertToTypeWithItemAttributes(this)
                ->SetEnumerable(this, propertyId, value);
        }

        return __super::SetEnumerable(propertyId, value);
    }

    //
    // Evolve typeHandlers explicitly so that simple typeHandlers can skip array
    // handling and only check instance objectArray for numeric propertyIds.
    //
    BOOL JavascriptArray::SetWritable(PropertyId propertyId, BOOL value)
    {
        ScriptContext* scriptContext = this->GetScriptContext();
        uint32 index;

        bool setLengthNonWritable = (propertyId == PropertyIds::length && !value);
        if (setLengthNonWritable || scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            return GetTypeHandler()->ConvertToTypeWithItemAttributes(this)
                ->SetWritable(this, propertyId, value);
        }

        return __super::SetWritable(propertyId, value);
    }

    //
    // Evolve typeHandlers explicitly so that simple typeHandlers can skip array
    // handling and only check instance objectArray for numeric propertyIds.
    //
    BOOL JavascriptArray::SetConfigurable(PropertyId propertyId, BOOL value)
    {
        if (propertyId == PropertyIds::length)
        {
            Assert(!value); // Can't change array length configurable
            return true;
        }

        ScriptContext* scriptContext = this->GetScriptContext();

        uint32 index;
        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            return GetTypeHandler()->ConvertToTypeWithItemAttributes(this)
                ->SetConfigurable(this, propertyId, value);
        }

        return __super::SetConfigurable(propertyId, value);
    }

    //
    // Evolve typeHandlers explicitly so that simple typeHandlers can skip array
    // handling and only check instance objectArray for numeric propertyIds.
    //
    BOOL JavascriptArray::SetAttributes(PropertyId propertyId, PropertyAttributes attributes)
    {
        ScriptContext* scriptContext = this->GetScriptContext();

        // SetAttributes on "length" is not expected. DefineOwnProperty uses SetWritable. If this is
        // changed, we need to handle it here.
        Assert(propertyId != PropertyIds::length);

        uint32 index;
        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            return GetTypeHandler()->ConvertToTypeWithItemAttributes(this)
                ->SetItemAttributes(this, index, attributes);
        }

        return __super::SetAttributes(propertyId, attributes);
    }

    //
    // Evolve typeHandlers explicitly so that simple typeHandlers can skip array
    // handling and only check instance objectArray for numeric propertyIds.
    //
    BOOL JavascriptArray::SetAccessors(PropertyId propertyId, Var getter, Var setter, PropertyOperationFlags flags)
    {
        ScriptContext* scriptContext = this->GetScriptContext();

        uint32 index;
        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            return GetTypeHandler()->ConvertToTypeWithItemAttributes(this)
                ->SetItemAccessors(this, index, getter, setter);
        }

        return __super::SetAccessors(propertyId, getter, setter, flags);
    }

    //
    // Evolve typeHandlers explicitly so that simple typeHandlers can skip array
    // handling and only check instance objectArray for numeric propertyIds.
    //
    BOOL JavascriptArray::SetItemWithAttributes(uint32 index, Var value, PropertyAttributes attributes)
    {
        return GetTypeHandler()->ConvertToTypeWithItemAttributes(this)
            ->SetItemWithAttributes(this, index, value, attributes);
    }

    //
    // Evolve typeHandlers explicitly so that simple typeHandlers can skip array
    // handling and only check instance objectArray for numeric propertyIds.
    //
    BOOL JavascriptArray::SetItemAttributes(uint32 index, PropertyAttributes attributes)
    {
        return GetTypeHandler()->ConvertToTypeWithItemAttributes(this)
            ->SetItemAttributes(this, index, attributes);
    }

    //
    // Evolve typeHandlers explicitly so that simple typeHandlers can skip array
    // handling and only check instance objectArray for numeric propertyIds.
    //
    BOOL JavascriptArray::SetItemAccessors(uint32 index, Var getter, Var setter)
    {
        return GetTypeHandler()->ConvertToTypeWithItemAttributes(this)
            ->SetItemAccessors(this, index, getter, setter);
    }

    // Check if this objectArray isFrozen.
    BOOL JavascriptArray::IsObjectArrayFrozen()
    {
        // If this is still a JavascriptArray, it's not frozen.
        return false;
    }

    JavascriptEnumerator * JavascriptArray::GetIndexEnumerator(EnumeratorFlags flags, ScriptContext* requestContext)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(this);
#endif

        if (!!(flags & EnumeratorFlags::SnapShotSemantics))
        {
            return RecyclerNew(GetRecycler(), JavascriptArrayIndexSnapshotEnumerator, this, flags, requestContext);
        }
        return RecyclerNew(GetRecycler(), JavascriptArrayIndexEnumerator, this, flags, requestContext);
    }

    BOOL JavascriptArray::GetNonIndexEnumerator(JavascriptStaticEnumerator * enumerator, ScriptContext* requestContext)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(this);
#endif
        return enumerator->Initialize(nullptr, nullptr, this, EnumeratorFlags::SnapShotSemantics, requestContext, nullptr);
    }

    BOOL JavascriptArray::IsItemEnumerable(uint32 index)
    {
        return true;
    }

    //
    // Evolve typeHandlers explicitly so that simple typeHandlers can skip array
    // handling and only check instance objectArray for numeric propertyIds.
    //
    BOOL JavascriptArray::PreventExtensions()
    {
        return GetTypeHandler()->ConvertToTypeWithItemAttributes(this)->PreventExtensions(this);
    }

    //
    // Evolve typeHandlers explicitly so that simple typeHandlers can skip array
    // handling and only check instance objectArray for numeric propertyIds.
    //
    BOOL JavascriptArray::Seal()
    {
        return GetTypeHandler()->ConvertToTypeWithItemAttributes(this)->Seal(this);
    }

    //
    // Evolve typeHandlers explicitly so that simple typeHandlers can skip array
    // handling and only check instance objectArray for numeric propertyIds.
    //
    BOOL JavascriptArray::Freeze()
    {
        return GetTypeHandler()->ConvertToTypeWithItemAttributes(this)->Freeze(this);
    }

    BOOL JavascriptArray::GetSpecialPropertyName(uint32 index, JavascriptString ** propertyName, ScriptContext * requestContext)
    {
        if (index == 0)
        {
            *propertyName = requestContext->GetPropertyString(PropertyIds::length);
            return true;
        }
        return false;
    }

    // Returns the number of special non-enumerable properties this type has.
    uint JavascriptArray::GetSpecialPropertyCount() const
    {
        return _countof(specialPropertyIds);
    }

    // Returns the list of special non-enumerable properties for the type.
    PropertyId const * JavascriptArray::GetSpecialPropertyIds() const
    {
        return specialPropertyIds;
    }

    PropertyQueryFlags JavascriptArray::GetPropertyReferenceQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptArray::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }

    PropertyQueryFlags JavascriptArray::GetPropertyQuery(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        if (GetPropertyBuiltIns(propertyId, value))
        {
            return PropertyQueryFlags::Property_Found;
        }

        ScriptContext* scriptContext = GetScriptContext();
        uint32 index;
        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            return JavascriptConversion::BooleanToPropertyQueryFlags(this->GetItem(this, index, value, scriptContext));
        }

        return DynamicObject::GetPropertyQuery(originalInstance, propertyId, value, info, requestContext);
    }

    PropertyQueryFlags JavascriptArray::GetPropertyQuery(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        AssertMsg(!PropertyRecord::IsPropertyNameNumeric(propertyNameString->GetString(), propertyNameString->GetLength()),
            "Numeric property names should have been converted to uint or PropertyRecord*");

        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetPropertyBuiltIns(propertyRecord->GetPropertyId(), value))
        {
            return PropertyQueryFlags::Property_Found;
        }

        return DynamicObject::GetPropertyQuery(originalInstance, propertyNameString, value, info, requestContext);
    }

    BOOL JavascriptArray::GetPropertyBuiltIns(PropertyId propertyId, Var* value)
    {
        //
        // length being accessed. Return array length
        //
        if (propertyId == PropertyIds::length)
        {
            *value = JavascriptNumber::ToVar(this->GetLength(), GetScriptContext());
            return true;
        }

        return false;
    }

    PropertyQueryFlags JavascriptArray::HasItemQuery(uint32 index)
    {
        Var value = nullptr;
        return JavascriptConversion::BooleanToPropertyQueryFlags(this->DirectGetItemAt<Var>(index, &value));
    }

    PropertyQueryFlags JavascriptArray::GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext* requestContext)
    {
        return JavascriptConversion::BooleanToPropertyQueryFlags(this->DirectGetItemAt<Var>(index, value));
    }

    PropertyQueryFlags JavascriptArray::GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext* requestContext)
    {
        return JavascriptConversion::BooleanToPropertyQueryFlags(this->DirectGetItemAt<Var>(index, value));
    }

    BOOL JavascriptArray::DirectGetVarItemAt(uint32 index, Var *value, ScriptContext *requestContext)
    {
        return this->DirectGetItemAt<Var>(index, value);
    }

    PropertyQueryFlags JavascriptNativeIntArray::HasItemQuery(uint32 index)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(this);
#endif
        int32 value;
        return JavascriptConversion::BooleanToPropertyQueryFlags(this->DirectGetItemAt<int32>(index, &value));
    }

    PropertyQueryFlags JavascriptNativeFloatArray::HasItemQuery(uint32 index)
    {
        double dvalue;
        return JavascriptConversion::BooleanToPropertyQueryFlags(this->DirectGetItemAt<double>(index, &dvalue));
    }

    PropertyQueryFlags JavascriptNativeIntArray::GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext* requestContext)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(this);
#endif
        return JavascriptConversion::BooleanToPropertyQueryFlags(JavascriptNativeIntArray::DirectGetVarItemAt(index, value, requestContext));
    }

    BOOL JavascriptNativeIntArray::DirectGetVarItemAt(uint32 index, Var *value, ScriptContext *requestContext)
    {
        int32 intvalue;
        if (!this->DirectGetItemAt<int32>(index, &intvalue))
        {
            return FALSE;
        }
        *value = JavascriptNumber::ToVar(intvalue, requestContext);
        return TRUE;
    }

    PropertyQueryFlags JavascriptNativeIntArray::GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext* requestContext)
    {
        return JavascriptNativeIntArray::GetItemQuery(originalInstance, index, value, requestContext);
    }

    PropertyQueryFlags JavascriptNativeFloatArray::GetItemQuery(Var originalInstance, uint32 index, Var* value, ScriptContext* requestContext)
    {
        return JavascriptConversion::BooleanToPropertyQueryFlags(JavascriptNativeFloatArray::DirectGetVarItemAt(index, value, requestContext));
    }

    BOOL JavascriptNativeFloatArray::DirectGetVarItemAt(uint32 index, Var *value, ScriptContext *requestContext)
    {
        double dvalue;
        int32 ivalue;
        if (!this->DirectGetItemAt<double>(index, &dvalue))
        {
            return FALSE;
        }
        if (*(uint64*)&dvalue == 0ull)
        {
            *value = TaggedInt::ToVarUnchecked(0);
        }
        else if (JavascriptNumber::TryGetInt32Value(dvalue, &ivalue) && !TaggedInt::IsOverflow(ivalue))
        {
            *value = TaggedInt::ToVarUnchecked(ivalue);
        }
        else
        {
            *value = JavascriptNumber::ToVarWithCheck(dvalue, requestContext);
        }
        return TRUE;
    }

    PropertyQueryFlags JavascriptNativeFloatArray::GetItemReferenceQuery(Var originalInstance, uint32 index, Var* value, ScriptContext* requestContext)
    {
        return JavascriptNativeFloatArray::GetItemQuery(originalInstance, index, value, requestContext);
    }

    BOOL JavascriptArray::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(this);
#endif
        uint32 indexValue;
        if (propertyId == PropertyIds::length)
        {
            return this->SetLength(value);
        }
        else if (GetScriptContext()->IsNumericPropertyId(propertyId, &indexValue))
        {
            // Call this or subclass method
            return SetItem(indexValue, value, flags);
        }
        else
        {
            return DynamicObject::SetProperty(propertyId, value, flags, info);
        }
    }

    BOOL JavascriptArray::SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        AssertMsg(!PropertyRecord::IsPropertyNameNumeric(propertyNameString->GetString(), propertyNameString->GetLength()),
            "Numeric property names should have been converted to uint or PropertyRecord*");

#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(this);
#endif
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && propertyRecord->GetPropertyId() == PropertyIds::length)
        {
            return this->SetLength(value);
        }

        return DynamicObject::SetProperty(propertyNameString, value, flags, info);
    }

    BOOL JavascriptArray::SetPropertyWithAttributes(PropertyId propertyId, Var value, PropertyAttributes attributes, PropertyValueInfo* info, PropertyOperationFlags flags, SideEffects possibleSideEffects)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(this);
#endif
        ScriptContext* scriptContext = GetScriptContext();

        if (propertyId == PropertyIds::length)
        {
            Assert(attributes == PropertyWritable);
            Assert(IsWritable(propertyId) && !IsConfigurable(propertyId) && !IsEnumerable(propertyId));
            return this->SetLength(value);
        }

        uint32 index;
        if (scriptContext->IsNumericPropertyId(propertyId, &index))
        {
            // Call this or subclass method
            return SetItemWithAttributes(index, value, attributes);
        }

        return __super::SetPropertyWithAttributes(propertyId, value, attributes, info, flags, possibleSideEffects);
    }

    BOOL JavascriptArray::SetItem(uint32 index, Var value, PropertyOperationFlags flags)
    {
        this->DirectSetItemAt(index, value);
        return true;
    }

    BOOL JavascriptNativeIntArray::SetItem(uint32 index, Var value, PropertyOperationFlags flags)
    {
        int32 iValue;
        double dValue;
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(this);
#endif
        TypeId typeId = this->TrySetNativeIntArrayItem(value, &iValue, &dValue);
        if (typeId == TypeIds_NativeIntArray)
        {
            this->SetItem(index, iValue);
        }
        else if (typeId == TypeIds_NativeFloatArray)
        {
            reinterpret_cast<JavascriptNativeFloatArray*>(this)->DirectSetItemAt<double>(index, dValue);
        }
        else
        {
            this->DirectSetItemAt<Var>(index, value);
        }

        return TRUE;
    }

    TypeId JavascriptNativeIntArray::TrySetNativeIntArrayItem(Var value, int32 *iValue, double *dValue)
    {
        if (TaggedInt::Is(value))
        {
            int32 i = TaggedInt::ToInt32(value);
            if (i != JavascriptNativeIntArray::MissingItem)
            {
                *iValue = i;
                return TypeIds_NativeIntArray;
            }
        }
        if (JavascriptNumber::Is_NoTaggedIntCheck(value))
        {
            bool isInt32;
            int32 i;
            double d = JavascriptNumber::GetValue(value);
            if (JavascriptNumber::TryGetInt32OrUInt32Value(d, &i, &isInt32))
            {
                if (isInt32 && i != JavascriptNativeIntArray::MissingItem)
                {
                    *iValue = i;
                    return TypeIds_NativeIntArray;
                }
            }
            else
            {
                *dValue = d;
                JavascriptNativeIntArray::ToNativeFloatArray(this);
                return TypeIds_NativeFloatArray;
            }
        }

        JavascriptNativeIntArray::ToVarArray(this);
        return TypeIds_Array;
    }

    BOOL JavascriptNativeIntArray::SetItem(uint32 index, int32 iValue)
    {
        if (iValue == JavascriptNativeIntArray::MissingItem)
        {
            JavascriptArray *varArr = JavascriptNativeIntArray::ToVarArray(this);
            varArr->DirectSetItemAt(index, JavascriptNumber::ToVar(iValue, GetScriptContext()));
            return TRUE;
        }

        this->DirectSetItemAt(index, iValue);
        return TRUE;
    }

    BOOL JavascriptNativeFloatArray::SetItem(uint32 index, Var value, PropertyOperationFlags flags)
    {
        double dValue;
        TypeId typeId = this->TrySetNativeFloatArrayItem(value, &dValue);
        if (typeId == TypeIds_NativeFloatArray)
        {
            this->SetItem(index, dValue);
        }
        else
        {
            this->DirectSetItemAt(index, value);
        }
        return TRUE;
    }

    TypeId JavascriptNativeFloatArray::TrySetNativeFloatArrayItem(Var value, double *dValue)
    {
        if (TaggedInt::Is(value))
        {
            *dValue = (double)TaggedInt::ToInt32(value);
            return TypeIds_NativeFloatArray;
        }
        else if (JavascriptNumber::Is_NoTaggedIntCheck(value))
        {
            *dValue = JavascriptNumber::GetValue(value);
            return TypeIds_NativeFloatArray;
        }

        JavascriptNativeFloatArray::ToVarArray(this);
        return TypeIds_Array;
    }

    BOOL JavascriptNativeFloatArray::SetItem(uint32 index, double dValue)
    {
        if (*(uint64*)&dValue == *(uint64*)&JavascriptNativeFloatArray::MissingItem)
        {
            JavascriptArray *varArr = JavascriptNativeFloatArray::ToVarArray(this);
            varArr->DirectSetItemAt(index, JavascriptNumber::ToVarNoCheck(dValue, GetScriptContext()));
            return TRUE;
        }

        this->DirectSetItemAt<double>(index, dValue);
        return TRUE;
    }

    BOOL JavascriptArray::DeleteItem(uint32 index, PropertyOperationFlags flags)
    {
        return this->DirectDeleteItemAt<Var>(index);
    }

    BOOL JavascriptNativeIntArray::DeleteItem(uint32 index, PropertyOperationFlags flags)
    {
        return this->DirectDeleteItemAt<int32>(index);
    }

    BOOL JavascriptNativeFloatArray::DeleteItem(uint32 index, PropertyOperationFlags flags)
    {
        return this->DirectDeleteItemAt<double>(index);
    }

    BOOL JavascriptArray::GetEnumerator(JavascriptStaticEnumerator * enumerator, EnumeratorFlags flags, ScriptContext* requestContext, ForInCache * forInCache)
    {
#if ENABLE_COPYONACCESS_ARRAY
        JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Var>(this);
#endif
        return enumerator->Initialize(nullptr, this, this, flags, requestContext, forInCache);
    }

    BOOL JavascriptArray::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->Append(_u('['));

        if (this->length < 10)
        {
            auto funcPtr = [&]()
            {
                ENTER_PINNED_SCOPE(JavascriptString, valueStr);
                valueStr = JavascriptArray::JoinHelper(this, GetLibrary()->GetCommaDisplayString(), requestContext);
                stringBuilder->Append(valueStr->GetString(), valueStr->GetLength());
                LEAVE_PINNED_SCOPE();
            };

            if (!requestContext->GetThreadContext()->IsScriptActive())
            {
                BEGIN_JS_RUNTIME_CALL(requestContext);
                {
                    funcPtr();
                }
                END_JS_RUNTIME_CALL(requestContext);
            }
            else
            {
                funcPtr();
            }
        }
        else
        {
            stringBuilder->AppendCppLiteral(_u("..."));
        }

        stringBuilder->Append(_u(']'));

        return TRUE;
    }

    BOOL JavascriptArray::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(_u("Object, (Array)"));
        return TRUE;
    }

    bool JavascriptNativeArray::Is(Var aValue)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(aValue);
        return JavascriptNativeArray::Is(typeId);
    }

    bool JavascriptNativeArray::Is(TypeId typeId)
    {
        return JavascriptNativeIntArray::Is(typeId) || JavascriptNativeFloatArray::Is(typeId);
    }

    JavascriptNativeArray* JavascriptNativeArray::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptNativeArray'");

        return static_cast<JavascriptNativeArray *>(RecyclableObject::FromVar(aValue));
    }

    bool JavascriptNativeIntArray::Is(Var aValue)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(aValue);
        return JavascriptNativeIntArray::Is(typeId);
    }

#if ENABLE_COPYONACCESS_ARRAY
    bool JavascriptCopyOnAccessNativeIntArray::Is(Var aValue)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(aValue);
        return JavascriptCopyOnAccessNativeIntArray::Is(typeId);
    }
#endif

    bool JavascriptNativeIntArray::Is(TypeId typeId)
    {
        return typeId == TypeIds_NativeIntArray;
    }

#if ENABLE_COPYONACCESS_ARRAY
    bool JavascriptCopyOnAccessNativeIntArray::Is(TypeId typeId)
    {
        return typeId == TypeIds_CopyOnAccessNativeIntArray;
    }
#endif

    bool JavascriptNativeIntArray::IsNonCrossSite(Var aValue)
    {
        bool ret = !TaggedInt::Is(aValue) && VirtualTableInfo<JavascriptNativeIntArray>::HasVirtualTable(aValue);
        Assert(ret == (JavascriptNativeIntArray::Is(aValue) && !JavascriptNativeIntArray::FromVar(aValue)->IsCrossSiteObject()));
        return ret;
    }

    JavascriptNativeIntArray* JavascriptNativeIntArray::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptNativeIntArray'");

        return static_cast<JavascriptNativeIntArray *>(RecyclableObject::FromVar(aValue));
    }

#if ENABLE_COPYONACCESS_ARRAY
    JavascriptCopyOnAccessNativeIntArray* JavascriptCopyOnAccessNativeIntArray::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptCopyOnAccessNativeIntArray'");

        return static_cast<JavascriptCopyOnAccessNativeIntArray *>(RecyclableObject::FromVar(aValue));
    }
#endif

    bool JavascriptNativeFloatArray::Is(Var aValue)
    {
        TypeId typeId = JavascriptOperators::GetTypeId(aValue);
        return JavascriptNativeFloatArray::Is(typeId);
    }

    bool JavascriptNativeFloatArray::Is(TypeId typeId)
    {
        return typeId == TypeIds_NativeFloatArray;
    }

    bool JavascriptNativeFloatArray::IsNonCrossSite(Var aValue)
    {
        bool ret = !TaggedInt::Is(aValue) && VirtualTableInfo<JavascriptNativeFloatArray>::HasVirtualTable(aValue);
        Assert(ret == (JavascriptNativeFloatArray::Is(aValue) && !JavascriptNativeFloatArray::FromVar(aValue)->IsCrossSiteObject()));
        return ret;
    }

    JavascriptNativeFloatArray* JavascriptNativeFloatArray::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptNativeFloatArray'");

        return static_cast<JavascriptNativeFloatArray *>(RecyclableObject::FromVar(aValue));
    }

    template int   Js::JavascriptArray::GetParamForIndexOf<unsigned int>(unsigned int, Js::Arguments const&, void*&, unsigned int&, Js::ScriptContext*);
    template bool  Js::JavascriptArray::ArrayElementEnumerator::MoveNext<void*>();
    template void  Js::JavascriptArray::SetArrayLiteralItem<void*>(unsigned int, void*);
    template void* Js::JavascriptArray::TemplatedIndexOfHelper<false, Js::TypedArrayBase, unsigned int>(Js::TypedArrayBase*, void*, unsigned int, unsigned int, Js::ScriptContext*);
    template void* Js::JavascriptArray::TemplatedIndexOfHelper<true, Js::TypedArrayBase, unsigned int>(Js::TypedArrayBase*, void*, unsigned int, unsigned int, Js::ScriptContext*);
} //namespace Js
