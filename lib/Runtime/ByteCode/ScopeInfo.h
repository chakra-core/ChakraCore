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
        struct MapSymbolData
        {
            ByteCodeGenerator* byteCodeGenerator;
            FuncInfo* func;
            int nonScopeSymbolCount;
        };

        struct SymbolInfo
        {
            union
            {
                PropertyId propertyId;
                PropertyRecord const* name;
            };
            SymbolType symbolType;
            bool hasFuncAssignment;
            bool isBlockVariable;
            bool isFuncExpr;
            bool isModuleExportStorage;
            bool isModuleImport;
        };

    private:
        Field(FunctionBody * const) parent;    // link to parent function body
        Field(ScopeInfo*) funcExprScopeInfo;   // optional func expr scope info
        Field(ScopeInfo*) paramScopeInfo;      // optional param scope info

        Field(BYTE) isDynamic : 1;             // isDynamic bit affects how deferredChild access global ref
        Field(BYTE) isObject : 1;              // isObject bit affects how deferredChild access closure symbols
        Field(BYTE) mustInstantiate : 1;       // the scope must be instantiated as an object/array
        Field(BYTE) isCached : 1;              // indicates that local vars and functions are cached across invocations
        Field(BYTE) isGlobalEval : 1;
        Field(BYTE) areNamesCached : 1;
        Field(BYTE) canMergeWithBodyScope : 1;
        Field(BYTE) hasLocalInClosure : 1;

        Field(Scope *) scope;
        Field(int) scopeId;
        Field(int) symbolCount;                // symbol count in this scope
        Field(SymbolInfo) symbols[];           // symbol PropertyIDs, index == sym.scopeSlot

    private:
        ScopeInfo(FunctionBody * parent, int symbolCount)
            : parent(parent), funcExprScopeInfo(nullptr), paramScopeInfo(nullptr), symbolCount(symbolCount), scope(nullptr), areNamesCached(false), canMergeWithBodyScope(true), hasLocalInClosure(false)
        {
        }

        void SetFuncExprScopeInfo(ScopeInfo* funcExprScopeInfo)
        {
            this->funcExprScopeInfo = funcExprScopeInfo;
        }

        void SetParamScopeInfo(ScopeInfo* paramScopeInfo)
        {
            this->paramScopeInfo = paramScopeInfo;
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

        static ScopeInfo* FromParent(FunctionBody* parent);
        static ScopeInfo* FromScope(ByteCodeGenerator* byteCodeGenerator, FunctionBody* parent, Scope* scope, ScriptContext *scriptContext);
        static void SaveParentScopeInfo(FuncInfo* parentFunc, FuncInfo* func);
        static void SaveScopeInfo(ByteCodeGenerator* byteCodeGenerator, FuncInfo* parentFunc, FuncInfo* func);

    public:
        FunctionBody * GetParent() const
        {
            return parent;
        }

        ScopeInfo* GetParentScopeInfo() const
        {
            return parent->GetScopeInfo();
        }

        ScopeInfo* GetFuncExprScopeInfo() const
        {
            return funcExprScopeInfo;
        }

        ScopeInfo* GetParamScopeInfo() const
        {
            return paramScopeInfo;
        }

        void SetScope(Scope *scope)
        {
            this->scope = scope;
        }

        Scope * GetScope() const
        {
            return scope;
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

        bool IsGlobalEval() const
        {
            return isGlobalEval;
        }

        bool GetCanMergeWithBodyScope() const
        {
            return canMergeWithBodyScope;
        }

        void SetHasLocalInClosure(bool has)
        {
            hasLocalInClosure = has;
        }

        bool GetHasOwnLocalInClosure() const
        {
            return hasLocalInClosure;
        }

        static void SaveScopeInfoForDeferParse(ByteCodeGenerator* byteCodeGenerator, FuncInfo* parentFunc, FuncInfo* func);

        void EnsurePidTracking(ScriptContext* scriptContext);

        void GetScopeInfo(Parser *parser, ByteCodeGenerator* byteCodeGenerator, FuncInfo* funcInfo, Scope* scope);

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
