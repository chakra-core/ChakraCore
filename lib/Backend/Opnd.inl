//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace IR {

///----------------------------------------------------------------------------
///
/// Opnd::Use
///
///     If this operand is not inUse, use it.  Otherwise, make a copy.
///
///----------------------------------------------------------------------------

inline Opnd *
Opnd::Use(Func *func)
{
    AssertMsg(!isDeleted, "Using deleted operand");
    if (!m_inUse)
    {
        m_inUse = true;
        return this;
    }

    Opnd * newOpnd = this->Copy(func);
    newOpnd->m_inUse = true;

    return newOpnd;
}

///----------------------------------------------------------------------------
///
/// Opnd::UnUse
///
///----------------------------------------------------------------------------

inline void
Opnd::UnUse()
{
    AssertMsg(m_inUse, "Expected inUse to be set...");
    m_inUse = false;
}

///----------------------------------------------------------------------------
///
/// Opnd::IsSymOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsSymOpnd() const
{
    return GetKind() == OpndKindSym;
}

///----------------------------------------------------------------------------
///
/// Opnd::AsSymOpnd
///
///     Use this opnd as a SymOpnd.
///
///----------------------------------------------------------------------------

inline SymOpnd *
Opnd::AsSymOpnd()
{
    AssertMsg(this->IsSymOpnd(), "Bad call to AsSymOpnd()");

    return reinterpret_cast<SymOpnd *>(this);
}

inline const SymOpnd *
Opnd::AsSymOpnd() const
{
    AssertMsg(this->IsSymOpnd(), "Bad call to AsSymOpnd() const");

    return reinterpret_cast<const SymOpnd*>(this);
}

inline PropertySymOpnd *
Opnd::AsPropertySymOpnd()
{
    AssertMsg(this->IsSymOpnd() && this->AsSymOpnd()->IsPropertySymOpnd(), "Bad call to AsPropertySymOpnd()");

    return reinterpret_cast<PropertySymOpnd *>(this);
}

inline const PropertySymOpnd *
Opnd::AsPropertySymOpnd() const
{
    AssertMsg(this->IsSymOpnd() && this->AsSymOpnd()->IsPropertySymOpnd(), "Bad call to AsPropertySymOpnd() const");

    return reinterpret_cast<const PropertySymOpnd *>(this);
}

///----------------------------------------------------------------------------
///
/// Opnd::IsRegOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsRegOpnd() const
{
    return GetKind() == OpndKindReg;
}

///----------------------------------------------------------------------------
///
/// Opnd::AsRegOpnd
///
///     Use this opnd as a RegOpnd.
///
///----------------------------------------------------------------------------

inline RegOpnd *
Opnd::AsRegOpnd()
{
    AssertMsg(this->IsRegOpnd(), "Bad call to AsRegOpnd()");

    return reinterpret_cast<RegOpnd *>(this);
}

inline const RegOpnd *
Opnd::AsRegOpnd() const
{
    AssertMsg(this->IsRegOpnd(), "Bad call to AsRegOpnd() const");

    return reinterpret_cast<const RegOpnd *>(this);
}

///----------------------------------------------------------------------------
///
/// Opnd::IsRegBVOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsRegBVOpnd() const
{
    return GetKind() == OpndKindRegBV;
}

///----------------------------------------------------------------------------
///
/// Opnd::AsRegBVOpnd
///
///     Use this opnd as a RegBVOpnd.
///
///----------------------------------------------------------------------------
inline RegBVOpnd *
Opnd::AsRegBVOpnd()
{
    AssertMsg(this->IsRegBVOpnd(), "Bad call to AsRegBVOpnd()");

    return reinterpret_cast<RegBVOpnd *>(this);
}

inline const RegBVOpnd *
Opnd::AsRegBVOpnd() const
{
    AssertMsg(this->IsRegBVOpnd(), "Bad call to AsRegBVOpnd() const");

    return reinterpret_cast<const RegBVOpnd *>(this);
}

///----------------------------------------------------------------------------
///
/// Opnd::IsIntConstOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsIntConstOpnd() const
{
    return GetKind() == OpndKindIntConst;
}

