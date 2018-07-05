//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

// Parse flags
enum
{
    fscrNil = 0,
    // Unused = 1 << 0,
    fscrReturnExpression = 1 << 1,   // call should return the last expression
    fscrImplicitThis = 1 << 2,   // 'this.' is optional (for Call)
    fscrWillDeferFncParse = 1 << 3,  // Heuristically choosing to defer parsing of functions
    fscrCanDeferFncParse = 1 << 4,   // Functionally able to defer parsing of functions
    fscrDynamicCode = 1 << 5,   // The code is being generated dynamically (eval, new Function, etc.)
    fscrDeferredFncIsGenerator = 1 << 6,
    fscrNoImplicitHandlers = 1 << 7,   // same as Opt NoConnect at start of block
    fscrCreateParserState = 1 << 8, // The parser should expose parser state information on the parse nodes.
                                    // This parser state includes the set of names which are captured by each function
                                    // and is stored in ParseNodeFnc::capturedNames.

#if DEBUG
    fscrEnforceJSON = 1 << 9,  // used together with fscrReturnExpression
                               // enforces JSON semantics in the parsing.
#endif

    fscrEval = 1 << 10,  // this expression has eval semantics (i.e., run in caller's context
    fscrEvalCode = 1 << 11,  // this is an eval expression
    fscrGlobalCode = 1 << 12,  // this is a global script
    fscrDeferredFncIsAsync = 1 << 13,
    fscrDeferredFncExpression = 1 << 14,  // the function decl node we deferred is an expression,
                                          // i.e., not a declaration statement
    fscrDeferredFnc = 1 << 15,  // the function we are parsing is deferred
    fscrNoPreJit = 1 << 16,  // ignore prejit global flag
    fscrAllowFunctionProxy = 1 << 17,  // Allow creation of function proxies instead of function bodies
    fscrIsLibraryCode = 1 << 18,  // Current code is engine library code written in Javascript
    fscrNoDeferParse = 1 << 19,  // Do not defer parsing
    fscrJsBuiltIn = 1 << 20, // Current code is a JS built in code written in JavaScript
#ifdef IR_VIEWER
    fscrIrDumpEnable = 1 << 21,  // Allow parseIR to generate an IR dump
#endif /* IRVIEWER */

                                 // Throw a ReferenceError when the global 'this' is used (possibly in a lambda),
                                 // for debugger when broken in a lambda that doesn't capture 'this'
    fscrDebuggerErrorOnGlobalThis = 1 << 22,
    fscrDeferredClassMemberFnc = 1 << 23,
    fscrConsoleScopeEval = 1 << 24,  //  The eval string is console eval or debugEval, used to have top level
                                     //  let/const in global scope instead of eval scope so that they can be preserved across console inputs
    fscrNoAsmJs = 1 << 25, // Disable generation of asm.js code
    fscrIsModuleCode = 1 << 26, // Current code should be parsed as a module body

    fscrDeferredFncIsMethod = 1 << 27,
    fscrAll = (1 << 28) - 1
};
