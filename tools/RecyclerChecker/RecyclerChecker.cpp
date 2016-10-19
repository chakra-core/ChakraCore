//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "clang/Frontend/FrontendPluginRegistry.h"
#include "clang/AST/AST.h"
#include "clang/AST/ASTConsumer.h"
#include "clang/AST/RecursiveASTVisitor.h"
#include "clang/Frontend/CompilerInstance.h"
#include "clang/Sema/Sema.h"
#include "llvm/Support/raw_ostream.h"

using namespace clang;
using namespace std;


namespace
{

class MainVisitor;
class CheckAllocationsInFunctionVisitor :
        public RecursiveASTVisitor<CheckAllocationsInFunctionVisitor>
{
public:
    CheckAllocationsInFunctionVisitor(MainVisitor* mainVisitor, FunctionDecl* functionDecl):
        _mainVisitor(mainVisitor),
        _functionDecl(functionDecl)
    {
    }

    bool VisitCXXNewExpr(CXXNewExpr* newExpression);

private:
    MainVisitor* _mainVisitor;
    FunctionDecl* _functionDecl;
};

class MainVisitor:
        public RecursiveASTVisitor<MainVisitor>
{
public:
    bool VisitCXXRecordDecl(CXXRecordDecl* recordDecl)
    {
        std::string typeName = recordDecl->getQualifiedNameAsString();

        // Ignore (system/non-GC types) before seeing "Memory::NoWriteBarrierField"
        if (!_barrierTypeDefined)
        {
            if (typeName != "Memory::NoWriteBarrierField")
            {
                return true;
            }

            _barrierTypeDefined = true;
        }

        if (!recordDecl->hasDefinition())
        {
            return true;
        }

        bool hasUnbarrieredPointer = false;
        const FieldDecl* unbarrieredPointerField = nullptr;
        bool hasBarrieredField = false;

        for (auto field : recordDecl->fields())
        {
            const QualType qualType = field->getType();
            const Type* type = qualType.getTypePtr();

            if (type->isPointerType())
            {
                hasUnbarrieredPointer = true;
                if (!unbarrieredPointerField)
                {
                    unbarrieredPointerField = field;
                }
            }
            else
            {
                auto fieldTypeName = qualType.getAsString();
                if (fieldTypeName.find("WriteBarrierPtr") == 0)  // starts with
                {
                    hasBarrieredField = true;
                }
                else if (type->isCompoundType())
                {
                    // If the field is a compound type,
                    // check if it is a fully barriered type or
                    // has unprotected pointer fields
                    if (_pointerClasses.find(fieldTypeName) != _pointerClasses.end())
                    {
                        hasUnbarrieredPointer = true;
                        if (!unbarrieredPointerField)
                            unbarrieredPointerField = field;
                    }
                    else if (_barrieredClasses.find(fieldTypeName) != _barrieredClasses.end())
                    {
                        hasBarrieredField = true;
                    }
                }
            }
        }

        if (hasUnbarrieredPointer)
        {
            _pointerClasses.insert(typeName);
        }

        if (hasBarrieredField)
        {
            llvm::outs() << "Type with barrier found: \"" << typeName << "\"\n";
            if (!hasUnbarrieredPointer)
            {
                _barrieredClasses.insert(typeName);
            }
            else
            {
                llvm::outs() << "ERROR: Unbarriered field: \""
                             << unbarrieredPointerField->getQualifiedNameAsString()
                             << "\"\n";
            }
        }

        return true;
    }

    bool VisitFunctionDecl(FunctionDecl* functionDecl)
    {
        if (functionDecl->hasBody())
        {
            CheckAllocationsInFunctionVisitor visitor(this, functionDecl);

            visitor.TraverseDecl(functionDecl);
        }

        return true;
    }

    void RecordWriteBarrierAllocation(const QualType& allocationType)
    {
        _barrierAllocations.insert(allocationType.getTypePtr());
    }

    void RecordRecyclerAllocation(const string& allocationFunction, const string& type)
    {
        _allocatorTypeMap[allocationFunction].insert(type);
    }

    MainVisitor():
        _barrierTypeDefined(false)
    {
    }

#define Dump(coll) \
    dump(#coll, _##coll)

    void Inspect()
    {
        Dump(pointerClasses);
        Dump(barrieredClasses);

        llvm::outs() << "Recycler allocations\n";
        for (auto item : _allocatorTypeMap)
        {
            dump(item.first.c_str(), item.second);
        }

        Dump(barrierAllocations);
    }

private:
    template <class Set, class DumpItemFunc>
    void dump(const char* name, const Set& set, const DumpItemFunc& func)
    {
        llvm::outs() << "-------------------------\n\n";
        llvm::outs() << name << "\n";
        llvm::outs() << "-------------------------\n\n";
        for (auto item : set)
        {
            func(llvm::outs(), item);
        }
        llvm::outs() << "-------------------------\n\n";
    }

    template <class Item>
    void dump(const char* name, const set<Item>& set)
    {
        dump(name, set, [](raw_ostream& out, const Item& item)
        {
            out << "  " << item << "\n";
        });
    }

    void dump(const char* name, const set<const Type*> set)
    {
        dump(name, set, [this](raw_ostream& out, const Type* type)
        {
            out << "  " << QualType(type, 0).getAsString() << "\n";

            // Examine underlying "canonical" type (strip typedef) and try to
            // match declaration. If we find it in _pointerClasses, the type
            // is missing write-barrier decorations.
            auto r = type->getCanonicalTypeInternal()->getAsCXXRecordDecl();
            if (r)
            {
                auto typeName = r->getQualifiedNameAsString();
                if (_pointerClasses.find(typeName) != _pointerClasses.end())
                {
                    out << "    ERROR: Unbarried type " << typeName << "\n";
                }
            }
        });
    }

    bool _barrierTypeDefined;
    map<string, set<string> > _allocatorTypeMap;
    set<string> _pointerClasses;
    set<string> _barrieredClasses;
    set<const Type*> _barrierAllocations;
};

static bool isCastToRecycler(const CXXStaticCastExpr* castNode)
{
    // Not a cast
    if (castNode == nullptr)
    {
        return false;
    }

    QualType targetType = castNode->getTypeAsWritten();
    if (const IdentifierInfo* info = targetType.getBaseTypeIdentifier())
    {
        return info->getName().equals("Recycler");
    }
    else
    {
        printf("Can't get base type identifier");
        return false;
    }
}

bool CheckAllocationsInFunctionVisitor::VisitCXXNewExpr(CXXNewExpr* newExpr)
{
    if (newExpr->getNumPlacementArgs() > 1)
    {
        const Expr* firstArgNode = newExpr->getPlacementArg(0);

        // Check if the first argument to new is a static cast
        // AllocatorNew in Chakra always does a static_cast to the AllocatorType
        CXXStaticCastExpr* castNode = nullptr;
        if (firstArgNode != nullptr &&
            (castNode = const_cast<CXXStaticCastExpr*>(dyn_cast<CXXStaticCastExpr>(firstArgNode))))
        {
            if (isCastToRecycler(castNode))
            {
                const Expr* secondArgNode = newExpr->getPlacementArg(1);

                // Chakra has two types of allocating functions- throwing and non-throwing
                // However, recycler allocations are always throwing, so the second parameter
                // should be the address of the allocator function
                auto unaryNode = cast<UnaryOperator>(secondArgNode);
                if (unaryNode != nullptr && unaryNode->getOpcode() == UnaryOperatorKind::UO_AddrOf)
                {
                    Expr* subExpr = unaryNode->getSubExpr();
                    if (DeclRefExpr* declRef = cast<DeclRefExpr>(subExpr))
                    {
                        QualType allocatedType = newExpr->getAllocatedType();
                        string allocatedTypeStr = allocatedType.getAsString();
                        auto declNameInfo = declRef->getNameInfo();
                        auto allocationFunctionStr = declNameInfo.getName().getAsString();
                        _mainVisitor->RecordRecyclerAllocation(allocationFunctionStr, allocatedTypeStr);

                        if (allocationFunctionStr.find("WithBarrier") != string::npos)
                        {
                            llvm::outs() << "In \"" << _functionDecl->getQualifiedNameAsString() << "\"\n";
                            llvm::outs() << "  Allocating \"" << allocatedTypeStr << "\" in write barriered memory\n";
                            _mainVisitor->RecordWriteBarrierAllocation(allocatedType);
                        }
                    }
                    else
                    {
                        llvm::errs()<<"Expected DeclRefExpr:\n";
                        subExpr->dump();
                    }
                }
                else
                {
                    llvm::errs()<<"Expected unary node:\n";
                    secondArgNode->dump();
                }
            }
        }
    }

    return true;
}

class RecyclerCheckerConsumer : public ASTConsumer
{
public:
    RecyclerCheckerConsumer(CompilerInstance& compilerInstance):
        _instance(compilerInstance)
    {
    }

    void HandleTranslationUnit(ASTContext& context)
    {
        MainVisitor mainVisitor;
        mainVisitor.TraverseDecl(context.getTranslationUnitDecl());

        mainVisitor.Inspect();
    }

private:
    CompilerInstance& _instance;
};

class RecyclerCheckerAction : public PluginASTAction {
protected:
    std::unique_ptr<ASTConsumer> CreateASTConsumer(CompilerInstance& compilerInstance, llvm::StringRef) override
    {
        return llvm::make_unique<RecyclerCheckerConsumer>(compilerInstance);
    }

    bool ParseArgs(const CompilerInstance& compilerInstance,
        const std::vector<std::string>& args) override
    {
        for (unsigned i = 0; i < args.size(); i++)
        {
            llvm::errs()<<"RecyclerChecker arg = "<<args[i]<<"\n";
        }

        if (!args.empty() && args[0] == "help")
        {
            llvm::errs()<<"Help for RecyclerChecker plugin goes here\n";
        }

        return true;
    }
};

}

static FrontendPluginRegistry::Add<RecyclerCheckerAction> recyclerPlugin("check-recycler", "Checks the recycler allocations");
