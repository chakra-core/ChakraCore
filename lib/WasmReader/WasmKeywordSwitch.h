//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// GENERATED FILE, DO NOT HAND MODIFIY
// Generated with the following command line: cscript SExprScan.js WasmKeywords.h WasmKeywordSwitch.h
// This should be regenerated whenever the keywords change

    case 'g':
        if (p[0] == 'e')
        {
            switch(p[1])
            {
            case 't':
                switch(p[2])
                {
                case 'p':
                    if (p[3] == 'a' && p[4] == 'r' && p[5] == 'a' && p[6] == 'm')
                    {
                    p += 7;
                    token = wtkGETPARAM;
                    goto LKeyword;
                    }
                    break;
                case '_':
                    if (p[3] == 'l' && p[4] == 'o' && p[5] == 'c' && p[6] == 'a' && p[7] == 'l')
                    {
                    p += 8;
                    token = wtkGETLOCAL;
                    goto LKeyword;
                    }
                    break;
                }
                break;
            }
        }
        goto LError;
    case 's':
        switch(p[0])
        {
        case 'e':
            switch(p[1])
            {
            case 't':
                if (p[2] == '_' && p[3] == 'l' && p[4] == 'o' && p[5] == 'c' && p[6] == 'a' && p[7] == 'l')
                {
                p += 8;
                token = wtkSETLOCAL;
                goto LKeyword;
                }
                break;
            case 'l':
                if (p[2] == 'e' && p[3] == 'c' && p[4] == 't')
                {
                p += 5;
                token = wtkSELECT;
                goto LKeyword;
                }
                break;
            }
            break;
        }
        goto LError;
    case 'c':
        if (p[0] == 'a' && p[1] == 'l')
        {
            switch(p[2])
            {
            case 'l':
                switch(p[3])
                {
                case '_':
                    switch(p[4])
                    {
                    case 'i':
                        switch(p[5])
                        {
                        case 'n':
                            if (p[6] == 'd' && p[7] == 'i' && p[8] == 'r' && p[9] == 'e' && p[10] == 'c' && p[11] == 't')
                            {
                            p += 12;
                            token = wtkCALL_INDIRECT;
                            goto LKeyword;
                            }
                            break;
                        case 'm':
                            if (p[6] == 'p' && p[7] == 'o' && p[8] == 'r' && p[9] == 't')
                            {
                            p += 10;
                            token = wtkCALL_IMPORT;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    }
                    break;
                }
                p += 3;
                token = wtkCALL;
                goto LKeyword;
                break;
            }
        }
        goto LError;
    case 'b':
        switch(p[0])
        {
        case 'l':
            if (p[1] == 'o' && p[2] == 'c' && p[3] == 'k')
            {
            p += 4;
            token = wtkBLOCK;
            goto LKeyword;
            }
            break;
        case 'r':
            switch(p[1])
            {
            case '_':
                switch(p[2])
                {
                case 'i':
                    if (p[3] == 'f')
                    {
                    p += 4;
                    token = wtkBR_IF;
                    goto LKeyword;
                    }
                    break;
                case 't':
                    if (p[3] == 'a' && p[4] == 'b' && p[5] == 'l' && p[6] == 'e')
                    {
                    p += 7;
                    token = wtkBR_TABLE;
                    goto LKeyword;
                    }
                    break;
                }
                break;
            }
            p += 1;
            token = wtkBR;
            goto LKeyword;
            break;
        }
        goto LError;
    case 'i':
        switch(p[0])
        {
        case 'f':
            switch(p[1])
            {
            case '_':
                if (p[2] == 'e' && p[3] == 'l' && p[4] == 's' && p[5] == 'e')
                {
                p += 6;
                token = wtkIF_ELSE;
                goto LKeyword;
                }
                break;
            }
            p += 1;
            token = wtkIF;
            goto LKeyword;
            break;
        case 'm':
            if (p[1] == 'p' && p[2] == 'o' && p[3] == 'r' && p[4] == 't')
            {
            p += 5;
            token = wtkIMPORT;
            goto LKeyword;
            }
            break;
        case '8':
            p += 1;
            token = wtkI8;
            goto LKeyword;
            break;
        case '1':
            if (p[1] == '6')
            {
            p += 2;
            token = wtkI16;
            goto LKeyword;
            }
            break;
        case '3':
            switch(p[1])
            {
            case '2':
                switch(p[2])
                {
                case '.':
                    switch(p[3])
                    {
                    case 't':
                        if (p[4] == 'r' && p[5] == 'u' && p[6] == 'n' && p[7] == 'c')
                        {
                            switch(p[8])
                            {
                            case '_':
                                switch(p[9])
                                {
                                case 's':
                                    if (p[10] == '/')
                                    {
                                        switch(p[11])
                                        {
                                        case 'f':
                                            switch(p[12])
                                            {
                                            case '3':
                                                if (p[13] == '2')
                                                {
                                                p += 14;
                                                token = wtkTRUNC_S_F32_I32;
                                                goto LKeyword;
                                                }
                                                break;
                                            case '6':
                                                if (p[13] == '4')
                                                {
                                                p += 14;
                                                token = wtkTRUNC_S_F64_I32;
                                                goto LKeyword;
                                                }
                                                break;
                                            }
                                            break;
                                        }
                                    }
                                    break;
                                case 'u':
                                    if (p[10] == '/')
                                    {
                                        switch(p[11])
                                        {
                                        case 'f':
                                            switch(p[12])
                                            {
                                            case '3':
                                                if (p[13] == '2')
                                                {
                                                p += 14;
                                                token = wtkTRUNC_U_F32_I32;
                                                goto LKeyword;
                                                }
                                                break;
                                            case '6':
                                                if (p[13] == '4')
                                                {
                                                p += 14;
                                                token = wtkTRUNC_U_F64_I32;
                                                goto LKeyword;
                                                }
                                                break;
                                            }
                                            break;
                                        }
                                    }
                                    break;
                                }
                                break;
                            }
                        }
                        break;
                    case 'r':
                        switch(p[4])
                        {
                        case 'e':
                            if (p[5] == 'i' && p[6] == 'n' && p[7] == 't' && p[8] == 'e' && p[9] == 'r' && p[10] == 'p' && p[11] == 'r' && p[12] == 'e' && p[13] == 't' && p[14] == '/' && p[15] == 'f' && p[16] == '3' && p[17] == '2')
                            {
                            p += 18;
                            token = wtkREINTERPRET_F32_I32;
                            goto LKeyword;
                            }
                            break;
                        case 'o':
                            switch(p[5])
                            {
                            case 't':
                                switch(p[6])
                                {
                                case 'r':
                                    p += 7;
                                    token = wtkROR_I32;
                                    goto LKeyword;
                                    break;
                                case 'l':
                                    p += 7;
                                    token = wtkROL_I32;
                                    goto LKeyword;
                                    break;
                                }
                                break;
                            }
                            break;
                        }
                        break;
                    case 'l':
                        switch(p[4])
                        {
                        case 'o':
                            if (p[5] == 'a')
                            {
                                switch(p[6])
                                {
                                case 'd':
                                    switch(p[7])
                                    {
                                    case '8':
                                        p += 8;
                                        token = wtkLOAD8U_I32;
                                        goto LKeyword;
                                        break;
                                    case '1':
                                        if (p[8] == '6')
                                        {
                                        p += 9;
                                        token = wtkLOAD16U_I32;
                                        goto LKeyword;
                                        }
                                        break;
                                    }
                                    p += 7;
                                    token = wtkLOAD_I32;
                                    goto LKeyword;
                                    break;
                                }
                            }
                            break;
                        case 't':
                            switch(p[5])
                            {
                            case 's':
                                p += 6;
                                token = wtkLTS_I32;
                                goto LKeyword;
                                break;
                            case 'u':
                                p += 6;
                                token = wtkLTU_I32;
                                goto LKeyword;
                                break;
                            }
                            break;
                        case 'e':
                            switch(p[5])
                            {
                            case 's':
                                p += 6;
                                token = wtkLES_I32;
                                goto LKeyword;
                                break;
                            case 'u':
                                p += 6;
                                token = wtkLEU_I32;
                                goto LKeyword;
                                break;
                            }
                            break;
                        }
                        break;
                    case 's':
                        switch(p[4])
                        {
                        case 't':
                            if (p[5] == 'o' && p[6] == 'r')
                            {
                                switch(p[7])
                                {
                                case 'e':
                                    switch(p[8])
                                    {
                                    case '8':
                                        p += 9;
                                        token = wtkSTORE8_I32;
                                        goto LKeyword;
                                        break;
                                    case '1':
                                        if (p[9] == '6')
                                        {
                                        p += 10;
                                        token = wtkSTORE16_I32;
                                        goto LKeyword;
                                        }
                                        break;
                                    }
                                    p += 8;
                                    token = wtkSTORE_I32;
                                    goto LKeyword;
                                    break;
                                }
                            }
                            break;
                        case 'h':
                            switch(p[5])
                            {
                            case 'l':
                                p += 6;
                                token = wtkSHL_I32;
                                goto LKeyword;
                                break;
                            case 'r':
                                p += 6;
                                token = wtkSHRU_I32;
                                goto LKeyword;
                                break;
                            }
                            break;
                        case 'u':
                            if (p[5] == 'b')
                            {
                            p += 6;
                            token = wtkSUB_I32;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'n':
                        switch(p[4])
                        {
                        case 'o':
                            if (p[5] == 't')
                            {
                            p += 6;
                            token = wtkNOT_I32;
                            goto LKeyword;
                            }
                            break;
                        case 'e':
                            if (p[5] == 'q')
                            {
                            p += 6;
                            token = wtkNEQ_I32;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'c':
                        switch(p[4])
                        {
                        case 'l':
                            if (p[5] == 'z')
                            {
                            p += 6;
                            token = wtkCLZ_I32;
                            goto LKeyword;
                            }
                            break;
                        case 'o':
                            if (p[5] == 'n' && p[6] == 's' && p[7] == 't')
                            {
                            p += 8;
                            token = wtkCONST_I32;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'd':
                        if (p[4] == 'i')
                        {
                            switch(p[5])
                            {
                            case 'v':
                                switch(p[6])
                                {
                                case 's':
                                    p += 7;
                                    token = wtkDIVS_I32;
                                    goto LKeyword;
                                    break;
                                case 'u':
                                    p += 7;
                                    token = wtkDIVU_I32;
                                    goto LKeyword;
                                    break;
                                }
                                break;
                            }
                        }
                        break;
                    case 'm':
                        switch(p[4])
                        {
                        case 'o':
                            switch(p[5])
                            {
                            case 'd':
                                switch(p[6])
                                {
                                case 's':
                                    p += 7;
                                    token = wtkMODS_I32;
                                    goto LKeyword;
                                    break;
                                case 'u':
                                    p += 7;
                                    token = wtkMODU_I32;
                                    goto LKeyword;
                                    break;
                                }
                                break;
                            }
                            break;
                        case 'u':
                            if (p[5] == 'l')
                            {
                            p += 6;
                            token = wtkMUL_I32;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'a':
                        switch(p[4])
                        {
                        case 'n':
                            if (p[5] == 'd')
                            {
                            p += 6;
                            token = wtkAND_I32;
                            goto LKeyword;
                            }
                            break;
                        case 'd':
                            if (p[5] == 'd')
                            {
                            p += 6;
                            token = wtkADD_I32;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'o':
                        if (p[4] == 'r')
                        {
                        p += 5;
                        token = wtkOR_I32;
                        goto LKeyword;
                        }
                        break;
                    case 'x':
                        if (p[4] == 'o' && p[5] == 'r')
                        {
                        p += 6;
                        token = wtkXOR_I32;
                        goto LKeyword;
                        }
                        break;
                    case 'g':
                        switch(p[4])
                        {
                        case 't':
                            switch(p[5])
                            {
                            case 's':
                                p += 6;
                                token = wtkGTS_I32;
                                goto LKeyword;
                                break;
                            case 'u':
                                p += 6;
                                token = wtkGTU_I32;
                                goto LKeyword;
                                break;
                            }
                            break;
                        case 'e':
                            switch(p[5])
                            {
                            case 's':
                                p += 6;
                                token = wtkGES_I32;
                                goto LKeyword;
                                break;
                            case 'u':
                                p += 6;
                                token = wtkGEU_I32;
                                goto LKeyword;
                                break;
                            }
                            break;
                        }
                        break;
                    case 'e':
                        switch(p[4])
                        {
                        case 'q':
                            switch(p[5])
                            {
                            case 'z':
                                p += 6;
                                token = wtkEQZ_I32;
                                goto LKeyword;
                                break;
                            }
                            p += 5;
                            token = wtkEQ_I32;
                            goto LKeyword;
                            break;
                        }
                        break;
                    }
                    break;
                }
                p += 2;
                token = wtkI32;
                goto LKeyword;
                break;
            }
            break;
        case '6':
            if (p[1] == '4')
            {
            p += 2;
            token = wtkI64;
            goto LKeyword;
            }
            break;
        }
        goto LError;
    case 'l':
        switch(p[0])
        {
        case 'o':
            switch(p[1])
            {
            case 'o':
                if (p[2] == 'p')
                {
                p += 3;
                token = wtkLOOP;
                goto LKeyword;
                }
                break;
            case 'c':
                if (p[2] == 'a' && p[3] == 'l')
                {
                p += 4;
                token = wtkLOCAL;
                goto LKeyword;
                }
                break;
            }
            break;
        case 'a':
            if (p[1] == 'b' && p[2] == 'e' && p[3] == 'l')
            {
            p += 4;
            token = wtkLABEL;
            goto LKeyword;
            }
            break;
        }
        goto LError;
    case 'r':
        switch(p[0])
        {
        case 'e':
            switch(p[1])
            {
            case 't':
                if (p[2] == 'u' && p[3] == 'r' && p[4] == 'n')
                {
                p += 5;
                token = wtkRETURN;
                goto LKeyword;
                }
                break;
            case 's':
                if (p[2] == 'u' && p[3] == 'l' && p[4] == 't')
                {
                p += 5;
                token = wtkRESULT;
                goto LKeyword;
                }
                break;
            }
            break;
        }
        goto LError;
    case 'n':
        if (p[0] == 'o' && p[1] == 'p')
        {
        p += 2;
        token = wtkNOP;
        goto LKeyword;
        }
        goto LError;
    case 'f':
        switch(p[0])
        {
        case 'u':
            if (p[1] == 'n' && p[2] == 'c')
            {
            p += 3;
            token = wtkFUNC;
            goto LKeyword;
            }
            break;
        case '3':
            switch(p[1])
            {
            case '2':
                switch(p[2])
                {
                case '.':
                    switch(p[3])
                    {
                    case 'c':
                        switch(p[4])
                        {
                        case 'o':
                            switch(p[5])
                            {
                            case 'n':
                                switch(p[6])
                                {
                                case 'v':
                                    if (p[7] == 'e' && p[8] == 'r' && p[9] == 't')
                                    {
                                        switch(p[10])
                                        {
                                        case '_':
                                            switch(p[11])
                                            {
                                            case 's':
                                                if (p[12] == '/' && p[13] == 'i' && p[14] == '3' && p[15] == '2')
                                                {
                                                p += 16;
                                                token = wtkCONVERT_S_I32_F32;
                                                goto LKeyword;
                                                }
                                                break;
                                            case 'u':
                                                if (p[12] == '/' && p[13] == 'i' && p[14] == '3' && p[15] == '2')
                                                {
                                                p += 16;
                                                token = wtkCONVERT_U_I32_F32;
                                                goto LKeyword;
                                                }
                                                break;
                                            }
                                            break;
                                        }
                                    }
                                    break;
                                case 's':
                                    if (p[7] == 't')
                                    {
                                    p += 8;
                                    token = wtkCONST_F32;
                                    goto LKeyword;
                                    }
                                    break;
                                }
                                break;
                            case 'p':
                                if (p[6] == 'y' && p[7] == 's' && p[8] == 'i' && p[9] == 'g' && p[10] == 'n')
                                {
                                p += 11;
                                token = wtkCOPYSIGN_F32;
                                goto LKeyword;
                                }
                                break;
                            }
                            break;
                        case 'e':
                            if (p[5] == 'i' && p[6] == 'l')
                            {
                            p += 7;
                            token = wtkCEIL_F32;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'd':
                        switch(p[4])
                        {
                        case 'e':
                            if (p[5] == 'm' && p[6] == 'o' && p[7] == 't' && p[8] == 'e' && p[9] == '/' && p[10] == 'f' && p[11] == '6' && p[12] == '4')
                            {
                            p += 13;
                            token = wtkDEMOTE_F64_F32;
                            goto LKeyword;
                            }
                            break;
                        case 'i':
                            if (p[5] == 'v')
                            {
                            p += 6;
                            token = wtkDIV_F32;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'r':
                        if (p[4] == 'e' && p[5] == 'i' && p[6] == 'n' && p[7] == 't' && p[8] == 'e' && p[9] == 'r' && p[10] == 'p' && p[11] == 'r' && p[12] == 'e' && p[13] == 't' && p[14] == '/' && p[15] == 'i' && p[16] == '3' && p[17] == '2')
                        {
                        p += 18;
                        token = wtkREINTERPRET_I32_F32;
                        goto LKeyword;
                        }
                        break;
                    case 'n':
                        switch(p[4])
                        {
                        case 'e':
                            switch(p[5])
                            {
                            case 'g':
                                p += 6;
                                token = wtkNEG_F32;
                                goto LKeyword;
                                break;
                            case 'q':
                                p += 6;
                                token = wtkNEQ_F32;
                                goto LKeyword;
                                break;
                            }
                            break;
                        }
                        break;
                    case 'a':
                        switch(p[4])
                        {
                        case 'b':
                            if (p[5] == 's')
                            {
                            p += 6;
                            token = wtkABS_F32;
                            goto LKeyword;
                            }
                            break;
                        case 'd':
                            if (p[5] == 'd')
                            {
                            p += 6;
                            token = wtkADD_F32;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'f':
                        if (p[4] == 'l' && p[5] == 'o' && p[6] == 'o' && p[7] == 'r')
                        {
                        p += 8;
                        token = wtkFLOOR_F32;
                        goto LKeyword;
                        }
                        break;
                    case 's':
                        switch(p[4])
                        {
                        case 'q':
                            if (p[5] == 'r' && p[6] == 't')
                            {
                            p += 7;
                            token = wtkSQRT_F32;
                            goto LKeyword;
                            }
                            break;
                        case 't':
                            if (p[5] == 'o' && p[6] == 'r' && p[7] == 'e')
                            {
                            p += 8;
                            token = wtkSTORE_F32;
                            goto LKeyword;
                            }
                            break;
                        case 'u':
                            if (p[5] == 'b')
                            {
                            p += 6;
                            token = wtkSUB_F32;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'l':
                        switch(p[4])
                        {
                        case 't':
                            p += 5;
                            token = wtkLT_F32;
                            goto LKeyword;
                            break;
                        case 'e':
                            p += 5;
                            token = wtkLE_F32;
                            goto LKeyword;
                            break;
                        case 'o':
                            if (p[5] == 'a' && p[6] == 'd')
                            {
                            p += 7;
                            token = wtkLOAD_F32;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'g':
                        switch(p[4])
                        {
                        case 't':
                            p += 5;
                            token = wtkGT_F32;
                            goto LKeyword;
                            break;
                        case 'e':
                            p += 5;
                            token = wtkGE_F32;
                            goto LKeyword;
                            break;
                        }
                        break;
                    case 'm':
                        if (p[4] == 'u' && p[5] == 'l')
                        {
                        p += 6;
                        token = wtkMUL_F32;
                        goto LKeyword;
                        }
                        break;
                    case 'e':
                        if (p[4] == 'q')
                        {
                        p += 5;
                        token = wtkEQ_F32;
                        goto LKeyword;
                        }
                        break;
                    }
                    break;
                }
                p += 2;
                token = wtkF32;
                goto LKeyword;
                break;
            }
            break;
        case '6':
            switch(p[1])
            {
            case '4':
                switch(p[2])
                {
                case '.':
                    switch(p[3])
                    {
                    case 'c':
                        switch(p[4])
                        {
                        case 'o':
                            switch(p[5])
                            {
                            case 'n':
                                switch(p[6])
                                {
                                case 'v':
                                    if (p[7] == 'e' && p[8] == 'r' && p[9] == 't')
                                    {
                                        switch(p[10])
                                        {
                                        case '_':
                                            switch(p[11])
                                            {
                                            case 's':
                                                if (p[12] == '/' && p[13] == 'i' && p[14] == '3' && p[15] == '2')
                                                {
                                                p += 16;
                                                token = wtkCONVERT_S_I32_F64;
                                                goto LKeyword;
                                                }
                                                break;
                                            case 'u':
                                                if (p[12] == '/' && p[13] == 'i' && p[14] == '3' && p[15] == '2')
                                                {
                                                p += 16;
                                                token = wtkCONVERT_U_I32_F64;
                                                goto LKeyword;
                                                }
                                                break;
                                            }
                                            break;
                                        }
                                    }
                                    break;
                                case 's':
                                    if (p[7] == 't')
                                    {
                                    p += 8;
                                    token = wtkCONST_F64;
                                    goto LKeyword;
                                    }
                                    break;
                                }
                                break;
                            case 'p':
                                if (p[6] == 'y' && p[7] == 's' && p[8] == 'i' && p[9] == 'g' && p[10] == 'n')
                                {
                                p += 11;
                                token = wtkCOPYSIGN_F64;
                                goto LKeyword;
                                }
                                break;
                            }
                            break;
                        case 'e':
                            if (p[5] == 'i' && p[6] == 'l')
                            {
                            p += 7;
                            token = wtkCEIL_F64;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'p':
                        if (p[4] == 'r' && p[5] == 'o' && p[6] == 'm' && p[7] == 'o' && p[8] == 't' && p[9] == 'e' && p[10] == '/' && p[11] == 'f' && p[12] == '3' && p[13] == '2')
                        {
                        p += 14;
                        token = wtkPROMOTE_F32_F64;
                        goto LKeyword;
                        }
                        break;
                    case 'm':
                        switch(p[4])
                        {
                        case 'o':
                            if (p[5] == 'd')
                            {
                            p += 6;
                            token = wtkMOD_F64;
                            goto LKeyword;
                            }
                            break;
                        case 'i':
                            if (p[5] == 'n')
                            {
                            p += 6;
                            token = wtkMIN_F64;
                            goto LKeyword;
                            }
                            break;
                        case 'a':
                            if (p[5] == 'x')
                            {
                            p += 6;
                            token = wtkMAX_F64;
                            goto LKeyword;
                            }
                            break;
                        case 'u':
                            if (p[5] == 'l')
                            {
                            p += 6;
                            token = wtkMUL_F64;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'n':
                        switch(p[4])
                        {
                        case 'e':
                            switch(p[5])
                            {
                            case 'g':
                                p += 6;
                                token = wtkNEG_F64;
                                goto LKeyword;
                                break;
                            case 'q':
                                p += 6;
                                token = wtkNEQ_F64;
                                goto LKeyword;
                                break;
                            }
                            break;
                        }
                        break;
                    case 'a':
                        switch(p[4])
                        {
                        case 'b':
                            if (p[5] == 's')
                            {
                            p += 6;
                            token = wtkABS_F64;
                            goto LKeyword;
                            }
                            break;
                        case 'd':
                            if (p[5] == 'd')
                            {
                            p += 6;
                            token = wtkADD_F64;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'f':
                        if (p[4] == 'l' && p[5] == 'o' && p[6] == 'o' && p[7] == 'r')
                        {
                        p += 8;
                        token = wtkFLOOR_F64;
                        goto LKeyword;
                        }
                        break;
                    case 's':
                        switch(p[4])
                        {
                        case 'q':
                            if (p[5] == 'r' && p[6] == 't')
                            {
                            p += 7;
                            token = wtkSQRT_F64;
                            goto LKeyword;
                            }
                            break;
                        case 't':
                            if (p[5] == 'o' && p[6] == 'r' && p[7] == 'e')
                            {
                            p += 8;
                            token = wtkSTORE_F64;
                            goto LKeyword;
                            }
                            break;
                        case 'u':
                            if (p[5] == 'b')
                            {
                            p += 6;
                            token = wtkSUB_F64;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'd':
                        if (p[4] == 'i' && p[5] == 'v')
                        {
                        p += 6;
                        token = wtkDIV_F64;
                        goto LKeyword;
                        }
                        break;
                    case 'l':
                        switch(p[4])
                        {
                        case 't':
                            p += 5;
                            token = wtkLT_F64;
                            goto LKeyword;
                            break;
                        case 'e':
                            p += 5;
                            token = wtkLE_F64;
                            goto LKeyword;
                            break;
                        case 'o':
                            if (p[5] == 'a' && p[6] == 'd')
                            {
                            p += 7;
                            token = wtkLOAD_F64;
                            goto LKeyword;
                            }
                            break;
                        }
                        break;
                    case 'g':
                        switch(p[4])
                        {
                        case 't':
                            p += 5;
                            token = wtkGT_F64;
                            goto LKeyword;
                            break;
                        case 'e':
                            p += 5;
                            token = wtkGE_F64;
                            goto LKeyword;
                            break;
                        }
                        break;
                    case 'e':
                        if (p[4] == 'q')
                        {
                        p += 5;
                        token = wtkEQ_F64;
                        goto LKeyword;
                        }
                        break;
                    }
                    break;
                }
                p += 2;
                token = wtkF64;
                goto LKeyword;
                break;
            }
            break;
        }
        goto LError;
    case 'p':
        if (p[0] == 'a' && p[1] == 'r' && p[2] == 'a' && p[3] == 'm')
        {
        p += 4;
        token = wtkPARAM;
        goto LKeyword;
        }
        goto LError;
    case 'm':
        switch(p[0])
        {
        case 'o':
            if (p[1] == 'd' && p[2] == 'u' && p[3] == 'l' && p[4] == 'e')
            {
            p += 5;
            token = wtkMODULE;
            goto LKeyword;
            }
            break;
        case 'e':
            if (p[1] == 'm' && p[2] == 'o' && p[3] == 'r' && p[4] == 'y')
            {
            p += 5;
            token = wtkMEMORY;
            goto LKeyword;
            }
            break;
        }
        goto LError;
    case 'e':
        if (p[0] == 'x' && p[1] == 'p' && p[2] == 'o' && p[3] == 'r' && p[4] == 't')
        {
        p += 5;
        token = wtkEXPORT;
        goto LKeyword;
        }
        goto LError;
    case 't':
        switch(p[0])
        {
        case 'a':
            if (p[1] == 'b' && p[2] == 'l' && p[3] == 'e')
            {
            p += 4;
            token = wtkTABLE;
            goto LKeyword;
            }
            break;
        case 'y':
            if (p[1] == 'p' && p[2] == 'e')
            {
            p += 3;
            token = wtkTYPE;
            goto LKeyword;
            }
            break;
        }
        goto LError;

