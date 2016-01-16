//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

namespace Wasm
{
    enum SExprTokenType
    {
        wtkNONE,
#define WASM_KEYWORD(token, name) wtk##token,
#include "WasmKeywords.h"
#define WASM_TOKEN(token) wtk##token,
#include "WasmTokens.h"
        wtkLIMIT
    };

    struct SExprToken
    {
        SExprToken() : tk(wtkNONE) {}
        SExprTokenType tk;

        union
        {
            LPUTF8 m_sz;
            int64 lng;
            double dbl;
        } u;
    };

    class SExprScanner
    {
        friend class SExprParser;

        SExprScanner(ArenaAllocator * alloc);
        void Init(SExprParseContext * context, SExprToken * token);
        SExprTokenType Scan();
        void UndoScan();
        void ScanToken(SExprTokenType tok);

    private:
        ArenaAllocator *    m_alloc;
        SExprParseContext * m_context;
        SExprToken *        m_token;
        LPCUTF8             m_currentChar;
        SExprToken          m_nextToken;
    };

} // namespace Wasm
