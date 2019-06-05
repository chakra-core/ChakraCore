function test0(){
  function f() { obj.p9 = obj.p1 }
  let obj = {};
  obj.p1 = null;
  obj.p2 = null;
  obj.p3 = null;
  obj.p4 = null;
  obj.p5 = null;
  obj.p6 = null;
  obj.p7 = null;
  obj.p8 = null;
  f();
  +obj.p9;
};

// generate profile
test0();

// Run Simple JIT
test0();

// run JITted code
test0();

print("Pass");
