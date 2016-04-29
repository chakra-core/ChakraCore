//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

// -----------------------------------------------------------------------------------------------------------------------------
// ContinuousPageStack declaration
// -----------------------------------------------------------------------------------------------------------------------------

template<const size_t InitialPageCount = 1>
class ContinuousPageStack : protected Allocator
{
protected:
    class Iterator
    {
    protected:
        size_t nextTop;

    protected:
        inline Iterator(const ContinuousPageStack &stack);

    public:
        inline size_t Position() const;
        inline operator bool() const;
    };

    friend class Iterator;

private:
    PageAllocator *const pageAllocator;
    PageAllocation *pageAllocation;
    size_t bufferSize;

protected:
    size_t nextTop;

protected:
    inline ContinuousPageStack(PageAllocator *const pageAllocator, void (*const outOfMemoryFunc)());
    ~ContinuousPageStack();

    inline char *Buffer() const;

public:
    inline bool IsEmpty() const;
    inline void Clear();

    inline size_t Position() const;

    inline char* Push(const size_t size);
    inline char* Top(const size_t size) const;
    inline char* Pop(const size_t size);
    inline void UnPop(const size_t size);
    inline void PopTo(const size_t position);

private:
    void Resize(size_t requestedSize);
};

// -----------------------------------------------------------------------------------------------------------------------------
// ContinuousPageStackOfFixedElements declaration
// -----------------------------------------------------------------------------------------------------------------------------

template<class T, const size_t InitialPageCount = 1>
class ContinuousPageStackOfFixedElements : public ContinuousPageStack<InitialPageCount>
{
public:
    class Iterator : public ContinuousPageStack<InitialPageCount>::Iterator
    {
    private:
        const ContinuousPageStackOfFixedElements &stack;

    public:
        inline Iterator(const ContinuousPageStackOfFixedElements &stack);

        inline T &operator *() const;
        inline T *operator ->() const;
        inline Iterator &operator ++(); // pre-increment
        inline Iterator operator ++(int); // post-increment
    };

    friend class Iterator;

public:
    inline ContinuousPageStackOfFixedElements(PageAllocator *const pageAllocator, void (*const outOfMemoryFunc)());

public:
    inline char* Push();
    inline T* Top() const;
    inline T* Pop();
    inline void UnPop();
    inline void PopTo(const size_t position);
};

// -----------------------------------------------------------------------------------------------------------------------------
// ContinuousPageStackOfVariableElements declaration
// -----------------------------------------------------------------------------------------------------------------------------

template<class T, const size_t InitialPageCount = 1>
class ContinuousPageStackOfVariableElements : public ContinuousPageStack<InitialPageCount>
{
public:
    class Iterator : public ContinuousPageStack<InitialPageCount>::Iterator
    {
    private:
        size_t topElementSize;
        const ContinuousPageStackOfVariableElements &stack;

    public:
        inline Iterator(const ContinuousPageStackOfVariableElements &stack);

        inline T &operator *() const;
        inline T *operator ->() const;
        inline Iterator &operator ++(); // pre-increment
        inline Iterator operator ++(int); // post-increment
    };

    friend class Iterator;

private:
    class VariableElement
    {
    private:
        const size_t previousElementSize;
        char data[0];

    public:
        inline VariableElement(const size_t previousElementSize);

    public:
        template<class ActualT> inline static size_t Size();
        inline size_t PreviousElementSize() const;
        inline char *Data();
    };

private:
    size_t topElementSize;

public:
    inline ContinuousPageStackOfVariableElements(PageAllocator *const pageAllocator, void (*const outOfMemoryFunc)());

public:
    template<class ActualT> inline char* Push();
    inline T* Top() const;
    inline T* Pop();
    template<class ActualT> inline void UnPop();
    inline void PopTo(const size_t position);
};

// -----------------------------------------------------------------------------------------------------------------------------
// ContinuousPageStack definition
// -----------------------------------------------------------------------------------------------------------------------------

// --------
// Iterator
// --------

template<const size_t InitialPageCount>
inline ContinuousPageStack<InitialPageCount>::Iterator::Iterator(const ContinuousPageStack &stack)
    : nextTop(stack.nextTop)
{
}

template<const size_t InitialPageCount>
inline size_t ContinuousPageStack<InitialPageCount>::Iterator::Position() const
{
    return nextTop;
}

