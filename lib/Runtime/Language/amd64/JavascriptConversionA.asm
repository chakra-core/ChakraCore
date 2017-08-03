;-------------------------------------------------------------------------------------------------------
; Copyright (C) Microsoft. All rights reserved.
; Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
;-------------------------------------------------------------------------------------------------------
include ksamd64.inc

        _TEXT SEGMENT

align 16
?ULongToDouble@JavascriptConversion@Js@@SAN_K@Z PROC FRAME
        .endprolog

        test rcx, rcx
        js msbSet
        cvtsi2sd xmm0, rcx
        jmp doneULongToDouble
    msbSet:
        mov rdx, rcx
        and rcx, 1 ; Save lsb
        shr rdx, 1 ; divide by 2
        or rcx, rdx ; put back lsb if it was set
        cvtsi2sd xmm0, rcx ; do conversion
        addsd xmm0, xmm0 ; xmm0 * 2
    doneULongToDouble:
        ret

?ULongToDouble@JavascriptConversion@Js@@SAN_K@Z ENDP

       _TEXT ENDS
        end