///----------------------------------------------------------------------------
///
/// Opnd::AsIntConstOpnd
///
///     Use this opnd as an IntConstOpnd.
///
///----------------------------------------------------------------------------

inline IntConstOpnd *
Opnd::AsIntConstOpnd()
{
    AssertMsg(this->IsIntConstOpnd(), "Bad call to AsIntConstOpnd()");

    return reinterpret_cast<IntConstOpnd *>(this);
}

inline const IntConstOpnd *
Opnd::AsIntConstOpnd() const
{
    AssertMsg(this->IsIntConstOpnd(), "Bad call to AsIntConstOpnd() const");

    return reinterpret_cast<const IntConstOpnd *>(this);
}

///----------------------------------------------------------------------------
///
/// Opnd::IsInt64ConstOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsInt64ConstOpnd() const
{
    return GetKind() == OpndKindInt64Const;
}

///----------------------------------------------------------------------------
///
/// Opnd::AsInt64ConstOpnd
///
///     Use this opnd as an Int64ConstOpnd.
///
///----------------------------------------------------------------------------

inline Int64ConstOpnd *
Opnd::AsInt64ConstOpnd()
{
    AssertMsg(this->IsInt64ConstOpnd(), "Bad call to AsInt64ConstOpnd()");
    return reinterpret_cast<Int64ConstOpnd *>(this);
}

inline const Int64ConstOpnd *
Opnd::AsInt64ConstOpnd() const
{
    AssertMsg(this->IsInt64ConstOpnd(), "Bad call to AsInt64ConstOpnd() const");
    return reinterpret_cast<const Int64ConstOpnd *>(this);
}


///----------------------------------------------------------------------------
///
/// Opnd::IsFloatConstOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsFloatConstOpnd() const
{
    return GetKind() == OpndKindFloatConst;
}

///----------------------------------------------------------------------------
///
/// Opnd::AsFloatConstOpnd
///
///     Use this opnd as a FloatConstOpnd.
///
///----------------------------------------------------------------------------

inline FloatConstOpnd *
Opnd::AsFloatConstOpnd()
{
    AssertMsg(this->IsFloatConstOpnd(), "Bad call to AsFloatConstOpnd()");

    return reinterpret_cast<FloatConstOpnd *>(this);
}

inline const FloatConstOpnd *
Opnd::AsFloatConstOpnd() const
{
    AssertMsg(this->IsFloatConstOpnd(), "Bad call to AsFloatConstOpnd() const");

    return reinterpret_cast<const FloatConstOpnd *>(this);
}

///----------------------------------------------------------------------------
///
/// Opnd::IsFloat32ConstOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsFloat32ConstOpnd() const
{
    return GetKind() == OpndKindFloat32Const;
}

///----------------------------------------------------------------------------
///
/// Opnd::AsFloat32ConstOpnd
///
///     Use this opnd as a FloatConstOpnd.
///
///----------------------------------------------------------------------------

inline Float32ConstOpnd *
Opnd::AsFloat32ConstOpnd()
{
    AssertMsg(this->IsFloat32ConstOpnd(), "Bad call to AsFloat32ConstOpnd()");
    return reinterpret_cast<Float32ConstOpnd *>(this);
}

///----------------------------------------------------------------------------
///
/// Opnd::IsSimd128ConstOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsSimd128ConstOpnd() const
{
    return GetKind() == OpndKindSimd128Const;
}

///----------------------------------------------------------------------------
///
/// Opnd::AsSimd128ConstOpnd
///
///     Use this opnd as a Simd128ConstOpnd.
///
///----------------------------------------------------------------------------

inline Simd128ConstOpnd *
Opnd::AsSimd128ConstOpnd()
{
    AssertMsg(this->IsSimd128ConstOpnd(), "Bad call to AsSimd128ConstOpnd()");

    return reinterpret_cast<Simd128ConstOpnd *>(this);
}

inline const Simd128ConstOpnd *
Opnd::AsSimd128ConstOpnd() const
{
    AssertMsg(this->IsSimd128ConstOpnd(), "Bad call to AsSimd128ConstOpnd() const");

    return reinterpret_cast<const Simd128ConstOpnd *>(this);
}

