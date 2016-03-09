//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//          (name                 , ID                   , SectionFlag)
WASM_SECTION(Signatures           , "signatures"         , fSectNone  )
WASM_SECTION(ImportTable          , "import_table"       , fSectIgnore)
WASM_SECTION(FunctionSignatures   , "function_signatures", fSectNone  )
WASM_SECTION(IndirectFunctionTable, "function_table"     , fSectIgnore)
WASM_SECTION(Memory               , "memory"             , fSectIgnore)
WASM_SECTION(ExportTable          , "export_table"       , fSectIgnore)
WASM_SECTION(FunctionBodies       , "function_bodies"    , fSectNone  )
WASM_SECTION(StartFunction        , "start_function"     , fSectIgnore)
WASM_SECTION(DataSegments         , "data_segments"      , fSectNone  )
WASM_SECTION(Names                , "names"              , fSectIgnore)
WASM_SECTION(End                  , "end"                , fSectNone  )

#undef WASM_SECTION
