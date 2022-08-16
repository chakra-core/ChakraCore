//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#include <wchar.h>

// =================
// Runtime Includes
// =================
#include "Runtime.h"
#include "ByteCode/StatementReader.h"
#include "Language/EHBailoutData.h"
#include "Language/AsmJsTypes.h"
#include "Language/AsmJsModule.h"
#include "Language/ProfilingHelpers.h"
#include "Language/FunctionCodeGenRuntimeData.h"
#include "Language/JavascriptMathOperators.h"
#include "Language/JavascriptMathOperators.inl"
#include "Language/JavascriptStackWalker.h"
#include "Language/CodeGenRecyclableData.h"
#include "Library/Generators/JavascriptGenerator.h"
#include "Library/Regex/JavascriptRegularExpression.h"
#include "Library/Functions/StackScriptFunction.h"
#include "Library/JavascriptProxy.h"
#include "Library/Generators/JavascriptGeneratorFunction.h"

#include "Language/InterpreterStackFrame.h"

#include "Library/Functions/StackScriptFunction.h"

// SIMD
#include "Language/SimdOps.h"

// =================
// Common Includes
// =================
#include "DataStructures/Pair.h"
#include "DataStructures/HashTable.h"
// =================

//
// Defines
//

#define Fatal()     Js::Throw::FatalInternalError()

// By default, do encode large user constants for security.
#ifndef MD_ENCODE_LG_CONSTS
#define MD_ENCODE_LG_CONSTS true
#endif

//
// Forward refs
//

class Func;
class Loop;


//
// Typedefs
//

const int32     IntConstMax = INT_MAX;
const int32     IntConstMin = INT_MIN;
const int32     Int8ConstMax = _I8_MAX;
const int32     Int8ConstMin = _I8_MIN;
const int32     Int16ConstMax = _I16_MAX;
const int32     Int16ConstMin = _I16_MIN;
const int32     Int32ConstMax = _I32_MAX;
const int32     Int32ConstMin = _I32_MIN;
const int32     Uint8ConstMax = _UI8_MAX;
const int32     Uint8ConstMin = 0;
const int32     Uint16ConstMax = _UI16_MAX;
const int32     Uint16ConstMin = 0;

#if defined(_M_X64) || defined(_M_ARM32_OR_ARM64)
// Arm VFPv3-D32 has 32 double registers and 16 int registers total 48.
// Arm64 has 32 vector registers and 32 int registers total 64.
// Amd64 has 16 int and 16 xmm registers and slot for NOREG makes it 33 hence 64 bit version
// for Amd64 & Arm
    typedef BVUnit64  BitVector;
#else
// x86 has only 8 int registers & 8 xmm registers
// 32 bit vector is sufficient to address all the registers
    typedef BVUnit32 BitVector;
#endif

#if DBG_DUMP || defined(ENABLE_IR_VIEWER)
enum IRDumpFlags
{
    IRDumpFlags_None = 0x0,
    IRDumpFlags_AsmDumpMode = 0x1,
    IRDumpFlags_SimpleForm = 0x2,
    IRDumpFlags_SkipEndLine = 0x4,
    IRDumpFlags_SkipByteCodeOffset = 0x8,
};
#endif

//
// BackEnd includes
//

#include "JITTimeProfileInfo.h"
#include "JITRecyclableObject.h"
#include "FixedFieldInfo.h"
#include "JITTimePolymorphicInlineCache.h"
#include "JITTimePolymorphicInlineCacheInfo.h"
#include "CodeGenWorkItemType.h"
#include "CodeGenAllocators.h"
#include "JITTimeConstructorCache.h"
#include "JITTypeHandler.h"
#include "JITType.h"
#include "EquivalentTypeSet.h"
#include "ObjTypeSpecFldInfo.h"
#include "FunctionCodeGenJitTimeData.h"
#include "ServerScriptContext.h"
#include "JITOutput.h"
#include "AsmJsJITInfo.h"
#include "FunctionJITRuntimeInfo.h"
#include "JITTimeFunctionBody.h"
#include "FunctionJITTimeInfo.h"
#include "JITTimeWorkItem.h"
#include "NativeCodeData.h"
#include "IRType.h"
#include "md.h"
#include "../Runtime/ByteCode/BackendOpCodeAttr.h"
#include "BackendOpCodeAttrAsmJs.h"

#include "JnHelperMethod.h"
#include "Reg.h"
#include "Sym.h"
#include "SymTable.h"
#include "IR.h"
#include "Opnd.h"
#include "IntConstMath.h"
#include "IntOverflowDoesNotMatterRange.h"
#include "IntConstantBounds.h"
#include "ValueRelativeOffset.h"
#include "IntBounds.h"
#include "InductionVariable.h"
#include "ValueInfo.h"
#include "GlobOptBlockData.h"
#include "GlobOpt.h"
#include "GlobOptIntBounds.h"
#include "GlobOptArrays.h"
#include "QueuedFullJitWorkItem.h"
#include "CodeGenWorkItem.h"
#include "SimpleJitProfilingHelpers.h"
#if defined(_M_X64)
#include "PrologEncoder.h"
#endif
#include "Func.h"
#include "TempTracker.h"
#include "FlowGraph.h"

#include "PDataManager.h"

#include "ServerThreadContext.h"
#include "CaseNode.h"
#include "SwitchIRBuilder.h"
#include "IRBuilder.h"
#include "IRBuilderAsmJs.h"
#include "BackwardPass.h"
#include "Lower.h"
#include "Security.h"
#include "Peeps.h"
#include "LinearScan.h"
#include "SimpleLayout.h"
#include "Encoder.h"
#include "EmitBuffer.h"
#include "InterpreterThunkEmitter.h"
#include "JITThunkEmitter.h"
#include "InliningHeuristics.h"
#include "InliningDecider.h"
#include "Inline.h"
#include "NativeCodeGenerator.h"
#include "Region.h"
#include "BailOut.h"
#include "InlineeFrameInfo.h"
#include "IRViewer.h"

#if DBG
# include "DbCheckPostLower.h"
#endif

//
// Inlines
//

#include "Sym.inl"
#include "IR.inl"
#include "Opnd.inl"

