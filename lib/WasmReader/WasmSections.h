//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//          (internalName         , ID                   , SectionFlag, Precedent         )
WASM_SECTION(Custom               , "custom"             , fSectNone  , Limit             )
WASM_SECTION(Signatures           , "type"               , fSectNone  , Limit             )
WASM_SECTION(ImportTable          , "import"             , fSectNone  , Limit             )
WASM_SECTION(FunctionSignatures   , "function"           , fSectNone  , Signatures        )
WASM_SECTION(IndirectFunctionTable, "table"              , fSectNone  , Limit             )
WASM_SECTION(Memory               , "memory"             , fSectNone  , Limit             )
WASM_SECTION(Global               , "global"             , fSectNone  , Limit             )
WASM_SECTION(ExportTable          , "export"             , fSectNone  , Limit             )
WASM_SECTION(StartFunction        , "start"              , fSectNone  , Signatures        )
WASM_SECTION(Element              , "element"            , fSectNone  , Limit             )
WASM_SECTION(FunctionBodies       , "code"               , fSectNone  , FunctionSignatures)
WASM_SECTION(DataSegments         , "data"               , fSectNone  , Limit             )
WASM_SECTION(Names                , "name"               , fSectIgnore, Signatures        )
#undef WASM_SECTION
