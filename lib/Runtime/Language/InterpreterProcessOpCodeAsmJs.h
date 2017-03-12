//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#ifdef ASMJS_PLAT
#define PROCESS_FALLTHROUGH_ASM(name, func) \
    case OpCodeAsmJs::name:
#define PROCESS_FALLTHROUGH_ASM_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name:

#define PROCESS_READ_LAYOUT_ASMJS(name, layout, suffix) \
    CompileAssert(OpCodeInfoAsmJs<OpCodeAsmJs::name>::Layout == OpLayoutTypeAsmJs::layout); \
    const unaligned OpLayout##layout##suffix * playout = m_reader.layout##suffix(ip); \
    Assert((playout != nullptr) == (Js::OpLayoutTypeAsmJs::##layout != Js::OpLayoutTypeAsmJs::Empty)); // Make sure playout is used

#define PROCESS_NOPASMJS_COMMON(name, layout, suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, layout, suffix); \
        break; \
    }

#define PROCESS_NOPASMJS(name, func) PROCESS_NOPASMJS_COMMON(name, func,)

#define PROCESS_BR_ASM(name, func) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, AsmBr,); \
        ip = func(playout); \
        break; \
    }


#define PROCESS_GET_ELEM_SLOT_ASM_COMMON(name, func, layout, suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, layout, suffix); \
        SetNonVarReg(playout->Value, \
                func(GetNonVarReg(playout->Instance), playout)); \
        break; \
    }

#define PROCESS_GET_ELEM_SLOT_ASM(name, func, layout) PROCESS_GET_ELEM_SLOT_ASM_COMMON(name, func, layout,)


#define PROCESS_FUNCtoA1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, AsmReg1, suffix); \
        SetReg(playout->R0, \
                func(GetScriptContext())); \
        break; \
    }

#define PROCESS_FUNCtoA1Mem(name, func) PROCESS_FUNCtoA1Mem_COMMON(name, func,)


#define PROCESS_CUSTOM_ASMJS_COMMON(name, func, layout, suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, layout, suffix); \
        func(playout); \
        break; \
    }

#define PROCESS_CUSTOM_ASMJS(name, func, layout) PROCESS_CUSTOM_ASMJS_COMMON(name, func, layout,)

#define PROCESS_VtoI1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, AsmReg1, suffix); \
        SetRegRawInt(playout->R0, \
                func()); \
        break; \
    }

#define PROCESS_VtoI1Mem(name, func) PROCESS_VtoI1Mem_COMMON(name, func,)

#define PROCESS_I2toI1Ctx_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
        { \
        PROCESS_READ_LAYOUT_ASMJS(name, Int3, suffix); \
        SetRegRawInt(playout->I0, \
                func(GetRegRawInt(playout->I1), GetRegRawInt(playout->I2), scriptContext)); \
        break; \
        }

#define PROCESS_I2toI1Ctx(name, func) PROCESS_I2toI1Mem_COMMON(name, func,)

#define PROCESS_I2toI1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
        { \
        PROCESS_READ_LAYOUT_ASMJS(name, Int3, suffix); \
        SetRegRawInt(playout->I0, \
                func(GetRegRawInt(playout->I1), GetRegRawInt(playout->I2))); \
        break; \
        }

#define PROCESS_I2toI1Mem(name, func) PROCESS_I2toI1Mem_COMMON(name, func,)

#define PROCESS_L2toL1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
        { \
        PROCESS_READ_LAYOUT_ASMJS(name, Long3, suffix); \
        SetRegRawInt64(playout->L0, \
                func(GetRegRawInt64(playout->L1), GetRegRawInt64(playout->L2))); \
        break; \
        }

#define PROCESS_L2toL1Mem(name, func) PROCESS_L2toL1Mem_COMMON(name, func,)

#define PROCESS_L2toL1Ctx_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
        { \
        PROCESS_READ_LAYOUT_ASMJS(name, Long3, suffix); \
        SetRegRawInt64(playout->L0, \
                func(GetRegRawInt64(playout->L1), GetRegRawInt64(playout->L2), scriptContext)); \
        break; \
        }

#define PROCESS_L2toL1Ctx(name, func) PROCESS_L2toL1Ctx_COMMON(name, func,)

#define PROCESS_I2toI1MemDConv_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
        { \
        PROCESS_READ_LAYOUT_ASMJS(name, Int3, suffix); \
        SetRegRawInt(playout->I0, \
                JavascriptConversion::ToInt32(\
                func((unsigned int)GetRegRawInt(playout->I1), (unsigned int)GetRegRawInt(playout->I2)))); \
        break; \
        }

#define PROCESS_I2toI1MemDConv(name, func) PROCESS_I2toI1MemDConv_COMMON(name, func,)

#define PROCESS_F2toF1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                { \
                PROCESS_READ_LAYOUT_ASMJS(name, Float3, suffix); \
                SetRegRawFloat(playout->F0, \
                func(GetRegRawFloat(playout->F1), GetRegRawFloat(playout->F2))); \
                break; \
                }

#define PROCESS_D2toD1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Double3, suffix); \
        SetRegRawDouble(playout->D0, \
                func(GetRegRawDouble(playout->D1), GetRegRawDouble(playout->D2))); \
        break; \
                }

#define PROCESS_D2toD1Mem(name, func) PROCESS_D2toD1Mem_COMMON(name, func,)
#define PROCESS_F2toF1Mem(name, func) PROCESS_F2toF1Mem_COMMON(name, func,)

#define PROCESS_EMPTYASMJS(name, func) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, Empty, ); \
        func(); \
        break; \
    }

#define PROCESS_I1toI1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, Int2, suffix); \
        SetRegRawInt(playout->I0, \
                func(GetRegRawInt(playout->I1))); \
        break; \
    }
#define PROCESS_I1toI1Mem(name, func) PROCESS_I1toI1Mem_COMMON(name, func,)

#define PROCESS_L1toL1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, Long2, suffix); \
        SetRegRawInt64(playout->L0, \
                func(GetRegRawInt64(playout->L1))); \
        break; \
    }
#define PROCESS_L1toL1Mem(name, func) PROCESS_L1toL1Mem_COMMON(name, func,)

#define PROCESS_I1toL1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Long1Int1, suffix); \
        SetRegRawInt64(playout->L0, \
                func(GetRegRawInt(playout->I1))); \
        break; \
                                                                }

#define PROCESS_U1toL1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Long1Int1, suffix); \
        SetRegRawInt64(playout->L0, \
                func((unsigned int)GetRegRawInt(playout->I1))); \
        break; \
                                                                }

#define PROCESS_L1toF1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Float1Long1, suffix); \
        SetRegRawFloat(playout->F0, \
                func(GetRegRawInt64(playout->L1))); \
        break; \
                                                                }

#define PROCESS_L1toD1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Double1Long1, suffix); \
        SetRegRawDouble(playout->D0, \
                func(GetRegRawInt64(playout->L1))); \
        break; \
                                                                }

#define PROCESS_D1toD1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Double2, suffix); \
        SetRegRawDouble(playout->D0, \
                GetRegRawDouble(playout->D1)); \
        break; \
                                                                }

#define PROCESS_D1toD1(name, func) PROCESS_D1toD1_COMMON(name, func,)

#define PROCESS_D1toD1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, Double2, suffix); \
        SetRegRawDouble(playout->D0, \
                func(GetRegRawDouble(playout->D1))); \
        break; \
    }

#define PROCESS_F1toF1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float2, suffix); \
    SetRegRawFloat(playout->F0, \
    func(GetRegRawFloat(playout->F1))); \
    break; \
    }

#define PROCESS_D1toF1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float1Double1, suffix); \
    SetRegRawFloat(playout->F0, \
            func(GetRegRawDouble(playout->D1))); \
    break; \
    }

#define PROCESS_I1toF1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float1Int1, suffix); \
    SetRegRawFloat(playout->F0, \
            func(GetRegRawInt(playout->I1))); \
    break; \
    }

#define PROCESS_D1toD1Mem(name, func) PROCESS_D1toD1Mem_COMMON(name, func,)
#define PROCESS_F1toF1Mem(name, func) PROCESS_F1toF1Mem_COMMON(name, func,)
#define PROCESS_D1toF1Mem(name, func) PROCESS_D1toF1Mem_COMMON(name, func,)
#define PROCESS_I1toF1Mem(name, func) PROCESS_I1toF1Mem_COMMON(name, func,)

#define PROCESS_IP_TARG_ASM_IMPL(name, func, layoutSize) \
    case OpCodeAsmJs::name: \
    { \
    Assert(!switchProfileMode); \
    ip = func<layoutSize, INTERPRETERPROFILE>(ip); \
if (switchProfileMode) \
        { \
        m_reader.SetIP(ip); \
        return nullptr; \
        } \
        break; \
    }
#define PROCESS_IP_TARG_ASM_COMMON(name, func, suffix) PROCESS_IP_TARG_ASM##suffix(name, func)

#define PROCESS_IP_TARG_ASM_Large(name, func) PROCESS_IP_TARG_ASM_IMPL(name, func, Js::LargeLayout)
#define PROCESS_IP_TARG_ASM_Medium(name, func) PROCESS_IP_TARG_ASM_IMPL(name, func, Js::MediumLayout)
#define PROCESS_IP_TARG_ASM_Small(name, func) PROCESS_IP_TARG_ASM_IMPL(name, func, Js::SmallLayout)

#define PROCESS_L1toI1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Int1Long1, suffix); \
        SetRegRawInt(playout->I0, \
                func(GetRegRawInt64(playout->L1))); \
        break; \
                                                                }
#define PROCESS_L1toI1Mem(name, func) PROCESS_L1toI1Mem_COMMON(name, func,)

#define PROCESS_D1toI1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Int1Double1, suffix); \
        SetRegRawInt(playout->I0, \
                func(GetRegRawDouble(playout->D1))); \
        break; \
                                                                }
#define PROCESS_D1toI1Mem(name, func) PROCESS_D1toI1Mem_COMMON(name, func,)

#define PROCESS_F1toI1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
       PROCESS_READ_LAYOUT_ASMJS(name, Int1Float1, suffix); \
       SetRegRawInt(playout->I0, \
                func(GetRegRawFloat(playout->F1))); \
       break; \
                                                                }
#define PROCESS_F1toI1Mem(name, func) PROCESS_F1toI1Mem_COMMON(name, func,)

#define PROCESS_D1toL1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
       PROCESS_READ_LAYOUT_ASMJS(name, Long1Double1, suffix); \
       SetRegRawInt64(playout->L0, \
                func(GetRegRawDouble(playout->D1))); \
       break; \
                                                                }
#define PROCESS_D1toL1Mem(name, func) PROCESS_D1toL1Mem_COMMON(name, func,)


#define PROCESS_L1toD1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
       PROCESS_READ_LAYOUT_ASMJS(name, Double1Long1, suffix); \
       SetRegRawDouble(playout->D0, \
                func(GetRegRawInt64(playout->L1))); \
       break; \
                                                                }
#define PROCESS_L1toD1Mem(name, func) PROCESS_L1toD1Mem_COMMON(name, func,)

#define PROCESS_I1toD1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Double1Int1, suffix); \
        SetRegRawDouble(playout->D0, \
                func(GetRegRawInt(playout->I1))); \
        break; \
                                                                }
