//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
// Portions of this file are copyright 2014 Mozilla Foundation, available under the Apache 2.0 license.
//-------------------------------------------------------------------------------------------------------


//-------------------------------------------------------------------------------------------------------
// Copyright 2014 Mozilla Foundation
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http ://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLanguagePch.h"

#ifdef ASMJS_PLAT
#include "ByteCode/ByteCodeWriter.h"
#include "ByteCode/AsmJsByteCodeWriter.h"
#include "Language/AsmJsByteCodeGenerator.h"

namespace Js
{
    const char16 * AsmJsType::toChars() const
    {
        switch (which_)
        {
        case Double:      return _u("double");
        case MaybeDouble: return _u("double?");
        case DoubleLit:   return _u("doublelit");
        case Float:       return _u("float");
        case Floatish:    return _u("floatish");
        case FloatishDoubleLit: return _u("FloatishDoubleLit");
        case MaybeFloat:  return _u("float?");
        case Fixnum:      return _u("fixnum");
        case Int:         return _u("int");
        case Signed:      return _u("signed");
        case Unsigned:    return _u("unsigned");
        case Intish:      return _u("intish");
        case Void:        return _u("void");
        case Int32x4:     return _u("SIMD.Int32x4");
        case Int64x2:     return _u("SIMD.Int64x2");
        case Bool32x4:    return _u("SIMD.Bool32x4");
        case Bool16x8:    return _u("SIMD.Bool16x8");
        case Bool8x16:    return _u("SIMD.Bool8x16");
        case Float32x4:   return _u("SIMD.Float32x4");
        case Float64x2:   return _u("SIMD.Float64x2");
        case Int16x8:     return _u("SIMD.Int16x8");
        case Int8x16:     return _u("SIMD.Int8x16");
        case Uint32x4:    return _u("SIMD.Uint32x4");
        case Uint16x8:    return _u("SIMD.Uint16x8");
        case Uint8x16:    return _u("SIMD.Uint8x16");
        }
        Assert(false);
        return _u("none");
    }

    bool AsmJsType::isSIMDType() const
    {
        return isSIMDInt32x4()  || isSIMDInt16x8()   || isSIMDInt8x16()   ||
               isSIMDBool32x4() || isSIMDBool16x8()  || isSIMDBool8x16()  ||
               isSIMDUint32x4() || isSIMDUint16x8()  || isSIMDUint8x16()  ||
               isSIMDFloat32x4()|| isSIMDFloat64x2();
    }

    bool AsmJsType::isSIMDInt32x4() const
    {
        return which_ == Int32x4;
    }
    bool AsmJsType::isSIMDBool32x4() const
    {
        return which_ == Bool32x4;
    }
    bool AsmJsType::isSIMDBool16x8() const
    {
        return which_ == Bool16x8;
    }
    bool AsmJsType::isSIMDBool8x16() const
    {
        return which_ == Bool8x16;
    }
    bool AsmJsType::isSIMDInt16x8() const
    {
        return which_ == Int16x8;
    }
    bool AsmJsType::isSIMDInt8x16() const
    {
        return which_ == Int8x16;
    }
    bool AsmJsType::isSIMDFloat32x4() const
    {
        return which_ == Float32x4;
    }
    bool AsmJsType::isSIMDFloat64x2() const
    {
        return which_ == Float64x2;
    }
    bool AsmJsType::isSIMDInt64x2() const
    {
        return which_ == Int64x2;
    }

    bool AsmJsType::isSIMDUint32x4() const
    {
        return which_ == Uint32x4;
    }
    bool AsmJsType::isSIMDUint16x8() const
    {
        return which_ == Uint16x8;
    }
    bool AsmJsType::isSIMDUint8x16() const
    {
        return which_ == Uint8x16;
    }

    bool AsmJsType::isVarAsmJsType() const
    {
        return isInt() || isMaybeDouble() || isMaybeFloat();
    }

    bool AsmJsType::isExtern() const
    {
        return isDouble() || isSigned();
    }

    bool AsmJsType::isVoid() const
    {
        return which_ == Void;
    }

    bool AsmJsType::isFloatish() const
    {
        return isMaybeFloat() || which_ == Floatish;
    }

    bool AsmJsType::isFloatishDoubleLit() const
    {
        return isFloatish() || isDoubleLit();
    }

