//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

// TODO, this should be elsewhere
#define RecyclerNewImplicitRoot(recycler,T,...) AllocatorNewBase(Recycler, recycler, NoThrowAllocImplicitRoot, T, __VA_ARGS__)

// Forward reference
class RecyclerTestObject;

// A "Location" represents a location in memory where object references (ReccylerTestObject *s) are stored.
// It includes a "type" that defines how the reference should be read or written.
// This allows us to specify special handling for certain location types,
// such as pinned roots, software write barrier locations, tagged locations for tracked objects, etc.

class Location
{
private:
    enum class Type
    {
        Scanned,        // For regular locations in scanned objects (or stack locations)
        Rooted,         // This is a non-heap location that's explicitly rooted.
        Barrier,        // Write barrier location
        Tagged,         // Tagged location (used in tracked objects)
        ImplicitRoot,   // Uses ImplicitRootBit to hold reference
    };

    class ImplicitRootHolder
    {
    private:
        Field(RecyclerTestObject *) reference;

    public:
        ImplicitRootHolder(RecyclerTestObject * reference)
            : reference(reference)
        {
            VerifyCondition(reference != nullptr);
        }

        RecyclerTestObject * GetReference() const
        {
            VerifyCondition(this->reference != nullptr);
            return this->reference;
        }
    };

    Location(RecyclerTestObject ** location, Type type) :
        location(location), type(type)
    {
        VerifyCondition(location != nullptr);
    }

public:
    static Location Scanned(RecyclerTestObject ** location)
    {
        return Location(location, Type::Scanned);
    }

    static Location Rooted(RecyclerTestObject ** location)
    {
        return Location(location, Type::Rooted);
    }

    static Location Barrier(RecyclerTestObject ** location)
    {
        return Location(location, Type::Barrier);
    }

    static Location Tagged(RecyclerTestObject ** location)
    {
        return Location(location, Type::Tagged);
    }

    static Location ImplicitRoot(RecyclerTestObject ** location)
    {
        return Location(location, Type::ImplicitRoot);
    }

    RecyclerTestObject * Get()
    {
        switch (type)
        {
            case Type::Scanned:
                return *location;

            case Type::Rooted:
                return *location;

            case Type::Barrier:
                return *location;

            case Type::Tagged:
                return Untag(*location);

            case Type::ImplicitRoot:
                // The stored location actually contains a pointer to the ImplicitRootHolder
                ImplicitRootHolder * holder = (ImplicitRootHolder *)(*location);
                if (holder == nullptr)
                {
                    return nullptr;
                }

                return holder->GetReference();
        }

        // Shouldn't get here
        VerifyCondition(false);
        return nullptr;
    }

    void Set(RecyclerTestObject * value)
    {
        switch (type)
        {
            case Type::Scanned:
                *location = value;
                return;

            case Type::Rooted:
                Unroot();
                *location = value;
                Root();
                return;

            case Type::Barrier:
                *location = value;
                if (value != nullptr)
                {
                    RecyclerWriteBarrierManager::WriteBarrier(location);
                }
                return;

            case Type::Tagged:
                *location = Tag(value);
                return;

            case Type::ImplicitRoot:
                // The stored location actually contains a pointer to the ImplicitRootHolder
                ImplicitRootHolder * holder = (ImplicitRootHolder *)(*location);
                if (holder != nullptr)
                {
                    // Free the implicit root
                    // TODO: This should be better encapsulated
                    RecyclerHeapObjectInfo heapObject;
                    bool found = recyclerInstance->FindHeapObject(holder, FindHeapObjectFlags_VerifyFreeBitForAttribute, heapObject);
                    VerifyCondition(found);

                    // Zero pointers in order to eliminate false-positives
                    memset(holder, 0, heapObject.GetSize());

                    bool success = heapObject.ClearImplicitRootBit();
                    VerifyCondition(success);

                    *location = nullptr;
                }

                if (value != nullptr)
                {
                    // Create new holder object
                    holder = RecyclerNewImplicitRoot(recyclerInstance, ImplicitRootHolder, value);
                    VerifyCondition(holder != nullptr);

                    // Store holder back to the location
                    *location = (RecyclerTestObject *)holder;
                }

                return;
        }

        // Shouldn't get here
        VerifyCondition(false);
    }

    static RecyclerTestObject * Tag(RecyclerTestObject * untagged)
    {
        if (untagged == nullptr)
        {
            return nullptr;
        }
        else
        {
            size_t value = (size_t)untagged;
            VerifyCondition((value & 0x1) == 0);
            value |= 0x1;
            return (RecyclerTestObject *)value;
        }
    }

    static RecyclerTestObject * Untag(RecyclerTestObject * tagged)
    {
        if (tagged == nullptr)
        {
            return nullptr;
        }
        else
        {
            size_t value = (size_t)tagged;
            VerifyCondition((value & 0x1) == 0x1);
            value &= ~(0x1);
            return (RecyclerTestObject *)value;
        }
    }

private:
    void Unroot()
    {
        if (*location != nullptr)
        {
            recyclerInstance->RootRelease(*location);
        }
    }

    void Root()
    {
        if (*location != nullptr)
        {
            recyclerInstance->RootAddRef(*location);
        }
    }

private:
    RecyclerTestObject ** location;
    Type type;
};



