//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//          (name                 , ID                   , SectionFlag, precendent        , subsequent        )
WASM_SECTION(Signatures           , "signatures"         , fSectNone  , Invalid           , FunctionSignatures)
WASM_SECTION(ImportTable          , "import_table"       , fSectIgnore, Invalid           , Invalid           )
WASM_SECTION(FunctionSignatures   , "function_signatures", fSectNone  , Signatures        , FunctionBodies    )
WASM_SECTION(IndirectFunctionTable, "function_table"     , fSectNone  , FunctionSignatures, Invalid           )
WASM_SECTION(Memory               , "memory"             , fSectNone  , Invalid           , Invalid           )
WASM_SECTION(ExportTable          , "export_table"       , fSectNone  , FunctionSignatures, Invalid           )
WASM_SECTION(FunctionBodies       , "function_bodies"    , fSectNone  , FunctionSignatures, Invalid           )
WASM_SECTION(StartFunction        , "start_function"     , fSectIgnore, Signatures        , Invalid           )
WASM_SECTION(DataSegments         , "data_segments"      , fSectNone  , Invalid           , Invalid           )
WASM_SECTION(Names                , "names"              , fSectIgnore, Invalid           , Invalid           )
WASM_SECTION(End                  , "end"                , fSectNone  , Invalid           , Invalid           )

#undef WASM_SECTION
