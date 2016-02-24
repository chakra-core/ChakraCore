// Line terminator sequences - standard

// CRLF
WScript.Echo("Text before CRLF<-- prints");
--> WScript.Echo("Text after CRLF<-- does not print");

// CR
WScript.Echo("Text before CR<-- prints");--> WScript.Echo("Text after CR<-- does not print");

// LF
WScript.Echo("Text before LF<-- prints");
--> WScript.Echo("Text after LF<-- does not print");

// LS u+2028
WScript.Echo("Text before LS<-- prints"); --> WScript.Echo("Text after LS<-- does not print");

// PS u+2029
WScript.Echo("Text before PS<-- prints"); --> WScript.Echo("Text after PS<-- does not print");


// Line terminator sequences - non-standard

// CRLS
WScript.Echo("Text before CRLS<-- prints"); --> WScript.Echo("Text after CRLS<-- does not print");

// CRPS
WScript.Echo("Text before CRPS<-- prints"); --> WScript.Echo("Text after CRPS<-- does not print");

// PSLS
WScript.Echo("Text before PSLS<-- prints");  --> WScript.Echo("Text after PSLS<-- does not print");

// LSPS
WScript.Echo("Text before LSPS<-- prints");  --> WScript.Echo("Text after LSPS<-- does not print");

// ASI
WScript.Echo("HTML start comment does not affect automatic semicolon insertion")
<!--

WScript.Echo("HTML end comment does not affect automatic semicolon insertion")
-->

// Non-HTML comment parses correctly
var a = 1; a-->a; WScript.Echo("Post-decrement with a greater-than comparison does not get interpreted as a comment");