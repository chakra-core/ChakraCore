//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
// This file defines String.prototype tag functions and their parameters.

#ifndef TAGENTRY
#error "TAGENTRY macro isn't defined!"
#define TAGENTRY(...) // Stop editors from seeing this as one big syntax error
#endif

//       Name       Tag         Properties
TAGENTRY(Anchor,    _u("a"),       _u("name") )
TAGENTRY(Big,       _u("big")              )
TAGENTRY(Blink,     _u("blink")            )
TAGENTRY(Bold,      _u("b")                )
TAGENTRY(Fixed,     _u("tt")               )
TAGENTRY(FontColor, _u("font"),    _u("color"))
TAGENTRY(FontSize,  _u("font"),    _u("size") )
TAGENTRY(Italics,   _u("i")                )
TAGENTRY(Link,      _u("a"),       _u("href") )
TAGENTRY(Small,     _u("small")            )
TAGENTRY(Strike,    _u("strike")           )
TAGENTRY(Sub,       _u("sub")              )
TAGENTRY(Sup,       _u("sup")              )
