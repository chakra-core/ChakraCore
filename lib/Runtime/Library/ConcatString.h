//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    // Upon construction, LiteralStringWithPropertyStringPtr is guaranteed to point to a recycler-allocated
    // buffer, so it passes the LiteralString assertions. However, upon receiving a property record pointer,
    // this object will update to point to the copy of its data in that property record, meaning that using
    // this class is just like using PropertyString: you can't take a reference to the underlying buffer via
    // GetSz or GetString, drop the reference to the owning string, and expect the buffer to stay alive.
    class LiteralStringWithPropertyStringPtr : public LiteralString
    {
    private:
        Field(PropertyString*) propertyString;
        Field(const Js::PropertyRecord*) propertyRecord;

    public:
        virtual void GetPropertyRecord(_Out_ PropertyRecord const** propRecord, bool dontLookupFromDictionary = false) override;
        void GetPropertyRecordImpl(_Out_ PropertyRecord const** propRecord, bool dontLookupFromDictionary = false);
        virtual void CachePropertyRecord(_In_ PropertyRecord const* propertyRecord) override;
        void CachePropertyRecordImpl(_In_ PropertyRecord const* propertyRecord);
        virtual void const * GetOriginalStringReference() override;

        virtual RecyclableObject* CloneToScriptContext(ScriptContext* requestContext) override;

        bool HasPropertyRecord() const { return propertyRecord != nullptr; }

        PropertyString * GetPropertyString() const;
        PropertyString * GetOrAddPropertyString(); // Get if it's there, otherwise bring it in.
        void SetPropertyString(PropertyString * propStr);

        template <typename StringType> static LiteralStringWithPropertyStringPtr * ConvertString(StringType * originalString);

        static uint GetOffsetOfPropertyString() { return offsetof(LiteralStringWithPropertyStringPtr, propertyString); }

        static JavascriptString *
        NewFromCString(const char * cString, const CharCount charCount, JavascriptLibrary *const library);

        static JavascriptString *
        NewFromWideString(const char16 * wString, const CharCount charCount, JavascriptLibrary *const library);

        static JavascriptString * CreateEmptyString(JavascriptLibrary *const library);

    protected:
        LiteralStringWithPropertyStringPtr(StaticType* stringTypeStatic);
        LiteralStringWithPropertyStringPtr(const char16 * wString, const CharCount stringLength, JavascriptLibrary *const library);

        DEFINE_VTABLE_CTOR(LiteralStringWithPropertyStringPtr, LiteralString);

    public:
        virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
        {
            return VTableValue::VtableLiteralStringWithPropertyStringPtr;
        }
    };

    template <> bool VarIsImpl<LiteralStringWithPropertyStringPtr>(RecyclableObject * obj);

    // Base class for concat strings.
    // Concat string is a virtual string, or a non-leaf node in concat string tree.
    // It does not hold characters by itself but has one or more child nodes.
    // Only leaf nodes (which are not concat strings) hold the actual characters.
    // The flattening happens on demand (call GetString() or GetSz()),
    // until then we don't create actual big char16* buffer, just keep concat tree as a tree.
    // The result of flattening the concat string tree is concat of all leaf nodes from left to right.
    // Usage pattern:
    //   // Create concat tree using one of non-abstract derived classes.
    //   JavascriptString* result = concatTree->GetString();    // At this time we flatten the tree into 1 actual wchat_t* string.
    class ConcatStringBase : public LiteralString // vtable will be switched to LiteralString's vtable after flattening
    {
        friend JavascriptString;

    protected:
        ConcatStringBase(StaticType* stringTypeStatic);
        DEFINE_VTABLE_CTOR_ABSTRACT(ConcatStringBase, LiteralString);

        virtual void CopyVirtual(_Out_writes_(m_charLength) char16 *const buffer,
            StringCopyInfoStack &nestedStringTreeCopyInfos, const byte recursionDepth) = 0;
        void CopyImpl(_Out_writes_(m_charLength) char16 *const buffer,
            int itemCount, _In_reads_(itemCount) JavascriptString * const * items,
            StringCopyInfoStack &nestedStringTreeCopyInfos, const byte recursionDepth);

        // Subclass can call this to implement GetSz and use the actual type to avoid virtual call to Copy.
        template <typename ConcatStringType> const char16 * GetSzImpl();
    public:
        virtual const char16* GetSz() = 0;     // Force subclass to call GetSzImpl with the real type to avoid virtual calls
        using JavascriptString::Copy;
        virtual bool IsTree() const override sealed;
    };

    // Concat string with N (or less) child nodes.
    // Use it when you know the number of children at compile time.
    // There could be less nodes than N. When we find 1st NULL node going incrementing the index,
    // we see this as indication that the rest on the right are empty and stop iterating.
    // Usage pattern:
    //   ConcatStringN<3>* tree = ConcatStringN<3>::New(scriptContext);
    //   tree->SetItem(0, javascriptString1);
    //   tree->SetItem(1, javascriptString2);
    //   tree->SetItem(2, javascriptString3);
    template <int N>
    class ConcatStringN : public ConcatStringBase
    {
        friend JavascriptString;

    protected:
        ConcatStringN(StaticType* stringTypeStatic, bool doZeroSlotsAndLength = true);
        DEFINE_VTABLE_CTOR(ConcatStringN<N>, ConcatStringBase);

        virtual void CopyVirtual(_Out_writes_(m_charLength) char16 *const buffer, StringCopyInfoStack &nestedStringTreeCopyInfos, const byte recursionDepth) override
        {
#pragma prefast(suppress: __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY, "WDGVSO:14980704 The CopyImpl method uses GetLength() to ensure we only access m_charLength elements of buffer.")
            __super::CopyImpl(buffer, N, AddressOf(m_slots[0]), nestedStringTreeCopyInfos, recursionDepth);
        }
        virtual int GetRandomAccessItemsFromConcatString(Js::JavascriptString * const *& items) const
        {
            items = AddressOf(m_slots[0]);
            return N;
        }

    public:
        static ConcatStringN<N>* New(ScriptContext* scriptContext);
        const char16 * GetSz() override sealed;
        void SetItem(_In_range_(0, N - 1) int index, JavascriptString* value);

    protected:
        Field(JavascriptString*) m_slots[N];   // These contain the child nodes. 1 slot is per 1 item (JavascriptString*).
    };

    // Concat string that uses binary tree, each node has 2 children.
    // Usage pattern:
    //   ConcatString* str = ConcatString::New(javascriptString1, javascriptString2);
    // Note: it's preferred you would use the following for concats, that would figure out whether concat string is optimal or create a new string is better.
    //   JavascriptString::Concat(javascriptString1, javascriptString2);
    class ConcatString sealed : public ConcatStringN<2>
    {
        ConcatString(JavascriptString* a, JavascriptString* b);
    protected:
        DEFINE_VTABLE_CTOR(ConcatString, ConcatStringN<2>);
    public:
        static ConcatString* New(JavascriptString* a, JavascriptString* b);
        static const int MaxDepth = 1000;

        JavascriptString *LeftString() const { Assert(!IsFinalized()); return m_slots[0]; }
        JavascriptString *RightString() const { Assert(!IsFinalized()); return m_slots[1]; }
    };

    // Concat string with any number of child nodes, can grow dynamically.
    // Usage pattern:
    //   ConcatStringBuilder* tree = ConcatStringBuider::New(scriptContext, 5);
    //   tree->Append(javascriptString1);
    //   tree->Append(javascriptString2);
    //   ...
    // Implementation details:
    // - uses chunks, max chunk size is specified by c_maxChunkSlotCount, until we fit into that, we realloc, otherwise create new chunk.
    // - We use chunks in order to avoid big allocations, we don't expect lots of reallocs, that why chunk size is relatively big.
    // - the chunks are linked using m_prevChunk field. flattening happens from left to right, i.e. first we need to get
    //   head chunk -- the one that has m_prevChunk == NULL.
    class ConcatStringBuilder sealed : public ConcatStringBase
    {
        friend JavascriptString;
        ConcatStringBuilder(ScriptContext* scriptContext, int initialSlotCount);
        ConcatStringBuilder(const ConcatStringBuilder& other);
        void AllocateSlots(int requestedSlotCount);
        ConcatStringBuilder* GetHead() const;

    protected:
        DEFINE_VTABLE_CTOR(ConcatStringBuilder, ConcatStringBase);
        virtual void CopyVirtual(_Out_writes_(m_charLength) char16 *const buffer, StringCopyInfoStack &nestedStringTreeCopyInfos, const byte recursionDepth) override sealed;

    public:
        static ConcatStringBuilder* New(ScriptContext* scriptContext, int initialSlotCount);
        const char16 * GetSz() override sealed;
        void Append(JavascriptString* str);

    private:
        // MAX number of slots in one chunk. Until we fit into this, we realloc, otherwise create new chunk.
        static const int c_maxChunkSlotCount = 1024;
        int GetItemCount() const;

        Field(Field(JavascriptString*)*) m_slots; // Array of child nodes.
        Field(int) m_slotCount;   // Number of allocated slots (1 slot holds 1 item) in this chunk.
        Field(int) m_count;       // Actual number of items in this chunk.
        Field(ConcatStringBuilder*) m_prevChunk;
    };

    // Concat string that wraps another string.
    // Use it when you need to wrap something with e.g. { and }.
    // Usage pattern:
    //   result = ConcatStringWrapping<_u('{'), _u('}')>::New(result);
    template <char16 L, char16 R>
    class ConcatStringWrapping sealed : public ConcatStringBase
    {
        friend JavascriptString;
        ConcatStringWrapping(JavascriptString* inner);
        JavascriptString* GetFirstItem() const;
        JavascriptString* GetLastItem() const;

    protected:
        DEFINE_VTABLE_CTOR(ConcatStringWrapping, ConcatStringBase);
        virtual void CopyVirtual(_Out_writes_(m_charLength) char16 *const buffer, StringCopyInfoStack &nestedStringTreeCopyInfos, const byte recursionDepth) override sealed
        {
            const_cast<ConcatStringWrapping *>(this)->EnsureAllSlots();
#pragma prefast(suppress: __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY, "WDGVSO:14980704 The CopyImpl method uses GetLength() to ensure we only access m_charLength elements of buffer.")
            __super::CopyImpl(buffer, _countof(m_slots), AddressOf(m_slots[0]), nestedStringTreeCopyInfos, recursionDepth);
        }
        virtual int GetRandomAccessItemsFromConcatString(Js::JavascriptString * const *& items) const override sealed
        {
            const_cast<ConcatStringWrapping *>(this)->EnsureAllSlots();
            items = AddressOf(m_slots[0]);
            return _countof(m_slots);
        }
    public:
        static ConcatStringWrapping<L, R>* New(JavascriptString* inner);
        const char16 * GetSz() override sealed;
    private:
        void EnsureAllSlots()
        {
            m_slots[0] = this->GetFirstItem();
            m_slots[1] = m_inner;
            m_slots[2] = this->GetLastItem();
        }

        Field(JavascriptString *) m_inner;

        // Use the padding space for the concat
        Field(JavascriptString *) m_slots[3];
    };

    // Make sure the padding doesn't add tot he size of ConcatStringWrapping
