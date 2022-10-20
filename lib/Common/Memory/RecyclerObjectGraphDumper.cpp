//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "CommonMemoryPch.h"

#ifdef RECYCLER_DUMP_OBJECT_GRAPH

RecyclerObjectGraphDumper::RecyclerObjectGraphDumper(Recycler * recycler, RecyclerObjectGraphDumper::Param * param) :
    recycler(recycler),
    param(param),
    dumpObjectName(nullptr),
    dumpObject(nullptr),
    isOutOfMemory(false)
#ifdef PROFILE_RECYCLER_ALLOC
    , dumpObjectTypeInfo(nullptr)
#endif
{
    recycler->objectGraphDumper = this;
}

RecyclerObjectGraphDumper::~RecyclerObjectGraphDumper()
{
    recycler->objectGraphDumper = nullptr;
}

void RecyclerObjectGraphDumper::BeginDumpObject(char16 const * name)
{
    Assert(dumpObjectName == nullptr);
    Assert(dumpObject == nullptr);
    dumpObjectName = name;
}

void RecyclerObjectGraphDumper::BeginDumpObject(char16 const * name, void * address)
{
    Assert(dumpObjectName == nullptr);
    Assert(dumpObject == nullptr);
    swprintf_s(tempObjectName, _countof(tempObjectName), _u("%s %p"), name, address);
    dumpObjectName = tempObjectName;
}

void RecyclerObjectGraphDumper::BeginDumpObject(void * objectAddress)
{
    Assert(dumpObjectName == nullptr);
    Assert(dumpObject == nullptr);
    this->dumpObject = objectAddress;
#ifdef PROFILE_RECYCLER_ALLOC
    if (recycler->trackerDictionary)
    {
        Recycler::TrackerData * trackerData = recycler->GetTrackerData(objectAddress);

        if (trackerData != nullptr)
        {
            this->dumpObjectTypeInfo = trackerData->typeinfo;
            this->dumpObjectIsArray = trackerData->isArray;
        }
        else
        {
            Assert(false);
            this->dumpObjectTypeInfo = nullptr;
            this->dumpObjectIsArray = false;
        }
    }
#endif
}

void RecyclerObjectGraphDumper::EndDumpObject()
{
    Assert(this->dumpObjectName != nullptr || this->dumpObject != nullptr);
    this->dumpObjectName = nullptr;
    this->dumpObject = nullptr;
}
void RecyclerObjectGraphDumper::DumpObjectReference(void * objectAddress, bool remark)
{
    if (this->param == nullptr || !this->param->dumpRootOnly || recycler->collectionState == CollectionStateFindRoots)
    {
        if (this->param != nullptr && this->param->dumpReferenceFunc)
        {
            if (!this->param->dumpReferenceFunc(this->dumpObjectName, this->dumpObject, objectAddress))
                return;
        }
        Output::Print(_u("\""));
        if (this->dumpObjectName)
        {
            Output::Print(_u("%s"), this->dumpObjectName);
        }
        else
        {
            Assert(this->dumpObject != nullptr);
#ifdef PROFILE_RECYCLER_ALLOC
            RecyclerObjectDumper::DumpObject(this->dumpObjectTypeInfo, this->dumpObjectIsArray, this->dumpObject);
#else
            Output::Print(_u("Address %p"), objectAddress);
#endif
        }

        Output::Print(remark? _u("\" => \"") : _u("\" -> \""));
        recycler->DumpObjectDescription(objectAddress);

        Output::Print(_u("\"\n"));
    }
}
#endif
