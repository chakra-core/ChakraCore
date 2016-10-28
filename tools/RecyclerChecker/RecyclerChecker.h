//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "Helpers.h"

using namespace std;
using namespace clang;

class MainVisitor:
    public RecursiveASTVisitor<MainVisitor>
{
private:
    bool _barrierTypeDefined;
    map<string, set<string>> _allocatorTypeMap;
    set<string> _pointerClasses;
    set<string> _barrieredClasses;
    set<const Type*> _barrierAllocations;

public:
    MainVisitor(): _barrierTypeDefined(false) {}

    bool VisitCXXRecordDecl(CXXRecordDecl* recordDecl);
    bool VisitFunctionDecl(FunctionDecl* functionDecl);

    void RecordWriteBarrierAllocation(const QualType& allocationType);
    void RecordRecyclerAllocation(
        const string& allocationFunction, const string& type);

    void Inspect();

private:
    template <class Set, class DumpItemFunc>
    void dump(const char* name, const Set& set, const DumpItemFunc& func);

    template <class Item>
    void dump(const char* name, const set<Item>& set);

    void dump(const char* name, const set<const Type*> set);
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

private:
    MainVisitor* _mainVisitor;
    FunctionDecl* _functionDecl;
};

class RecyclerCheckerConsumer: public ASTConsumer
{
public:
    RecyclerCheckerConsumer(CompilerInstance& compilerInstance) {}

    void HandleTranslationUnit(ASTContext& context);
};

class RecyclerCheckerAction: public PluginASTAction
{
protected:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(
        CompilerInstance& compilerInstance, llvm::StringRef) override;

    bool ParseArgs(
        const CompilerInstance& compilerInstance,
        const std::vector<std::string>& args) override;
};
