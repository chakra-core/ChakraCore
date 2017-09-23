//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
//----------------------------------------------------------------------------
//
// File: SList.h
//
// Template for Singly Linked List
//
//----------------------------------------------------------------------------

class FakeCount
{
protected:
    void IncrementCount() {}
    void DecrementCount() {}
    void SetCount(uint count) {}
    void AddCount(FakeCount& c) {}
public:
    uint Count() const { return 0; }
};

class RealCount
{
protected:
    RealCount() : count(0) {}
    void IncrementCount() { count++; }
    void DecrementCount() { count--; }
    void SetCount(uint count) { this->count = count; }
    void AddCount(RealCount const& c) { this->count += c.Count(); }
public:
    uint Count() const { return count; }
private:
    Field(uint) count;
};

#if DBG
typedef RealCount DefaultCount;
#else
typedef FakeCount DefaultCount;
#endif

template <typename TData,
          typename TAllocator = ArenaAllocator,
          typename TCount = DefaultCount> class SListBase;

template <typename TAllocator>
class SListNodeBase
{
public:
    Field(SListNodeBase*, TAllocator) Next() const { return next; }
    Field(SListNodeBase*, TAllocator)& Next() { return next; }

protected:
    // The next node can be a real node with data, or it point back to the start of the list
    Field(SListNodeBase*, TAllocator) next;
};

template <typename TData, typename TAllocator>
class SListNode : public SListNodeBase<TAllocator>
{
    friend class SListBase<TData, TAllocator, FakeCount>;
    friend class SListBase<TData, TAllocator, RealCount>;
public:
    TData* GetData()
    {
        return &data;
    }
private:
    SListNode() : data() {}

    // Constructing with parameter
    template <typename TParam>
    SListNode(const TParam& param) : data(param) {}

    // Constructing with parameter
    template <typename TParam1, typename TParam2>
    SListNode(const TParam1& param1, const TParam2& param2) : data(param1, param2) {}

    Field(TData, TAllocator) data;
};

template<typename TData, typename TAllocator, typename TCount>
class SListBase : protected SListNodeBase<TAllocator>, public TCount
{
private:
    typedef SListNodeBase<TAllocator> NodeBase;
    typedef SListNode<TData, TAllocator> Node;

    bool IsHead(NodeBase const * node) const
    {
        return (node == this);
    }

public:
    class Iterator
    {
    public:
        Iterator() : list(nullptr), current(nullptr) {}
        Iterator(SListBase const * list) : list(list), current(list) {};

        bool IsValid() const
        {
            return (current != nullptr && !list->IsHead(current));
        }
        void Reset()
        {
            current = list;
        }

        // forceinline only needed for SListBase<FlowEdge *, RealCount>::Iterator::Next()
        __forceinline
        bool Next()
        {
            Assert(current != nullptr);
            if (list->IsHead(current->Next()))
            {
                current = nullptr;
                return false;
            }
            current = current->Next();
            return true;
        }
        Field(TData, TAllocator) const& Data() const
        {
            Assert(this->IsValid());
            return ((Node *)current)->data;
        }
        Field(TData, TAllocator)& Data()
        {
            Assert(this->IsValid());
            return ((Node *)current)->data;
        }
    protected:
        SListBase const * list;
        NodeBase const * current;
    };

    class EditingIterator : public Iterator
    {
    public:
        EditingIterator() : Iterator(), last(nullptr) {};
        EditingIterator(SListBase * list) : Iterator(list), last(nullptr) {};

        bool Next()
        {
            if (last != nullptr && last->Next() != this->current)
            {
                this->current = last;
            }
            else
            {
                last = this->current;
            }
            return Iterator::Next();
        }

        void UnlinkCurrent()
        {
            UnlinkCurrentNode();
        }

        void RemoveCurrent(TAllocator * allocator)
        {
            const NodeBase *dead = this->current;
            UnlinkCurrent();

            auto freeFunc = TypeAllocatorFunc<TAllocator, TData>::GetFreeFunc();

            AllocatorFree(allocator, freeFunc, (Node *) dead, sizeof(Node));
        }

