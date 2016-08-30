//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

/*
    Intl Object
    Intl Collator
    Intl Number Format
    Intl DateTime Format
    Chakra Implementation should be hidden from the user
*/

function Run() {
    var coll = Intl.Collator();
    var numFormat = Intl.NumberFormat();
    var dttmFormat = Intl.DateTimeFormat();

    WScript.Echo('PASSED');/**bp:
    locals(1);
	evaluate('coll',4);
	evaluate('numFormat',4);
	evaluate('dttmFormat',4);
	evaluate('coll.compare.toString() == \'function() {\\n    [native code]\\n}\'');
    evaluate('coll.resolvedOptions.toString() == \'function() {\\n    [native code]\\n}\'');
    evaluate('numFormat.format.toString() == \'function() {\\n    [native code]\\n}\'');
    evaluate('numFormat.resolvedOptions.toString() == \'function() {\\n    [native code]\\n}\'');
    evaluate('dttmFormat.format.toString() == \'function() {\\n    [native code]\\n}\'');
    evaluate('dttmFormat.resolvedOptions.toString() == \'function() {\\n    [native code]\\n}\'');
	**/
}

var x; /**bp:evaluate('Intl.Collator')**/
WScript.Attach(Run);
WScript.Detach(Run);