#define PROCESS_I1toD1Mem(name, func) PROCESS_I1toD1Mem_COMMON(name, func,)

#define PROCESS_U1toD1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, Double1Int1, suffix); \
        SetRegRawDouble(playout->D0, \
                func((unsigned int)GetRegRawInt(playout->I1)) ); \
        break; \
    }
#define PROCESS_U1toD1Mem(name, func) PROCESS_U1toD1Mem_COMMON(name, func,)

#define PROCESS_U1toF1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, Float1Int1, suffix); \
        SetRegRawFloat(playout->F0, \
                func((unsigned int)GetRegRawInt(playout->I1)) ); \
        break; \
    }

#define PROCESS_U1toF1Mem(name, func) PROCESS_U1toF1Mem_COMMON(name, func,)

#define PROCESS_F1toD1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Double1Float1, suffix); \
        SetRegRawDouble(playout->D0, \
                func((float)GetRegRawFloat(playout->F1))); \
        break; \
                                                                }

#define PROCESS_F1toD1Mem(name, func) PROCESS_F1toD1Mem_COMMON(name, func,)


#define PROCESS_R1toD1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Double1Reg1, suffix); \
        SetRegRawDouble(playout->D0, \
                func(GetReg(playout->R1),scriptContext)); \
        break; \
                                }

#define PROCESS_R1toD1Mem(name, func) PROCESS_R1toD1Mem_COMMON(name, func,)

#define PROCESS_R1toF1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Float1Reg1, suffix); \
        SetRegRawFloat(playout->F0, \
        (float)func(GetReg(playout->R1), scriptContext)); \
        break; \
                                }

#define PROCESS_R1toF1Mem(name, func) PROCESS_R1toF1Mem_COMMON(name, func,)


#define PROCESS_R1toI1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Int1Reg1, suffix); \
        SetRegRawInt(playout->I0, \
                func(GetReg(playout->R1),scriptContext)); \
        break; \
                                                                }

#define PROCESS_R1toI1Mem(name, func) PROCESS_R1toI1Mem_COMMON(name, func,)

#define PROCESS_R1toL1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Long1Reg1, suffix); \
        SetRegRawInt64(playout->L0, \
                func(GetReg(playout->R1),scriptContext)); \
        break; \
                                                                }

#define PROCESS_R1toL1Mem(name, func) PROCESS_R1toL1Mem_COMMON(name, func,)

#define PROCESS_D1toR1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Reg1Double1, suffix); \
        SetRegRaw(playout->R0, \
            func(GetRegRawDouble(playout->D1),scriptContext)); \
        break; \
                                }

#define PROCESS_F1toR1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Reg1Float1, suffix); \
        SetRegRawFloat(playout->R0, \
            GetRegRawFloat(playout->F1)); \
        break; \
                                }

#define PROCESS_D1toR1Mem(name, func) PROCESS_D1toR1Mem_COMMON(name, func,)
#define PROCESS_F1toR1Mem(name, func) PROCESS_F1toR1Mem_COMMON(name, func,)

#define PROCESS_C1toI1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Int1Const1, suffix); \
        SetRegRawInt( playout->I0, playout->C1 ); \
        break; \
                                                                }
#define PROCESS_C1toI1(name, func) PROCESS_C1toI1_COMMON(name, func,)

#define PROCESS_F1toI1Ctx_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Int1Float1, suffix); \
        SetRegRawInt( playout->I0, func(GetRegRawFloat(playout->F1),scriptContext)); \
        break; \
                                                                }
#define PROCESS_F1toI1Ctx(name, func) PROCESS_F1toI1Ctx_COMMON(name, func,)

#define PROCESS_F1toL1Ctx_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Long1Float1, suffix); \
        SetRegRawInt64( playout->L0, func(GetRegRawFloat(playout->F1),scriptContext)); \
        break; \
                                                                }
#define PROCESS_F1toL1Ctx(name, func) PROCESS_F1toL1Ctx_COMMON(name, func,)

#define PROCESS_D1toI1Ctx_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name,Int1Double1, suffix); \
        SetRegRawInt( playout->I0, func(GetRegRawDouble(playout->D1),scriptContext)); \
        break; \
                                                                }
#define PROCESS_D1toI1Ctx(name, func) PROCESS_D1toI1Ctx_COMMON(name, func,)

#define PROCESS_D1toL1Ctx_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Long1Double1, suffix); \
        SetRegRawInt64( playout->L0, func(GetRegRawDouble(playout->D1),scriptContext)); \
        break; \
                                                                }
#define PROCESS_D1toL1Ctx(name, func) PROCESS_D1toL1Ctx_COMMON(name, func,)

#define PROCESS_C1toL1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Long1Const1, suffix); \
        SetRegRawInt64( playout->L0, playout->C1 ); \
        break; \
                                                                }
#define PROCESS_C1toL1(name, func) PROCESS_C1toL1_COMMON(name, func,)

#define PROCESS_C1toF1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Float1Const1, suffix); \
        SetRegRawFloat( playout->F0, playout->C1 ); \
        break; \
                                                                }

#define PROCESS_C1toF1(name, func) PROCESS_C1toF1_COMMON(name, func,)

#define PROCESS_C1toD1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Double1Const1, suffix); \
        SetRegRawDouble( playout->D0, playout->C1 ); \
        break; \
                                                                }

#define PROCESS_C1toD1(name, func) PROCESS_C1toD1_COMMON(name, func,)

#define PROCESS_I1toR1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Reg1Int1, suffix); \
        SetRegRawInt(playout->R0, \
              GetRegRawInt(playout->I1)); \
        break; \
                                }

#define PROCESS_I1toR1Mem(name, func) PROCESS_I1toR1Mem_COMMON(name, func,)

#define PROCESS_I1toR1Out_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Reg1Int1, suffix); \
            func(playout->R0, GetRegRawInt(playout->I1)); \
        break; \
                                                                }
#define PROCESS_F1toR1Out_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Reg1Float1, suffix); \
            func(playout->R0, GetRegRawFloat(playout->F1)); \
        break; \
                                                                }

#define PROCESS_L1toR1Out_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Reg1Long1, suffix); \
            func(playout->R0, GetRegRawInt64(playout->L1)); \
        break; \
                                                                }

#define PROCESS_D1toR1Out_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Reg1Double1, suffix); \
            func(playout->R0, GetRegRawDouble(playout->D1)); \
        break; \
                                                                }

#define PROCESS_L2toI1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Int1Long2, suffix); \
        SetRegRawInt(playout->I0, \
                func(GetRegRawInt64(playout->L1),GetRegRawInt64(playout->L2))); \
        break; \
                                                                }

#define PROCESS_D2toI1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Int1Double2, suffix); \
        SetRegRawInt(playout->I0, \
                func(GetRegRawDouble(playout->D1),GetRegRawDouble(playout->D2))); \
        break; \
                                                                }

#define PROCESS_F2toI1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
                                                                { \
        PROCESS_READ_LAYOUT_ASMJS(name, Int1Float2, suffix); \
        SetRegRawInt(playout->I0, \
              func(GetRegRawFloat(playout->F1), GetRegRawFloat(playout->F2))); \
        break; \
                                                                }

#define PROCESS_D2toI1Mem(name, func) PROCESS_D2toI1Mem_COMMON(name, func,)
#define PROCESS_F2toI1Mem(name, func) PROCESS_F2toI1Mem_COMMON(name, func,)

#define PROCESS_BR_ASM_Mem_COMMON(name, func,suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, BrInt2, suffix); \
        if (func(GetRegRawInt(playout->I1), GetRegRawInt(playout->I2))) \
        { \
            ip = m_reader.SetCurrentRelativeOffset(ip, playout->RelativeJumpOffset); \
        } \
        break; \
    }
#define PROCESS_BR_ASM_Mem(name, func) PROCESS_BR_ASM_Mem_COMMON(name, func,)

#define PROCESS_BR_ASM_Const_COMMON(name, func,suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, BrInt1Const1, suffix); \
        if (func(GetRegRawInt(playout->I1), playout->C1)) \
        { \
            ip = m_reader.SetCurrentRelativeOffset(ip, playout->RelativeJumpOffset); \
        } \
        break; \
    }
#define PROCESS_BR_ASM_Const(name, func) PROCESS_BR_ASM_Const_COMMON(name, func,)

#define PROCESS_BR_ASM_MemStack_COMMON(name, func,suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, BrInt1, suffix); \
        if (GetRegRawInt(playout->I1)) \
        { \
            ip = m_reader.SetCurrentRelativeOffset(ip, playout->RelativeJumpOffset); \
        } \
        break; \
    }
#define PROCESS_BR_ASM_MemStack(name, func) PROCESS_BR_ASM_MemStack_COMMON(name, func,)

#define PROCESS_BR_ASM_MemStackF_COMMON(name, func,suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, BrInt1, suffix); \
        if (!GetRegRawInt(playout->I1)) \
        { \
            ip = m_reader.SetCurrentRelativeOffset(ip, playout->RelativeJumpOffset); \
        } \
        break; \
    }
#define PROCESS_BR_ASM_MemStackF(name, func) PROCESS_BR_ASM_MemStackF_COMMON(name, func,)

#define PROCESS_TEMPLATE_ASMJS_COMMON(name, func, layout, suffix, type) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, layout, suffix); \
        func<OpLayout##layout##suffix,type>(playout); \
        break; \
    }


// initializers
#define PROCESS_SIMD_F4_1toR1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
        PROCESS_READ_LAYOUT_ASMJS(name, Reg1Float32x4_1, suffix); \
        func(playout->R0, GetRegRawSimd(playout->F4_1)); \
        break; \
    }
#define PROCESS_SIMD_F4_1toR1Mem(name, func) PROCESS_SIMD_F4_1toR1Mem_COMMON(name, func,)

#define PROCESS_SIMD_I4_1toR1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Reg1Int32x4_1, suffix); \
    func(playout->R0, GetRegRawSimd(playout->I4_1)); \
    break; \
    }
#define PROCESS_SIMD_I4_1toR1Mem(name, func) PROCESS_SIMD_I4_1toR1Mem_COMMON(name, func,)

#define PROCESS_SIMD_I1toB4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool32x4_1Int1, suffix); \
    SetRegRawSimd(playout->B4_0, func((GetRegRawInt(playout->I1)) ? -1 : 0)); \
    break; \
    }
#define PROCESS_SIMD_I1toB4_1(name, func) PROCESS_SIMD_I1toB4_1_COMMON(name, func,)

#define PROCESS_SIMD_I1toB8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool16x8_1Int1, suffix); \
    SetRegRawSimd(playout->B8_0, func((GetRegRawInt(playout->I1)) ? -1 : 0)); \
    break; \
    }
#define PROCESS_SIMD_I1toB8_1(name, func) PROCESS_SIMD_I1toB8_1_COMMON(name, func,)

#define PROCESS_SIMD_I1toB16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool8x16_1Int1, suffix); \
    SetRegRawSimd(playout->B16_0, func((GetRegRawInt(playout->I1)) ? -1 : 0)); \
    break; \
    }
#define PROCESS_SIMD_I1toB16_1(name, func) PROCESS_SIMD_I1toB16_1_COMMON(name, func,)