        Field(TData, TAllocator) * InsertNodeBefore(TAllocator * allocator)
        {
            Assert(last != nullptr);
            Node * newNode = AllocatorNew(TAllocator, allocator, Node);
            if (newNode)
            {
                newNode->Next() = last->Next();
                const_cast<NodeBase *>(last)->Next() = newNode;
                const_cast<SListBase *>(this->list)->IncrementCount();
                last = newNode;
                return &newNode->data;
            }
            return nullptr;
        }

        Field(TData, TAllocator) * InsertNodeBeforeNoThrow(TAllocator * allocator)
        {
            Assert(last != nullptr);
            Node * newNode = AllocatorNewNoThrow(TAllocator, allocator, Node);
            if (newNode)
            {
                newNode->Next() = last->Next();
                const_cast<NodeBase *>(last)->Next() = newNode;
                const_cast<SListBase *>(this->list)->IncrementCount();
                last = newNode;
                return &newNode->data;
            }
            return nullptr;
        }

        template <typename TParam1, typename TParam2>
        Field(TData, TAllocator) * InsertNodeBefore(TAllocator * allocator, TParam1 param1, TParam2 param2)
        {
            Assert(last != nullptr);
            Node * newNode = AllocatorNew(TAllocator, allocator, Node, param1, param2);
            if (newNode)
            {
                newNode->Next() = last->Next();
                const_cast<NodeBase *>(last)->Next() = newNode;
                const_cast<SListBase *>(this->list)->IncrementCount();
                last = newNode;
                return &newNode->data;
            }
            return nullptr;
        }

        bool InsertBefore(TAllocator * allocator, TData const& data)
        {
            Assert(last != nullptr);
            Node * newNode = AllocatorNew(TAllocator, allocator, Node, data);
            if (newNode)
            {
                newNode->Next() = last->Next();
                const_cast<NodeBase *>(last)->Next() = newNode;
                const_cast<SListBase *>(this->list)->IncrementCount();
                last = newNode;
                return true;
            }
            return false;
        }

        void MoveCurrentTo(SListBase * toList)
        {
            NodeBase * node = UnlinkCurrentNode();
            node->Next() = toList->Next();
            toList->Next() = node;
            toList->IncrementCount();
        }

        void SetNext(SListBase * newNext)
        {
            Assert(last != nullptr);
            const_cast<NodeBase *>(last)->Next() = newNext;
        }

    private:
        NodeBase const * last;

        NodeBase * UnlinkCurrentNode()
        {
            NodeBase * unlinkedNode = const_cast<NodeBase *>(this->current);
            Assert(this->current != nullptr);
            Assert(!this->list->IsHead(this->current));
            Assert(last != nullptr);

            const_cast<NodeBase *>(last)->Next() = this->current->Next();
            this->current = last;
            last = nullptr;
            const_cast<SListBase *>(this->list)->DecrementCount();
            return unlinkedNode;
        }
    };

    inline Iterator GetIterator() const { return Iterator(this); }
    inline EditingIterator GetEditingIterator() { return EditingIterator(this); }

    explicit SListBase()
    {
        Reset();
    }

    ~SListBase()
    {
        AssertMsg(this->Empty(), "SListBase need to be cleared explicitly with an allocator");
    }

    void Reset()
    {
        this->Next() = this;
        this->SetCount(0);
    }

    __forceinline
    void Clear(TAllocator * allocator)
    {
        NodeBase * current = this->Next();
        while (!this->IsHead(current))
        {
            NodeBase * next = current->Next();

            auto freeFunc = TypeAllocatorFunc<TAllocator, TData>::GetFreeFunc();

            AllocatorFree(allocator, freeFunc, (Node *)current, sizeof(Node));
            current = next;
        }

        this->Reset();
    }

