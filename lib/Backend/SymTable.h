//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

class SymTable
{
    typedef JsUtil::Pair<SymID, Js::PropertyId> SymIdPropIdPair;
    typedef JsUtil::BaseDictionary<SymIdPropIdPair, PropertySym *, JitArenaAllocator, PrimeSizePolicy> PropertyMap;
    typedef JsUtil::BaseDictionary<Js::PropertyId, BVSparse<JitArenaAllocator> *, JitArenaAllocator, PowerOf2SizePolicy> PropertyEquivBvMap;

private:
    static const int    k_symTableSize = 8192;
    static const int    k_maxImplicitParamSlot = 4;
    Sym *               m_table[k_symTableSize];
    StackSym *          m_implicitParams[k_maxImplicitParamSlot];
    PropertyMap *       m_propertyMap;
    Func *              m_func;
    SymID               m_currentID;
    SymID               m_IDAdjustment;

public:
    PropertyEquivBvMap *m_propertyEquivBvMap;

public:
    SymTable() : m_currentID(0), m_func(nullptr), m_IDAdjustment(0)
    {
        memset(m_table, 0, sizeof(m_table));
        memset(m_implicitParams, 0, sizeof(m_implicitParams));
    }


    void                Init(Func* func);
    void                Add(Sym * newSym);
    Sym *               Find(SymID id) const;
    StackSym *          FindStackSym(SymID id) const;
    PropertySym *       FindPropertySym(SymID id) const;
    PropertySym *       FindPropertySym(SymID stackSymID, Js::PropertyId propertyId) const;
    void                SetStartingID(SymID startingID);
    void                IncreaseStartingID(SymID IDIncrease);
    SymID               NewID();
    StackSym *          GetArgSlotSym(Js::ArgSlot argSlotNum);
    StackSym *          GetImplicitParam(Js::ArgSlot paramSlotNum);
    SymID               GetMaxSymID() const;
    void                ClearStackSymScratch();
    void                SetIDAdjustment() { m_IDAdjustment = m_currentID;}
    void                ClearIDAdjustment() { m_IDAdjustment = 0; }
    SymID               MapSymID(SymID id) {  return id + m_IDAdjustment; }
private:
    int                 Hash(SymID id) const;
};

#define FOREACH_SYM_IN_TABLE(sym, table) \
    for (uint i = 0; i < table->k_symTableSize; i++) \
    { \
        for (Sym *sym = table->m_table[i]; sym != nullptr; sym = sym->m_next) \
        {
#define NEXT_SYM_IN_TABLE } }
