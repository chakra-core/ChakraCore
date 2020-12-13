//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

template <typename T, typename HeapAllocatorT = HeapAllocator>
class AutoPtr : public BasePtr<T>
{
public:
    AutoPtr(T * ptr) : BasePtr<T>(ptr) {}
    ~AutoPtr()
    {
        Clear();
    }

    AutoPtr& operator=(T * ptr)
    {
        Clear();
        this->ptr = ptr;
        return *this;
    }

private:
    void Clear()
    {
        if (this->ptr != nullptr)
        {
            AllocatorDelete(HeapAllocatorT, &HeapAllocatorT::Instance, this->ptr);
            this->ptr = nullptr;
        }
    }
};

template <typename T>
class AutoArrayPtr : public BasePtr<T>
{
protected:
    size_t m_elementCount;
public:
    AutoArrayPtr(T * ptr, size_t elementCount) : BasePtr<T>(ptr), m_elementCount(elementCount) {}
    ~AutoArrayPtr()
    {
        Clear();
    }

    void Set(T* ptr, int elementCount)
    {
        Clear();
        this->ptr = ptr;
        this->m_elementCount = elementCount;
    }

private:
    void Clear()
    {
        if (this->ptr != nullptr)
        {
            HeapDeleteArray(m_elementCount, this->ptr);
            this->ptr = nullptr;
        }
    }
};

template <typename T>
class AutoArrayAndItemsPtr : public AutoArrayPtr<T>
{
public:
    AutoArrayAndItemsPtr(T * ptr, size_t elementCount) : AutoArrayPtr<T>(ptr, elementCount) {}

    ~AutoArrayAndItemsPtr()
    {
        Clear();
    }

private:
    void Clear()
    {
        if (this->ptr != nullptr){
            for (size_t i = 0; i < this->m_elementCount; i++)
            {
                if (this->ptr[i] != nullptr)
                {
                    this->ptr[i]->CleanUp();
                    this->ptr[i] = nullptr;
                }
            }

            HeapDeleteArray(this->m_elementCount, this->ptr);
            this->ptr = nullptr;
        }
    }
};

template <typename T>
class AutoReleasePtr : public BasePtr<T>
{
    using BasePtr<T>::ptr;
public:
    AutoReleasePtr(T * ptr = nullptr) : BasePtr<T>(ptr) {}
    ~AutoReleasePtr()
    {
        Release();
    }

    void Release()
    {
        if (ptr != nullptr)
        {
            ptr->Release();
            this->ptr = nullptr;
        }
    }
};

template <typename T>
class AutoCOMPtr : public AutoReleasePtr<T>
{
    using BasePtr<T>::ptr;
public:
    AutoCOMPtr(T * ptr = nullptr) : AutoReleasePtr<T>(ptr)
    {
        if (ptr != nullptr)
        {
            ptr->AddRef();
        }
    }

    AutoCOMPtr(const AutoCOMPtr<T>& other)
    {
        if ((ptr = other.ptr) != nullptr)
        {
            ptr->AddRef();
        }
    }

    template <class Q>
    HRESULT QueryInterface(Q** pp) const
    {
        return ptr->QueryInterface(__uuidof(Q), (void**)pp);
    }

    void Attach(T* p2)
    {
        if (ptr)
        {
            ptr->Release();
        }
        ptr = p2;
    }

    operator T*() const
    {
        return (T*)ptr;
    }

    T& operator*() const
    {
        Assert(ptr != nullptr);
        return *ptr;
    }

    T* operator=(T* lp)
    {
        return (T*)ComPtrAssign((IUnknown**)&ptr, lp);
    }

    T** operator&()
    {
        return &ptr;
    }

    void SetPtr(T* ptr)
    {
        this->ptr = ptr;
    }

private:
    static IUnknown* ComPtrAssign(IUnknown** pp, IUnknown* lp)
    {
        if (pp == nullptr)
        {
            return nullptr;
        }

        if (lp != nullptr)
        {
            lp->AddRef();
        }

        if (*pp)
        {
            (*pp)->Release();
        }
        
        *pp = lp;
        return lp;
    }
};

template <typename T>
class AutoDiscardPTR : public BasePtr<T>
{
public:
    AutoDiscardPTR(T * ptr) : BasePtr<T>(ptr) {}
    ~AutoDiscardPTR()
    {
        Clear();
    }

    AutoDiscardPTR& operator=(T * ptr)
    {
        Clear();
        this->ptr = ptr;
        return *this;
    }

private:
    void Clear()
    {
        if (this->ptr != nullptr)
        {
            this->ptr->Discard();
            this->ptr = nullptr;
        }
    }
};

template <typename T>
class AutoCoTaskMemFreePtr : public BasePtr<T>
{
public:
    AutoCoTaskMemFreePtr(T* ptr) : BasePtr<T>(ptr) {}
    ~AutoCoTaskMemFreePtr()
    {
        Clear();
    }

private:
    void Clear()
    {
        if (this->ptr != nullptr)
        {
            CoTaskMemFree(this->ptr);
            this->ptr = nullptr;
        }
    }
};