#define PROCESS_SIMD_B4_1toR1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Reg1Bool32x4_1, suffix); \
    func(playout->R0, GetRegRawSimd(playout->B4_1)); \
    break; \
    }
#define PROCESS_SIMD_B4_1toR1Mem(name, func, suffix) PROCESS_B4_1toR1Mem_COMMON(name, func, suffix)

#define PROCESS_SIMD_B8_1toR1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Reg1Bool16x8_1, suffix); \
    func(playout->R0, GetRegRawSimd(playout->B8_1)); \
    break; \
    }
#define PROCESS_SIMD_B8_1toR1Mem(name, func, suffix) PROCESS_B8_1toR1Mem_COMMON(name, func, suffix)

#define PROCESS_SIMD_B16_1toR1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Reg1Bool8x16_1, suffix); \
    func(playout->R0, GetRegRawSimd(playout->B16_1)); \
    break; \
    }
#define PROCESS_SIMD_B16_1toR1Mem(name, func, suffix) PROCESS_B16_1toR1Mem_COMMON(name, func, suffix)

#define PROCESS_SIMD_I16_1toR1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Reg1Int8x16_1, suffix); \
    func(playout->R0, GetRegRawSimd(playout->I16_1)); \
    break; \
    }
#define PROCESS_SIMD_I16_1toR1Mem(name, func, suffix) PROCESS_I16_1toR1Mem_COMMON(name, func, suffix)

#define PROCESS_SIMD_D2_1toR1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Reg1Float64x2_1, suffix); \
    func(playout->R0, GetRegRawSimd(playout->D2_1)); \
    break; \
    }
#define PROCESS_SIMD_D2_1toR1Mem(name, func) PROCESS_SIMD_D2_1toR1Mem_COMMON(name, func,)

#define PROCESS_SIMD_I8_1toR1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Reg1Int16x8_1, suffix); \
    func(playout->R0, GetRegRawSimd(playout->I8_1)); \
    break; \
    }
#define PROCESS_SIMD_I8_1toR1Mem(name, func) PROCESS_SIMD_I8_1toR1Mem_COMMON(name, func,)

#define PROCESS_SIMD_U4_1toR1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Reg1Uint32x4_1, suffix); \
    func(playout->R0, GetRegRawSimd(playout->U4_1)); \
    break; \
    }
#define PROCESS_SIMD_U4_1toR1Mem(name, func) PROCESS_SIMD_U4_1toR1Mem_COMMON(name, func,)

#define PROCESS_SIMD_U8_1toR1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Reg1Uint16x8_1, suffix); \
    func(playout->R0, GetRegRawSimd(playout->U8_1)); \
    break; \
    }
#define PROCESS_SIMD_U8_1toR1Mem(name, func) PROCESS_SIMD_U8_1toR1Mem_COMMON(name, func,)

#define PROCESS_SIMD_U16_1toR1Mem_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Reg1Uint8x16_1, suffix); \
    func(playout->R0, GetRegRawSimd(playout->U16_1)); \
    break; \
    }
#define PROCESS_SIMD_U16_1toR1Mem(name, func) PROCESS_SIMD_U16_1toR1Mem_COMMON(name, func,)

#define PROCESS_SIMD_I1toI4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_1Int1, suffix); \
    SetRegRawSimd(playout->I4_0, func(GetRegRawInt(playout->I1))); \
    break; \
    }
#define PROCESS_SIMD_I1toI4_1(name, func) PROCESS_SIMD_I1toI4_1_COMMON(name, func,)

#define PROCESS_SIMD_I1toI16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int8x16_1Int1, suffix); \
    SetRegRawSimd(playout->I16_0, func(static_cast<int8>(GetRegRawInt(playout->I1)))); \
    break; \
    }
#define PROCESS_SIMD_I1toI16_1(name, func, suffix) PROCESS_SIMD_I1toI16_1_COMMON(name, func, suffix)

#define PROCESS_SIMD_I1toI8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int16x8_1Int1, suffix); \
    SetRegRawSimd(playout->I8_0, func((int16)GetRegRawInt(playout->I1))); \
    break; \
    }
#define PROCESS_SIMD_I1toI8_1_1(name, func) PROCESS_SIMD_I1toI8_1_COMMON(name, func,)

#define PROCESS_SIMD_I1toU4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_1Int1, suffix); \
    SetRegRawSimd(playout->U4_0, func((uint32)GetRegRawInt(playout->I1))); \
    break; \
    }
#define PROCESS_SIMD_I1toU4_1(name, func) PROCESS_SIMD_I1toU4_1_COMMON(name, func,)

#define PROCESS_SIMD_I1toU8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint16x8_1Int1, suffix); \
    SetRegRawSimd(playout->U8_0, func((uint16)GetRegRawInt(playout->I1))); \
    break; \
    }
#define PROCESS_SIMD_I1toU8_1(name, func) PROCESS_SIMD_I1toU8_1_COMMON(name, func,)

#define PROCESS_SIMD_I1toU16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint8x16_1Int1, suffix); \
    SetRegRawSimd(playout->U16_0, func((uint8)GetRegRawInt(playout->I1))); \
    break; \
    }
#define PROCESS_SIMD_I1toU16_1(name, func) PROCESS_SIMD_I1toU16_1_COMMON(name, func,)

#define PROCESS_SIMD_F1toF4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_1Float1, suffix); \
    SetRegRawSimd(playout->F4_0, func(GetRegRawFloat(playout->F1))); \
    break; \
    }
#define PROCESS_SIMD_F1toF4_1(name, func) PROCESS_SIMD_F1toF4_1_COMMON(name, func,)

#define PROCESS_SIMD_D1toD2_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float64x2_1Double1, suffix); \
    SetRegRawSimd(playout->D2_0, func(GetRegRawDouble(playout->D1))); \
    break; \
    }
#define PROCESS_SIMD_D1toD2_1(name, func) PROCESS_SIMD_D1toD2_1_COMMON(name, func,)

//// Value transfer
#define PROCESS_SIMD_F4toF4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_1Float4, suffix); \
    SetRegRawSimd(playout->F4_0, func(GetRegRawFloat(playout->F1), GetRegRawFloat(playout->F2), GetRegRawFloat(playout->F3), GetRegRawFloat(playout->F4))); \
    break; \
    }
#define PROCESS_SIMD_F4toF4_1(name, func) PROCESS_SIMD_F4toF4_1_COMMON(name, func,)

#define PROCESS_SIMD_I4toI4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_1Int4, suffix); \
    SetRegRawSimd(playout->I4_0, func(GetRegRawInt(playout->I1), GetRegRawInt(playout->I2), GetRegRawInt(playout->I3), GetRegRawInt(playout->I4))); \
    break; \
    }
#define PROCESS_SIMD_I4toI4_1(name, func) PROCESS_SIMD_I4toI4_1_COMMON(name, func,)

#define PROCESS_SIMD_I4toU4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_1Int4, suffix); \
    SetRegRawSimd(playout->U4_0, func((uint32)GetRegRawInt(playout->I1), (uint32)GetRegRawInt(playout->I2), (uint32)GetRegRawInt(playout->I3), (uint32)GetRegRawInt(playout->I4))); \
    break; \
    }
#define PROCESS_SIMD_I4toU4_1(name, func) PROCESS_SIMD_I4toU4_1_COMMON(name, func,)

#define PROCESS_SIMD_D2toD2_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float64x2_1Double2, suffix); \
    SetRegRawSimd(playout->D2_0, func(GetRegRawDouble(playout->D1), GetRegRawDouble(playout->D2))); \
    break; \
    }
#define PROCESS_SIMD_D2toD2_1(name, func) PROCESS_SIMD_D2toD2_1_COMMON(name, func,)

//// Conversions
#define PROCESS_SIMD_D2_1toF4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_1Float64x2_1, suffix); \
    SetRegRawSimd(playout->F4_0, \
    func(GetRegRawSimd(playout->D2_1))); \
    break; \
    }
#define PROCESS_SIMD_D2_1toF4_1(name, func) PROCESS_SIMD_D2_1toF4_1_COMMON(name, func,)

#define PROCESS_SIMD_I4_1toF4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_1Int32x4_1, suffix); \
    SetRegRawSimd(playout->F4_0, \
    func(GetRegRawSimd(playout->I4_1))); \
    break; \
    }
#define PROCESS_SIMD_I4_1toF4_1_1(name, func) PROCESS_SIMD_I4_1toF4_1_COMMON(name, func,)

#define PROCESS_SIMD_I8_1toF4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_1Int16x8_1, suffix); \
    SetRegRawSimd(playout->F4_0, \
    func(GetRegRawSimd(playout->I8_1))); \
    break; \
    }
#define PROCESS_SIMD_I8_1toF4_1_1(name, func) PROCESS_SIMD_I8_1toF4_1_COMMON(name, func,)

#define PROCESS_SIMD_I16_1toF4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_1Int8x16_1, suffix); \
    SetRegRawSimd(playout->F4_0, \
    func(GetRegRawSimd(playout->I16_1))); \
    break; \
    }
#define PROCESS_SIMD_I16_1toF4_1_1(name, func) PROCESS_SIMD_I16_1toF4_1_COMMON(name, func,)

#define PROCESS_SIMD_U4_1toF4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_1Uint32x4_1, suffix); \
    SetRegRawSimd(playout->F4_0, \
    func(GetRegRawSimd(playout->U4_1))); \
    break; \
    }
#define PROCESS_SIMD_U4_1toF4_1_1(name, func) PROCESS_SIMD_U4_1toF4_1_COMMON(name, func,)

#define PROCESS_SIMD_U8_1toF4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_1Uint16x8_1, suffix); \
    SetRegRawSimd(playout->F4_0, \
    func(GetRegRawSimd(playout->U8_1))); \
    break; \
    }
#define PROCESS_SIMD_U8_1toF4_1_1(name, func) PROCESS_SIMD_U8_1toF4_1_COMMON(name, func,)

#define PROCESS_SIMD_U16_1toF4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_1Uint8x16_1, suffix); \
    SetRegRawSimd(playout->F4_0, \
    func(GetRegRawSimd(playout->U16_1))); \
    break; \
    }
#define PROCESS_SIMD_U16_1toF4_1_1(name, func) PROCESS_SIMD_U16_1toF4_1_COMMON(name, func,)



#define PROCESS_SIMD_D2_1toI4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_1Float64x2_1, suffix); \
    SetRegRawSimd(playout->I4_0, \
    func(GetRegRawSimd(playout->D2_1))); \
    break; \
    }
#define PROCESS_SIMD_D2_1toI4_1(name, func) PROCESS_SIMD_D2_1toI4_1_COMMON(name, func,)

#define PROCESS_SIMD_F4_1toI4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_1Float32x4_1, suffix); \
    SetRegRawSimd(playout->I4_0, \
    func(GetRegRawSimd(playout->F4_1))); \
    break; \
    }
#define PROCESS_SIMD_F4_1toI4_1(name, func) PROCESS_SIMD_F4_1toI4_1_COMMON(name, func,)

#define PROCESS_SIMD_I8_1toI4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_1Int16x8_1, suffix); \
    SetRegRawSimd(playout->I4_0, \
    func(GetRegRawSimd(playout->I8_1))); \
    break; \
    }
