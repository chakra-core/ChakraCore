//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//          (name                 , ID                   , SectionFlag, Precedent         )
WASM_SECTION(Signatures           , "type"               , fSectNone  , Invalid           )
WASM_SECTION(ImportTable          , "import"             , fSectNone  , Invalid           )
WASM_SECTION(FunctionSignatures   , "function"           , fSectNone  , Signatures        )
WASM_SECTION(IndirectFunctionTable, "table"              , fSectNone  , FunctionSignatures)
WASM_SECTION(Memory               , "memory"             , fSectNone  , Invalid           )
WASM_SECTION(Global               , "global"             , fSectNone  , Invalid           )
WASM_SECTION(ExportTable          , "export"             , fSectNone  , FunctionSignatures)
WASM_SECTION(StartFunction        , "start"              , fSectNone  , FunctionSignatures)
WASM_SECTION(Element              , "element"            , fSectNone  , IndirectFunctionTable)
WASM_SECTION(FunctionBodies       , "code"               , fSectNone  , FunctionSignatures)
WASM_SECTION(DataSegments         , "data"               , fSectNone  , Memory            )
WASM_SECTION(Names                , "name"               , fSectIgnore, Signatures        )
WASM_SECTION(User                 , "user"               , fSectIgnore, Invalid           )
#undef WASM_SECTION
