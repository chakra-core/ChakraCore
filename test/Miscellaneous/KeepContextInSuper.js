//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

WScript.LoadScriptFile("..\\UnitTestFramework\\UnitTestFramework.js");

var tests = [
    {
        name: "The execution of the get or set attribute on an object should be \
performed with the same context from which they were called from.",
        body: function () {
            var a = {};
            var f = 'f';
            class C {
                // get and set should execute in the context of their caller, in
                // this example that would be "a"
                set f(x) { assert.isTrue(a === this); }
                get f() {
                    assert.isTrue(a === this);

                    // Necessary as get must be called as a function and should
                    // not throw an error
                    return function () { }
                }
                foo() { assert.isTrue(a === this) }
            }
            class D extends C {
                // The current context is "a" and not D since apply() and call()
                // overrided D.
                g() { super.f(); }
                h() { super.f = 5; }
                i() { super.f; }
                j() { super['f'] }
                k() { super['f']() }
                l() { super['f'] = 5 }
                m() { super[f] }
                n() { super[f]() }
                o() { super[f] = 5 }
                p() { super.foo() }
            }
            new D().g.apply(a);
            new D().g.call(a);
            new D().h.apply(a);
            new D().h.call(a);
            new D().i.apply(a);
            new D().i.call(a);
            new D().j.apply(a);
            new D().j.call(a);
            new D().k.apply(a);
            new D().k.call(a);
            new D().l.apply(a);
            new D().l.call(a);

            // These tests are not run as they are not currently fixed. Tests using a property
            // accessor with bracket notation (ex: super[prop]) will result in a LdElemI_A
            // opcode which cannot take another property to be used as the context during the
            // execution of the property access - this limitation may require the addition of
            // another opcode (possibly adding the opcode LdSuperElemI_A).

            // new D().m.apply(a);
            // new D().m.call(a);
            // new D().n.apply(a);
            // new D().n.call(a);
            // new D().o.apply(a);
            // new D().o.call(a);

            new D().p.apply(a);
            new D().p.call(a);
        }
    },
    {
        name: "The execution of the get or set attribute on an object should be \
performed with the same context from which they were called from. This test uses \
lambda functions as an alternative way of obtaining the class' context.",
        body: function () {
            var a = {};
            class C {
                set f(x) { assert.isTrue(a === (() => this)()); }
                get f() {
                    assert.isTrue(a === (() => this)());
                    return function () { }
                }
                foo() { assert.isTrue(a === this) }
            }
            class D extends C {
                g() { super.f(); }
                h() { super.f = 5; }
                i() { super.f; }
                j() { super['f'] }
                k() { super['f']() }
                l() { super['f'] = 5 }
                m() { super[f] }
                n() { super[f]() }
                o() { super[f] = 5 }
                p() { super.foo() }
            }
            new D().g.apply(a);
            new D().g.call(a);
            new D().h.apply(a);
            new D().h.call(a);
            new D().i.apply(a);
            new D().i.call(a);
            new D().j.apply(a);
            new D().j.call(a);
            new D().k.apply(a);
            new D().k.call(a);
            new D().l.apply(a);
            new D().l.call(a);
            // new D().m.apply(a);
            // new D().m.call(a);
            // new D().n.apply(a);
            // new D().n.call(a);
            // new D().o.apply(a);
            // new D().o.call(a);
            new D().p.apply(a);
            new D().p.call(a);
        }
    },
    {
        name: "The execution of the get or set attribute on an object should be \
performed with the same context from which they were called from. This test uses \
lambda functions as an alternative way of obtaining the class' context. This text \
also uses eval statements.",
        body: function () {
            var a = {};
            class C {
                set f(x) { eval("assert.isTrue(a === (() => this)())"); }
                get f() {
                    eval("assert.isTrue(a === (() => this)())");
                    return function () { }
                }
                foo() { assert.isTrue(a === this) }
            }
            class D extends C {
                g() { eval("super.f();") }
                h() { eval("super.f = 5;") }
                i() { eval("super.f;") }
                j() { eval("super['f']") }
                k() { eval("super['f']()") }
                l() { eval("super['f'] = 5") }
                m() { eval("super[f]") }
                n() { eval("super[f]()") }
                o() { eval("super[f] = 5") }
                p() { eval("super.foo()") }
            }
            eval("new D().g.apply(a);");
            eval("new D().g.call(a);");
            eval("new D().h.apply(a);");
            eval("new D().h.call(a);");
            eval("new D().i.apply(a);");
            eval("new D().i.call(a);");
            eval("new D().j.apply(a);");
            eval("new D().j.call(a);");
            eval("new D().k.apply(a);");
            eval("new D().k.call(a);");
            eval("new D().l.apply(a);");
            eval("new D().l.call(a);");
            // eval("new D().m.apply(a);");
            // eval("new D().m.call(a);");
            // eval("new D().n.apply(a);");
            // eval("new D().n.call(a);");
            // eval("new D().o.apply(a);");
            // eval("new D().o.call(a);");
            eval("new D().p.apply(a);");
            eval("new D().p.call(a);");
        }
    },
]

testRunner.runTests(tests, { verbose: WScript.Arguments[0] != "summary" });