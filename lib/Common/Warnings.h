//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

//=============================
// Enabled Warnings
//=============================
#pragma warning(default:4242)   // conversion possible loss of data

//=============================
// Disabled Warnings
//=============================
// Warnings that we don't care about
#pragma warning(disable: 4100)  // unreferenced formal parameter
#pragma warning(disable: 4127)  // constant expression for Trace/Assert
#pragma warning(disable: 4200)  // nonstandard extension used: zero-sized array in struct/union
#pragma warning(disable: 4201)  // nameless unions are part of C++
#pragma warning(disable: 4512)  // private operator= are good to have
#pragma warning(disable: 4481)  // allow use of abstract and override keywords

#pragma warning(disable: 4324)  // structure was padded due to alignment specifier

// warnings caused by normal optimizations
#if DBG
#else // DBG
#pragma warning(disable: 4702)  // unreachable code caused by optimizations
#pragma warning(disable: 4189)  // initialized but unused variable
#pragma warning(disable: 4390)  // empty controlled statement
#endif // DBG

// PREFAST warnings
#ifdef _PREFAST_
// Warnings that we don't care about
#pragma warning(disable:6322)       // Empty _except block
#pragma warning(disable:6255)       // _alloca indicates failure by raising a stack overflow exception.  Consider using _malloca instead.
#pragma warning(disable:28112)      // A variable (processNativeCodeSize) which is accessed via an Interlocked function must always be accessed via an Interlocked function. See line 1024:  It is not always safe to access a variable which is accessed via the Interlocked* family of functions in any other way.
#pragma warning(disable:28159)      // Consider using 'GetTickCount64' instead of 'GetTickCount'. Reason: GetTickCount overflows roughly every 49 days.  Code that does not take that into account can loop indefinitely.  GetTickCount64 operates on 64 bit values and does not have that problem

#pragma warning(disable:6011)       // potentially dereferencing null pointer
#pragma warning(disable:6235)       // Logical OR with non-zero constant on the left: 1 || <expr>
#pragma warning(disable:6236)       // Logical-OR with non-zero constant, e.g., <expr> || 1.  We end up with a lot of these in release builds because certain macros (notably CONFIG_FLAG) expand to compile-time constants in release builds and not in debug builds.
#pragma warning(disable:6327)       // False constant expr on left side of AND, so right side never evaluated for effects -- e.g., 0 && <expr>
#pragma warning(disable:6239)       // NONZEROLOGICALAND:  1 && <expr> ?
#pragma warning(disable:6240)       // LOGICALANDNONZERO:  <expr> && 1 ?
#pragma warning(disable:6271)       // extra argument provided beyond format string - typically due to macro issues
#pragma warning(disable:6323)       // use of arithmetic operator on boolean types
#pragma warning(disable:6340)       // sign mismatch on printf format string
#pragma warning(disable:6387)       // argument to system library function could be null, which is technically UB but generally just an AV (or harmless)
#pragma warning(disable:25037)      // True constant expr in AND, e.g., <expr> && 1.
#pragma warning(disable:25038)      // False constant expr in AND, e.g., <expr> && 0.
#pragma warning(disable:25039)      // True Constant Expr in OR.  Seems to be a duplicate of 6236.
#pragma warning(disable:25040)      // False Constant Expr in OR, e.g., <expr> || 0.
#pragma warning(disable:25041)      // 'if' condition is always true
#pragma warning(disable:25042)      // 'if' condition is always false
#pragma warning(disable:26434)      // function definition hides a non-virtual function
#pragma warning(disable:26437)      // avoid slicing - this is more of a guideline than a rule, and we don't do it often regardless
#pragma warning(disable:26439)      // noexcept specifier implied
#pragma warning(disable:26451)      // doing math on smaller types than possible
#pragma warning(disable:26454)      // arithmetic overflow at compile time (due to us doing "0 - 1" intentionally)
#pragma warning(disable:26495)      // member not initialized (generally due to our heavy use of out-of-thread-zeroing allocators)

#ifndef NTBUILD
// Would be nice to clean these up.
#pragma warning(disable:6054)       // String 'dumpName' might not be zero-terminated.
#pragma warning(disable:6244)       // Local declaration of 'Completed' hides previous declaration at line '76'
#pragma warning(disable:6246)       // Local declaration of 'i' hides declaration of the same name in outer scope.
#pragma warning(disable:6326)       // Potential comparison of a constant with another constant.
#endif
#endif // _PREFAST_
