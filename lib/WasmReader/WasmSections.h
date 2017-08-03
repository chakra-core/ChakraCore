//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

//          (internalName   , ID         , SectionFlag, Precedent )
WASM_SECTION(Custom         , "custom"   , fSectNone  , Limit     )
WASM_SECTION(Type           , "type"     , fSectNone  , Limit     )
WASM_SECTION(Import         , "import"   , fSectNone  , Limit     )
WASM_SECTION(Function       , "function" , fSectNone  , Type      )
WASM_SECTION(Table          , "table"    , fSectNone  , Limit     )
WASM_SECTION(Memory         , "memory"   , fSectNone  , Limit     )
WASM_SECTION(Global         , "global"   , fSectNone  , Limit     )
WASM_SECTION(Export         , "export"   , fSectNone  , Limit     )
WASM_SECTION(StartFunction  , "start"    , fSectNone  , Type      )
WASM_SECTION(Element        , "element"  , fSectNone  , Limit     )
WASM_SECTION(FunctionBodies , "code"     , fSectNone  , Function  )
WASM_SECTION(Data           , "data"     , fSectNone  , Limit     )
WASM_SECTION(Name           , "name"     , fSectIgnore, Type      )
#undef WASM_SECTION