    bool AsmJsType::isMaybeFloat() const
    {
        return isFloat() || which_ == MaybeFloat;
    }

    bool AsmJsType::isFloat() const
    {
        return which_ == Float;
    }

    bool AsmJsType::isMaybeDouble() const
    {
        return isDouble() || which_ == MaybeDouble;
    }

    bool AsmJsType::isDouble() const
    {
        return isDoubleLit() || which_ == Double;
    }

    bool AsmJsType::isDoubleLit() const
    {
        return which_ == DoubleLit;
    }

    bool AsmJsType::isIntish() const
    {
        return isInt() || which_ == Intish;
    }

    bool AsmJsType::isInt() const
    {
        return isSigned() || isUnsigned() || which_ == Int;
    }

    bool AsmJsType::isUnsigned() const
    {
        return which_ == Unsigned || which_ == Fixnum;
    }

    bool AsmJsType::isSigned() const
    {
        return which_ == Signed || which_ == Fixnum;
    }

    bool AsmJsType::operator!=(AsmJsType rhs) const
    {
        return which_ != rhs.which_;
    }

    bool AsmJsType::operator==(AsmJsType rhs) const
    {
        return which_ == rhs.which_;
    }

    bool AsmJsType::isSubType(AsmJsType type) const
    {
        switch (type.which_)
        {
        case Js::AsmJsType::Double:
            return isDouble();
            break;

        case Js::AsmJsType::MaybeDouble:
            return isMaybeDouble();
            break;
        case Js::AsmJsType::DoubleLit:
            return isDoubleLit();
            break;
        case Js::AsmJsType::Float:
            return isFloat();
            break;
        case Js::AsmJsType::MaybeFloat:
            return isMaybeFloat();
            break;
        case Js::AsmJsType::Floatish:
            return isFloatish();
            break;
        case Js::AsmJsType::FloatishDoubleLit:
            return isFloatishDoubleLit();
            break;
        case Js::AsmJsType::Fixnum:
            return which_ == Fixnum;
            break;
        case Js::AsmJsType::Int:
            return isInt();
            break;
        case Js::AsmJsType::Signed:
            return isSigned();
            break;
        case Js::AsmJsType::Unsigned:
            return isUnsigned();
            break;
        case Js::AsmJsType::Intish:
            return isIntish();
            break;
        case Js::AsmJsType::Void:
            return isVoid();
            break;
        default:
            break;
        }
        return false;
    }

    bool AsmJsType::isSuperType(AsmJsType type) const
    {
        return type.isSubType(which_);
    }

    Js::AsmJsRetType AsmJsType::toRetType() const
    {
        Which w = which_;
        // DoubleLit is for expressions only.
        if (w == DoubleLit)
        {
            w = Double;
        }
        return AsmJsRetType::Which(w);
    }

    /// RetType

    bool AsmJsRetType::operator!=(AsmJsRetType rhs) const
    {
        return which_ != rhs.which_;
    }

    bool AsmJsRetType::operator==(AsmJsRetType rhs) const
    {
        return which_ == rhs.which_;
    }

    Js::AsmJsType AsmJsRetType::toType() const
    {
        return AsmJsType::Which(which_);
    }

    Js::AsmJsVarType AsmJsRetType::toVarType() const
    {
        return AsmJsVarType::Which(which_);
    }

    Js::AsmJsRetType::Which AsmJsRetType::which() const
    {
        return which_;
    }

    AsmJsRetType::AsmJsRetType(AsmJSCoercion coercion)
    {
        switch (coercion)
        {
        case AsmJS_ToInt32: which_   = Signed; break;
        case AsmJS_ToNumber: which_  = Double; break;
        case AsmJS_FRound: which_    = Float; break;
#ifdef ENABLE_WASM_SIMD
        case AsmJS_Int32x4: which_ = Int32x4; break;
        case AsmJS_Bool32x4: which_ = Bool32x4; break;
        case AsmJS_Bool16x8: which_ = Bool16x8; break;
        case AsmJS_Bool8x16: which_ = Bool8x16; break;
        case AsmJS_Float32x4: which_ = Float32x4; break;
        case AsmJS_Float64x2: which_ = Float64x2; break;
        case AsmJS_Int16x8: which_  = Int16x8; break;
        case AsmJS_Int8x16: which_  = Int8x16; break;
        case AsmJS_Uint32x4: which_ = Uint32x4; break;
        case AsmJS_Uint16x8: which_ = Uint16x8; break;
        case AsmJS_Uint8x16: which_ = Uint8x16; break;
#endif
        }
    }

