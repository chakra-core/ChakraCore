//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RecyclerChecker.h"

bool MainVisitor::VisitCXXRecordDecl(CXXRecordDecl* recordDecl)
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
            if (StartsWith(fieldTypeName, "typename WriteBarrierFieldTypeTraits"))
            {
                // Note this only indicates the class is write-barrier annotated
                hasBarrieredField = true;
            }
            else if (type->isCompoundType())
            {
                // If the field is a compound type,
                // check if it is a fully barriered type or
                // has unprotected pointer fields
                if (Contains(_pointerClasses, fieldTypeName))
                {
                    hasUnbarrieredPointer = true;
                    if (!unbarrieredPointerField)
                    {
                        unbarrieredPointerField = field;
                    }
                }
                else if (Contains(_barrieredClasses, fieldTypeName))
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
        Log::outs() << "Type with barrier found: \"" << typeName << "\"\n";
        if (!hasUnbarrieredPointer)
        {
            _barrieredClasses.insert(typeName);
        }
        else
        {
            Log::errs() << "ERROR: Unbarriered field: \""
                        << unbarrieredPointerField->getQualifiedNameAsString()
                        << "\"\n";
        }
    }

    return true;
}

bool MainVisitor::VisitFunctionDecl(FunctionDecl* functionDecl)
{
    if (functionDecl->hasBody())
    {
        CheckAllocationsInFunctionVisitor visitor(this, functionDecl);

        visitor.TraverseDecl(functionDecl);
    }

    return true;
}

void MainVisitor::RecordWriteBarrierAllocation(const QualType& allocationType)
{
    _barrierAllocations.insert(allocationType.getTypePtr());
}

void MainVisitor::RecordRecyclerAllocation(const string& allocationFunction, const string& type)
{
    _allocatorTypeMap[allocationFunction].insert(type);
}

template <class Set, class DumpItemFunc>
void MainVisitor::dump(const char* name, const Set& set, const DumpItemFunc& func)
{
    Log::outs() << "-------------------------\n\n";
    Log::outs() << name << "\n";
    Log::outs() << "-------------------------\n\n";
    for (auto item : set)
    {
        func(Log::outs(), item);
    }
    Log::outs() << "-------------------------\n\n";
}

template <class Item>
void MainVisitor::dump(const char* name, const set<Item>& set)
{
    dump(name, set, [](raw_ostream& out, const Item& item)
    {
        out << "  " << item << "\n";
    });
}

void MainVisitor::dump(const char* name, const set<const Type*> set)
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
            if (Contains(_pointerClasses, typeName))
            {
                Log::errs() << "ERROR: Unbarried type " << typeName << "\n";
            }
        }
    });
}

void MainVisitor::Inspect()
{
#define Dump(coll) dump(#coll, _##coll)
    Dump(pointerClasses);
    Dump(barrieredClasses);

    Log::outs() << "Recycler allocations\n";
    for (auto item : _allocatorTypeMap)
    {
        dump(item.first.c_str(), item.second);
    }

    Dump(barrierAllocations);
#undef Dump
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
        return info->getName().equals("Recycler");
    }
    else
    {
        Log::errs() << "ERROR (plugin): Can't get base type identifier\n";
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
                            Log::outs() << "In \"" << _functionDecl->getQualifiedNameAsString() << "\"\n";
                            Log::outs() << "  Allocating \"" << allocatedTypeStr << "\" in write barriered memory\n";
                            _mainVisitor->RecordWriteBarrierAllocation(allocatedType);
                        }
                    }
                    else
                    {
                        Log::errs() << "Expected DeclRefExpr:\n";
                        subExpr->dump();
                    }
                }
                else
                {
                    Log::errs() << "Expected unary node:\n";
                    secondArgNode->dump();
                }
            }
        }
    }

    return true;
}

void RecyclerCheckerConsumer::HandleTranslationUnit(ASTContext& context)
{
    MainVisitor mainVisitor;
    mainVisitor.TraverseDecl(context.getTranslationUnitDecl());

    mainVisitor.Inspect();
}

std::unique_ptr<ASTConsumer> RecyclerCheckerAction::CreateASTConsumer(
    CompilerInstance& compilerInstance, llvm::StringRef)
{
    return llvm::make_unique<RecyclerCheckerConsumer>(compilerInstance);
}

bool RecyclerCheckerAction::ParseArgs(
    const CompilerInstance& compilerInstance, const std::vector<std::string>& args)
{
    for (auto i = args.begin(); i != args.end(); i++)
    {
        if (*i == "-verbose")
        {
            Log::SetLevel(Log::LogLevel::Verbose);
        }
        else
        {
            Log::errs()
                << "ERROR: Unrecognized check-recycler option: " << *i << "\n"
                << "Supported options:\n"
                << "  -verbose      Log verbose messages\n";
            return false;
        }
    }

    return true;
}

static FrontendPluginRegistry::Add<RecyclerCheckerAction> recyclerPlugin(
    "check-recycler", "Checks the recycler allocations");