#define PROCESS_SIMD_I8_1toI4_1(name, func) PROCESS_SIMD_I8_1toI4_1_COMMON(name, func,)

#define PROCESS_SIMD_I16_1toI4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_1Int8x16_1, suffix); \
    SetRegRawSimd(playout->I4_0, \
    func(GetRegRawSimd(playout->I16_1))); \
    break; \
    }
#define PROCESS_SIMD_I16_1toI4_1(name, func) PROCESS_SIMD_I16_1toI4_1_COMMON(name, func,)

#define PROCESS_SIMD_U8_1toI4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_1Uint16x8_1, suffix); \
    SetRegRawSimd(playout->I4_0, \
    func(GetRegRawSimd(playout->U8_1))); \
    break; \
    }
#define PROCESS_SIMD_U8_1toI4_1(name, func) PROCESS_SIMD_U8_1toI4_1_COMMON(name, func,)

#define PROCESS_SIMD_U16_1toI4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_1Uint8x16_1, suffix); \
    SetRegRawSimd(playout->I4_0, \
    func(GetRegRawSimd(playout->U16_1))); \
    break; \
    }
#define PROCESS_SIMD_U16_1toI4_1(name, func) PROCESS_SIMD_U16_1toI4_1_COMMON(name, func,)

#define PROCESS_SIMD_U4_1toI4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_1Uint32x4_1, suffix); \
    SetRegRawSimd(playout->I4_0, \
    func(GetRegRawSimd(playout->U4_1))); \
    break; \
    }
#define PROCESS_SIMD_U4_1toI4_1(name, func) PROCESS_SIMD_U4_1toI4_1_COMMON(name, func,)



#define PROCESS_SIMD_F4_1toI8_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int16x8_1Float32x4_1, suffix); \
    SetRegRawSimd(playout->I8_0, \
    func(GetRegRawSimd(playout->F4_1))); \
    break; \
    }
#define PROCESS_SIMD_F4_1toI8_1(name, func) PROCESS_SIMD_F4_1toI8_1_COMMON(name, func,)

#define PROCESS_SIMD_I4_1toI8_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int16x8_1Int32x4_1, suffix); \
    SetRegRawSimd(playout->I8_0, \
    func(GetRegRawSimd(playout->I4_1))); \
    break; \
    }
#define PROCESS_SIMD_I4_1toI8_1(name, func) PROCESS_SIMD_I4_1toI8_1_COMMON(name, func,)

#define PROCESS_SIMD_I16_1toI8_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int16x8_1Int8x16_1, suffix); \
    SetRegRawSimd(playout->I8_0, \
    func(GetRegRawSimd(playout->I16_1))); \
    break; \
    }
#define PROCESS_SIMD_I16_1toI8_1(name, func) PROCESS_SIMD_I16_1toI8_1_COMMON(name, func,)


// i16swizzle
#define PROCESS_SIMD_I16_1I16toI16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int8x16_2Int16, suffix); \
    uint32 lanes[16]; \
    lanes[0] = GetRegRawInt(playout->I2);   lanes[1] = GetRegRawInt(playout->I3); \
    lanes[2] = GetRegRawInt(playout->I4);   lanes[3] = GetRegRawInt(playout->I5); \
    lanes[4] = GetRegRawInt(playout->I6);   lanes[5] = GetRegRawInt(playout->I7); \
    lanes[6] = GetRegRawInt(playout->I8);   lanes[7] = GetRegRawInt(playout->I9); \
    lanes[8] = GetRegRawInt(playout->I10);  lanes[9] = GetRegRawInt(playout->I11); \
    lanes[10] = GetRegRawInt(playout->I12); lanes[11] = GetRegRawInt(playout->I13); \
    lanes[12] = GetRegRawInt(playout->I14); lanes[13] = GetRegRawInt(playout->I15); \
    lanes[14] = GetRegRawInt(playout->I16); lanes[15] = GetRegRawInt(playout->I17); \
    SetRegRawSimd(playout->I16_0, func(GetRegRawSimd(playout->I16_1), GetRegRawSimd(playout->I16_1), 16, lanes)); \
    break; \
    }
#define PROCESS_SIMD_I16_1I16toI16_1(name, func) PROCESS_SIMD_I16_1I16toI16_1_COMMON(name, func,)

// i16shuffle
#define PROCESS_SIMD_I16_2I16toI16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int8x16_3Int16, suffix); \
    uint32 lanes[16]; \
    lanes[0] = GetRegRawInt(playout->I3);   lanes[1] = GetRegRawInt(playout->I4); \
    lanes[2] = GetRegRawInt(playout->I5);   lanes[3] = GetRegRawInt(playout->I6); \
    lanes[4] = GetRegRawInt(playout->I7);   lanes[5] = GetRegRawInt(playout->I8); \
    lanes[6] = GetRegRawInt(playout->I9);  lanes[7] = GetRegRawInt(playout->I10); \
    lanes[8] = GetRegRawInt(playout->I11);  lanes[9] = GetRegRawInt(playout->I12); \
    lanes[10] = GetRegRawInt(playout->I13); lanes[11] = GetRegRawInt(playout->I14); \
    lanes[12] = GetRegRawInt(playout->I15); lanes[13] = GetRegRawInt(playout->I16); \
    lanes[14] = GetRegRawInt(playout->I17); lanes[15] = GetRegRawInt(playout->I18); \
    SetRegRawSimd(playout->I16_0, func(GetRegRawSimd(playout->I16_1), GetRegRawSimd(playout->I16_2), 16, lanes)); \
    break; \
    }
#define PROCESS_SIMD_I16_2I16toI16_1(name, func) PROCESS_SIMD_I16_2I16toI16_1_COMMON(name, func,)

#define PROCESS_SIMD_U4_1toI8_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int16x8_1Uint32x4_1, suffix); \
    SetRegRawSimd(playout->I8_0, \
    func(GetRegRawSimd(playout->U4_1))); \
    break; \
    }
#define PROCESS_SIMD_U4_1toI8_1(name, func) PROCESS_SIMD_U4_1toI8_1_COMMON(name, func,)

#define PROCESS_SIMD_U8_1toI8_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int16x8_1Uint16x8_1, suffix); \
    SetRegRawSimd(playout->I8_0, \
    func(GetRegRawSimd(playout->U8_1))); \
    break; \
    }
#define PROCESS_SIMD_U8_1toI8_1(name, func) PROCESS_SIMD_U8_1toI8_1_COMMON(name, func,)

#define PROCESS_SIMD_U16_1toI8_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int16x8_1Uint8x16_1, suffix); \
    SetRegRawSimd(playout->I8_0, \
    func(GetRegRawSimd(playout->U16_1))); \
    break; \
    }
#define PROCESS_SIMD_U16_1toI8_1(name, func) PROCESS_SIMD_U16_1toI8_1_COMMON(name, func,)


#define PROCESS_SIMD_F4_1toU4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_1Float32x4_1, suffix); \
    SetRegRawSimd(playout->U4_0, \
    func(GetRegRawSimd(playout->F4_1))); \
    break; \
    }
#define PROCESS_SIMD_F4_1toU4_1(name, func) PROCESS_SIMD_F4_1toU4_1_COMMON(name, func,)

#define PROCESS_SIMD_I4_1toU4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_1Int32x4_1, suffix); \
    SetRegRawSimd(playout->U4_0, \
    func(GetRegRawSimd(playout->I4_1))); \
    break; \
    }
#define PROCESS_SIMD_I4_1toU4_1(name, func) PROCESS_SIMD_I4_1toU4_1_COMMON(name, func,)

#define PROCESS_SIMD_I8_1toU4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_1Int16x8_1, suffix); \
    SetRegRawSimd(playout->U4_0, \
    func(GetRegRawSimd(playout->I8_1))); \
    break; \
    }
#define PROCESS_SIMD_I8_1toU4_1(name, func) PROCESS_SIMD_I8_1toU4_1_COMMON(name, func,)

#define PROCESS_SIMD_I16_1toU4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_1Int8x16_1, suffix); \
    SetRegRawSimd(playout->U4_0, \
    func(GetRegRawSimd(playout->I16_1))); \
    break; \
    }
#define PROCESS_SIMD_I16_1toU4_1(name, func) PROCESS_SIMD_I16_1toU4_1_COMMON(name, func,)

#define PROCESS_SIMD_U8_1toU4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_1Uint16x8_1, suffix); \
    SetRegRawSimd(playout->U4_0, \
    func(GetRegRawSimd(playout->U8_1))); \
    break; \
    }
#define PROCESS_SIMD_U8_1toU4_1(name, func) PROCESS_SIMD_U8_1toU4_1_COMMON(name, func,)

#define PROCESS_SIMD_U16_1toU4_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_1Uint8x16_1, suffix); \
    SetRegRawSimd(playout->U4_0, \
    func(GetRegRawSimd(playout->U16_1))); \
    break; \
    }
#define PROCESS_SIMD_U16_1toU4_1(name, func) PROCESS_SIMD_U16_1toU4_1_COMMON(name, func,)



#define PROCESS_SIMD_F4_1toU8_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint16x8_1Float32x4_1, suffix); \
    SetRegRawSimd(playout->U8_0, \
    func(GetRegRawSimd(playout->F4_1))); \
    break; \
    }
#define PROCESS_SIMD_F4_1toU8_1(name, func) PROCESS_SIMD_F4_1toU8_1_COMMON(name, func,)

#define PROCESS_SIMD_I4_1toU8_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint16x8_1Int32x4_1, suffix); \
    SetRegRawSimd(playout->U8_0, \
    func(GetRegRawSimd(playout->I4_1))); \
    break; \
    }
#define PROCESS_SIMD_I4_1toU8_1(name, func) PROCESS_SIMD_I4_1toU8_1_COMMON(name, func,)

#define PROCESS_SIMD_I8_1toU8_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint16x8_1Int16x8_1, suffix); \
    SetRegRawSimd(playout->U8_0, \
    func(GetRegRawSimd(playout->I8_1))); \
    break; \
    }
#define PROCESS_SIMD_I8_1toU8_1(name, func) PROCESS_SIMD_I8_1toU8_1_COMMON(name, func,)

#define PROCESS_SIMD_I16_1toU8_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint16x8_1Int8x16_1, suffix); \
    SetRegRawSimd(playout->U8_0, \
    func(GetRegRawSimd(playout->I16_1))); \
    break; \
    }
#define PROCESS_SIMD_I16_1toU8_1(name, func) PROCESS_SIMD_I16_1toU8_1_COMMON(name, func,)

#define PROCESS_SIMD_U4_1toU8_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint16x8_1Uint32x4_1, suffix); \
    SetRegRawSimd(playout->U8_0, \
    func(GetRegRawSimd(playout->U4_1))); \
    break; \
    }
#define PROCESS_SIMD_U4_1toU8_1(name, func) PROCESS_SIMD_U4_1toU8_1_COMMON(name, func,)

#define PROCESS_SIMD_U16_1toU8_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint16x8_1Uint8x16_1, suffix); \
    SetRegRawSimd(playout->U8_0, \
    func(GetRegRawSimd(playout->U16_1))); \
    break; \
    }
