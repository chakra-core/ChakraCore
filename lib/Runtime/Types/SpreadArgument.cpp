//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeTypePch.h"
#include "Types/SpreadArgument.h"
namespace Js
{
    bool SpreadArgument::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_SpreadArgument;
    }
    SpreadArgument* SpreadArgument::FromVar(Var aValue)
    {
        Assert(SpreadArgument::Is(aValue));
        return static_cast<SpreadArgument*>(aValue);
    }

    SpreadArgument::SpreadArgument(Var iterator, bool useDirectCall, DynamicType * type)
        : DynamicObject(type), iteratorIndices(nullptr)
    {
        Assert(iterator != nullptr);
        ScriptContext * scriptContext = this->GetScriptContext();
        Assert(iteratorIndices == nullptr);

        if (useDirectCall)
        {
            if (JavascriptArray::Is(iterator))
            {
                JavascriptArray *array = JavascriptArray::FromVar(iterator);
                if (!array->HasNoMissingValues())
                {
                    AssertAndFailFast();
                }

                uint32 length = array->GetLength();
                if (length > 0)
                {
                    iteratorIndices = RecyclerNew(scriptContext->GetRecycler(), VarList, scriptContext->GetRecycler());
                    for (uint32 j = 0; j < length; j++)
                    {
                        Var element = nullptr;
                        if (array->DirectGetItemAtFull(j, &element))
                        {
                            iteratorIndices->Add(element);
                        }
                    }
                    // Array length shouldn't have changed as we determined that there is no missing values.
                    Assert(length == array->GetLength());
                }
            }
            else if (TypedArrayBase::Is(iterator))
            {
                TypedArrayBase *typedArray = TypedArrayBase::FromVar(iterator);

                if (typedArray->IsDetachedBuffer())
                {
                    JavascriptError::ThrowTypeError(scriptContext, JSERR_DetachedTypedArray);
                }

                uint32 length = typedArray->GetLength();
                if (length > 0)
                {
                    iteratorIndices = RecyclerNew(scriptContext->GetRecycler(), VarList, scriptContext->GetRecycler());
                    for (uint32 j = 0; j < length; j++)
                    {
                        Var element = typedArray->DirectGetItemNoDetachCheck(j);
                        iteratorIndices->Add(element);
                    }
                }

                // The ArrayBuffer shouldn't have detached in above loop.
                Assert(!typedArray->IsDetachedBuffer());
            }
            else
            {
                Assert(false);
            }
        }
        else if (RecyclableObject::Is(iterator))
        {
            Var nextItem;
            while (JavascriptOperators::IteratorStepAndValue(RecyclableObject::FromVar(iterator), scriptContext, &nextItem))
            {
                if (iteratorIndices == nullptr)
                {
                    iteratorIndices = RecyclerNew(scriptContext->GetRecycler(), VarList, scriptContext->GetRecycler());
                }

                iteratorIndices->Add(nextItem);
            }
        }
        else
        {
            Assert(false);
        }
    }
} // namespace Js