    bool Empty() const { return this->IsHead(this->Next()); }
    bool HasOne() const { return !Empty() && this->IsHead(this->Next()->Next()); }
    bool HasTwo() const { return !Empty() && this->IsHead(this->Next()->Next()->Next()); }
    Field(TData, TAllocator) const& Head() const
        { Assert(!Empty()); return ((Node *)this->Next())->data; }
    Field(TData, TAllocator)& Head()
        { Assert(!Empty()); return ((Node *)this->Next())->data; }

    bool Prepend(TAllocator * allocator, TData const& data)
    {
        Node * newNode = AllocatorNew(TAllocator, allocator, Node, data);
        if (newNode)
        {
            newNode->Next() = this->Next();
            this->Next() = newNode;
            this->IncrementCount();
            return true;
        }
        return false;
    }

    bool PrependNoThrow(TAllocator * allocator, TData const& data)
    {
        Node * newNode = AllocatorNewNoThrow(TAllocator, allocator, Node, data);
        if (newNode)
        {
            newNode->Next() = this->Next();
            this->Next() = newNode;
            this->IncrementCount();
            return true;
        }
        return false;
    }

    Field(TData, TAllocator) * PrependNode(TAllocator * allocator)
    {
        Node * newNode = AllocatorNew(TAllocator, allocator, Node);
        if (newNode)
        {
            newNode->Next() = this->Next();
            this->Next() = newNode;
            this->IncrementCount();
            return &newNode->data;
        }
        return nullptr;
    }

    template <typename TParam>
    Field(TData, TAllocator) * PrependNode(TAllocator * allocator, TParam param)
    {
        Node * newNode = AllocatorNew(TAllocator, allocator, Node, param);
        if (newNode)
        {
            newNode->Next() = this->Next();
            this->Next() = newNode;
            this->IncrementCount();
            return &newNode->data;
        }
        return nullptr;
    }

    template <typename TParam1, typename TParam2>
    Field(TData, TAllocator) * PrependNode(TAllocator * allocator, TParam1 param1, TParam2 param2)
    {
        Node * newNode = AllocatorNew(TAllocator, allocator, Node, param1, param2);
        if (newNode)
        {
            newNode->Next() = this->Next();
            this->Next() = newNode;
            this->IncrementCount();
            return &newNode->data;
        }
        return nullptr;
    }

    void RemoveHead(TAllocator * allocator)
    {
        Assert(!this->Empty());

        NodeBase * node = this->Next();
        this->Next() = node->Next();

        auto freeFunc = TypeAllocatorFunc<TAllocator, TData>::GetFreeFunc();
        AllocatorFree(allocator, freeFunc, (Node *) node, sizeof(Node));
        this->DecrementCount();
    }

    bool Remove(TAllocator * allocator, TData const& data)
    {
        EditingIterator iter(this);
        while (iter.Next())
        {
            if (iter.Data() == data)
            {
                iter.RemoveCurrent(allocator);
                return true;
            }
        }
        return false;
    }

    bool Has(TData data) const
    {
        Iterator iter(this);
        while (iter.Next())
        {
            if (iter.Data() == data)
            {
                return true;
            }
        }
        return false;
    }

    void MoveTo(SListBase * list)
    {
        while (!Empty())
        {
            this->MoveHeadTo(list);
        }
    }

    void MoveHeadTo(SListBase * list)
    {
        Assert(!this->Empty());
        NodeBase * node = this->Next();
        this->Next() = node->Next();
        node->Next() = list->Next();
        list->Next() = node;

        list->IncrementCount();
        this->DecrementCount();
    }

    // Moves the first element that satisfies the predicate to the toList
    template<class Fn>
    Field(TData, TAllocator)* MoveTo(SListBase* toList, Fn predicate)
    {
        Assert(this != toList);

        EditingIterator iter(this);
        while (iter.Next())
        {
            if (predicate(iter.Data()))
            {
                Field(TData, TAllocator)* data = &iter.Data();
                iter.MoveCurrentTo(toList);
                return data;
            }
        }
        return nullptr;
    }

