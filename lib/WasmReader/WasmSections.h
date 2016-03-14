//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//          (name                 , ID                   , SectionFlag, Precedent         )
WASM_SECTION(Signatures           , "signatures"         , fSectNone  , Invalid           )
WASM_SECTION(ImportTable          , "import_table"       , fSectNone  , Invalid           )
WASM_SECTION(FunctionSignatures   , "function_signatures", fSectNone  , Signatures        )
WASM_SECTION(IndirectFunctionTable, "function_table"     , fSectNone  , FunctionSignatures)
WASM_SECTION(Memory               , "memory"             , fSectNone  , Invalid           )
WASM_SECTION(ExportTable          , "export_table"       , fSectNone  , FunctionSignatures)
WASM_SECTION(StartFunction        , "start_function"     , fSectNone  , FunctionSignatures)
WASM_SECTION(FunctionBodies       , "function_bodies"    , fSectNone  , FunctionSignatures)
WASM_SECTION(DataSegments         , "data_segments"      , fSectNone  , Memory            )
WASM_SECTION(Names                , "names"              , fSectIgnore, Invalid           )

#undef WASM_SECTION
