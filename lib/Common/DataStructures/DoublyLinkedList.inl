//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace JsUtil
{
    template<class T, class TAllocator>
    DoublyLinkedList<T, TAllocator>::DoublyLinkedList() : head(nullptr), tail(nullptr)
    {
    }

    template<class T, class TAllocator>
    T *DoublyLinkedList<T, TAllocator>::Head() const
    {
        return head;
    }

    template<class T, class TAllocator>
    T *DoublyLinkedList<T, TAllocator>::Tail() const
    {
        return tail;
    }

    template<class T, class TAllocator>
    bool DoublyLinkedList<T, TAllocator>::Contains(T *const element) const
    {
        return T::Contains(element, head);
    }

    template<class T, class TAllocator>
    bool DoublyLinkedList<T, TAllocator>::ContainsSubsequence(T *const first, T *const last) const
    {
        return T::ContainsSubsequence(first, last, head);
    }

    template<class T, class TAllocator>
    bool DoublyLinkedList<T, TAllocator>::IsEmpty()
    {
        return head == nullptr;
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::Clear()
    {
        tail = head = nullptr;
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::LinkToBeginning(T *const element)
    {
        T::LinkToBeginning(element, &head, &tail);
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::LinkToEnd(T *const element)
    {
        T::LinkToEnd(element, &head, &tail);
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::LinkBefore(T *const element, T *const nextElement)
    {
        T::LinkBefore(element, nextElement, &head, &tail);
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::LinkAfter(T *const element, T *const previousElement)
    {
        T::LinkAfter(element, previousElement, &head, &tail);
    }

    template<class T, class TAllocator>
    T *DoublyLinkedList<T, TAllocator>::UnlinkFromBeginning()
    {
        T *const element = head;
        if(element)
            T::UnlinkFromBeginning(element, &head, &tail);
        return element;
    }

    template<class T, class TAllocator>
    T *DoublyLinkedList<T, TAllocator>::UnlinkFromEnd()
    {
        T *const element = tail;
        if(element)
            T::UnlinkFromEnd(element, &head, &tail);
        return element;
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::UnlinkPartial(T *const element)
    {
        T::UnlinkPartial(element, &head, &tail);
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::Unlink(T *const element)
    {
        T::Unlink(element, &head, &tail);
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::MoveToBeginning(T *const element)
    {
        T::MoveToBeginning(element, &head, &tail);
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::UnlinkSubsequenceFromEnd(T *const first)
    {
        T::UnlinkSubsequenceFromEnd(first, &head, &tail);
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::UnlinkSubsequence(T *const first, T *const last)
    {
        T::UnlinkSubsequence(first, last, &head, &tail);
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::MoveSubsequenceToBeginning(T *const first, T *const last)
    {
        T::MoveSubsequenceToBeginning(first, last, &head, &tail);
    }
}