template<const size_t InitialPageCount>
inline ContinuousPageStack<InitialPageCount>::Iterator::operator bool() const
{
    return nextTop != 0;
}

// -------------------
// ContinuousPageStack
// -------------------

template<const size_t InitialPageCount>
inline ContinuousPageStack<InitialPageCount>::ContinuousPageStack(
    PageAllocator *const pageAllocator,
    void (*const outOfMemoryFunc)())
    : Allocator(outOfMemoryFunc), pageAllocator(pageAllocator), bufferSize(0), nextTop(0)
{
    Assert(pageAllocator);
}

template<const size_t InitialPageCount>
ContinuousPageStack<InitialPageCount>::~ContinuousPageStack()
{
    if(bufferSize && !pageAllocator->IsClosed())
        pageAllocator->ReleaseAllocation(pageAllocation);
}

template<const size_t InitialPageCount>
inline char *ContinuousPageStack<InitialPageCount>::Buffer() const
{
    Assert(bufferSize);
    return pageAllocation->GetAddress();
}

template<const size_t InitialPageCount>
inline bool ContinuousPageStack<InitialPageCount>::IsEmpty() const
{
    return nextTop == 0;
}

template<const size_t InitialPageCount>
inline void ContinuousPageStack<InitialPageCount>::Clear()
{
    nextTop = 0;
}

template<const size_t InitialPageCount>
inline size_t ContinuousPageStack<InitialPageCount>::Position() const
{
    return nextTop;
}

template<const size_t InitialPageCount>
inline char* ContinuousPageStack<InitialPageCount>::Push(const size_t size)
{
    Assert(size);
    if(bufferSize - nextTop < size)
        Resize(size);
    char* const res = Buffer() + nextTop;
    nextTop += size;
    return res;
}

template<const size_t InitialPageCount>
inline char* ContinuousPageStack<InitialPageCount>::Top(const size_t size) const
{
    if (nextTop == 0)
        return 0;
    else
    {
        Assert(size != 0);
        Assert(size <= nextTop);
        return Buffer() + (nextTop - size);
    }
}

template<const size_t InitialPageCount>
inline char* ContinuousPageStack<InitialPageCount>::Pop(const size_t size)
{
    if (nextTop == 0)
        return 0;
    else
    {
        Assert(size != 0);
        Assert(nextTop >= size);
        nextTop -= size;
        return Buffer() + nextTop;
    }
}

template<const size_t InitialPageCount>
inline void ContinuousPageStack<InitialPageCount>::UnPop(const size_t size)
{
    Assert(size != 0);
    Assert(nextTop + size <= bufferSize);
    nextTop += size;
}

template<const size_t InitialPageCount>
void ContinuousPageStack<InitialPageCount>::Resize(size_t requestedSize)
{
    Assert(requestedSize);
    Assert(requestedSize <= InitialPageCount * AutoSystemInfo::PageSize);

    if(!bufferSize)
    {
        pageAllocation = pageAllocator->AllocAllocation(InitialPageCount);
        if (!pageAllocation)
        {
            outOfMemoryFunc();
            AnalysisAssert(false);
        }
        bufferSize = pageAllocation->GetSize();
        return;
    }

    PageAllocation *const newPageAllocation = pageAllocator->AllocAllocation(pageAllocation->GetPageCount() * 2);
    if (!newPageAllocation)
    {
        outOfMemoryFunc();
        AnalysisAssert(false);
    }
    js_memcpy_s(newPageAllocation->GetAddress(), newPageAllocation->GetSize(), Buffer(), nextTop);
    pageAllocator->ReleaseAllocation(pageAllocation);
    pageAllocation = newPageAllocation;
    bufferSize = newPageAllocation->GetSize();
}

template<const size_t InitialPageCount>
inline void ContinuousPageStack<InitialPageCount>::PopTo(const size_t position)
{
    Assert(position <= nextTop);
    nextTop = position;
}

// -----------------------------------------------------------------------------------------------------------------------------
// ContinuousPageStackOfFixedElements definition
// -----------------------------------------------------------------------------------------------------------------------------

// --------
// Iterator
// --------

template<class T, const size_t InitialPageCount>
inline ContinuousPageStackOfFixedElements<T, InitialPageCount>::Iterator::Iterator(
    const ContinuousPageStackOfFixedElements &stack)
    : ContinuousPageStack<InitialPageCount>::Iterator(stack), stack(stack)
{
}

