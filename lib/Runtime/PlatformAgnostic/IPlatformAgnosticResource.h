//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifndef RUNTIME_PLATFORM_AGNOSTIC_COMMON_IFINALIZABLE
#define RUNTIME_PLATFORM_AGNOSTIC_COMMON_IFINALIZABLE

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
        const T *strBuf;
    public:
        StringBufferAutoPtr(const T *buffer) : strBuf(buffer) {}
        ~StringBufferAutoPtr()
        {
            delete[] strBuf;
        }
    };

#ifndef _WIN32
    typedef char16_t char16;
#endif
    template class StringBufferAutoPtr<char16>;
    template class StringBufferAutoPtr<unsigned char>;
} // namespace Intl
} // namespace PlatformAgnostic

#endif /* RUNTIME_PLATFORM_AGNOSTIC_COMMON_IFINALIZABLE */
