//Configuration: Configs\lessmath.xml
//Testcase Number: 4592
//Bailout Testing: ON
//Switches: -PrintSystemException  -ArrayMutationTestSeed:14309800  -maxinterpretcount:1 -maxsimplejitruncount:2 -MinMemOpCount:0 -werexceptionsupport  -MinSwitchJumpTableSize:2 -force:fieldcopyprop -bgjit- -loopinterpretcount:1 -MaxLinearStringCaseCount:2 -MaxLinearIntCaseCount:2 -forcedeferparse -force:fieldPRE -off:polymorphicinlinecache -off:simplejit -force:inline -sse:3 -force:rejit -ForceStaticInterpreterThunk -ForceJITCFGCheck -UseJITTrampoline- -force:atom -force:fixdataprops -PoisonStringLoad- -force:ScriptFunctionWithInlineCache -off:cachedScope -off:fefixedmethods -off:stackargopt -PoisonVarArrayLoad- -ForceArrayBTree -off:checkthis -on:CaptureByteCodeRegUse -PoisonTypedArrayLoad- -oopjit- -off:ArrayLengthHoist -PoisonIntArrayLoad- -off:DelayCapture -PoisonFloatArrayLoad- -off:ArrayCheckHoist -off:aggressiveinttypespec -off:lossyinttypespec -off:ParallelParse -off:trackintusage -off:floattypespec
//Baseline Switches: -nonative -werexceptionsupport  -PrintSystemException  -ArrayMutationTestSeed:14309800
//Arch: x86
//Flavor: test
//BuildName: chakrafull.full
//BuildRun: 01231_0400
//BuildId: 
//BuildHash: 
//BinaryPath: C:\Builds\ChakraFull_x86_test
//MachineName: master-vm
//Reduced Switches: -printsystemexception -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport -bgjit- -loopinterpretcount:1 -oopjit- -bvt -off:simplejit 
//noRepro switches0: -printsystemexception -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport -bgjit- -loopinterpretcount:1 -oopjit- -bvt -off:simplejit  -off:DynamicProfile
//noRepro switches1: -printsystemexception -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport -bgjit- -loopinterpretcount:1 -oopjit- -bvt -off:simplejit  -off:FastPath
//noRepro switches2: -printsystemexception -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport -bgjit- -loopinterpretcount:1 -oopjit- -bvt -off:simplejit  -off:InterpreterProfile
//noRepro switches3: -printsystemexception -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport -bgjit- -loopinterpretcount:1 -oopjit- -bvt -off:simplejit  -off:JITLoopBody
//noRepro switches4: -printsystemexception -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport -bgjit- -loopinterpretcount:1 -oopjit- -bvt -off:simplejit  -off:ObjTypeSpec
function test0() {
  var obj0 = {};
  var litObj0 = {};
  var func0 = function () {
  };
  obj0.method0 = func0;
  var __loopvar0 = 8;
  for (var __loopSecondaryVar0_0 = 8;;) {
    if (__loopvar0 <= 6) {
      break;
    }
    __loopvar0 = 3;
    function func5() {
    }
    function func6() {
      ({
        v5: function () {
          func5();
        }
      });
    }
    var __loopvar1 = 3;
    while (this) {
      __loopvar1++;
      if (__loopvar1 >= 8) {
        break;
      }
      litObj0.prop1 = obj0.method0();
      continue;
      if (true) {
        func6;
      } else {
        (function () {
          func6();
        }());
        for (var _strvar0 of ary) {
        }
      }
    }
  }
}

test0();
test0();
WScript.Echo("Pass");

// === Output ===
// command: E:\\BinariesCache\\1812\00029.27247_181214.1751_t-nhng_1b40a2baa24e6b9b20178f54a406dc7faa5949bd\bin\x86_test\jshost.exe -printsystemexception -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport -bgjit- -loopinterpretcount:1 -oopjit- -bvt -off:simplejit  step.js
// exitcode: 0xC0000005
// stdout:
// 
// stderr:
// FATAL ERROR: jshost.exe failed due to exception code c0000005
// 
// 
// 