///----------------------------------------------------------------------------
///
/// Opnd::IsHelperCallOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsHelperCallOpnd() const
{
    return GetKind() == OpndKindHelperCall;
}

///----------------------------------------------------------------------------
///
/// Opnd::AsHelperCallOpnd
///
///     Use this opnd as a HelperCallOpnd.
///
///----------------------------------------------------------------------------

inline HelperCallOpnd *
Opnd::AsHelperCallOpnd()
{
    AssertMsg(this->IsHelperCallOpnd(), "Bad call to AsHelperCallOpnd()");

    return reinterpret_cast<HelperCallOpnd *>(this);
}

inline const HelperCallOpnd *
Opnd::AsHelperCallOpnd() const
{
    AssertMsg(this->IsHelperCallOpnd(), "Bad call to AsHelperCallOpnd() const");

    return reinterpret_cast<const HelperCallOpnd *>(this);
}

///----------------------------------------------------------------------------
///
/// Opnd::IsAddrOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsAddrOpnd() const
{
    return GetKind() == OpndKindAddr;
}

///----------------------------------------------------------------------------
///
/// Opnd::AsAddrOpnd
///
///     Use this opnd as an AddrOpnd.
///
///----------------------------------------------------------------------------

inline AddrOpnd *
Opnd::AsAddrOpnd()
{
    AssertMsg(this->IsAddrOpnd(), "Bad call to AsAddrOpnd()");

    return reinterpret_cast<AddrOpnd *>(this);
}

inline const AddrOpnd *
Opnd::AsAddrOpnd() const
{
    AssertMsg(this->IsAddrOpnd(), "Bad call to AsAddrOpnd() const");

    return reinterpret_cast<const AddrOpnd *>(this);
}

///----------------------------------------------------------------------------
///
/// Opnd::IsIndirOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsIndirOpnd() const
{
    return GetKind() == OpndKindIndir;
}

///----------------------------------------------------------------------------
///
/// Opnd::AsIndirOpnd
///
///     Use this opnd as an IndirOpnd.
///
///----------------------------------------------------------------------------

inline IndirOpnd *
Opnd::AsIndirOpnd()
{
    AssertMsg(this->IsIndirOpnd(), "Bad call to AsIndirOpnd()");

    return reinterpret_cast<IndirOpnd *>(this);
}

inline const IndirOpnd *
Opnd::AsIndirOpnd() const
{
    AssertMsg(this->IsIndirOpnd(), "Bad call to AsIndirOpnd() const");

    return reinterpret_cast<const IndirOpnd *>(this);
}

///----------------------------------------------------------------------------
///
/// Opnd::IsMemRefOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsMemRefOpnd() const
{
    return GetKind() == OpndKindMemRef;
}

///----------------------------------------------------------------------------
///
/// Opnd::AsMemRefOpnd
///
///     Use this opnd as a MemRefOpnd.
///
///----------------------------------------------------------------------------

inline MemRefOpnd *
Opnd::AsMemRefOpnd()
{
    AssertMsg(this->IsMemRefOpnd(), "Bad call to AsMemRefOpnd()");

    return reinterpret_cast<MemRefOpnd *>(this);
}

inline const MemRefOpnd *
Opnd::AsMemRefOpnd() const
{
    AssertMsg(this->IsMemRefOpnd(), "Bad call to AsMemRefOpnd() const");

    return reinterpret_cast<const MemRefOpnd *>(this);
}

///----------------------------------------------------------------------------
///
/// Opnd::IsLabelOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsLabelOpnd() const
{
    return GetKind() == OpndKindLabel;
}

///----------------------------------------------------------------------------
///
/// Opnd::AsLabelOpnd
///
///     Use this opnd as a LabelOpnd.
///
///----------------------------------------------------------------------------

inline LabelOpnd *
Opnd::AsLabelOpnd()
{
    AssertMsg(this->IsLabelOpnd(), "Bad call to AsLabelOpnd()");

    return reinterpret_cast<LabelOpnd *>(this);
}

inline const LabelOpnd *
Opnd::AsLabelOpnd() const
{
    AssertMsg(this->IsLabelOpnd(), "Bad call to AsLabelOpnd() const");

    return reinterpret_cast<const LabelOpnd *>(this);
}

