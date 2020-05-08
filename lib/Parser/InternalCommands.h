//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// The Internal Command mechanism currently supports OpCodes with one or two parameters
// and a return value. Any such can be enabled as Internal commands by adding them below

// command name, expected parameters
Command(Conv_Obj, 1)
Command(ToInteger, 1)
Command(ToLength, 1)

#undef Command
