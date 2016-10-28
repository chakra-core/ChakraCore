//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//===----------------------------------------------------------------------===//
//
// Defines a checker for proper use of Recycler Write Barrier functionality
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/StringSwitch.h"
#include "clang/StaticAnalyzer/Core/BugReporter/BugType.h"
#include "clang/StaticAnalyzer/Core/Checker.h"
#include "clang/StaticAnalyzer/Core/CheckerRegistry.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CallEvent.h"
#include "clang/StaticAnalyzer/Core/PathSensitive/CheckerContext.h"

#include <cassert>

using namespace clang;
using namespace ento;

class RecyclerChecker
    : public Checker<check::PostStmt<CXXNewExpr> > {
    std::unique_ptr<BugType> InvalidAllocatorBugType;

public:
    RecyclerChecker();

    void checkPostStmt(const CXXNewExpr *newExpr, CheckerContext& ctx) const;
};

static BugType* createRecyclerCheckerError(StringRef name) {
  return new BugType(
      new CheckerBase(),
      name, "Recycler Checker Error");
}

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
        //printf("Cast to %s\n", info->getName().str().c_str());

        return info->getName().equals("Recycler");
    }
    else
    {
        printf("Can't get base type identifier");
        return false;
    }
}

RecyclerChecker::RecyclerChecker()
{
  InvalidAllocatorBugType.reset(createRecyclerCheckerError("Invalid type"));
}

void RecyclerChecker::checkPostStmt(const CXXNewExpr* newExpr, CheckerContext& ctx) const
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
            //printf("Expr is %s\n", firstArgNode->getStmtClassName());

            if (isCastToRecycler(castNode))
            {
                //printf("Recycler allocation found\n");
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
                        auto declNameInfo = declRef->getNameInfo();
                        printf("AllocFunc: %s\n", declNameInfo.getName().getAsString().c_str());

                        if (const IdentifierInfo* info = newExpr->getAllocatedType().getBaseTypeIdentifier())
                        {
                            printf("Type: %s\n", info->getName().str().c_str());
                        }
                        else
                        {
                            printf("Can't get base type identifier\n");
                        }
                    }
                    else
                    {
                        printf("Expected DeclRefExpr:\n");
                        subExpr->dump();
                    }
                }
                else
                {
                    printf("Expected unary node:\n");
                    secondArgNode->dump();
                }
                //printf("-------------------\n");
            }
/*
            else
            {
                printf("Not a cast node? 0x%x\n", castNode);
                firstArgNode->dump();
                printf("\n");
            }
*/
        }
    }
}

static void initRecyclerChecker(CheckerManager &mgr) {
    mgr.registerChecker<RecyclerChecker>();
}


extern "C" void clang_registerCheckers(CheckerRegistry &registry) {
    registry.addChecker(initRecyclerChecker, "chakra.RecyclerChecker",
        "Check Recycler allocator usage");
}

extern "C" const char clang_analyzerAPIVersionString[] =
    CLANG_ANALYZER_API_VERSION_STRING;
