//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace JsUtil
{
    namespace
    {
        template <class T1, class T2, bool isT1Smaller>
        struct ChooseSmallerHelper
        {
            typedef T2 type;
        };

        template <class T1, class T2>
        struct ChooseSmallerHelper<T1, T2, true>
        {
            typedef T1 type;
        };

        template <class T1, class T2>
        using ChooseSmaller = typename ChooseSmallerHelper<T1, T2, (sizeof(T1) < sizeof(T2))>::type;

        template <class TValue>
        class ValueEntryData
        {
        protected:
            TValue value;    // data of entry
        public:
            int next;        // Index of next entry, -1 if last
        };

        template <class TKey, class TValue>
        class KeyValueEntryDataLayout1
        {
        protected:
            TValue value;    // data of entry
            TKey key;        // key of entry
        public:
            int next;        // Index of next entry, -1 if last
        };

        template <class TKey, class TValue>
        class KeyValueEntryDataLayout2
        {
        protected:
            TValue value;    // data of entry
        public:
            int next;        // Index of next entry, -1 if last
        protected:
            TKey key;        // key of entry
        };

        // Packing matters because we make so many dictionary entries.
        // The int pointing to the next item in the list may be included
        // either after the value or after the key, depending on which
        // packs better.
        template <class TKey, class TValue>
        using KeyValueEntryData = ChooseSmaller<KeyValueEntryDataLayout1<TKey, TValue>, KeyValueEntryDataLayout2<TKey, TValue>>;

        template <class TValue, class TData = ValueEntryData<TValue>>
        class ValueEntry : public TData
        {
        protected:
            void Set(TValue const& value)
            {
                this->value = value;
            }

        public:
            static bool SupportsCleanup()
            {
                return false;
            }

            static bool NeedsCleanup(ValueEntry<TValue, TData>&)
            {
                return false;
            }

            void Clear()
            {
                ClearValue<TValue>::Clear(&this->value);
            }

            TValue const& Value() const { return this->value; }
            TValue& Value() { return this->value; }
            void SetValue(TValue const& value) { this->value = value; }
        };

        // Used by BaseHashSet, the default is that the key is the same as the value
        template <class TKey, class TValue>
        class ImplicitKeyValueEntry : public ValueEntry<TValue>
        {
        public:
            TKey Key() const { return ValueToKey<TKey, TValue>::ToKey(this->value); }

            void Set(TKey const& key, TValue const& value)
            {
                __super::Set(value);
            }
        };

        template <class TKey, class TValue>
        class KeyValueEntry : public ValueEntry<TValue, KeyValueEntryData<TKey, TValue>>
        {
        protected:
            void Set(TKey const& key, TValue const& value)
            {
                __super::Set(value);
                this->key = key;
            }

        public:
            TKey const& Key() const { return this->key; }

            void Clear()
            {
                __super::Clear();
                this->key = TKey();
            }
        };
    }

    template<class TKey, class TValue>
    struct ValueToKey
    {
        static TKey ToKey(const TValue &value) { return static_cast<TKey>(value); }
    };

    template <class TValue>
    struct ClearValue
    {
        static inline void Clear(TValue* value) { *value = TValue(); }
    };

    template <class TKey>
    class KeyEntry : public ValueEntry<TKey>
    {
    public:
        TKey const& Key() const { return this->value; }
    };

    template <class TKey, class TValue, template <class K, class V> class THashEntry>
    class DefaultHashedEntry : public THashEntry<TKey, TValue>
    {
    public:
        template<typename Comparer, typename TLookup>
        inline bool KeyEquals(TLookup const& otherKey, hash_t otherHashCode)
        {
            return Comparer::Equals(this->Key(), otherKey);
        }

        template<typename Comparer>
        inline hash_t GetHashCode()
        {
            return ((Comparer::GetHashCode(this->Key()) & 0x7fffffff) << 1) | 1;
        }

        void Set(TKey const& key, TValue const& value, int hashCode)
        {
            __super::Set(key, value);
        }
    };

    template <class TKey, template <class K> class THashKeyEntry>
    class DefaultHashedKeyEntry : public THashKeyEntry<TKey>
    {
    public:
        template<typename Comparer, typename TLookup>
        inline bool KeyEquals(TLookup const& otherKey, hash_t otherHashCode)
        {
            return Comparer::Equals(this->Key(), otherKey);
        }

        template<typename Comparer>
        inline hash_t GetHashCode()
        {
            return ((Comparer::GetHashCode(this->Key()) & 0x7fffffff) << 1) | 1;
        }

        void Set(TKey const& key, int hashCode)
        {
            __super::Set(key);
        }
    };

    template <class TKey, class TValue, template <class K, class V> class THashEntry>
    class CacheHashedEntry : public THashEntry<TKey, TValue>
    {
        hash_t hashCode;    // Lower 31 bits of hash code << 1 | 1, 0 if unused
    public:
        static const int INVALID_HASH_VALUE = 0;
        template<typename Comparer, typename TLookup>
        inline bool KeyEquals(TLookup const& otherKey, hash_t otherHashCode)
        {
            Assert(TAGHASH(Comparer::GetHashCode(this->Key())) == this->hashCode);
            return this->hashCode == otherHashCode && Comparer::Equals(this->Key(), otherKey);
        }

        template<typename Comparer>
        inline hash_t GetHashCode()
        {
            Assert(TAGHASH(Comparer::GetHashCode(this->Key())) == this->hashCode);
            return hashCode;
        }

        void Set(TKey const& key, TValue const& value, hash_t hashCode)
        {
            __super::Set(key, value);
            this->hashCode = hashCode;
        }

        void Clear()
        {
            __super::Clear();
            this->hashCode = INVALID_HASH_VALUE;
        }
    };

    template <class TKey, class TValue>
    class SimpleHashedEntry : public DefaultHashedEntry<TKey, TValue, ImplicitKeyValueEntry> {};

    template <class TKey, class TValue>
    class HashedEntry : public CacheHashedEntry<TKey, TValue, ImplicitKeyValueEntry> {};

    template <class TKey, class TValue>
    class SimpleDictionaryEntry : public DefaultHashedEntry<TKey, TValue, KeyValueEntry>  {};

    template <class TKey, class TValue>
    class DictionaryEntry: public CacheHashedEntry<TKey, TValue, KeyValueEntry> {};

    template <class TKey, class TValue>
    class WeakRefValueDictionaryEntry: public SimpleDictionaryEntry<TKey, TValue>
    {
    public:
        void Clear()
        {
            this->key = TKey();
            this->value = TValue();
        }

        static bool SupportsCleanup()
        {
            return true;
        }

        static bool NeedsCleanup(WeakRefValueDictionaryEntry<TKey, TValue> const& entry)
        {
            TValue weakReference = entry.Value();

            return (weakReference == nullptr || weakReference->Get() == nullptr);
        }
    };

    template <class TKey>
    class SimpleDictionaryKeyEntry : public DefaultHashedKeyEntry<TKey, KeyEntry> {};
}
