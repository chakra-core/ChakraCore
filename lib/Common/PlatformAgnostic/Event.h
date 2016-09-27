//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef RUNTIME_PLATFORM_AGNOSTIC_COMMON_EVENT
#define RUNTIME_PLATFORM_AGNOSTIC_COMMON_EVENT

namespace PlatformAgnostic
{
class Event
{
public:
    typedef HANDLE EventHandle;

private:
    EventHandle handle;

    Event(const Event &) : handle(0)
    {
    }

public:
    Event(const bool autoReset, const bool signaled = false);

    EventHandle Handle() const
    {
        return handle;
    }

    operator bool() const
    {
        return handle != nullptr;
    }

    ~Event();

    void Close() const;

    void Set() const;

    void Reset() const;

    bool Wait(const unsigned int milliseconds = INFINITE) const;
};
} // namespace PlatformAgnostic

#endif // RUNTIME_PLATFORM_AGNOSTIC_COMMON_EVENT