    AsmJsRetType::AsmJsRetType(Which w) : which_(w)
    {

    }

    AsmJsRetType::AsmJsRetType() : which_(Which(-1))
    {

    }

    /// VarType

    bool AsmJsVarType::operator!=(AsmJsVarType rhs) const
    {
        return which_ != rhs.which_;
    }

    bool AsmJsVarType::operator==(AsmJsVarType rhs) const
    {
        return which_ == rhs.which_;
    }

    Js::AsmJsVarType AsmJsVarType::FromCheckedType(AsmJsType type)
    {
        Assert( type.isInt() || type.isMaybeDouble() || type.isFloatish() || type.isSIMDType());

        if (type.isMaybeDouble())
            return Double;
        else if (type.isFloatish())
            return Float;
        else if (type.isInt())
            return Int;
        else
        {
            // SIMD type
            return AsmJsVarType::Which(type.GetWhich());
        }

    }

    Js::AsmJSCoercion AsmJsVarType::toCoercion() const
    {
        switch (which_)
        {
        case Int:     return AsmJS_ToInt32;
        case Double:  return AsmJS_ToNumber;
        case Float:   return AsmJS_FRound;
        case Int32x4:   return AsmJS_Int32x4;
        case Bool32x4:  return AsmJS_Bool32x4;
        case Bool16x8:  return AsmJS_Bool16x8;
        case Bool8x16:  return AsmJS_Bool8x16;
        case Float32x4: return AsmJS_Float32x4;
        case Float64x2: return AsmJS_Float64x2;
        case Int16x8:   return AsmJS_Int16x8;
        case Int8x16:   return AsmJS_Int8x16;
        case Uint32x4:   return AsmJS_Uint32x4;
        case Uint16x8:   return AsmJS_Uint16x8;
        case Uint8x16:   return AsmJS_Uint8x16;
        }
        Assert(false);
        return AsmJS_ToInt32;
    }

    Js::AsmJsType AsmJsVarType::toType() const
    {
        return AsmJsType::Which(which_);
    }

    Js::AsmJsVarType::Which AsmJsVarType::which() const
    {
        return which_;
    }

    AsmJsVarType::AsmJsVarType(AsmJSCoercion coercion)
    {
        switch (coercion)
        {
        case AsmJS_ToInt32: which_ = Int; break;
        case AsmJS_ToNumber: which_ = Double; break;
        case AsmJS_FRound: which_ = Float; break;
        case AsmJS_Int32x4: which_ = Int32x4; break;
        case AsmJS_Bool32x4: which_ = Bool32x4; break;
        case AsmJS_Bool16x8: which_ = Bool16x8; break;
        case AsmJS_Bool8x16: which_ = Bool8x16; break;
        case AsmJS_Float32x4: which_ = Float32x4; break;
        case AsmJS_Float64x2: which_ = Float64x2; break;
        case AsmJS_Int16x8: which_ = Int16x8; break;
        case AsmJS_Int8x16: which_ = Int8x16; break;
        case AsmJS_Uint32x4: which_ = Uint32x4; break;
        case AsmJS_Uint16x8: which_ = Uint16x8; break;
        case AsmJS_Uint8x16: which_ = Uint8x16; break;
        }
    }

    AsmJsVarType::AsmJsVarType(Which w) : which_(w)
    {

    }

    AsmJsVarType::AsmJsVarType() : which_(Which(-1))
    {

    }

    AsmJsVarBase::AsmJsVarBase(PropertyName name, AsmJsSymbol::SymbolType type, bool isMutable /*= true*/) :
        AsmJsSymbol(name, type)
        , mType(AsmJsVarType::Double)
        , mLocation(Js::Constants::NoRegister)
        , mIsMutable(isMutable)
    {
        Assert(AsmJsVarBase::Is(this));
    }

    bool AsmJsVarBase::Is(AsmJsSymbol* sym)
    {
        return AsmJsArgument::Is(sym) || AsmJsConstantImport::Is(sym) || AsmJsVar::Is(sym);
    }


    Js::AsmJsType AsmJsModuleArg::GetType() const
    {
        return AsmJsType::Void;
    }

