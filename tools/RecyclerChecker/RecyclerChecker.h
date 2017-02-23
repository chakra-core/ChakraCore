//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <unordered_set>
#include <queue>
#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Rewrite/Core/Rewriter.h"
#include "clang/Sema/Sema.h"
#include "Helpers.h"

using namespace std;
using namespace clang;

// To record seen allocators with a Type
//
enum AllocationTypes
{
    Unknown = 0x0,                  // e.g. template dependent
    NonRecycler = 0x1,              // Heap, Arena, JitArena, ...
    Recycler = 0x2,                 // Recycler
    WriteBarrier = 0x4,             // Recycler write barrier

    RecyclerWriteBarrier = Recycler | WriteBarrier,
};

class MainVisitor:
    public RecursiveASTVisitor<MainVisitor>
{
private:
    CompilerInstance& _compilerInstance;
    ASTContext& _context;
    Rewriter _rewriter;
    bool _fix;      // whether user requested to fix missing annotations
    bool _fixed;    // whether this plugin committed any annotation fixes

    bool _barrierTypeDefined;
    map<string, set<string>> _allocatorTypeMap;
    set<string> _pointerClasses;
    set<string> _barrieredClasses;

    map<const Type*, int> _allocationTypes;     // {type -> AllocationTypes}

public:
    MainVisitor(CompilerInstance& compilerInstance, ASTContext& context, bool fix);

    const ASTContext& getContext() const { return _context; }

    bool VisitCXXRecordDecl(CXXRecordDecl* recordDecl);
    bool VisitFunctionDecl(FunctionDecl* functionDecl);

    void RecordAllocation(QualType qtype, AllocationTypes allocationType);
    void RecordRecyclerAllocation(
        const string& allocationFunction, const string& type);
    void Inspect();
    bool ApplyFix();

private:
    template <class Set, class DumpItemFunc>
    void dump(const char* name, const Set& set, const DumpItemFunc& func);
    template <class Item>
    void dump(const char* name, const set<Item>& set);
    void dump(const char* name, const unordered_set<const Type*> set);

    template <class PushFieldType>
    void ProcessUnbarrieredFields(CXXRecordDecl* recordDecl, const PushFieldType& pushFieldType);

    bool MatchType(const string& type, const char* source, const char** pSourceEnd);
    const char* GetFieldTypeAnnotation(QualType qtype);
};

class CheckAllocationsInFunctionVisitor:
    public RecursiveASTVisitor<CheckAllocationsInFunctionVisitor>
{
public:
    CheckAllocationsInFunctionVisitor(
            MainVisitor* mainVisitor, FunctionDecl* functionDecl)
        : _mainVisitor(mainVisitor), _functionDecl(functionDecl)
    {}

    bool VisitCXXNewExpr(CXXNewExpr* newExpression);
    bool VisitCallExpr(CallExpr* callExpr);

private:
    MainVisitor* _mainVisitor;
    FunctionDecl* _functionDecl;

    template <class A0, class A1, class T>
    void VisitAllocate(const A0& getArg0, const A1& getArg1, const T& getAllocType);
};

class RecyclerCheckerConsumer: public ASTConsumer
{
private:
    CompilerInstance& _compilerInstance;
    bool _fix;  // whether user requested to fix missing annotations

public:
    RecyclerCheckerConsumer(CompilerInstance& compilerInstance, bool fix)
        : _compilerInstance(compilerInstance), _fix(fix)
    {}

    void HandleTranslationUnit(ASTContext& context);
};

class RecyclerCheckerAction: public PluginASTAction
{
private:
    bool _fix;  // whether user requested to fix missing annotations

public:
    RecyclerCheckerAction() : _fix(false) {}

protected:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance& compilerInstance, llvm::StringRef) override;

    bool ParseArgs(
        const CompilerInstance& compilerInstance,
        const std::vector<std::string>& args) override;
};