    template<class Fn>
    Field(TData, TAllocator)* Find(Fn predicate)
    {
        Iterator iter(this);
        while(iter.Next())
        {
            if(predicate(iter.Data()))
            {
                return &iter.Data();
            }
        }
        return nullptr;
    }

    template<class Fn>
    void Iterate(Fn fn)
    {
        Iterator iter(this);
        while(iter.Next())
        {
            fn(iter.Data());
        }
    }

    void Reverse()
    {
        NodeBase * prev = this;
        NodeBase * current = this->Next();
        while (!this->IsHead(current))
        {
            NodeBase * next = current->Next();
            current->Next() = prev;
            prev = current;
            current = next;
        }
        current->Next() = prev;
    }

    bool Equals(SListBase const& other)
    {
        SListBase::Iterator iter(this);
        SListBase::Iterator iter2(&other);
        while (iter.Next())
        {
            if (!iter2.Next() || iter.Data() != iter2.Data())
            {
                return false;
            }
        }
        return !iter2.Next();
    }

    bool CopyTo(TAllocator * allocator, SListBase& to) const
    {
        return CopyTo<DefaultCopyElement>(allocator, to);
    }

    template <void (*CopyElement)(
        Field(TData, TAllocator) const& from, Field(TData, TAllocator)& to)>
    bool CopyTo(TAllocator * allocator, SListBase& to) const
    {
        to.Clear(allocator);
        SListBase::Iterator iter(this);
        NodeBase ** next = &to.Next();
        while (iter.Next())
        {
            Node * node = AllocatorNew(TAllocator, allocator, Node);
            if (node == nullptr)
            {
                return false;
            }
            CopyElement(iter.Data(), node->data);
            *next = node;
            next = &node->Next();
            *next = &to;            // Do this every time, in case an OOM exception occurs, to keep the list correct
            to.IncrementCount();
        }
        return true;
    }

    template <class Fn>
    void Map(Fn fn) const
    {
        MapUntil([fn](Field(TData, TAllocator)& data) { fn(data); return false; });
    }

    template <class Fn>
    bool MapUntil(Fn fn) const
    {
        Iterator iter(this);
        while (iter.Next())
        {
            if (fn(iter.Data()))
            {
                return true;
            }
        }
        return false;
    }

private:
    static void DefaultCopyElement(
        Field(TData, TAllocator) const& from, Field(TData, TAllocator)& to)
    {
        to = from;
    }

    // disable copy constructor
    SListBase(SListBase const& list);
};


template <typename TData, typename TAllocator = ArenaAllocator>
class SListBaseCounted : public SListBase<TData, TAllocator, RealCount>
{
};

template <typename TData, typename TAllocator = ArenaAllocator, typename TCount = DefaultCount>
class SList : public SListBase<TData, TAllocator, TCount>
{
public:
    class EditingIterator : public SListBase<TData, TAllocator, TCount>::EditingIterator
    {
    public:
        EditingIterator() : SListBase<TData, TAllocator, TCount>::EditingIterator() {}
        EditingIterator(SList * list) : SListBase<TData, TAllocator, TCount>::EditingIterator(list) {}
        void RemoveCurrent()
        {
            __super::RemoveCurrent(Allocator());
        }
        Field(TData, TAllocator) * InsertNodeBefore()
        {
            return __super::InsertNodeBefore(Allocator());
        }
        bool InsertBefore(TData const& data)
        {
            return __super::InsertBefore(Allocator(), data);
        }
    private:
        TAllocator * Allocator() const
        {
            return ((SList const *)this->list)->allocator;
        }
    };

    inline EditingIterator GetEditingIterator() { return EditingIterator(this); }