    Js::AsmJsType AsmJsMathConst::GetType() const
    {
        return AsmJsType::Double;
    }

    AsmJsFunctionDeclaration::AsmJsFunctionDeclaration(PropertyName name, AsmJsSymbol::SymbolType type, ArenaAllocator* allocator) :
        AsmJsSymbol(name, type)
        , mAllocator(allocator)
        , mReturnType(AsmJsRetType::Void)
        , mArgCount(Constants::InvalidArgSlot)
        , mLocation(0)
        , mReturnTypeKnown(false)
        , mArgumentsType(nullptr)
    {
        Assert(AsmJsFunctionDeclaration::Is(this));
    }

    bool AsmJsFunctionDeclaration::Is(AsmJsSymbol* sym)
    {
        return (
            AsmJsFunc::Is(sym) ||
            AsmJsFunctionTable::Is(sym) ||
            AsmJsImportFunction::Is(sym) ||
            AsmJsMathFunction::Is(sym) ||
            AsmJsTypedArrayFunction::Is(sym) ||
            AsmJsClosureFunction::Is(sym)
        );
    }

    bool AsmJsFunctionDeclaration::EnsureArgCount(ArgSlot count)
    {
        if (mArgCount == Constants::InvalidArgSlot)
        {
            SetArgCount(count);
            return true;
        }
        else
        {
            return mArgCount == count;
        }
    }

    void AsmJsFunctionDeclaration::SetArgCount(ArgSlot count )
    {
        Assert( mArgumentsType == nullptr );
        Assert(mArgCount == Constants::InvalidArgSlot);
        Assert(count != Constants::InvalidArgSlot);
        mArgCount = count;
        if( count > 0 )
        {
            mArgumentsType = AnewArrayZ( mAllocator, AsmJsType, count );
        }
    }

    AsmJsType* AsmJsFunctionDeclaration::GetArgTypeArray()
    {
        return mArgumentsType;
    }

    bool AsmJsFunctionDeclaration::CheckAndSetReturnType(Js::AsmJsRetType val)
    {
        const auto IsValid = [this](Js::AsmJsRetType val) {
            return AsmJsMathFunction::Is(this) || (
                val != AsmJsRetType::Fixnum && val != AsmJsRetType::Unsigned && val != AsmJsRetType::Floatish
            );
        };
        Assert(IsValid(val));
        if (mReturnTypeKnown)
        {
            Assert(IsValid(mReturnType));
            return mReturnType.toType().isSubType(val.toType());
        }
        mReturnType = val;
        mReturnTypeKnown = true;
        return true;
    }

    Js::AsmJsType AsmJsFunctionDeclaration::GetType() const
    {
        return mReturnType.toType();
    }

    bool AsmJsFunctionDeclaration::EnsureArgType(AsmJsVarBase* arg, ArgSlot index)
    {
        if (mArgumentsType[index].GetWhich() == -1)
        {
            SetArgType(arg, index);
            return true;
        }
        else
        {
            return mArgumentsType[index] == arg->GetType();
        }
    }

    bool AsmJsFunctionDeclaration::SupportsArgCall( ArgSlot argCount, AsmJsType* args, AsmJsRetType& retType )
    {
        // we will assume the first reference to the function is correct, until proven wrong
        if (GetArgCount() == Constants::InvalidArgSlot)
        {
            SetArgCount(argCount);

            for (ArgSlot i = 0; i < argCount; i++)
            {
                if (args[i].isSubType(AsmJsType::Double))
                {
                    mArgumentsType[i] = AsmJsType::Double;
                }
                else if (args[i].isSubType(AsmJsType::Float))
                {
                    mArgumentsType[i] = AsmJsType::Float;
                }
                else if (args[i].isSubType(AsmJsType::Int))
                {
                    mArgumentsType[i] = AsmJsType::Int;
                }
                else
                {
                    // call did not have valid argument type
                    return false;
                }
            }
            retType = mReturnType;
            return true;
        }
        else if( argCount == GetArgCount() )
        {
            for(ArgSlot i = 0; i < argCount; i++ )
            {
                if (!args[i].isSubType(mArgumentsType[i]))
                {
                    return false;
                }
            }
            retType = mReturnType;
            return true;
        }
        return false;
    }

