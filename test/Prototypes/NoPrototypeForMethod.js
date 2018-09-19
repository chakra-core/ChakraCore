var o = {
    f(){}
}
WScript.Echo(o.f.hasOwnProperty("prototype"));

class C{
    f(){}
}
WScript.Echo(new C().f.hasOwnProperty("prototype"));
WScript.Echo(new C().constructor.hasOwnProperty("prototype"));