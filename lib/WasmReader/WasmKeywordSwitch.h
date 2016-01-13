//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// GENERATED FILE, DO NOT HAND MODIFIY
// Generated with the following command line: cscript SExprScan.js WasmKeywords.h WasmKeywordSwitch.h
// This should be regenerated whenever the keywords change

    case 'g':
        switch(p[0])
        {
        case 'e':
            switch(p[1])
            {
            case 't':
                switch(p[2])
                {
                case 'n':
                    if (p[3] == 'e' && p[4] == 'a')
                    {
                        switch(p[5])
                        {
                        case 'r':
                            switch(p[6])
                            {
                            case 'u':
                                switch(p[7])
                                {
                                case 'n':
                                    if (p[8] == 'a' && p[9] == 'l' && p[10] == 'i' && p[11] == 'g' && p[12] == 'n' && p[13] == 'e')
                                    {
                                        switch(p[14])
                                        {
                                        case 'd':
                                            switch(p[15])
                                            {
                                            case 's':
                                                p += 16;
                                                token = wtkGET_NEAR_UNALIGNED_S;
                                                goto LKeyword;
                                                break;
                                            case 'u':
                                                p += 16;
                                                token = wtkGET_NEAR_UNALIGNED_U;
                                                goto LKeyword;
                                                break;
                                            }
                                            break;
                                        }
                                    }
                                    break;
                                }
                                p += 7;
                                token = wtkGET_NEAR_U;
                                goto LKeyword;
                                break;
                            case 's':
                                p += 7;
                                token = wtkGET_NEAR_S;
                                goto LKeyword;
                                break;
                            }
                            break;
                        }
                    }
                    break;
                case 'f':
                    if (p[3] == 'a')
                    {
                        switch(p[4])
                        {
                        case 'r':
                            switch(p[5])
                            {
                            case 'u':
                                switch(p[6])
                                {
                                case 'n':
                                    if (p[7] == 'a' && p[8] == 'l' && p[9] == 'i' && p[10] == 'g' && p[11] == 'n' && p[12] == 'e')
                                    {
                                        switch(p[13])
                                        {
                                        case 'd':
                                            switch(p[14])
                                            {
                                            case 's':
                                                p += 15;
                                                token = wtkGET_FAR_UNALIGNED_S;
                                                goto LKeyword;
                                                break;
                                            case 'u':
                                                p += 15;
                                                token = wtkGET_FAR_UNALIGNED_U;
                                                goto LKeyword;
                                                break;
                                            }
                                            break;
                                        }
                                    }
                                    break;
                                }
                                p += 6;
                                token = wtkGET_FAR_U;
                                goto LKeyword;
                                break;
                            case 's':
                                p += 6;
                                token = wtkGET_FAR_S;
                                goto LKeyword;
                                break;
                            }
                            break;
                        }
                    }
                    break;
                case 'p':
                    if (p[3] == 'a' && p[4] == 'r' && p[5] == 'a' && p[6] == 'm')
                    {
                    p += 7;
                    token = wtkGETPARAM;
                    goto LKeyword;
                    }
                    break;
                case 'l':
                    if (p[3] == 'o' && p[4] == 'c' && p[5] == 'a' && p[6] == 'l')
                    {
                    p += 7;
                    token = wtkGETLOCAL;
                    goto LKeyword;
                    }
                    break;
                case 'g':
                    if (p[3] == 'l' && p[4] == 'o' && p[5] == 'b' && p[6] == 'a' && p[7] == 'l')
                    {
                    p += 8;
                    token = wtkGETGLOBAL;
                    goto LKeyword;
                    }
                    break;
                }
                break;
            }
            break;
        case 'l':
            if (p[1] == 'o' && p[2] == 'b' && p[3] == 'a' && p[4] == 'l')
            {
            p += 5;
            token = wtkGLOBAL;
            goto LKeyword;
            }
            break;
        }
        goto LError;
    case 's':
        switch(p[0])
        {
        case 'e':
            switch(p[1])
            {
            case 't':
                switch(p[2])
                {
                case 'n':
                    if (p[3] == 'e' && p[4] == 'a')
                    {
                        switch(p[5])
                        {
                        case 'r':
                            switch(p[6])
                            {
                            case 'u':
                                switch(p[7])
                                {
                                case 'n':
                                    if (p[8] == 'a' && p[9] == 'l' && p[10] == 'i' && p[11] == 'g' && p[12] == 'n' && p[13] == 'e')
                                    {
                                        switch(p[14])
                                        {
                                        case 'd':
                                            switch(p[15])
                                            {
                                            case 's':
                                                p += 16;
                                                token = wtkSET_NEAR_UNALIGNED_S;
                                                goto LKeyword;
                                                break;
                                            case 'u':
                                                p += 16;
                                                token = wtkSET_NEAR_UNALIGNED_U;
                                                goto LKeyword;
                                                break;
                                            }
                                            break;
                                        }
                                    }
                                    break;
                                }
                                p += 7;
                                token = wtkSET_NEAR_U;
                                goto LKeyword;
                                break;
                            case 's':
                                p += 7;
                                token = wtkSET_NEAR_S;
                                goto LKeyword;
                                break;
                            }
                            break;
                        }
                    }
                    break;
                case 'f':
                    if (p[3] == 'a')
                    {
                        switch(p[4])
                        {
                        case 'r':
                            switch(p[5])
                            {
                            case 'u':
                                switch(p[6])
                                {
                                case 'n':
                                    if (p[7] == 'a' && p[8] == 'l' && p[9] == 'i' && p[10] == 'g' && p[11] == 'n' && p[12] == 'e')
                                    {
                                        switch(p[13])
                                        {
                                        case 'd':
                                            switch(p[14])
                                            {
                                            case 's':
                                                p += 15;
                                                token = wtkSET_FAR_UNALIGNED_S;
                                                goto LKeyword;
                                                break;
                                            case 'u':
                                                p += 15;
                                                token = wtkSET_FAR_UNALIGNED_U;
                                                goto LKeyword;
                                                break;
                                            }
                                            break;
                                        }
                                    }
                                    break;
                                }
                                p += 6;
                                token = wtkSET_FAR_U;
                                goto LKeyword;
                                break;
                            case 's':
                                p += 6;
                                token = wtkSET_FAR_S;
                                goto LKeyword;
                                break;
                            }
                            break;
                        }
                    }
                    break;
                case 'l':
                    if (p[3] == 'o' && p[4] == 'c' && p[5] == 'a' && p[6] == 'l')
                    {
                    p += 7;
                    token = wtkSETLOCAL;
                    goto LKeyword;
                    }
                    break;
                case 'g':
                    if (p[3] == 'l' && p[4] == 'o' && p[5] == 'b' && p[6] == 'a' && p[7] == 'l')
                    {
                    p += 8;
                    token = wtkSETGLOBAL;
                    goto LKeyword;
                    }
                    break;
                }
                break;
            }
            break;
        case 'w':
            if (p[1] == 'i' && p[2] == 't' && p[3] == 'c' && p[4] == 'h')
            {
            p += 5;
            token = wtkSWITCH;
            goto LKeyword;
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
            if (p[1] == 'e' && p[2] == 'a' && p[3] == 'k')
            {
            p += 4;
            token = wtkBREAK;
            goto LKeyword;
            }
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
        case 'n':
            if (p[1] == 'v' && p[2] == 'o' && p[3] == 'k' && p[4] == 'e')
            {
            p += 5;
            token = wtkINVOKE;
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
                    case 's':
                        switch(p[4])
                        {
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
                                token = wtkSHR_I32;
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
                    case 'l':
                        switch(p[4])
                        {
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
                    case 'c':
                        if (p[4] == 'o' && p[5] == 'n' && p[6] == 's' && p[7] == 't')
                        {
                        p += 8;
                        token = wtkCONST_I32;
                        goto LKeyword;
                        }
                        break;
                    case 'e':
                        if (p[4] == 'q')
                        {
                        p += 5;
                        token = wtkEQ_I32;
                        goto LKeyword;
                        }
                        break;
                    case 'n':
                        if (p[4] == 'e' && p[5] == 'q')
                        {
                        p += 6;
                        token = wtkNEQ_I32;
                        goto LKeyword;
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
    case 'c':
        switch(p[0])
        {
        case 'a':
            switch(p[1])
            {
            case 'l':
                if (p[2] == 'l')
                {
                p += 3;
                token = wtkCALL;
                goto LKeyword;
                }
                break;
            case 's':
                if (p[2] == 't')
                {
                p += 3;
                token = wtkCAST;
                goto LKeyword;
                }
                break;
            }
            break;
        case 'o':
            if (p[1] == 'n' && p[2] == 'v' && p[3] == 'e' && p[4] == 'r')
            {
                switch(p[5])
                {
                case 't':
                    switch(p[6])
                    {
                    case 's':
                        p += 7;
                        token = wtkCONVERTS;
                        goto LKeyword;
                        break;
                    case 'u':
                        p += 7;
                        token = wtkCONVERTU;
                        goto LKeyword;
                        break;
                    }
                    p += 6;
                    token = wtkCONVERT;
                    goto LKeyword;
                    break;
                }
            }
            break;
        }
        goto LError;
    case 'd':
        switch(p[0])
        {
        case 'i':
            if (p[1] == 's' && p[2] == 'p' && p[3] == 'a' && p[4] == 't' && p[5] == 'c' && p[6] == 'h')
            {
            p += 7;
            token = wtkDISPATCH;
            goto LKeyword;
            }
            break;
        case 'e':
            if (p[1] == 's' && p[2] == 't' && p[3] == 'r' && p[4] == 'u' && p[5] == 'c' && p[6] == 't')
            {
            p += 7;
            token = wtkDESTRUCT;
            goto LKeyword;
            }
            break;
        case 'a':
            if (p[1] == 't' && p[2] == 'a')
            {
            p += 3;
            token = wtkDATA;
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
                    case 'd':
                        if (p[4] == 'i' && p[5] == 'v')
                        {
                        p += 6;
                        token = wtkDIV_F32;
                        goto LKeyword;
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
                    case 'c':
                        if (p[4] == 'o' && p[5] == 'n' && p[6] == 's' && p[7] == 't')
                        {
                        p += 8;
                        token = wtkCONST_F32;
                        goto LKeyword;
                        }
                        break;
                    case 'a':
                        if (p[4] == 'd' && p[5] == 'd')
                        {
                        p += 6;
                        token = wtkADD_F32;
                        goto LKeyword;
                        }
                        break;
                    case 's':
                        if (p[4] == 'u' && p[5] == 'b')
                        {
                        p += 6;
                        token = wtkSUB_F32;
                        goto LKeyword;
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
                    case 'n':
                        if (p[4] == 'e' && p[5] == 'q')
                        {
                        p += 6;
                        token = wtkNEQ_F32;
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
                    case 'c':
                        if (p[4] == 'o' && p[5] == 'n' && p[6] == 's' && p[7] == 't')
                        {
                        p += 8;
                        token = wtkCONST_F64;
                        goto LKeyword;
                        }
                        break;
                    case 'a':
                        if (p[4] == 'd' && p[5] == 'd')
                        {
                        p += 6;
                        token = wtkADD_F64;
                        goto LKeyword;
                        }
                        break;
                    case 's':
                        if (p[4] == 'u' && p[5] == 'b')
                        {
                        p += 6;
                        token = wtkSUB_F64;
                        goto LKeyword;
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
                    case 'n':
                        if (p[4] == 'e' && p[5] == 'q')
                        {
                        p += 6;
                        token = wtkNEQ_F64;
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
        if (p[0] == 'a' && p[1] == 'b' && p[2] == 'l' && p[3] == 'e')
        {
        p += 4;
        token = wtkTABLE;
        goto LKeyword;
        }
        goto LError;
    case 'a':
        if (p[0] == 's' && p[1] == 's' && p[2] == 'e' && p[3] == 'r' && p[4] == 't' && p[5] == 'e' && p[6] == 'q')
        {
        p += 7;
        token = wtkASSERTEQ;
        goto LKeyword;
        }
        goto LError;

