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
        T::LinkToBeginning(element, AddressOf(head), AddressOf(tail));
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::LinkToEnd(T *const element)
    {
        T::LinkToEnd(element, AddressOf(head), AddressOf(tail));
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::LinkBefore(T *const element, T *const nextElement)
    {
        T::LinkBefore(element, nextElement, AddressOf(head), AddressOf(tail));
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::LinkAfter(T *const element, T *const previousElement)
    {
        T::LinkAfter(element, previousElement, AddressOf(head), AddressOf(tail));
    }

    template<class T, class TAllocator>
    T *DoublyLinkedList<T, TAllocator>::UnlinkFromBeginning()
    {
        T *const element = head;
        if(element)
            T::UnlinkFromBeginning(element, AddressOf(head), AddressOf(tail));
        return element;
    }

    template<class T, class TAllocator>
    T *DoublyLinkedList<T, TAllocator>::UnlinkFromEnd()
    {
        T *const element = tail;
        if(element)
            T::UnlinkFromEnd(element, AddressOf(head), AddressOf(tail));
        return element;
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::UnlinkPartial(T *const element)
    {
        T::UnlinkPartial(element, AddressOf(head), AddressOf(tail));
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::Unlink(T *const element)
    {
        T::Unlink(element, AddressOf(head), AddressOf(tail));
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::MoveToBeginning(T *const element)
    {
        T::MoveToBeginning(element, AddressOf(head), AddressOf(tail));
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::UnlinkSubsequenceFromEnd(T *const first)
    {
        T::UnlinkSubsequenceFromEnd(first, AddressOf(head), AddressOf(tail));
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::UnlinkSubsequence(T *const first, T *const last)
    {
        T::UnlinkSubsequence(first, last, AddressOf(head), AddressOf(tail));
    }

    template<class T, class TAllocator>
    void DoublyLinkedList<T, TAllocator>::MoveSubsequenceToBeginning(T *const first, T *const last)
    {
        T::MoveSubsequenceToBeginning(first, last, AddressOf(head), AddressOf(tail));
    }
}
