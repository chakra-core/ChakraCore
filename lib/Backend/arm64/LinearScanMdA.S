;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Copyright (c) ChakraCore Project Contributors. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------

#include "unixasmmacros.inc"

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LinearScanMD::SaveAllRegistersAndBailOut(BailOutRecord *const bailOutRecord)

NESTED_ENTRY _ZN12LinearScanMD26SaveAllRegistersAndBailOutEP13BailOutRecord, _TEXT, NoHandler

    ; x0 == bailOutRecord
    ; lr == return address

    ; Save all registers except the above, which would have already been saved by jitted code if necessary
    ldr x17, [x0] ; bailOutRecord->globalBailOutRecordDataTable
    ldr x17, [x17] ; bailOutRecord->globalBailOutRecordDataTable->registerSaveSpace
    str x1, [x17, #1*8]
    stp x2, x3, [x17, #2*8]
    stp x4, x5, [x17, #4*8]
    stp x6, x7, [x17, #6*8]
    stp x8, x9, [x17, #8*8]
    stp x10, x11, [x17, #10*8]
    stp x12, x13, [x17, #12*8]
    stp x14, x15, [x17, #14*8]
    str x16, [x17, #16*8]
    ; skip x17/x18
    stp x19, x20, [x17, #19*8]
    stp x21, x22, [x17, #21*8]
    stp x23, x24, [x17, #23*8]
    stp x25, x26, [x17, #25*8]
    stp x27, x28, [x17, #27*8]
    str fp, [x17, #29*8]
    ; skip lr, sp, zr
    add x17, x17, #33*8
    stp d0, d1, [x17, #0*8]
    stp d2, d3, [x17, #2*8]
    stp d4, d5, [x17, #4*8]
    stp d6, d7, [x17, #6*8]
    stp d8, d9, [x17, #8*8]
    stp d10, d11, [x17, #10*8]
    stp d12, d13, [x17, #12*8]
    stp d14, d15, [x17, #14*8]
    stp d16, d17, [x17, #16*8]
    stp d18, d19, [x17, #18*8]
    stp d20, d21, [x17, #20*8]
    stp d22, d23, [x17, #22*8]
    stp d24, d25, [x17, #24*8]
    stp d26, d27, [x17, #26*8]
    stp d28, d29, [x17, #28*8]
    ;stp d30, d31, [x17, #30*8]

    b C_FUNC(_ZN13BailOutRecord7BailOutEPKS_)

NESTED_END _ZN12LinearScanMD26SaveAllRegistersAndBailOutEP13BailOutRecord, _TEXT

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LinearScanMD::SaveAllRegistersAndBranchBailOut(BranchBailOutRecord *const bailOutRecord, const BOOL condition)

NESTED_ENTRY _ZN12LinearScanMD32SaveAllRegistersAndBranchBailOutEP19BranchBailOutRecordi, _TEXT, NoHandler

    ; x0 == bailOutRecord
    ; x1 == condition
    ; lr == return address

    ; Save all registers except the above, which would have already been saved by jitted code if necessary
    ldr x17, [x0] ; bailOutRecord->globalBailOutRecordDataTable
    ldr x17, [x17] ; bailOutRecord->globalBailOutRecordDataTable->registerSaveSpace
    stp x2, x3, [x17, #2*8]
    stp x4, x5, [x17, #4*8]
    stp x6, x7, [x17, #6*8]
    stp x8, x9, [x17, #8*8]
    stp x10, x11, [x17, #10*8]
    stp x12, x13, [x17, #12*8]
    stp x14, x15, [x17, #14*8]
    str x16, [x17, #16*8]
    ; skip x17/x18
    stp x19, x20, [x17, #19*8]
    stp x21, x22, [x17, #21*8]
    stp x23, x24, [x17, #23*8]
    stp x25, x26, [x17, #25*8]
    stp x27, x28, [x17, #27*8]
    str fp, [x17, #29*8]
    ; skip lr, sp, zr
    add x17, x17, #33*8
    stp d0, d1, [x17, #0*8]
    stp d2, d3, [x17, #2*8]
    stp d4, d5, [x17, #4*8]
    stp d6, d7, [x17, #6*8]
    stp d8, d9, [x17, #8*8]
    stp d10, d11, [x17, #10*8]
    stp d12, d13, [x17, #12*8]
    stp d14, d15, [x17, #14*8]
    stp d16, d17, [x17, #16*8]
    stp d18, d19, [x17, #18*8]
    stp d20, d21, [x17, #20*8]
    stp d22, d23, [x17, #22*8]
    stp d24, d25, [x17, #24*8]
    stp d26, d27, [x17, #26*8]
    stp d28, d29, [x17, #28*8]
    ;stp d30, d31, [x17, #30*8]

    b C_FUNC(_ZN19BranchBailOutRecord7BailOutEPKS_i)

NESTED_END _ZN12LinearScanMD32SaveAllRegistersAndBranchBailOutEP19BranchBailOutRecordi, _TEXT