#define PROCESS_SIMD_U16_1toU8_1(name, func) PROCESS_SIMD_U16_1toU8_1_COMMON(name, func,)



#define PROCESS_SIMD_F4_1toU16_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint8x16_1Float32x4_1, suffix); \
    SetRegRawSimd(playout->U16_0, \
    func(GetRegRawSimd(playout->F4_1))); \
    break; \
    }
#define PROCESS_SIMD_F4_1toU16_1(name, func) PROCESS_SIMD_F4_1toU16_1_COMMON(name, func,)

#define PROCESS_SIMD_I4_1toU16_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint8x16_1Int32x4_1, suffix); \
    SetRegRawSimd(playout->U16_0, \
    func(GetRegRawSimd(playout->I4_1))); \
    break; \
    }
#define PROCESS_SIMD_I4_1toU16_1(name, func) PROCESS_SIMD_I4_1toU16_1_COMMON(name, func,)

#define PROCESS_SIMD_I8_1toU16_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint8x16_1Int16x8_1, suffix); \
    SetRegRawSimd(playout->U16_0, \
    func(GetRegRawSimd(playout->I8_1))); \
    break; \
    }
#define PROCESS_SIMD_I8_1toU16_1(name, func) PROCESS_SIMD_I8_1toU16_1_COMMON(name, func,)

#define PROCESS_SIMD_I16_1toU16_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint8x16_1Int8x16_1, suffix); \
    SetRegRawSimd(playout->U16_0, \
    func(GetRegRawSimd(playout->I16_1))); \
    break; \
    }
#define PROCESS_SIMD_I16_1toU16_1(name, func) PROCESS_SIMD_I16_1toU16_1_COMMON(name, func,)

#define PROCESS_SIMD_U4_1toU16_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint8x16_1Uint32x4_1, suffix); \
    SetRegRawSimd(playout->U16_0, \
    func(GetRegRawSimd(playout->U4_1))); \
    break; \
    }
#define PROCESS_SIMD_U4_1toU16_1(name, func) PROCESS_SIMD_U4_1toU16_1_COMMON(name, func,)

#define PROCESS_SIMD_U8_1toU16_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint8x16_1Uint16x8_1, suffix); \
    SetRegRawSimd(playout->U16_0, \
    func(GetRegRawSimd(playout->U8_1))); \
    break; \
    }
#define PROCESS_SIMD_U8_1toU16_1(name, func) PROCESS_SIMD_U8_1toU16_1_COMMON(name, func,)

#define PROCESS_SIMD_F4_1toI16_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int8x16_1Float32x4_1, suffix); \
    SetRegRawSimd(playout->I16_0, \
    func(GetRegRawSimd(playout->F4_1))); \
    break; \
    }
#define PROCESS_SIMD_F4_1toI16_1(name, func, suffix) PROCESS_SIMD_F4_1toI16_1_COMMON(name, func, suffix)

#define PROCESS_SIMD_I4_1toI16_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int8x16_1Int32x4_1, suffix); \
    SetRegRawSimd(playout->I16_0, \
    func(GetRegRawSimd(playout->I4_1))); \
    break; \
    }
#define PROCESS_SIMD_I4_1toI16_1(name, func, suffix) PROCESS_SIMD_I4_1toI16_1_COMMON(name, func, suffix)

#define PROCESS_SIMD_I8_1toI16_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int8x16_1Int16x8_1, suffix); \
    SetRegRawSimd(playout->I16_0, \
    func(GetRegRawSimd(playout->I8_1))); \
    break; \
    }
#define PROCESS_SIMD_I8_1toI16_1(name, func) PROCESS_SIMD_I8_1toI16_1_COMMON(name, func,)

#define PROCESS_SIMD_U4_1toI16_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int8x16_1Uint32x4_1, suffix); \
    SetRegRawSimd(playout->I16_0, \
    func(GetRegRawSimd(playout->U4_1))); \
    break; \
    }
#define PROCESS_SIMD_U4_1toI16_1(name, func) PROCESS_SIMD_U4_1toI16_1_COMMON(name, func,)

#define PROCESS_SIMD_U8_1toI16_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int8x16_1Uint16x8_1, suffix); \
    SetRegRawSimd(playout->I16_0, \
    func(GetRegRawSimd(playout->U8_1))); \
    break; \
    }
#define PROCESS_SIMD_U8_1toI16_1(name, func) PROCESS_SIMD_U8_1toI16_1_COMMON(name, func,)

#define PROCESS_SIMD_U16_1toI16_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int8x16_1Uint8x16_1, suffix); \
    SetRegRawSimd(playout->I16_0, \
    func(GetRegRawSimd(playout->U16_1))); \
    break; \
    }
#define PROCESS_SIMD_U16_1toI16_1(name, func) PROCESS_SIMD_U16_1toI16_1_COMMON(name, func,)


#define PROCESS_SIMD_F4_1toD2_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float64x2_1Float32x4_1, suffix); \
    SetRegRawSimd(playout->D2_0, \
    func(GetRegRawSimd(playout->F4_1))); \
    break; \
    }
#define PROCESS_SIMD_F4_1toD2_1(name, func) PROCESS_SIMD_F4_1toD2_1_COMMON(name, func,)

#define PROCESS_SIMD_I4_1toD2_1_COMMON(name, func, suffix)\
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float64x2_1Int32x4_1, suffix); \
    SetRegRawSimd(playout->D2_0, \
    func(GetRegRawSimd(playout->I4_1))); \
    break; \
    }
#define PROCESS_SIMD_I4_1toD2_1(name, func) PROCESS_SIMD_I4_1toD2_1_COMMON(name, func,)

// unary ops
#define PROCESS_SIMD_B4_1toI1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int1Bool32x4_1, suffix); \
    SetRegRawInt(playout->I0, func(GetRegRawSimd(playout->B4_1))); \
    break; \
    }
#define PROCESS_SIMD_B4_1toI1(name, func, suffix) PROCESS_SIMD_B4_1toI1_COMMON(name, func, suffix)

#define PROCESS_SIMD_B8_1toI1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int1Bool16x8_1, suffix); \
    SetRegRawInt(playout->I0, func(GetRegRawSimd(playout->B8_1))); \
    break; \
    }
#define PROCESS_SIMD_B8_1toI1(name, func, suffix) PROCESS_SIMD_B8_1toI1_COMMON(name, func, suffix)

#define PROCESS_SIMD_B16_1toI1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int1Bool8x16_1, suffix); \
    SetRegRawInt(playout->I0, func(GetRegRawSimd(playout->B16_1))); \
    break; \
    }
#define PROCESS_SIMD_B16_1toI1(name, func, suffix) PROCESS_SIMD_B16_1toI1_COMMON(name, func, suffix)

#define PROCESS_SIMD_F4_1toF4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_2, suffix); \
    SetRegRawSimd(playout->F4_0, func(GetRegRawSimd(playout->F4_1))); \
    break; \
    }
#define PROCESS_SIMD_F4_1toF4_1  (name, func) PROCESS_SIMD_F4_1toF4_1_COMMON(name, func,)

#define PROCESS_SIMD_I4_1toI4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_2, suffix); \
    SetRegRawSimd(playout->I4_0, func(GetRegRawSimd(playout->I4_1))); \
    break; \
    }
#define PROCESS_SIMD_I4_1toI4_1  (name, func) PROCESS_SIMD_I4_1toI4_1_COMMON(name, func,)

#define PROCESS_SIMD_B4_1toB4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool32x4_2, suffix); \
    SetRegRawSimd(playout->B4_0, func(GetRegRawSimd(playout->B4_1))); \
    break; \
    }
#define PROCESS_SIMD_B4_1toB4_1  (name, func,) PROCESS_B4_1toB4_1_COMMON(name, func,)
#define PROCESS_SIMD_I8_1toI8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int16x8_2, suffix); \
    SetRegRawSimd(playout->I8_0, func(GetRegRawSimd(playout->I8_1))); \
    break; \
    }
#define PROCESS_SIMD_I8_1toI8_1  (name, func) PROCESS_SIMD_I8_1toI8_1_COMMON(name, func,)

#define PROCESS_SIMD_B8_1toB8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool16x8_2, suffix); \
    SetRegRawSimd(playout->B8_0, func(GetRegRawSimd(playout->B8_1))); \
    break; \
    }
#define PROCESS_SIMD_B8_1toB8_1  (name, func,) PROCESS_B8_1toB8_1_COMMON(name, func,)

#define PROCESS_SIMD_B16_1toB16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool8x16_2, suffix); \
    SetRegRawSimd(playout->B16_0, func(GetRegRawSimd(playout->B16_1))); \
    break; \
    }
#define PROCESS_SIMD_B16_1toB16_1  (name, func,) PROCESS_B16_1toB16_1_COMMON(name, func,)



#define PROCESS_SIMD_I16_1toI16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int8x16_2, suffix); \
    SetRegRawSimd(playout->I16_0, func(GetRegRawSimd(playout->I16_1))); \
    break; \
    }
#define PROCESS_SIMD_I16_1toI16_1  (name, func,) PROCESS_I16_1toI16_1_COMMON(name, func,)
#define PROCESS_SIMD_U4_1toU4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_2, suffix); \
    SetRegRawSimd(playout->U4_0, func(GetRegRawSimd(playout->U4_1))); \
    break; \
    }
#define PROCESS_SIMD_U4_1toU4_1  (name, func) PROCESS_SIMD_U4_1toU4_1_COMMON(name, func,)

#define PROCESS_SIMD_U8_1toU8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint16x8_2, suffix); \
    SetRegRawSimd(playout->U8_0, func(GetRegRawSimd(playout->U8_1))); \
    break; \
    }
#define PROCESS_SIMD_U8_1toU8_1(name, func) PROCESS_SIMD_U8_1toU8_1_COMMON(name, func,)

#define PROCESS_SIMD_U16_1toU16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint8x16_2, suffix); \
    SetRegRawSimd(playout->U16_0, func(GetRegRawSimd(playout->U16_1))); \
    break; \
    }
#define PROCESS_SIMD_U16_1toU16_1(name, func) PROCESS_SIMD_U16_1toU16_1_COMMON(name, func,)

#define PROCESS_SIMD_D2_1toD2_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float64x2_2, suffix); \
    SetRegRawSimd(playout->D2_0, func(GetRegRawSimd(playout->D2_1))); \
    break; \
    }
#define PROCESS_SIMD_D2_1toD2_1  (name, func) PROCESS_SIMD_D2_1toD2_1_COMMON(name, func,)


// binary ops
#define PROCESS_SIMD_F4_2toF4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_3, suffix); \
    SetRegRawSimd(playout->F4_0, func(GetRegRawSimd(playout->F4_1), GetRegRawSimd(playout->F4_2))); \
    break; \
    }
#define PROCESS_SIMD_F4_2toF4_1(name, func) PROCESS_SIMD_F4_2toF4_1_COMMON(name, func,)

#define PROCESS_SIMD_F4_2toB4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool32x4_1Float32x4_2, suffix); \
    SetRegRawSimd(playout->B4_0, func(GetRegRawSimd(playout->F4_1), GetRegRawSimd(playout->F4_2))); \
    break; \
    }
