//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
 Internationalization - Step Into tests
*/

function Run() {
    var intl = new Intl.Collator();
    intl.compare('a','b');/**bp:resume('step_into');locals()**/
    WScript.Echo('PASS');
}

WScript.Attach(Run);