#if defined(TARGET_64)
    CompileAssert(sizeof(ConcatStringWrapping<_u('"'), _u('"')>) == 64);
#else
    CompileAssert(sizeof(ConcatStringWrapping<_u('"'), _u('"')>) == 32);
#endif

    // Concat string with N child nodes. Use it when you don't know the number of children at compile time.
    // Usage pattern:
    //   ConcatStringMulti* tree = ConcatStringMulti::New(3, scriptContext);
    //   tree->SetItem(0, javascriptString1);
    //   tree->SetItem(1, javascriptString2);
    //   tree->SetItem(2, javascriptString3);
    class ConcatStringMulti sealed : public ConcatStringBase
    {
        friend JavascriptString;

    protected:
        ConcatStringMulti(uint slotCount, JavascriptString * a1, JavascriptString * a2, StaticType* stringTypeStatic);
        DEFINE_VTABLE_CTOR(ConcatStringMulti, ConcatStringBase);

        virtual void CopyVirtual(_Out_writes_(m_charLength) char16 *const buffer, StringCopyInfoStack &nestedStringTreeCopyInfos, const byte recursionDepth) override
        {
            Assert(IsFilled());
#pragma prefast(suppress: __WARNING_POTENTIAL_BUFFER_OVERFLOW_HIGH_PRIORITY, "WDGVSO:14980704 The CopyImpl method uses GetLength() to ensure we only access m_charLength elements of buffer.")
            __super::CopyImpl(buffer, slotCount, AddressOf(m_slots[0]), nestedStringTreeCopyInfos, recursionDepth);
        }
        virtual int GetRandomAccessItemsFromConcatString(Js::JavascriptString * const *& items) const
        {
            Assert(IsFilled());
            items = AddressOf(m_slots[0]);
            return slotCount;
        }

    public:
        static ConcatStringMulti * New(uint slotCount, JavascriptString * a1, JavascriptString * a2, ScriptContext* scriptContext);
        const char16 * GetSz() override sealed;
        static size_t GetAllocSize(uint slotCount);
        void SetItem(_In_range_(0, slotCount - 1) uint index, JavascriptString* value);

        static uint32 GetOffsetOfSlotCount() { return offsetof(ConcatStringMulti, slotCount); }
        static uint32 GetOffsetOfSlots() { return offsetof(ConcatStringMulti, m_slots); }
    protected:
        Field(uint) slotCount;
        Field(uint)   __alignment;
        Field(size_t) __alignmentPTR;
        Field(JavascriptString*) m_slots[];   // These contain the child nodes.

#if DBG
        bool IsFilled() const;
#endif


    public:
        virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
        {
            return VTableValue::VtableConcatStringMulti;
        }
    };

    template <> bool VarIsImpl<ConcatStringMulti>(RecyclableObject* obj);
}
