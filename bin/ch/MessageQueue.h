//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class MessageBase
{
private:
    unsigned int m_time;
    unsigned int m_id;

    static unsigned int s_messageCount;

    MessageBase(const MessageBase&);

public:
    MessageBase(unsigned int time) : m_time(time), m_id(s_messageCount++) { }
    virtual ~MessageBase() { }

    void BeginTimer() { m_time += GetTickCount(); };
    unsigned int GetTime() { return m_time; };
    unsigned int GetId() { return m_id; };

    virtual HRESULT Call(LPCSTR fileName) = 0;
};

template <typename T>
class SortedList 
{
    template <typename T>
    struct DListNode
    {
        T data;
        DListNode<T>* prev;
        DListNode<T>* next;

    public:
        DListNode(const T& data) :
            data(data),
            prev(nullptr),
            next(nullptr)
        { }
    };

public:
    SortedList():
        head(nullptr)
    {
    }

    ~SortedList()
    {
        while (head != nullptr)
        {
            Remove(head);
        }
    }

    // Scan through the sorted list
    // Insert before the first node that satisfies the LessThan function
    // This function maintains the invariant that the list is always sorted
    void Insert(const T& data)
    {
        DListNode<T>* curr = head;
        DListNode<T>* node = new DListNode<T>(data);
        DListNode<T>* prev = nullptr; 

        // Now, if we have to insert, we have to insert *after* some node
        while (curr != nullptr)
        {
            if (T::LessThan(data, curr->data))
            {
                break;
            }
            prev = curr;
            curr = curr->next;
        }

        InsertAfter(node, prev);
    }

    T Pop()
    {
        T data = head->data;
        Remove(head);
        return data;
    }

    template <typename PredicateFn>
    void Remove(PredicateFn fn)
    {
        DListNode<T>* node = head;

        while (node != nullptr)
        {
            if (fn(node->data))
            {
                Remove(node);
                return;
            }
        }
    }

    bool IsEmpty()
    {
        return head == nullptr;
    }

private:
    void Remove(DListNode<T>* node)
    {
        if (node->prev == nullptr)
        {
            head = node->next;
        }
        else
        {
            node->prev->next = node->next;
        }

        if (node->next != nullptr)
        {
            node->next->prev = node->prev;
        }

        delete node;
    }

    void InsertAfter(DListNode<T>* newNode, DListNode<T>* node)
    {
        // If the list is empty, just set head to newNode
        if (head == nullptr)
        {
            Assert(node == nullptr);
            head = newNode;
            return;
        }

        // If node is null here, we must be trying to insert before head
        if (node == nullptr)
        {
            newNode->next = head;
            head->prev = newNode;
            head = newNode;
            return;
        }

        Assert(node);
        newNode->next = node->next;
        newNode->prev = node;

        if (node->next)
        {
            node->next->prev = newNode;
        }

        node->next = newNode;
    }

    DListNode<T>* head;
};

class MessageQueue
{
    struct ListEntry
    {
        unsigned int time;
        MessageBase* message;

        ListEntry(unsigned int time, MessageBase* message):
            time(time),
            message(message)
        { }

        static bool LessThan(const ListEntry& first, const ListEntry& second)
        {
            return first.time < second.time;
        }
    };

    SortedList<ListEntry> m_queue;

public:
    void InsertSorted(MessageBase *message)
    {
        message->BeginTimer();
        unsigned int time = message->GetTime();
        m_queue.Insert(ListEntry(time, message));
    }

    MessageBase* PopAndWait()
    {
        Assert(!m_queue.IsEmpty());

        ListEntry entry = m_queue.Pop();
        MessageBase *tmp = entry.message;

        int waitTime = tmp->GetTime() - GetTickCount();
        if(waitTime > 0)
        {
            Sleep(waitTime);
        }

        return tmp;
    }

    bool IsEmpty()
    {
        return m_queue.IsEmpty();
    }

    void RemoveById(unsigned int id)
    {
        // Search for the message with the correct id, and delete it. Can be updated
        // to a hash to improve speed, if necessary.
        m_queue.Remove([id](const ListEntry& entry) 
        {
            MessageBase *msg = entry.message;
            if(msg->GetId() == id)
            {
                delete msg;
                return true;
            }

            return false;
        });
    }

    void RemoveAll()
    {
        m_queue.Remove([](const ListEntry& entry) { 
            MessageBase* msg = entry.message;
            delete msg;
            return true;
        });
    }

    HRESULT ProcessAll(LPCSTR fileName)
    {
        while(!IsEmpty())
        {
            MessageBase *msg = PopAndWait();

            // Omit checking return value for async function, since it shouldn't affect others.
            msg->Call(fileName);
            delete msg;

            ChakraRTInterface::JsTTDNotifyYield();
        }
        return S_OK;
    }
};

//
// A custom message helper class to assist defining messages handled by callback functions.
//
template <class Func, class CustomBase>
class CustomMessage : public CustomBase
{
private:
    Func m_func;

public:
    CustomMessage(unsigned int time, JsValueRef customArg, const Func& func) :
        CustomBase(time, customArg), m_func(func)
    {}

    virtual HRESULT Call(LPCSTR fileName) override
    {
        return m_func(*this);
    }
};
