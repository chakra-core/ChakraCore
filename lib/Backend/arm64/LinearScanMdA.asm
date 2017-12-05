;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------

    OPT 2 ; disable listing
#include "ksarm64.h"
    OPT 1 ; re-enable listing

    TTL Lib\Backend\arm64\LinearScanMdA.asm

    EXPORT |?SaveAllRegistersAndBailOut@LinearScanMD@@SAXQEAVBailOutRecord@@@Z|
    EXPORT |?SaveAllRegistersAndBranchBailOut@LinearScanMD@@SAXQEAVBranchBailOutRecord@@H@Z|

    ; BailOutRecord::BailOut(BailOutRecord const * bailOutRecord)
    IMPORT |?BailOut@BailOutRecord@@SAPEAXPEBV1@@Z|

    ; BranchBailOutRecord::BailOut(BranchBailOutRecord const * bailOutRecord, BOOL cond)
    IMPORT |?BailOut@BranchBailOutRecord@@SAPEAXPEBV1@H@Z|

    TEXTAREA

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LinearScanMD::SaveAllRegistersAndBailOut(BailOutRecord *const bailOutRecord)

    NESTED_ENTRY ?SaveAllRegistersAndBailOut@LinearScanMD@@SAXQEAVBailOutRecord@@@Z

    ; x0 == bailOutRecord
    ; lr == return address

    ; Save all registers except the above, which would have already been saved by jitted code if necessary
    ldr x16, [x0] ; bailOutRecord->globalBailOutRecordDataTable
    ldr x16, [x16] ; bailOutRecord->globalBailOutRecordDataTable->registerSaveSpace
    str x1, [x16, #1*8]
    stp x2, x3, [x16, #2*8]
    stp x4, x5, [x16, #4*8]
    stp x6, x7, [x16, #6*8]
    stp x8, x9, [x16, #8*8]
    stp x10, x11, [x16, #10*8]
    stp x12, x13, [x16, #12*8]
    stp x14, x15, [x16, #14*8]
    ; skip x16/x17/x18
    stp x19, x20, [x16, #19*8]
    stp x21, x22, [x16, #21*8]
    stp x23, x24, [x16, #23*8]
    stp x25, x26, [x16, #25*8]
    stp x27, x28, [x16, #27*8]
    str fp, [x16, #29*8]
    ; skip lr, sp, zr
    add x16, x16, #33*8
    stp d0, d1, [x16, #0*8]
    stp d2, d3, [x16, #2*8]
    stp d4, d5, [x16, #4*8]
    stp d6, d7, [x16, #6*8]
    stp d8, d9, [x16, #8*8]
    stp d10, d11, [x16, #10*8]
    stp d12, d13, [x16, #12*8]
    stp d14, d15, [x16, #14*8]
    stp d16, d17, [x16, #16*8]
    stp d18, d19, [x16, #18*8]
    stp d20, d21, [x16, #20*8]
    stp d22, d23, [x16, #22*8]
    stp d24, d25, [x16, #24*8]
    stp d26, d27, [x16, #26*8]
    stp d28, d29, [x16, #28*8]
    ;stp d30, d31, [x16, #30*8]

    b |?BailOut@BailOutRecord@@SAPEAXPEBV1@@Z|

    NESTED_END

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;
; LinearScanMD::SaveAllRegistersAndBranchBailOut(BranchBailOutRecord *const bailOutRecord, const BOOL condition)

    NESTED_ENTRY ?SaveAllRegistersAndBranchBailOut@LinearScanMD@@SAXQEAVBranchBailOutRecord@@H@Z

    ; x0 == bailOutRecord
    ; x1 == condition
    ; lr == return address

    ; Save all registers except the above, which would have already been saved by jitted code if necessary
    ldr x16, [x0] ; bailOutRecord->globalBailOutRecordDataTable
    ldr x16, [x16] ; bailOutRecord->globalBailOutRecordDataTable->registerSaveSpace
    stp x2, x3, [x16, #2*8]
    stp x4, x5, [x16, #4*8]
    stp x6, x7, [x16, #6*8]
    stp x8, x9, [x16, #8*8]
    stp x10, x11, [x16, #10*8]
    stp x12, x13, [x16, #12*8]
    stp x14, x15, [x16, #14*8]
    ; skip x16/x17/x18
    stp x19, x20, [x16, #19*8]
    stp x21, x22, [x16, #21*8]
    stp x23, x24, [x16, #23*8]
    stp x25, x26, [x16, #25*8]
    stp x27, x28, [x16, #27*8]
    str fp, [x16, #29*8]
    ; skip lr, sp, zr
    add x16, x16, #33*8
    stp d0, d1, [x16, #0*8]
    stp d2, d3, [x16, #2*8]
    stp d4, d5, [x16, #4*8]
    stp d6, d7, [x16, #6*8]
    stp d8, d9, [x16, #8*8]
    stp d10, d11, [x16, #10*8]
    stp d12, d13, [x16, #12*8]
    stp d14, d15, [x16, #14*8]
    stp d16, d17, [x16, #16*8]
    stp d18, d19, [x16, #18*8]
    stp d20, d21, [x16, #20*8]
    stp d22, d23, [x16, #22*8]
    stp d24, d25, [x16, #24*8]
    stp d26, d27, [x16, #26*8]
    stp d28, d29, [x16, #28*8]
    ;stp d30, d31, [x16, #30*8]

    b |?BailOut@BranchBailOutRecord@@SAPEAXPEBV1@H@Z|

    NESTED_END

;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;;

    END