#define PROCESS_SIMD_F4_2toB4_1(name, func) PROCESS_SIMD_F4_2toB4_1_COMMON(name, func,)

#define PROCESS_SIMD_I4_2toI4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_3, suffix); \
    SetRegRawSimd(playout->I4_0, func(GetRegRawSimd(playout->I4_1), GetRegRawSimd(playout->I4_2))); \
    break; \
    }
#define PROCESS_SIMD_I4_2toI4_1(name, func) PROCESS_SIMD_I4_2toI4_1_COMMON(name, func,)

#define PROCESS_SIMD_I4_2toB4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool32x4_1Int32x4_2, suffix); \
    SetRegRawSimd(playout->B4_0, func(GetRegRawSimd(playout->I4_1), GetRegRawSimd(playout->I4_2))); \
    break; \
    }
#define PROCESS_SIMD_I4_2toB4_1(name, func) PROCESS_SIMD_I4_2toB4_1_COMMON(name, func,)

#define PROCESS_SIMD_B4_2toB4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool32x4_3, suffix); \
    SetRegRawSimd(playout->B4_0, func(GetRegRawSimd(playout->B4_1), GetRegRawSimd(playout->B4_2))); \
    break; \
    }
#define PROCESS_SIMD_B4_2toB4_1(name, func, suffix) PROCESS_SIMD_B4_2toB4_1_COMMON(name, func, suffix)

#define PROCESS_SIMD_B8_2toB8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool16x8_3, suffix); \
    SetRegRawSimd(playout->B8_0, func(GetRegRawSimd(playout->B8_1), GetRegRawSimd(playout->B8_2))); \
    break; \
    }
#define PROCESS_SIMD_B8_2toB8_1(name, func, suffix) PROCESS_SIMD_B8_2toB8_1_COMMON(name, func, suffix)

#define PROCESS_SIMD_B16_2toB16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool8x16_3, suffix); \
    SetRegRawSimd(playout->B16_0, func(GetRegRawSimd(playout->B16_1), GetRegRawSimd(playout->B16_2))); \
    break; \
    }
#define PROCESS_SIMD_B16_2toB16_1(name, func, suffix) PROCESS_SIMD_B16_2toB16_1_COMMON(name, func, suffix)

#define PROCESS_SIMD_I16_2toI16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int8x16_3, suffix); \
    SetRegRawSimd(playout->I16_0, func(GetRegRawSimd(playout->I16_1), GetRegRawSimd(playout->I16_2))); \
    break; \
    }
#define PROCESS_SIMD_I16_2toI16_1(name, func, suffix) PROCESS_SIMD_I16_2toI16_1_COMMON(name, func, suffix)

#define PROCESS_SIMD_I16_2toB16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool8x16_1Int8x16_2, suffix); \
    SetRegRawSimd(playout->B16_0, func(GetRegRawSimd(playout->I16_1), GetRegRawSimd(playout->I16_2))); \
    break; \
    }
#define PROCESS_SIMD_I16_2toB16_1(name, func) PROCESS_SIMD_I16_2toB16_1_COMMON(name, func,)

#define PROCESS_SIMD_I8_2toI8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int16x8_3, suffix); \
    SetRegRawSimd(playout->I8_0, func(GetRegRawSimd(playout->I8_1), GetRegRawSimd(playout->I8_2))); \
    break; \
    }
#define PROCESS_SIMD_I8_2toI8_1(name, func) PROCESS_SIMD_I8_2toI8_1_COMMON(name, func,)

#define PROCESS_SIMD_I8_2toB8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool16x8_1Int16x8_2, suffix); \
    SetRegRawSimd(playout->B8_0, func(GetRegRawSimd(playout->I8_1), GetRegRawSimd(playout->I8_2))); \
    break; \
    }
#define PROCESS_SIMD_I8_2toB8_1(name, func) PROCESS_SIMD_I8_2toB8_1_COMMON(name, func,)

#define PROCESS_SIMD_U4_2toU4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_3, suffix); \
    SetRegRawSimd(playout->U4_0, func(GetRegRawSimd(playout->U4_1), GetRegRawSimd(playout->U4_2))); \
    break; \
    }
#define PROCESS_SIMD_U4_2toU4_1(name, func) PROCESS_SIMD_U4_2toU4_1_COMMON(name, func,)

#define PROCESS_SIMD_U4_2toB4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool32x4_1Uint32x4_2, suffix); \
    SetRegRawSimd(playout->B4_0, func(GetRegRawSimd(playout->U4_1), GetRegRawSimd(playout->U4_2))); \
    break; \
    }
#define PROCESS_SIMD_U4_2toB4_1(name, func) PROCESS_SIMD_U4_2toB4_1_COMMON(name, func,)

#define PROCESS_SIMD_U8_2toU8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint16x8_3, suffix); \
    SetRegRawSimd(playout->U8_0, func(GetRegRawSimd(playout->U8_1), GetRegRawSimd(playout->U8_2))); \
    break; \
    }
#define PROCESS_SIMD_U8_2toU8_1(name, func) PROCESS_SIMD_U8_2toU8_1_COMMON(name, func,)

#define PROCESS_SIMD_U8_2toB8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool16x8_1Uint16x8_2, suffix); \
    SetRegRawSimd(playout->B8_0, func(GetRegRawSimd(playout->U8_1), GetRegRawSimd(playout->U8_2))); \
    break; \
    }
#define PROCESS_SIMD_U8_2toB8_1(name, func) PROCESS_SIMD_U8_2toB8_1_COMMON(name, func,)

#define PROCESS_SIMD_U16_2toU16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint8x16_3, suffix); \
    SetRegRawSimd(playout->U16_0, func(GetRegRawSimd(playout->U16_1), GetRegRawSimd(playout->U16_2))); \
    break; \
    }
#define PROCESS_SIMD_U16_2toU16_1(name, func) PROCESS_SIMD_U16_2toU16_1_COMMON(name, func,)

#define PROCESS_SIMD_U16_2toB16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool8x16_1Uint8x16_2, suffix); \
    SetRegRawSimd(playout->B16_0, func(GetRegRawSimd(playout->U16_1), GetRegRawSimd(playout->U16_2))); \
    break; \
    }
#define PROCESS_SIMD_U16_2toB16_1(name, func) PROCESS_SIMD_U16_2toB16_1_COMMON(name, func,)

#define PROCESS_SIMD_D2_2toD2_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float64x2_3, suffix); \
    SetRegRawSimd(playout->D2_0, func(GetRegRawSimd(playout->D2_1), GetRegRawSimd(playout->D2_2))); \
    break; \
    }
#define PROCESS_SIMD_D2_2toD2_1(name, func) PROCESS_SIMD_D2_2toD2_1_COMMON(name, func,)


// ternary
#define PROCESS_SIMD_F4_3toF4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_4, suffix); \
    SetRegRawSimd(playout->F4_0, func(GetRegRawSimd(playout->F4_1), GetRegRawSimd(playout->F4_2), GetRegRawSimd(playout->F4_3))); \
    break; \
    }
#define PROCESS_SIMD_F4_3toF4_1(name, func) PROCESS_SIMD_F4_3toF4_1_COMMON(name, func,)

#define PROCESS_SIMD_B4_1I4_2toI4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_1Bool32x4_1Int32x4_2, suffix); \
    SetRegRawSimd(playout->I4_0, func(GetRegRawSimd(playout->B4_1), GetRegRawSimd(playout->I4_2), GetRegRawSimd(playout->I4_3))); \
    break; \
    }
#define PROCESS_SIMD_B4_1I4_2toI4_1(name, func) PROCESS_SIMD_B4_1I4_2toI4_1_COMMON(name, func,)

#define PROCESS_SIMD_B8_1I8_2toI8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int16x8_1Bool16x8_1Int16x8_2, suffix); \
    SetRegRawSimd(playout->I8_0, func(GetRegRawSimd(playout->B8_1), GetRegRawSimd(playout->I8_2), GetRegRawSimd(playout->I8_3))); \
    break; \
    }
#define PROCESS_SIMD_B8_1I8_2toI8_1(name, func) PROCESS_SIMD_B8_1I8_2toI8_1_COMMON(name, func,)

#define PROCESS_SIMD_B16_1I16_2toI16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int8x16_1Bool8x16_1Int8x16_2, suffix); \
    SetRegRawSimd(playout->I16_0, func(GetRegRawSimd(playout->B16_1), GetRegRawSimd(playout->I16_2), GetRegRawSimd(playout->I16_3))); \
    break; \
    }
#define PROCESS_SIMD_B16_1I16_2toI16_1(name, func) PROCESS_SIMD_B16_1I16_2toI16_1_COMMON(name, func,)

#define PROCESS_SIMD_B4_1U4_2toU4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_1Bool32x4_1Uint32x4_2, suffix); \
    SetRegRawSimd(playout->U4_0, func(GetRegRawSimd(playout->B4_1), GetRegRawSimd(playout->U4_2), GetRegRawSimd(playout->U4_3))); \
    break; \
    }
#define PROCESS_SIMD_B4_1U4_2toU4_1(name, func) PROCESS_SIMD_B4_1U4_2toU4_1_COMMON(name, func,)

#define PROCESS_SIMD_B8_1U8_2toU8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint16x8_1Bool16x8_1Uint16x8_2, suffix); \
    SetRegRawSimd(playout->U8_0, func(GetRegRawSimd(playout->B8_1), GetRegRawSimd(playout->U8_2), GetRegRawSimd(playout->U8_3))); \
    break; \
    }
#define PROCESS_SIMD_B8_1U8_2toU8_1(name, func) PROCESS_SIMD_B8_1U8_2toU8_1_COMMON(name, func,)

#define PROCESS_SIMD_B16_1U16_2toU16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint8x16_1Bool8x16_1Uint8x16_2, suffix); \
    SetRegRawSimd(playout->U16_0, func(GetRegRawSimd(playout->B16_1), GetRegRawSimd(playout->U16_2), GetRegRawSimd(playout->U16_3))); \
    break; \
    }
#define PROCESS_SIMD_B16_1U16_2toU16_1(name, func) PROCESS_SIMD_B16_1U16_2toU16_1_COMMON(name, func,)

#define PROCESS_SIMD_D2_3toD2_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float64x2_4, suffix); \
    SetRegRawSimd(playout->D2_0, func(GetRegRawSimd(playout->D2_1), GetRegRawSimd(playout->D2_2), GetRegRawSimd(playout->D2_3))); \
    break; \
    }
#define PROCESS_SIMD_D2_3toD2_1(name, func) PROCESS_SIMD_D2_3toD2_1_COMMON(name, func,)

#define PROCESS_SIMD_B4_1F4_2toF4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_1Bool32x4_1Float32x4_2, suffix); \
    SetRegRawSimd(playout->F4_0, func(GetRegRawSimd(playout->B4_1), GetRegRawSimd(playout->F4_2), GetRegRawSimd(playout->F4_3))); \
    break; \
    }
#define PROCESS_SIMD_B4_1F4_2toF4_1(name, func) PROCESS_SIMD_B4_1F4_2toF4_1_COMMON(name, func,)

