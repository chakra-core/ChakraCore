//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if ENABLE_TTD

namespace TTD
{
    //A class that manages inflation maps during heap state restoration
    class InflateMap
    {
    private:
        TTDIdentifierDictionary<TTD_PTR_ID, Js::DynamicTypeHandler*> m_handlerMap;
        TTDIdentifierDictionary<TTD_PTR_ID, Js::Type*> m_typeMap;

        //The maps for script contexts and objects 
        TTDIdentifierDictionary<TTD_LOG_TAG, Js::GlobalObject*> m_tagToGlobalObjectMap; //get the script context from here
        TTDIdentifierDictionary<TTD_PTR_ID, Js::RecyclableObject*> m_objectMap;

        //The maps for inflated function bodies
        TTDIdentifierDictionary<TTD_PTR_ID, Js::FunctionBody*> m_functionBodyMap;
        TTDIdentifierDictionary<TTD_PTR_ID, Js::FrameDisplay*> m_environmentMap;
        TTDIdentifierDictionary<TTD_PTR_ID, Js::Var*> m_slotArrayMap;

        //A dictionary for the Promise related bits (not typesafe and a bit ugly but I prefer it to creating multiple additional collections)
        JsUtil::BaseDictionary<TTD_PTR_ID, void*, HeapAllocator> m_promiseDataMap;

        //A set we use to pin all the inflated objects live during/after inflate process (to avoid accidental collection)
        ReferencePinSet* m_inflatePinSet;
        ReferencePinSet* m_oldInflatePinSet;

        //Temp data structures for holding info during the inflate process
        TTDIdentifierDictionary<TTD_PTR_ID, Js::RecyclableObject*> m_oldObjectMap;
        TTDIdentifierDictionary<TTD_PTR_ID, Js::FunctionBody*> m_oldFunctionBodyMap;

        JsUtil::BaseHashSet<Js::PropertyId, HeapAllocator> m_propertyReset;

    public:
        InflateMap();
        ~InflateMap();

        void PrepForInitialInflate(ThreadContext* threadContext, uint32 ctxCount, uint32 handlerCount, uint32 typeCount, uint32 objectCount, uint32 bodyCount, uint32 envCount, uint32 slotCount);
        void PrepForReInflate(uint32 ctxCount, uint32 handlerCount, uint32 typeCount, uint32 objectCount, uint32 bodyCount, uint32 envCount, uint32 slotCount);
        void CleanupAfterInflate();

        bool IsObjectAlreadyInflated(TTD_PTR_ID objid) const;
        bool IsFunctionBodyAlreadyInflated(TTD_PTR_ID fbodyid) const;

        Js::RecyclableObject* FindReusableObjectIfExists(TTD_PTR_ID objid) const;
        Js::FunctionBody* FindReusableFunctionBodyIfExists(TTD_PTR_ID fbodyid) const;

        ////

        Js::DynamicTypeHandler* LookupHandler(TTD_PTR_ID handlerId) const;
        Js::Type* LookupType(TTD_PTR_ID typeId) const;

        Js::ScriptContext* LookupScriptContext(TTD_LOG_TAG sctag) const;
        Js::RecyclableObject* LookupObject(TTD_PTR_ID objid) const;

        Js::FunctionBody* LookupFunctionBody(TTD_PTR_ID functionId) const;
        Js::FrameDisplay* LookupEnvironment(TTD_PTR_ID envid) const;
        Js::Var* LookupSlotArray(TTD_PTR_ID slotid) const;

        ////

        void AddDynamicHandler(TTD_PTR_ID handlerId, Js::DynamicTypeHandler* value);
        void AddType(TTD_PTR_ID typeId, Js::Type* value);

        void AddScriptContext(TTD_LOG_TAG sctag, Js::ScriptContext* value);
        void AddObject(TTD_PTR_ID objid, Js::RecyclableObject* value);

        void AddInflationFunctionBody(TTD_PTR_ID functionId, Js::FunctionBody* value);
        void AddEnvironment(TTD_PTR_ID envId, Js::FrameDisplay* value);
        void AddSlotArray(TTD_PTR_ID slotId, Js::Var* value);

        ////

        //Get the inflate stack and property reset set to use for depends on processing
        JsUtil::BaseHashSet<Js::PropertyId, HeapAllocator>& GetPropertyResetSet();

        Js::Var InflateTTDVar(TTDVar var) const;

        ////

        template <typename T>
        bool IsPromiseInfoDefined(TTD_PTR_ID ptrId) const
        {
            return this->m_promiseDataMap.ContainsKey(ptrId);
        }

        template <typename T>
        void AddInflatedPromiseInfo(TTD_PTR_ID ptrId, T* data)
        {
            this->m_promiseDataMap.AddNew(ptrId, data);
        }

        template <typename T>
        T* LookupInflatedPromiseInfo(TTD_PTR_ID ptrId) const
        {
            return (T*)this->m_promiseDataMap.LookupWithKey(ptrId, nullptr);
        }
    };
}

#endif

