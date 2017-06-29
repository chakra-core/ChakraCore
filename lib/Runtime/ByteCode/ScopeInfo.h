//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js {
    //
    // ScopeInfo is used to persist Scope info of outer functions. When reparsing deferred nested
    // functions, use persisted ScopeInfo to restore outer closures.
    //
    class ScopeInfo
    {
        DECLARE_RECYCLER_VERIFY_MARK_FRIEND()

        struct MapSymbolData
        {
            FuncInfo* func;
            int nonScopeSymbolCount;
        };

        struct SymbolInfo
        {
            union
            {
                Field(PropertyId) propertyId;
                Field(PropertyRecord const*) name;
            };
            Field(SymbolType) symbolType;
            Field(bool) hasFuncAssignment;
            Field(bool) isBlockVariable;
            Field(bool) isFuncExpr;
            Field(bool) isModuleExportStorage;
            Field(bool) isModuleImport;
        };

    private:
        Field(ScopeInfo *) parent;               // link to parent scope info (if any)
        Field(FunctionInfo * const) functionInfo;// link to function owning this scope

        Field(BYTE) isDynamic : 1;             // isDynamic bit affects how deferredChild access global ref
        Field(BYTE) isObject : 1;              // isObject bit affects how deferredChild access closure symbols
        Field(BYTE) mustInstantiate : 1;       // the scope must be instantiated as an object/array
        Field(BYTE) isCached : 1;              // indicates that local vars and functions are cached across invocations
        Field(BYTE) areNamesCached : 1;
        Field(BYTE) canMergeWithBodyScope : 1;
        Field(BYTE) hasLocalInClosure : 1;

        FieldNoBarrier(Scope *) scope;
        Field(::ScopeType) scopeType;
        Field(int) scopeId;
        Field(int) symbolCount;                // symbol count in this scope
        Field(SymbolInfo) symbols[];           // symbol PropertyIDs, index == sym.scopeSlot

    private:
        ScopeInfo(FunctionInfo * function, int symbolCount)
            : functionInfo(function), /*funcExprScopeInfo(nullptr), paramScopeInfo(nullptr),*/ symbolCount(symbolCount), parent(nullptr), scope(nullptr), areNamesCached(false), hasLocalInClosure(false)/*, parentOnly(false)*/
        {
        }

        void SetSymbolId(int i, PropertyId propertyId)
        {
            Assert(!areNamesCached);
            Assert(i >= 0 && i < symbolCount);
            symbols[i].propertyId = propertyId;
        }

        void SetSymbolType(int i, SymbolType symbolType)
        {
            Assert(!areNamesCached);
            Assert(i >= 0 && i < symbolCount);
            symbols[i].symbolType = symbolType;
        }

        void SetHasFuncAssignment(int i, bool has)
        {
            Assert(!areNamesCached);
            Assert(i >= 0 && i < symbolCount);
            symbols[i].hasFuncAssignment = has;
        }

        void SetIsBlockVariable(int i, bool is)
        {
            Assert(!areNamesCached);
            Assert(i >= 0 && i < symbolCount);
            symbols[i].isBlockVariable = is;
        }

        void SetIsFuncExpr(int i, bool is)
        {
            Assert(!areNamesCached);
            Assert(i >= 0 && i < symbolCount);
            symbols[i].isFuncExpr = is;
        }

        void SetIsModuleExportStorage(int i, bool is)
        {
            Assert(!areNamesCached);
            Assert(i >= 0 && i < symbolCount);
            symbols[i].isModuleExportStorage = is;
        }

        void SetIsModuleImport(int i, bool is)
        {
            Assert(!areNamesCached);
            Assert(i >= 0 && i < symbolCount);
            symbols[i].isModuleImport = is;
        }

        void SetPropertyName(int i, PropertyRecord const* name)
        {
            Assert(!areNamesCached);
            Assert(i >= 0 && i < symbolCount);
            symbols[i].name = name;
        }

        PropertyId GetSymbolId(int i) const
        {
            Assert(!areNamesCached);
            Assert(i >= 0 && i < symbolCount);
            return symbols[i].propertyId;
        }

        SymbolType GetSymbolType(int i) const
        {
            Assert(i >= 0 && i < symbolCount);
            return symbols[i].symbolType;
        }

        bool GetHasFuncAssignment(int i)
        {
            Assert(i >= 0 && i < symbolCount);
            return symbols[i].hasFuncAssignment;
        }

        bool GetIsModuleExportStorage(int i)
        {
            Assert(i >= 0 && i < symbolCount);
            return symbols[i].isModuleExportStorage;
        }

        bool GetIsModuleImport(int i)
        {
            Assert(i >= 0 && i < symbolCount);
            return symbols[i].isModuleImport;
        }

        bool GetIsBlockVariable(int i)
        {
            Assert(i >= 0 && i < symbolCount);
            return symbols[i].isBlockVariable;
        }

        bool GetIsFuncExpr(int i)
        {
            Assert(i >= 0 && i < symbolCount);
            return symbols[i].isFuncExpr;
        }

        PropertyRecord const* GetPropertyName(int i)
        {
            Assert(areNamesCached);
            Assert(i >= 0 && i < symbolCount);
            return symbols[i].name;
        }

        void SaveSymbolInfo(Symbol* sym, MapSymbolData* mapSymbolData);

        static ScopeInfo* SaveScopeInfo(Scope * scope, ScriptContext * scriptContext);
        static ScopeInfo* SaveOneScopeInfo(Scope * scope, ScriptContext * scriptContext);

    public:
        FunctionInfo * GetFunctionInfo() const
        {
            return functionInfo;
        }

        ParseableFunctionInfo * GetParseableFunctionInfo() const
        {
            return functionInfo ? functionInfo->GetParseableFunctionInfo() : nullptr;
        }

        ScopeInfo* GetParentScopeInfo() const
        {
            return parent;//? parent->GetParseableFunctionInfo()->GetScopeInfo() : nullptr;
        }

        void SetParentScopeInfo(ScopeInfo * parent)
        {
            Assert(this->parent == nullptr);
            this->parent = parent;
        }

        Scope * GetScope() const
        {
            return scope;
        }

        void SetScope(Scope * scope)
        {
            this->scope = scope;
        }

        ::ScopeType GetScopeType() const
        {
            return scopeType;
        }

        void SetScopeType(::ScopeType type)
        {
            this->scopeType = type;
        }

        void SetScopeId(int id)
        {
            this->scopeId = id;
        }

        int GetScopeId() const
        {
            return scopeId;
        }

        int GetSymbolCount() const
        {
            return symbolCount;
        }

        bool IsObject() const
        {
            return isObject;
        }

        bool IsCached() const
        {
            return isCached;
        }

        void SetHasLocalInClosure(bool has)
        {
            hasLocalInClosure = has;
        }

        bool GetHasOwnLocalInClosure() const
        {
            return hasLocalInClosure;
        }

        static void SaveEnclosingScopeInfo(ByteCodeGenerator* byteCodeGenerator, /*FuncInfo* parentFunc,*/ FuncInfo* func);

        void EnsurePidTracking(ScriptContext* scriptContext);

        void ExtractScopeInfo(Parser *parser, /*ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo,*/ Scope* scope);

        //
        // Turn on capturesAll for a Scope temporarily. Restore old capturesAll when this object
        // goes out of scope.
        //
        class AutoCapturesAllScope
        {
        private:
            Scope* scope;
            bool oldCapturesAll;

        public:
            AutoCapturesAllScope(Scope* scope, bool turnOn);
            ~AutoCapturesAllScope();
            bool OldCapturesAll() const
            {
                return oldCapturesAll;
            }
        };
    };
}