template<class T, const size_t InitialPageCount>
inline T &ContinuousPageStackOfFixedElements<T, InitialPageCount>::Iterator::operator *() const
{
    Assert(*this);
    Assert(this->nextTop <= stack.nextTop);
    return *reinterpret_cast<T *>(&stack.Buffer()[this->nextTop - sizeof(T)]);
}

template<class T, const size_t InitialPageCount>
inline T *ContinuousPageStackOfFixedElements<T, InitialPageCount>::Iterator::operator ->() const
{
    return &**this;
}

template<class T, const size_t InitialPageCount>
inline typename ContinuousPageStackOfFixedElements<T, InitialPageCount>::Iterator &ContinuousPageStackOfFixedElements<T, InitialPageCount>::Iterator::operator ++() // pre-increment
{
    Assert(*this);
    this->nextTop -= sizeof(T);
    return *this;
}

template<class T, const size_t InitialPageCount>
inline typename ContinuousPageStackOfFixedElements<T, InitialPageCount>::Iterator ContinuousPageStackOfFixedElements<T, InitialPageCount>::Iterator::operator ++(int) // post-increment
{
    Iterator it(*this);
    ++*this;
    return it;
}

// ----------------------------------
// ContinuousPageStackOfFixedElements
// ----------------------------------

template<class T, const size_t InitialPageCount>
inline ContinuousPageStackOfFixedElements<T, InitialPageCount>::ContinuousPageStackOfFixedElements(
    PageAllocator *const pageAllocator,
    void (*const outOfMemoryFunc)())
    : ContinuousPageStack<InitialPageCount>(pageAllocator, outOfMemoryFunc)
{
}

template<class T, const size_t InitialPageCount>
inline char* ContinuousPageStackOfFixedElements<T, InitialPageCount>::Push()
{
    return ContinuousPageStack<InitialPageCount>::Push(sizeof(T));
}

template<class T, const size_t InitialPageCount>
inline T* ContinuousPageStackOfFixedElements<T, InitialPageCount>::Top() const
{
    return reinterpret_cast<T*>(ContinuousPageStack<InitialPageCount>::Top(sizeof(T)));
}

template<class T, const size_t InitialPageCount>
inline T* ContinuousPageStackOfFixedElements<T, InitialPageCount>::Pop()
{
    return reinterpret_cast<T*>(ContinuousPageStack<InitialPageCount>::Pop(sizeof(T)));
}

template<class T, const size_t InitialPageCount>
inline void ContinuousPageStackOfFixedElements<T, InitialPageCount>::UnPop()
{
    return ContinuousPageStack<InitialPageCount>::UnPop(sizeof(T));
}

template<class T, const size_t InitialPageCount>
inline void ContinuousPageStackOfFixedElements<T, InitialPageCount>::PopTo(const size_t position)
{
    ContinuousPageStack<InitialPageCount>::PopTo(position);
}

// -----------------------------------------------------------------------------------------------------------------------------
// ContinuousPageStackOfVariableElements definition
// -----------------------------------------------------------------------------------------------------------------------------

// --------
// Iterator
// --------

template<class T, const size_t InitialPageCount>
inline ContinuousPageStackOfVariableElements<T, InitialPageCount>::Iterator::Iterator(
    const ContinuousPageStackOfVariableElements &stack)
    : ContinuousPageStack<InitialPageCount>::Iterator(stack), topElementSize(stack.topElementSize), stack(stack)
{
}

template<class T, const size_t InitialPageCount>
inline T &ContinuousPageStackOfVariableElements<T, InitialPageCount>::Iterator::operator *() const
{
    Assert(*this);
    Assert(this->nextTop <= stack.nextTop);
    return *reinterpret_cast<T*>(reinterpret_cast<VariableElement *>(&stack.Buffer()[this->nextTop - this->topElementSize])->Data());
}

template<class T, const size_t InitialPageCount>
inline T *ContinuousPageStackOfVariableElements<T, InitialPageCount>::Iterator::operator ->() const
{
    return &**this;
}

template<class T, const size_t InitialPageCount>
inline typename ContinuousPageStackOfVariableElements<T, InitialPageCount>::Iterator &ContinuousPageStackOfVariableElements<T, InitialPageCount>::Iterator::operator ++() // pre-increment
{
    Assert(*this);
    Assert(this->nextTop <= stack.nextTop);
    topElementSize = reinterpret_cast<VariableElement *>(&stack.Buffer()[this->nextTop -= this->topElementSize])->PreviousElementSize();
    return *this;
}

