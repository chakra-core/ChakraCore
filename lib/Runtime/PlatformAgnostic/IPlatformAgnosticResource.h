//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace PlatformAgnostic
{
namespace Resource
{
    // Interface
    class IPlatformAgnosticResource
    {
    public:
        virtual void Cleanup() = 0;
    };

    template <typename T>
    class AutoPtr
    {
    private:
        const T *pointer;
    public:
        AutoPtr(const T *ptr) : pointer(ptr) {}
        ~AutoPtr()
        {
            if (pointer != nullptr)
            {
                delete pointer;
                pointer = nullptr;
            }
        }

        void setPointer(const T *ptr)
        {
            pointer = ptr;
        }
    };

    template <typename T>
    class StringBufferAutoPtr
    {
    private:
        const T *pointer;
    public:
        StringBufferAutoPtr(const T *ptr) : pointer(ptr) {}
        ~StringBufferAutoPtr()
        {
            if (pointer != nullptr)
            {
                delete[] pointer; // note we're using delete[] so we handle the whole buffer
                pointer = nullptr;
            }
        }

        void setPointer(const T *ptr)
        {
            pointer = ptr;
        }
    };

#ifndef _WIN32
    typedef char16_t char16;
#endif
    template class StringBufferAutoPtr<char16>;
    template class StringBufferAutoPtr<unsigned char>;
} // namespace Intl
} // namespace PlatformAgnostic
