//===- RecyclerChecker.cpp ---------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// Example clang plugin which simply prints the names of all the top-level decls
// in the input file.
//
//===----------------------------------------------------------------------===//

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
        
        if (!_barrierTypeDefined)
        {
            if (typeName == "Memory::NoWriteBarrierField")
            {
                _barrierTypeDefined = true;
            }
            else
            {
                return true;
            }
        }
        
        if (recordDecl->hasDefinition())
        {
            bool hasUnbarrieredPointer = false;
            const FieldDecl* unbarrieredPointerField = nullptr;
            bool hasBarrieredField = false;
            
            for (auto field : recordDecl->fields())
            {
                const QualType   qualType = field->getType();
                const Type*      type = qualType.getTypePtr();
                
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
                    if (fieldTypeName.find("WriteBarrierPtr") == 0)
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
                llvm::outs()<<"Type with barrier found: \""<<typeName<<"\"\n";
                if (!hasUnbarrieredPointer)
                {
                    _barrieredClasses.insert(typeName);
                }
                else
                {
                    llvm::outs()<<"Unbarriered field: \""<<unbarrieredPointerField->getQualifiedNameAsString()<<"\"\n";
                }
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

    void RecordWriteBarrierAllocation(const string& allocationType)
    {
        _barrierAllocations.insert(allocationType);
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
    dump(_##coll, #coll)
    
    void Inspect()
    {
        Dump(pointerClasses);
        Dump(barrieredClasses);

        llvm::outs()<<"Recycler allocations\n";
        for (auto item : _allocatorTypeMap)
        {
            dump(item.second, item.first.c_str());
        }
        
        Dump(barrierAllocations);
    }
    
private:
    void dump(const set<string>& set, const char* name)
    {
        llvm::outs()<<"-------------------------\n\n";
        llvm::outs()<<name<<"\n";
        llvm::outs()<<"-------------------------\n\n";
        for (auto item : set)
        {
            llvm::outs()<<"  "<<item<<"\n";
        }
        llvm::outs()<<"-------------------------\n\n";
    } 
    
    bool _barrierTypeDefined;
    map<string, set<string> > _allocatorTypeMap;
    set<string> _pointerClasses;
    set<string> _barrieredClasses;
    set<string> _barrierAllocations;
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
                            llvm::outs()<<"In \""<<_functionDecl->getQualifiedNameAsString()<<"\"\n";
                            llvm::outs()<<"  Allocating \""<<allocatedTypeStr<<"\" in write barriered memory\n";
                            _mainVisitor->RecordWriteBarrierAllocation(allocatedTypeStr);
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