    explicit SList(TAllocator * allocator) : allocator(allocator) {}
    ~SList()
    {
        Clear();
    }
    void Clear()
    {
        __super::Clear(allocator);
    }
    bool Prepend(TData const& data)
    {
        return __super::Prepend(allocator, data);
    }
    Field(TData, TAllocator) * PrependNode()
    {
        return __super::PrependNode(allocator);
    }
    template <typename TParam>
    Field(TData, TAllocator) * PrependNode(TParam param)
    {
        return __super::PrependNode(allocator, param);
    }
    template <typename TParam1, typename TParam2>
    Field(TData, TAllocator) * PrependNode(TParam1 param1, TParam2 param2)
    {
        return __super::PrependNode(allocator, param1, param2);
    }
    void RemoveHead()
    {
        __super::RemoveHead(allocator);
    }
    bool Remove(TData const& data)
    {
        return __super::Remove(allocator, data);
    }

    // Stack like interface
    bool Push(TData const& data)
    {
        return Prepend(data);
    }

    TData Pop()
    {
        TData data = this->Head();
        RemoveHead();
        return data;
    }

    Field(TData, TAllocator) const& Top() const
    {
        return this->Head();
    }
    Field(TData, TAllocator)& Top()
    {
        return this->Head();
    }

private:
    FieldNoBarrier(TAllocator *) allocator;
};

template <typename TData, typename TAllocator = ArenaAllocator>
class SListCounted : public SList<TData, TAllocator, RealCount>
{
public:
    explicit SListCounted(TAllocator * allocator)
        : SList<TData, TAllocator, RealCount>(allocator)
    {}
};


#define _FOREACH_LIST_ENTRY_EX(List, T, Iterator, iter, data, list) \
    auto iter = (list)->Get##Iterator(); \
    while (iter.Next()) \
    { \
        T& data = iter.Data();

#define _NEXT_LIST_ENTRY_EX  \
    }

#define _FOREACH_LIST_ENTRY(List, T, data, list) { _FOREACH_LIST_ENTRY_EX(List, T, Iterator, __iter, data, list)
#define _NEXT_LIST_ENTRY _NEXT_LIST_ENTRY_EX }

#define FOREACH_SLISTBASE_ENTRY(T, data, list) _FOREACH_LIST_ENTRY(SListBase, T, data, list)
#define NEXT_SLISTBASE_ENTRY _NEXT_LIST_ENTRY

#define FOREACH_SLISTBASE_ENTRY_EDITING(T, data, list, iter) _FOREACH_LIST_ENTRY_EX(SListBase, T, EditingIterator, iter, data, list)
#define NEXT_SLISTBASE_ENTRY_EDITING _NEXT_LIST_ENTRY_EX

#define FOREACH_SLISTBASECOUNTED_ENTRY(T, data, list) _FOREACH_LIST_ENTRY(SListBaseCounted, T, data, list)
#define NEXT_SLISTBASECOUNTED_ENTRY _NEXT_LIST_ENTRY

#define FOREACH_SLISTBASECOUNTED_ENTRY_EDITING(T, data, list, iter) _FOREACH_LIST_ENTRY_EX(SListBaseCounted, T, EditingIterator, iter, data, list)
#define NEXT_SLISTBASECOUNTED_ENTRY_EDITING _NEXT_LIST_ENTRY_EX

#define FOREACH_SLIST_ENTRY(T, data, list) _FOREACH_LIST_ENTRY(SList, T, data, list)
#define NEXT_SLIST_ENTRY _NEXT_LIST_ENTRY

#define FOREACH_SLIST_ENTRY_EDITING(T, data, list, iter) _FOREACH_LIST_ENTRY_EX(SList, T, EditingIterator, iter, data, list)
#define NEXT_SLIST_ENTRY_EDITING _NEXT_LIST_ENTRY_EX

#define FOREACH_SLISTCOUNTED_ENTRY(T, data, list) _FOREACH_LIST_ENTRY(SListCounted, T, data, list)
#define NEXT_SLISTCOUNTED_ENTRY _NEXT_LIST_ENTRY

#define FOREACH_SLISTCOUNTED_ENTRY_EDITING(T, data, list, iter) _FOREACH_LIST_ENTRY_EX(SListCounted, T, EditingIterator, iter, data, list)
#define NEXT_SLISTCOUNTED_ENTRY_EDITING _NEXT_LIST_ENTRY_EX
