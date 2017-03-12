//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace JsUtil
{
    template<class T, class TAllocator = ArenaAllocator>
    class DoublyLinkedListElement
    {
    private:
        Field(T*, TAllocator) previous;
        Field(T*, TAllocator) next;

    public:
        DoublyLinkedListElement();

    public:
        T *Previous() const;
        T *Next() const;

        template<class D> static bool Contains(D *const element,
            Field(D *, TAllocator) const head);
        template<class D> static bool ContainsSubsequence(D *const first, D *const last,
            Field(D *, TAllocator) const head);

        template<class D> static void LinkToBeginning(D *const element,
            Field(D *, TAllocator) *const head, Field(D *, TAllocator) *const tail);
        template<class D> static void LinkToEnd(D *const element,
            Field(D *, TAllocator) *const head, Field(D *, TAllocator) *const tail);
        template<class D> static void LinkBefore(D *const element, D *const nextElement,
            Field(D *, TAllocator) *const head, Field(D *, TAllocator) *const tail);
        template<class D> static void LinkAfter(D *const element, D *const previousElement,
            Field(D *, TAllocator) *const head, Field(D *, TAllocator) *const tail);
        template<class D> static void UnlinkFromBeginning(D *const element,
            Field(D *, TAllocator) *const head, Field(D *, TAllocator) *const tail);
        template<class D> static void UnlinkFromEnd(D *const element,
            Field(D *, TAllocator) *const head, Field(D *, TAllocator) *const tail);
        template<class D> static void UnlinkPartial(D *const element,
            Field(D *, TAllocator) *const head, Field(D *, TAllocator) *const tail);
        template<class D> static void Unlink(D *const element,
            Field(D *, TAllocator) *const head, Field(D *, TAllocator) *const tail);
        template<class D> static void MoveToBeginning(D *const element,
            Field(D *, TAllocator) *const head, Field(D *, TAllocator) *const tail);

        template<class D> static void UnlinkSubsequenceFromEnd(D *const first,
            Field(D *, TAllocator) *const head, Field(D *, TAllocator) *const tail);
        template<class D> static void UnlinkSubsequence(D *const first, D *const last,
            Field(D *, TAllocator) *const head, Field(D *, TAllocator) *const tail);
        template<class D> static void MoveSubsequenceToBeginning(D *const first, D *const last,
            Field(D *, TAllocator) *const head, Field(D *, TAllocator) *const tail);

        // JScriptDiag doesn't seem to like the PREVENT_COPY macro
    private:
        DoublyLinkedListElement(const DoublyLinkedListElement &other);
        DoublyLinkedListElement &operator =(const DoublyLinkedListElement &other);
    };
}
