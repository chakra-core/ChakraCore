/*
 Internationalization - Step Into tests
*/

function Run() {
    var intl = new Intl.Collator();
    intl.compare('a','b');/**bp:resume('step_into');locals()**/
    WScript.Echo('PASSED');
}

WScript.Attach(Run);