template<class T, const size_t InitialPageCount>
inline typename ContinuousPageStackOfVariableElements<T, InitialPageCount>::Iterator ContinuousPageStackOfVariableElements<T, InitialPageCount>::Iterator::operator ++(int) // post-increment
{
    Iterator it(*this);
    ++*this;
    return it;
}

// ---------------
// VariableElement
// ---------------

template<class T, const size_t InitialPageCount>
inline ContinuousPageStackOfVariableElements<T, InitialPageCount>::VariableElement::VariableElement(
    const size_t previousElementSize)
    : previousElementSize(previousElementSize)
{
}

template<class T, const size_t InitialPageCount>
template<class ActualT>
inline size_t ContinuousPageStackOfVariableElements<T, InitialPageCount>::VariableElement::Size()
{
    return sizeof(VariableElement) + sizeof(ActualT);
}

template<class T, const size_t InitialPageCount>
inline size_t ContinuousPageStackOfVariableElements<T, InitialPageCount>::VariableElement::PreviousElementSize() const
{
    return previousElementSize;
}

template<class T, const size_t InitialPageCount>
inline char* ContinuousPageStackOfVariableElements<T, InitialPageCount>::VariableElement::Data()
{
    return data;
}

// -------------------------------------
// ContinuousPageStackOfVariableElements
// -------------------------------------

template<class T, const size_t InitialPageCount>
inline ContinuousPageStackOfVariableElements<T, InitialPageCount>::ContinuousPageStackOfVariableElements(
    PageAllocator *const pageAllocator,
    void (*const outOfMemoryFunc)())
    : ContinuousPageStack<InitialPageCount>(pageAllocator, outOfMemoryFunc)
{
}

template<class T, const size_t InitialPageCount>
template<class ActualT>
inline char* ContinuousPageStackOfVariableElements<T, InitialPageCount>::Push()
{
    TemplateParameter::SameOrDerivedFrom<ActualT, T>(); // ActualT must be the same type as, or a type derived from, T
    VariableElement *const element =
        new(ContinuousPageStack<InitialPageCount>::Push(VariableElement::template
        Size<ActualT>())) VariableElement(topElementSize);
    topElementSize = VariableElement::template Size<ActualT>();
    return element->Data();
}

template<class T, const size_t InitialPageCount>
inline T* ContinuousPageStackOfVariableElements<T, InitialPageCount>::Top() const
{
    VariableElement* const element = reinterpret_cast<VariableElement*>(ContinuousPageStack<InitialPageCount>::Top(topElementSize));
    return element == 0 ? 0 : reinterpret_cast<T*>(element->Data());
}

template<class T, const size_t InitialPageCount>
inline T* ContinuousPageStackOfVariableElements<T, InitialPageCount>::Pop()
{
    VariableElement *const element = reinterpret_cast<VariableElement*>(ContinuousPageStack<InitialPageCount>::Pop(topElementSize));
    if (element == 0)
        return 0;
    else
    {
        topElementSize = element->PreviousElementSize();
        return reinterpret_cast<T*>(element->Data());
    }
}

template<class T, const size_t InitialPageCount>
template<class ActualT>
inline void ContinuousPageStackOfVariableElements<T, InitialPageCount>::UnPop()
{
    TemplateParameter::SameOrDerivedFrom<ActualT, T>(); // ActualT must be the same type as, or a type derived from, T
    ContinuousPageStack<InitialPageCount>::UnPop(VariableElement::template
    Size<ActualT>());
    Assert(reinterpret_cast<VariableElement*>(ContinuousPageStack<InitialPageCount>::Top(VariableElement::template
    Size<ActualT>()))->PreviousElementSize() == topElementSize);
    topElementSize = VariableElement::template Size<ActualT>();
}

template<class T, const size_t InitialPageCount>
inline void ContinuousPageStackOfVariableElements<T, InitialPageCount>::PopTo(const size_t position)
{
    Assert(position <= this->nextTop);
    if(position != this->nextTop)
    {
        Assert(position + sizeof(VariableElement) <= this->nextTop);
        topElementSize = reinterpret_cast<VariableElement *>(&this->Buffer()[position])->PreviousElementSize();
    }
    ContinuousPageStack<InitialPageCount>::PopTo(position);
}