    ArgSlot AsmJsFunctionDeclaration::GetArgByteSize(ArgSlot inArgCount) const
    {
        ArgSlot argSize = 0;
        if (AsmJsImportFunction::Is(this))
        {
            Assert(inArgCount != Constants::InvalidArgSlot);
            argSize = ArgSlotMath::Mul(inArgCount, (uint16)MachPtr);
        }
#if _M_IX86
        else
        {
            for (ArgSlot i = 0; i < GetArgCount(); i++)
            {
                if( GetArgType(i).isMaybeDouble() )
                {
                    argSize = ArgSlotMath::Add(argSize, sizeof(double));
                }
                else if (GetArgType(i).isIntish())
                {
                    argSize = ArgSlotMath::Add(argSize, sizeof(int));
                }
                else if (GetArgType(i).isFloatish())
                {
                    argSize = ArgSlotMath::Add(argSize, sizeof(float));
                }
                else
                {
                    AssertOrFailFast(UNREACHED);
                }
            }
        }
#elif _M_X64
        else
        {
            argSize = ArgSlotMath::Mul(GetArgCount(), (uint16)MachPtr);
        }
#else
        AssertOrFailFast(UNREACHED);
#endif
        return argSize;
    }

    AsmJsMathFunction::AsmJsMathFunction( PropertyName name, ArenaAllocator* allocator, ArgSlot argCount, AsmJSMathBuiltinFunction builtIn, OpCodeAsmJs op, AsmJsRetType retType, ... ) :
        AsmJsFunctionDeclaration( name, symbolType, allocator )
        , mBuiltIn( builtIn )
        , mOverload( nullptr )
        , mOpCode(op)
    {
        bool ret = CheckAndSetReturnType(retType);
        Assert(ret);
        va_list arguments;

        SetArgCount( argCount );
        va_start( arguments, retType );
        for(ArgSlot iArg = 0; iArg < argCount; iArg++)
        {
            SetArgType(static_cast<AsmJsType::Which>(va_arg(arguments, int)), iArg);
        }
        va_end(arguments);
    }

    void AsmJsMathFunction::SetOverload(AsmJsMathFunction* val)
    {
#if DBG
        AsmJsMathFunction* over = val->mOverload;
        while (over)
        {
            if (over == this)
            {
                Assert(false);
                break;
            }
            over = over->mOverload;
        }
#endif
        Assert(val->GetSymbolType() == GetSymbolType());
        if (this->mOverload)
        {
            this->mOverload->SetOverload(val);
        }
        else
        {
            mOverload = val;
        }
    }

    bool AsmJsMathFunction::CheckAndSetReturnType(Js::AsmJsRetType val)
    {
        return AsmJsFunctionDeclaration::CheckAndSetReturnType(val) || (mOverload && mOverload->CheckAndSetReturnType(val));
    }

    bool AsmJsMathFunction::SupportsArgCall(ArgSlot argCount, AsmJsType* args, AsmJsRetType& retType )
    {
        return AsmJsFunctionDeclaration::SupportsArgCall(argCount, args, retType) || (mOverload && mOverload->SupportsArgCall(argCount, args, retType));
    }

    bool AsmJsMathFunction::SupportsMathCall(ArgSlot argCount, AsmJsType* args, OpCodeAsmJs& op, AsmJsRetType& retType )
    {
        if (AsmJsFunctionDeclaration::SupportsArgCall(argCount, args, retType))
        {
            op = mOpCode;
            return true;
        }
        return mOverload && mOverload->SupportsMathCall(argCount, args, op, retType);
    }

    bool AsmJsMathFunction::IsFround(AsmJsFunctionDeclaration* sym)
    {
        return AsmJsMathFunction::Is(sym) && AsmJsMathFunction::FromSymbol(sym)->GetMathBuiltInFunction() == AsmJSMathBuiltin_fround;
    }

    WAsmJs::RegisterSpace*
        AllocateRegisterSpace(ArenaAllocator* alloc, WAsmJs::Types type)
    {
        switch(type)
        {
        case WAsmJs::INT32: return Anew(alloc, AsmJsRegisterSpace<int>, alloc);
        case WAsmJs::FLOAT32: return Anew(alloc, AsmJsRegisterSpace<float>, alloc);
        case WAsmJs::FLOAT64: return Anew(alloc, AsmJsRegisterSpace<double>, alloc);
#if TARGET_64
        case WAsmJs::INT64: return Anew(alloc, AsmJsRegisterSpace<int64>, alloc);
#endif
        default:
            AssertMsg(false, "Invalid native asm.js type");
            Js::Throw::InternalError();
        }
    }