///----------------------------------------------------------------------------
///
/// Opnd::IsImmediateOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsImmediateOpnd() const
{
    return this->IsIntConstOpnd() || this->IsInt64ConstOpnd() || this->IsAddrOpnd() || this->IsHelperCallOpnd();
}


inline bool Opnd::IsMemoryOpnd() const
{
    switch(GetKind())
    {
        case OpndKindSym:
        case OpndKindIndir:
        case OpndKindMemRef:
            return true;
    }
    return false;
}

///----------------------------------------------------------------------------
///
/// Opnd::IsConstOpnd
///
///----------------------------------------------------------------------------

inline bool
Opnd::IsConstOpnd() const
{
    bool result =  this->IsImmediateOpnd() || this->IsFloatConstOpnd();

    result = result || this->IsSimd128ConstOpnd();
    return result;
}

///----------------------------------------------------------------------------
///
/// RegOpnd::AsArrayRegOpnd
///
///----------------------------------------------------------------------------

inline ArrayRegOpnd *RegOpnd::AsArrayRegOpnd()
{
    Assert(IsArrayRegOpnd());
    return static_cast<ArrayRegOpnd *>(this);
}

///----------------------------------------------------------------------------
///
/// RegOpnd::GetReg
///
///----------------------------------------------------------------------------

inline RegNum
RegOpnd::GetReg() const
{
    return m_reg;
}

///----------------------------------------------------------------------------
///
/// RegOpnd::SetReg
///
///----------------------------------------------------------------------------

inline void
RegOpnd::SetReg(RegNum reg)
{
    m_reg = reg;
}

///----------------------------------------------------------------------------
///
/// IndirOpnd::GetBaseOpnd
///
///----------------------------------------------------------------------------

inline RegOpnd *
IndirOpnd::GetBaseOpnd()
{
    return this->m_baseOpnd;
}

inline RegOpnd const *
IndirOpnd::GetBaseOpnd() const
{
    return this->m_baseOpnd;
}

///----------------------------------------------------------------------------
///
/// IndirOpnd::GetIndexOpnd
///
///----------------------------------------------------------------------------

inline RegOpnd *
IndirOpnd::GetIndexOpnd()
{
    return m_indexOpnd;
}

inline RegOpnd const *
IndirOpnd::GetIndexOpnd() const
{
    return m_indexOpnd;
}

///----------------------------------------------------------------------------
///
/// IndirOpnd::GetOffset
///
///----------------------------------------------------------------------------

inline int32
IndirOpnd::GetOffset() const
{
    return m_offset;
}

///----------------------------------------------------------------------------
///
/// IndirOpnd::SetOffset
///
///----------------------------------------------------------------------------

inline void
IndirOpnd::SetOffset(int32 offset, bool dontEncode /* = false */)
{
    m_offset = offset;
    m_dontEncode = dontEncode;
}

///----------------------------------------------------------------------------
///
/// IndirOpnd::GetScale
///
///----------------------------------------------------------------------------

inline byte
IndirOpnd::GetScale() const
{
    return m_scale;
}

///----------------------------------------------------------------------------
///
/// IndirOpnd::SetScale
///
///----------------------------------------------------------------------------

inline void
IndirOpnd::SetScale(byte scale)
{
    m_scale = scale;
}

///----------------------------------------------------------------------------
///
/// MemRefOpnd::GetMemLoc
///
///----------------------------------------------------------------------------

inline intptr_t
MemRefOpnd::GetMemLoc() const
{
    return m_memLoc;
}

///----------------------------------------------------------------------------
///
/// MemRefOpnd::SetMemLoc
///
///----------------------------------------------------------------------------

inline void
MemRefOpnd::SetMemLoc(intptr_t pMemLoc)
{
    m_memLoc = pMemLoc;
}

inline LabelInstr *
LabelOpnd::GetLabel() const
{
    return m_label;
}

inline void
LabelOpnd::SetLabel(LabelInstr * labelInstr)
{
    m_label = labelInstr;
}

inline BVUnit32
RegBVOpnd::GetValue() const
{
    return m_value;
}

} // namespace IR