#define PROCESS_SIMD_I4_1D2_2toD2_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float64x2_1Int32x4_1Float64x2_2, suffix); \
    SetRegRawSimd(playout->D2_0, func(GetRegRawSimd(playout->I4_1), GetRegRawSimd(playout->D2_2), GetRegRawSimd(playout->D2_3))); \
    break; \
    }
#define PROCESS_SIMD_I4_1D2_2toD2_1(name, func) PROCESS_SIMD_I4_1D2_2toD2_1_COMMON(name, func,)

// Extract Lane
#define PROCESS_SIMD_F4_1I1toF1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float1Float32x4_1Int1, suffix); \
    SetRegRawFloat(playout->F0, func(GetRegRawSimd(playout->F4_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_F4_1I1toF1(name, func) PROCESS_SIMD_F4_1I1toF1_COMMON(name, func,)

#define PROCESS_SIMD_I4_1I1toI1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int1Int32x4_1Int1, suffix); \
    SetRegRawInt(playout->I0, func(GetRegRawSimd(playout->I4_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_I4_1I1toI1(name, func) PROCESS_SIMD_I4_1I1toI1_COMMON(name, func,)

#define PROCESS_SIMD_I8_1I1toI1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int1Int16x8_1Int1, suffix); \
    SetRegRawInt(playout->I0, func(GetRegRawSimd(playout->I8_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_I8_1I1toI1(name, func) PROCESS_SIMD_I8_1I1toI1_COMMON(name, func,)

#define PROCESS_SIMD_I16_1I1toI1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int1Int8x16_1Int1, suffix); \
    SetRegRawInt(playout->I0, func(GetRegRawSimd(playout->I16_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_I16_1I1toI1(name, func, suffix) PROCESS_SIMD_I16_1I1toI1_COMMON(name, func, suffix)


#define PROCESS_SIMD_U4_1I1toI1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int1Uint32x4_1Int1, suffix); \
    SetRegRawInt(playout->I0, func(GetRegRawSimd(playout->U4_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_U4_1I1toI1(name, func) PROCESS_SIMD_U4_1I1toI1_COMMON(name, func,)

#define PROCESS_SIMD_U8_1I1toI1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int1Uint16x8_1Int1, suffix); \
    SetRegRawInt(playout->I0, (uint16)func(GetRegRawSimd(playout->U8_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_U8_1I1toI1(name, func) PROCESS_SIMD_U8_1I1toI1_COMMON(name, func,)

#define PROCESS_SIMD_U16_1I1toI1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int1Uint8x16_1Int1, suffix); \
    SetRegRawInt(playout->I0, (uint8)func(GetRegRawSimd(playout->U16_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_U16_1I1toI1(name, func) PROCESS_SIMD_U16_1I1toI1_COMMON(name, func,)

#define PROCESS_SIMD_B4_1I1toI1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int1Bool32x4_1Int1, suffix); \
    SetRegRawInt(playout->I0, func(GetRegRawSimd(playout->B4_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_B4_1I1toI1(name, func) PROCESS_SIMD_B4_1I1toI1_COMMON(name, func,)

#define PROCESS_SIMD_B8_1I1toI1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int1Bool16x8_1Int1, suffix); \
    SetRegRawInt(playout->I0, (uint16)func(GetRegRawSimd(playout->B8_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_B8_1I1toI1(name, func) PROCESS_SIMD_B8_1I1toI1_COMMON(name, func,)

#define PROCESS_SIMD_B16_1I1toI1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int1Bool8x16_1Int1, suffix); \
    SetRegRawInt(playout->I0, (uint8)func(GetRegRawSimd(playout->B16_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_B16_1I1toI1(name, func) PROCESS_SIMD_B16_1I1toI1_COMMON(name, func,)

// Replace Lane
#define PROCESS_SIMD_I4_1I2toI4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_2Int2, suffix); \
    SetRegRawSimd(playout->I4_0, func(GetRegRawSimd(playout->I4_1), GetRegRawInt(playout->I2), GetRegRawInt(playout->I3))); \
    break; \
    }
#define PROCESS_SIMD_I4_1I2toI4_1(name, func) PROCESS_SIMD_I4_1I2toI4_1_COMMON(name, func,)

#define PROCESS_SIMD_I16_1I2toI16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int8x16_2Int2, suffix); \
    SetRegRawSimd(playout->I16_0, func(GetRegRawSimd(playout->I16_1), GetRegRawInt(playout->I2), static_cast<int8>(GetRegRawInt(playout->I3)))); \
    break; \
    }
#define PROCESS_SIMD_I16_1I2toI16_1(name, func, suffix) PROCESS_SIMD_I16_1I2toI16_1_COMMON(name, func, suffix)

#define PROCESS_SIMD_F4_1I1F1toF4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_2Int1Float1, suffix); \
    SetRegRawSimd(playout->F4_0, func(GetRegRawSimd(playout->F4_1), GetRegRawInt(playout->I2), GetRegRawFloat(playout->F3))); \
    break; \
    }
#define PROCESS_SIMD_F4_1I1F1toF4_1_1(name, func) PROCESS_SIMD_F4_1I1F1toF4_1_COMMON(name, func,)

#define PROCESS_SIMD_I8_1I2toI8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int16x8_2Int2, suffix); \
    SetRegRawSimd(playout->I8_0, func(GetRegRawSimd(playout->I8_1), GetRegRawInt(playout->I2), (int16)GetRegRawInt(playout->I3))); \
    break; \
    }
#define PROCESS_SIMD_I8_1I2toI8_1(name, func) PROCESS_SIMD_I8_1I2toI8_1_COMMON(name, func,)

#define PROCESS_SIMD_U4_1I2toU4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_2Int2, suffix); \
    SetRegRawSimd(playout->U4_0, func(GetRegRawSimd(playout->U4_1), GetRegRawInt(playout->I2), (uint32)GetRegRawInt(playout->I3))); \
    break; \
    }
#define PROCESS_SIMD_U4_1I2toU4_1(name, func) PROCESS_SIMD_U4_1I2toU4_1_COMMON(name, func,)

#define PROCESS_SIMD_U8_1I2toU8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint16x8_2Int2, suffix); \
    SetRegRawSimd(playout->U8_0, func(GetRegRawSimd(playout->U8_1), GetRegRawInt(playout->I2), (uint16)GetRegRawInt(playout->I3))); \
    break; \
    }
#define PROCESS_SIMD_U8_1I2toU8_1(name, func) PROCESS_SIMD_U8_1I2toU8_1_COMMON(name, func,)

#define PROCESS_SIMD_U16_1I2toU16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint8x16_2Int2, suffix); \
    SetRegRawSimd(playout->U16_0, func(GetRegRawSimd(playout->U16_1), GetRegRawInt(playout->I2), (uint8)GetRegRawInt(playout->I3))); \
    break; \
    }
#define PROCESS_SIMD_U16_1I2toU16_1_1(name, func) PROCESS_SIMD_U16_1I2toU16_1_COMMON(name, func,)

#define PROCESS_SIMD_B4_1I2toB4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool32x4_2Int2, suffix); \
    SetRegRawSimd(playout->B4_0, func(GetRegRawSimd(playout->B4_1), GetRegRawInt(playout->I2), (GetRegRawInt(playout->I3)) ? -1 : 0)); \
    break; \
    }
#define PROCESS_SIMD_B4_1I2toB4_1(name, func) PROCESS_SIMD_B4_1I2toB4_1_COMMON(name, func,)

#define PROCESS_SIMD_B8_1I2toB8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool16x8_2Int2, suffix); \
    SetRegRawSimd(playout->B8_0, func(GetRegRawSimd(playout->B8_1), GetRegRawInt(playout->I2), (GetRegRawInt(playout->I3)) ? -1 : 0)); \
    break; \
    }
#define PROCESS_SIMD_B8_1I2toB8_1(name, func) PROCESS_SIMD_B8_1I2toB8_1_COMMON(name, func,)

#define PROCESS_SIMD_B16_1I2toB16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Bool8x16_2Int2, suffix); \
    SetRegRawSimd(playout->B16_0, func(GetRegRawSimd(playout->B16_1), GetRegRawInt(playout->I2), (GetRegRawInt(playout->I3)) ? -1 : 0)); \
    break; \
    }
#define PROCESS_SIMD_B16_1I2toB16_1_1(name, func) PROCESS_SIMD_B16_1I2toB16_1_COMMON(name, func,)

// f4swizzle
#define PROCESS_SIMD_F4_1I4toF4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_2Int4, suffix); \
    uint32 lanes[4]; \
    lanes[0] = GetRegRawInt(playout->I2);   lanes[1] = GetRegRawInt(playout->I3); \
    lanes[2] = GetRegRawInt(playout->I4);   lanes[3] = GetRegRawInt(playout->I5); \
    SetRegRawSimd(playout->F4_0, func(GetRegRawSimd(playout->F4_1), GetRegRawSimd(playout->F4_1), 4, lanes)); \
    break; \
    }
#define PROCESS_SIMD_F4_1I4toF4_1(name, func) PROCESS_SIMD_F4_1I4toF4_1_COMMON(name, func,)

// f4shuffle
#define PROCESS_SIMD_F4_2I4toF4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float32x4_3Int4, suffix); \
    uint32 lanes[4]; \
    lanes[0] = GetRegRawInt(playout->I3); lanes[1] = GetRegRawInt(playout->I4); \
    lanes[2] = GetRegRawInt(playout->I5); lanes[3] = GetRegRawInt(playout->I6); \
    SetRegRawSimd(playout->F4_0, func(GetRegRawSimd(playout->F4_1), GetRegRawSimd(playout->F4_2), 4, lanes)); \
    break; \
    }
#define PROCESS_SIMD_F4_1I4toF4_1(name, func) PROCESS_SIMD_F4_1I4toF4_1_COMMON(name, func,)

// i4swizzle
#define PROCESS_SIMD_I4_1I4toI4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_2Int4, suffix); \
    uint32 lanes[4]; \
    lanes[0] = GetRegRawInt(playout->I2);   lanes[1] = GetRegRawInt(playout->I3); \
    lanes[2] = GetRegRawInt(playout->I4);   lanes[3] = GetRegRawInt(playout->I5); \
    SetRegRawSimd(playout->I4_0, func(GetRegRawSimd(playout->I4_1), GetRegRawSimd(playout->I4_1), 4, lanes)); \
    break; \
    }
#define PROCESS_SIMD_I4_1I4toI4_1(name, func) PROCESS_SIMD_I4_1I4toI4_1_COMMON(name, func,)

// i4shuffle
#define PROCESS_SIMD_I4_2I4toI4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_3Int4, suffix); \
    uint32 lanes[4]; \
    lanes[0] = GetRegRawInt(playout->I3); lanes[1] = GetRegRawInt(playout->I4); \
    lanes[2] = GetRegRawInt(playout->I5); lanes[3] = GetRegRawInt(playout->I6); \
    SetRegRawSimd(playout->I4_0, func(GetRegRawSimd(playout->I4_1), GetRegRawSimd(playout->I4_2), 4, lanes)); \
    break; \
    }
#define PROCESS_SIMD_I4_1I4toI4_1(name, func) PROCESS_SIMD_I4_1I4toI4_1_COMMON(name, func,)


// i8swizzle
#define PROCESS_SIMD_I8_1I8toI8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int16x8_2Int8, suffix); \
    uint32 lanes[8]; \
    lanes[0] = GetRegRawInt(playout->I2); lanes[1] = GetRegRawInt(playout->I3); \
    lanes[2] = GetRegRawInt(playout->I4); lanes[3] = GetRegRawInt(playout->I5); \
    lanes[4] = GetRegRawInt(playout->I6); lanes[5] = GetRegRawInt(playout->I7); \
    lanes[6] = GetRegRawInt(playout->I8); lanes[7] = GetRegRawInt(playout->I9); \
    SetRegRawSimd(playout->I8_0, func(GetRegRawSimd(playout->I8_1), GetRegRawSimd(playout->I8_1), 8, lanes)); \
    break; \
    }
#define PROCESS_SIMD_I8_1I8toI8_1(name, func) PROCESS_SIMD_I8_1I8toI8_1_COMMON(name, func,)

// i8shuffle
#define PROCESS_SIMD_I8_2I8toI8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int16x8_3Int8, suffix); \
    uint32 lanes[8]; \
    lanes[0] = GetRegRawInt(playout->I3); lanes[1] = GetRegRawInt(playout->I4); \
    lanes[2] = GetRegRawInt(playout->I5); lanes[3] = GetRegRawInt(playout->I6); \
    lanes[4] = GetRegRawInt(playout->I7); lanes[5] = GetRegRawInt(playout->I8); \
    lanes[6] = GetRegRawInt(playout->I9); lanes[7] = GetRegRawInt(playout->I10); \
    SetRegRawSimd(playout->I8_0, func(GetRegRawSimd(playout->I8_1), GetRegRawSimd(playout->I8_2), 8, lanes)); \
    break; \
    }
#define PROCESS_SIMD_I8_1I8toI8_1(name, func) PROCESS_SIMD_I8_1I8toI8_1_COMMON(name, func,)

// u4swizzle
#define PROCESS_SIMD_U4_1I4toU4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_2Int4, suffix); \
    uint32 lanes[4]; \
    lanes[0] = GetRegRawInt(playout->I2);   lanes[1] = GetRegRawInt(playout->I3); \
    lanes[2] = GetRegRawInt(playout->I4);   lanes[3] = GetRegRawInt(playout->I5); \
    SetRegRawSimd(playout->U4_0, func(GetRegRawSimd(playout->U4_1), GetRegRawSimd(playout->U4_1), 4, lanes)); \
    break; \
    }
#define PROCESS_SIMD_U4_1I4toU4_1(name, func) PROCESS_SIMD_U4_1I4toU4_1_COMMON(name, func,)

// u4shuffle
#define PROCESS_SIMD_U4_2I4toU4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_3Int4, suffix); \
    uint32 lanes[4]; \
    lanes[0] = GetRegRawInt(playout->I3); lanes[1] = GetRegRawInt(playout->I4); \
    lanes[2] = GetRegRawInt(playout->I5); lanes[3] = GetRegRawInt(playout->I6); \
    SetRegRawSimd(playout->U4_0, func(GetRegRawSimd(playout->U4_1), GetRegRawSimd(playout->U4_2), 4, lanes)); \
    break; \
    }
#define PROCESS_SIMD_U4_1I4toU4_1(name, func) PROCESS_SIMD_U4_1I4toU4_1_COMMON(name, func,)

// u8swizzle
#define PROCESS_SIMD_U8_1I8toU8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint16x8_2Int8, suffix); \
    uint32 lanes[8]; \
    lanes[0] = GetRegRawInt(playout->I2); lanes[1] = GetRegRawInt(playout->I3); \
    lanes[2] = GetRegRawInt(playout->I4); lanes[3] = GetRegRawInt(playout->I5); \
    lanes[4] = GetRegRawInt(playout->I6); lanes[5] = GetRegRawInt(playout->I7); \
    lanes[6] = GetRegRawInt(playout->I8); lanes[7] = GetRegRawInt(playout->I9); \
    SetRegRawSimd(playout->U8_0, func(GetRegRawSimd(playout->U8_1), GetRegRawSimd(playout->U8_1), 8, lanes)); \
    break; \
    }
#define PROCESS_SIMD_U8_1I8toU8_1(name, func) PROCESS_SIMD_U8_1I8toU8_1_COMMON(name, func,)

// u8shuffle
#define PROCESS_SIMD_U8_2I8toU8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint16x8_3Int8, suffix); \
    uint32 lanes[8]; \
    lanes[0] = GetRegRawInt(playout->I3); lanes[1] = GetRegRawInt(playout->I4); \
    lanes[2] = GetRegRawInt(playout->I5); lanes[3] = GetRegRawInt(playout->I6); \
    lanes[4] = GetRegRawInt(playout->I7); lanes[5] = GetRegRawInt(playout->I8); \
    lanes[6] = GetRegRawInt(playout->I9); lanes[7] = GetRegRawInt(playout->I10); \
    SetRegRawSimd(playout->U8_0, func(GetRegRawSimd(playout->U8_1), GetRegRawSimd(playout->U8_2), 8, lanes)); \
    break; \
    }
#define PROCESS_SIMD_U8_1I8toU8_1(name, func) PROCESS_SIMD_U8_1I8toU8_1_COMMON(name, func,)

// u16swizzle
#define PROCESS_SIMD_U16_1I16toU16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint8x16_2Int16, suffix); \
    uint32 lanes[16]; \
    lanes[0] = GetRegRawInt(playout->I2);   lanes[1] = GetRegRawInt(playout->I3); \
    lanes[2] = GetRegRawInt(playout->I4);   lanes[3] = GetRegRawInt(playout->I5); \
    lanes[4] = GetRegRawInt(playout->I6);   lanes[5] = GetRegRawInt(playout->I7); \
    lanes[6] = GetRegRawInt(playout->I8);   lanes[7] = GetRegRawInt(playout->I9); \
    lanes[8] = GetRegRawInt(playout->I10);  lanes[9] = GetRegRawInt(playout->I11); \
    lanes[10] = GetRegRawInt(playout->I12); lanes[11] = GetRegRawInt(playout->I13); \
    lanes[12] = GetRegRawInt(playout->I14); lanes[13] = GetRegRawInt(playout->I15); \
    lanes[14] = GetRegRawInt(playout->I16); lanes[15] = GetRegRawInt(playout->I17); \
    SetRegRawSimd(playout->U16_0, func(GetRegRawSimd(playout->U16_1), GetRegRawSimd(playout->U16_1), 16,lanes)); \
    break; \
    }
#define PROCESS_SIMD_U16_1I16toU16_1(name, func) PROCESS_SIMD_U16_1I16toU16_1_COMMON(name, func,)

// u16shuffle
#define PROCESS_SIMD_U16_2I16toU16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint8x16_3Int16, suffix); \
    uint32 lanes[16]; \
    lanes[0] = GetRegRawInt(playout->I3);   lanes[1] = GetRegRawInt(playout->I4); \
    lanes[2] = GetRegRawInt(playout->I5);   lanes[3] = GetRegRawInt(playout->I6); \
    lanes[4] = GetRegRawInt(playout->I7);   lanes[5] = GetRegRawInt(playout->I8); \
    lanes[6] = GetRegRawInt(playout->I9);  lanes[7] = GetRegRawInt(playout->I10); \
    lanes[8] = GetRegRawInt(playout->I11);  lanes[9] = GetRegRawInt(playout->I12); \
    lanes[10] = GetRegRawInt(playout->I13); lanes[11] = GetRegRawInt(playout->I14); \
    lanes[12] = GetRegRawInt(playout->I15); lanes[13] = GetRegRawInt(playout->I16); \
    lanes[14] = GetRegRawInt(playout->I17); lanes[15] = GetRegRawInt(playout->I18); \
    SetRegRawSimd(playout->U16_0, func(GetRegRawSimd(playout->U16_1), GetRegRawSimd(playout->U16_2), 16, lanes)); \
    break; \
    }
#define PROCESS_SIMD_U16_2I16toU16_1(name, func) PROCESS_SIMD_U16_2I16toU16_1_COMMON(name, func,)

// d2swizzle
#define PROCESS_SIMD_D2_1I2toD2_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float64x2_2Int2, suffix); \
    SetRegRawSimd(playout->D2_0, func<2>(GetRegRawSimd(playout->D2_1), GetRegRawSimd(playout->D2_1), GetRegRawInt(playout->I2), GetRegRawInt(playout->I3), 0, 0)); \
    break; \
    }
#define PROCESS_SIMD_D2_1I2toD2_1(name, func) PROCESS_SIMD_D2_1I2toD2_1_COMMON(name, func,)

// d2shuffle
#define PROCESS_SIMD_D2_2I2toD2_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Float64x2_3Int2, suffix); \
    SetRegRawSimd(playout->D2_0, func<2>(GetRegRawSimd(playout->D2_1), GetRegRawSimd(playout->D2_2), GetRegRawInt(playout->I3), GetRegRawInt(playout->I4), 0, 0)); \
    break; \
    }
#define PROCESS_SIMD_D2_2I4toD2_1(name, func) PROCESS_SIMD_D2_2I2toD2_1_COMMON(name, func,)

#define PROCESS_SIMD_I4_1I1toI4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int32x4_2Int1, suffix); \
    SetRegRawSimd(playout->I4_0, func(GetRegRawSimd(playout->I4_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_I4_1I1toI4_1(name, func) PROCESS_SIMD_I4_1I1toI4_1_COMMON(name, func,)


#define PROCESS_SIMD_I8_1I1toI8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int16x8_2Int1, suffix); \
    SetRegRawSimd(playout->I8_0, func(GetRegRawSimd(playout->I8_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_I8_1I1toI8_1(name, func) PROCESS_SIMD_I8_1I1toI8_1_COMMON(name, func,)

#define PROCESS_SIMD_I16_1I1toI16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Int8x16_2Int1, suffix); \
    SetRegRawSimd(playout->I16_0, func(GetRegRawSimd(playout->I16_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_I16_1I1toI16_1(name, func) PROCESS_SIMD_I16_1I1toI16_1_COMMON(name, func,)

#define PROCESS_SIMD_U4_1I1toU4_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint32x4_2Int1, suffix); \
    SetRegRawSimd(playout->U4_0, func(GetRegRawSimd(playout->U4_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_U4_1I1toU4_1(name, func) PROCESS_SIMD_U4_1I1toU4_1_COMMON(name, func,)

#define PROCESS_SIMD_U8_1I1toU8_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint16x8_2Int1, suffix); \
    SetRegRawSimd(playout->U8_0, func(GetRegRawSimd(playout->U8_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_U8_1I1toU8_1(name, func) PROCESS_SIMD_U8_1I1toU8_1_COMMON(name, func,)

#define PROCESS_SIMD_U16_1I1toU16_1_COMMON(name, func, suffix) \
    case OpCodeAsmJs::name: \
    { \
    PROCESS_READ_LAYOUT_ASMJS(name, Uint8x16_2Int1, suffix); \
    SetRegRawSimd(playout->U16_0, func(GetRegRawSimd(playout->U16_1), GetRegRawInt(playout->I2))); \
    break; \
    }
#define PROCESS_SIMD_U16_1I1toU16_1(name, func) PROCESS_SIMD_U16_1I1toU16_1_COMMON(name, func,)



#endif