    AsmJsFunc::AsmJsFunc(PropertyName name, ParseNodeFnc* pnodeFnc, ArenaAllocator* allocator, ScriptContext* scriptContext) :
        AsmJsFunctionDeclaration(name, AsmJsSymbol::ModuleFunction, allocator)
        , mCompileTime(0)
        , mVarMap(allocator)
        , mBodyNode(nullptr)
        , mFncNode(pnodeFnc)
        , mCurrentProfileId(0)
        , mTypedRegisterAllocator(
            allocator,
            AllocateRegisterSpace,
            1 << WAsmJs::SIMD
#if TARGET_32
            | 1 << WAsmJs::INT64
#endif
        )
        , mFuncInfo(pnodeFnc->funcInfo)
        , mFuncBody(nullptr)
        , mMaxArgOutDepth(0)
        , mDefined( false )
    {
    }

    /// AsmJsFunc
    AsmJsVarBase* AsmJsFunc::DefineVar( PropertyName name, bool isArg /*= false*/, bool isMutable /*= true*/ )
    {
        AsmJsVarBase* var = FindVar(name);
        if (var)
        {
            if (PHASE_TRACE1(AsmjsPhase))
            {
                Output::Print(_u("Variable redefinition: %s\n"), name->Psz());
            }
            return nullptr;
        }

        if (isArg)
        {
            // arg cannot be const
            Assert(isMutable);
            var = Anew( mAllocator, AsmJsArgument, name );
        }
        else
        {
            var = Anew(mAllocator, AsmJsVar, name, isMutable);
        }
        int addResult = mVarMap.AddNew(name->GetPropertyId(), var);
        if( addResult == -1 )
        {
            mAllocator->Free(var, isArg ? sizeof(AsmJsArgument) : sizeof(AsmJsVar));
            return nullptr;
        }
        return var;
    }

    ProfileId AsmJsFunc::GetNextProfileId()
    {
        ProfileId nextProfileId = mCurrentProfileId;
        UInt16Math::Inc(mCurrentProfileId);
        return nextProfileId;
    }

    AsmJsVarBase* AsmJsFunc::FindVar(const PropertyName name) const
    {
        return mVarMap.LookupWithKey(name->GetPropertyId(), nullptr);
    }

    void AsmJsFunc::ReleaseLocationGeneric(const EmitExpressionInfo* pnode)
    {
        if (pnode)
        {
            if (pnode->type.isIntish())
            {
                ReleaseLocation<int>(pnode);
            }
            else if (pnode->type.isMaybeDouble())
            {
                ReleaseLocation<double>(pnode);
            }
            else if (pnode->type.isFloatish())
            {
                ReleaseLocation<float>(pnode);
            }
        }
    }

    AsmJsSymbol* AsmJsFunc::LookupIdentifier(const PropertyName name, AsmJsLookupSource::Source* lookupSource /*= nullptr */) const
    {
        auto var = FindVar(name);
        if (var && lookupSource)
        {
            *lookupSource = AsmJsLookupSource::AsmJsFunction;
        }
        return var;
    }

    void AsmJsFunc::UpdateMaxArgOutDepth(int outParamsCount)
    {
        if (mMaxArgOutDepth < outParamsCount)
        {
            mMaxArgOutDepth = outParamsCount;
        }
    }

    bool AsmJsFunctionInfo::Init(AsmJsFunc* func)
    {
        func->CommitToFunctionInfo(this, func->GetFuncBody());

        Recycler* recycler = func->GetFuncBody()->GetScriptContext()->GetRecycler();
        mArgCount = func->GetArgCount();
        if (mArgCount > 0)
        {
            mArgType = RecyclerNewArrayLeaf(recycler, AsmJsVarType::Which, mArgCount);
        }

        // on x64, AsmJsExternalEntryPoint reads first 3 elements to figure out how to shadow args on stack
        // always alloc space for these such that we need to do less work in the entrypoint
        mArgSizesLength = max(mArgCount, 3ui16);
        mArgSizes = RecyclerNewArrayLeafZ(recycler, uint, mArgSizesLength);

        mReturnType = func->GetReturnType();
        mbyteCodeTJMap = RecyclerNew(recycler, ByteCodeToTJMap,recycler);

        for(ArgSlot i = 0; i < GetArgCount(); i++)
        {
            AsmJsType varType = func->GetArgType(i);
            SetArgType(AsmJsVarType::FromCheckedType(varType), i);
        }

        return true;
    }

