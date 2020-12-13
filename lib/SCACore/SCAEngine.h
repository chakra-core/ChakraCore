//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
namespace Js
{
    enum class SCADeepCloneType
    {
        None,
        Object,
        Map,
        Set,
        HostObject
    };

    //
    // SCAEngine performs the SCA algorithm by "cloning" a JavaScript var from Src representation
    // to Dst represetation. The representation could be a Var, or a location in a stream.
    //
    template <class Src, class Dst, class Cloner>
    class SCAEngine
    {
    private:
        // The map type that stores a map of cloned objects {Src->Dst}. Use
        // non leaf allocator because the map contains Vars.
        typedef JsUtil::BaseDictionary<Src, Dst, RecyclerNonLeafAllocator> ClonedObjectDictionary;

        Cloner* m_cloner;
        ClonedObjectDictionary* m_clonedObjects;
        Var* m_transferableVars;
        size_t m_cTransferableVars;

    private:
        SCAEngine(Cloner* cloner, Var* m_transferableVars, size_t cTransferableVars)
            : m_cloner(cloner),
            m_transferableVars(m_transferableVars),
            m_cTransferableVars(cTransferableVars)
        {
            Recycler* recycler = cloner->GetScriptContext()->GetRecycler();
            m_clonedObjects = RecyclerNew(recycler, ClonedObjectDictionary, recycler);
            m_cloner->SetEngine(this);
        }

    public:

        void Clone(Src src, Dst* dst)
        {
            PROBE_STACK(m_cloner->GetScriptContext(), Constants::MinStackDefault);

#if ENABLE_COPYONACCESS_ARRAY
            JavascriptLibrary::CheckAndConvertCopyOnAccessNativeIntArray<Src>(src);
#endif

            typename Cloner::SrcTypeId typeId = m_cloner->GetTypeId(src);

            if (m_cloner->TryClonePrimitive(typeId, src, dst))
            {
                return;
            }

            if (Cloner::ShouldLookupReference() && TryGetClonedObject(src, dst))
            {
                m_cloner->CloneObjectReference(src, *dst);
                return;
            }

            SCADeepCloneType deepClone;
            if (m_cloner->TryCloneObject(typeId, src, dst, &deepClone))
            {
                m_clonedObjects->Add(src, *dst);

                if (deepClone == SCADeepCloneType::Map)
                {
                    m_cloner->CloneMap(src, *dst);
                    deepClone = SCADeepCloneType::Object;
                }
                else if (deepClone == SCADeepCloneType::Set)
                {
                    m_cloner->CloneSet(src, *dst);
                    deepClone = SCADeepCloneType::Object;
                }

                if (deepClone == SCADeepCloneType::HostObject)
                {
                    m_cloner->CloneHostObjectProperties(typeId, src, *dst);
                }
                else if (deepClone == SCADeepCloneType::Object)
                {
                    m_cloner->CloneProperties(typeId, src, *dst);
                }
                return;
            }

            // Unsupported src type, throw
            m_cloner->ThrowSCAUnsupported();
        }

        void Clone(Src src)
        {
            Dst unused;
            Clone(src, &unused);
        }

        bool TryGetClonedObject(Src src, Dst* dst) const
        {
            return m_clonedObjects->TryGetValue(src, dst);
        }

        bool TryGetTransferredOrShared(Var source, size_t* outDestination)
        {
            if (m_transferableVars == nullptr)
            {
                return false;
            }

            for (size_t i = 0; i < m_cTransferableVars; i++)
            {
                if (m_transferableVars[i] == source)
                {
                    if (outDestination != nullptr)
                    {
                        *outDestination = i;
                    }

                    return true;
                }
            }

            return false;
        }

        Dst ClaimTransferable(size_t index, JavascriptLibrary* library)
        {
            AssertMsg(index < this->m_cTransferableVars, "Index out of range.");
            ArrayBuffer *ab = VarTo<ArrayBuffer>(m_transferableVars[index]);

            return ab;
        }

        static Dst Clone(Src root, Cloner* cloner, Var* transferableVars, size_t cTransferableVars)
        {
            SCAEngine<Src, Dst, Cloner> engine(cloner, transferableVars, cTransferableVars);
            Dst dst;
            engine.Clone(root, &dst);
            return dst;
        }
    };

    //
    // Helper class that simply contains a ScriptContext*.
    //
    class ScriptContextHolder
    {
    private:
        ScriptContext* m_scriptContext;

    public:
        ScriptContextHolder(ScriptContext* scriptContext)
            : m_scriptContext(scriptContext)
        {
        }

        ScriptContext* GetScriptContext() const
        {
            return m_scriptContext;
        }

        void ThrowIfFailed(HRESULT hr) const;

        void __declspec(noreturn) ThrowSCAUnsupported() const
        {
            // E_SCA_UNSUPPORTED
            ThrowIfFailed(E_FAIL);
        }

        void __declspec(noreturn) ThrowSCANewVersion() const
        {
            // E_SCA_NEWVERSION
            ThrowIfFailed(E_FAIL);
        }

        void __declspec(noreturn) ThrowSCADataCorrupt() const
        {
            // E_SCA_DATACORRUPT
            ThrowIfFailed(E_FAIL);
        }

        void __declspec(noreturn) ThrowSCAObjectDetached() const
        {
            // E_SCA_TRANSFERABLE_NEUTERED
            ThrowIfFailed(E_FAIL);
        }
    };

    //
    // Helper class to implement Cloner.
    //
    template <class TSrc, class TDst, class TSrcTypeId, class Cloner>
    class ClonerBase : public ScriptContextHolder
    {
    public:
        typedef TSrc Src;
        typedef TDst Dst;
        typedef TSrcTypeId SrcTypeId;
        typedef SCAEngine<Src, Dst, Cloner> Engine;

    private:
        Engine* m_engine;

    public:
        ClonerBase(ScriptContext* scriptContext)
            : ScriptContextHolder(scriptContext),
            m_engine(NULL)
        {
        }

        Engine* GetEngine() const
        {
            Assert(m_engine); // Must have been set
            return m_engine;
        }

        void SetEngine(Engine* engine)
        {
            Assert(!m_engine); // Can only be set once
            m_engine = engine;
        }
    };

}