    WAsmJs::TypedSlotInfo* AsmJsFunctionInfo::GetTypedSlotInfo(WAsmJs::Types type)
    {
        if ((uint32)type > WAsmJs::LIMIT)
        {
            Assert(false);
            Js::Throw::InternalError();
        }
        return &mTypedSlotInfos[type];
    }

    void AsmJsFunctionInfo::SetTotalSizeinBytes(uint32 totalSize)
    {
        AssertOrFailFast(mTotalSizeBytes == 0 && totalSize <= INT_MAX);
        mTotalSizeBytes = totalSize;
    }

    int AsmJsFunctionInfo::GetTotalSizeinBytes() const
    {
        AssertOrFailFast(mTotalSizeBytes <= INT_MAX);
        return (int)mTotalSizeBytes;
    }

    void AsmJsFunctionInfo::SetArgType(AsmJsVarType type, ArgSlot index)
    {
        Assert(mArgCount != Constants::InvalidArgSlot);
        AnalysisAssert(index < mArgCount);

        Assert(
            type.isInt() ||
            type.isInt64() ||
            type.isFloat() ||
            type.isDouble() ||
            type.isSIMD()
        );

        mArgType[index] = type.which();
        ArgSlot argSize = 0;

        if (type.isDouble())
        {
            argSize = sizeof(double);
        }
        else if (type.isSIMD())
        {
            argSize = sizeof(AsmJsSIMDValue);
        }
        else if (type.isInt64())
        {
            argSize = sizeof(int64);
        }
        else
        {
            // int and float are Js::Var
            argSize = (ArgSlot)MachPtr;
        }

        mArgByteSize = ArgSlotMath::Add(mArgByteSize, argSize);
        mArgSizes[index] = argSize;
    }

    Js::AsmJsType AsmJsArrayView::GetType() const
    {
        switch (mViewType)
        {
        case ArrayBufferView::TYPE_INT8:
        case ArrayBufferView::TYPE_INT16:
        case ArrayBufferView::TYPE_INT32:
        case ArrayBufferView::TYPE_UINT8:
        case ArrayBufferView::TYPE_UINT16:
        case ArrayBufferView::TYPE_UINT32:
            return AsmJsType::Intish;
        case ArrayBufferView::TYPE_FLOAT32:
            return AsmJsType::MaybeFloat;
        case ArrayBufferView::TYPE_FLOAT64:
            return AsmJsType::MaybeDouble;
        default:;
        }
        AssertMsg(false, "Unexpected array type");
        return AsmJsType::Intish;
    }

    bool AsmJsImportFunction::SupportsArgCall(ArgSlot argCount, AsmJsType* args, AsmJsRetType& retType )
    {
        for (ArgSlot i = 0; i < argCount ; i++)
        {
            if (!args[i].isExtern())
            {
                return false;
            }
        }
        return true;
    }

    AsmJsImportFunction::AsmJsImportFunction(PropertyName name, PropertyName field, ArenaAllocator* allocator) :
        AsmJsFunctionDeclaration(name, symbolType, allocator)
        , mField(field)
    {
        CheckAndSetReturnType(AsmJsRetType::Void);
    }


    bool AsmJsFunctionTable::SupportsArgCall(ArgSlot argCount, AsmJsType* args, AsmJsRetType& retType )
    {
        if (mAreArgumentsKnown)
        {
            return AsmJsFunctionDeclaration::SupportsArgCall(argCount, args, retType);
        }

        Assert(GetArgCount() == Constants::InvalidArgSlot);
        SetArgCount( argCount );

        retType = this->GetReturnType();

        for (ArgSlot i = 0; i < argCount ; i++)
        {
            if (args[i].isInt())
            {
                this->SetArgType(AsmJsType::Int, i);
            }
            else if (args[i].isDouble())
            {
                this->SetArgType(AsmJsType::Double, i);
            }
            else if (args[i].isFloat())
            {
                this->SetArgType(AsmJsType::Float, i);
            }
            else
            {
                // Function tables can only have int, double or float as arguments
                return false;
            }
        }
        mAreArgumentsKnown = true;
        return true;
    }
}
#endif
