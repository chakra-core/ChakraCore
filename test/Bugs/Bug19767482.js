//Configuration: Configs\full_with_strings.xml
//Testcase Number: 4018
//Switches: -PrintSystemException  -ArrayMutationTestSeed:2004204769  -maxinterpretcount:1 -maxsimplejitruncount:2 -MinMemOpCount:0 -werexceptionsupport  -MinSwitchJumpTableSize:2 -bgjit- -loopinterpretcount:1 -MaxLinearStringCaseCount:2 -MaxLinearIntCaseCount:2 -on:CaptureByteCodeRegUse -force:rejit -PoisonIntArrayLoad- -force:ScriptFunctionWithInlineCache -force:atom -force:fixdataprops
//Baseline Switches: -nonative -werexceptionsupport  -PrintSystemException  -ArrayMutationTestSeed:2004204769
//Arch: x86
//Flavor: debug
//BuildName: chakrafull.full
//BuildRun: 180827_0314
//BuildId: 
//BuildHash: 
//BinaryPath: \\chakrafs\fs\Builds\ChakraFull\unreleased\rs5\1808\00025.55398_180827.0925_sethb_4fd79162a0d5cbf937f5f47fa77d55a211ea9c2b\bin\x86_debug
//MachineName: master-vm
//Reduced Switches: -printsystemexception -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport -bgjit- -loopinterpretcount:1 -bvt -oopjit- 
//noRepro switches0: -printsystemexception -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport -bgjit- -loopinterpretcount:1 -bvt -oopjit-  -off:DynamicProfile
//noRepro switches1: -printsystemexception -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport -bgjit- -loopinterpretcount:1 -bvt -oopjit-  -off:InterpreterAutoProfile
//noRepro switches2: -printsystemexception -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport -bgjit- -loopinterpretcount:1 -bvt -oopjit-  -off:InterpreterProfile
//noRepro switches3: -printsystemexception -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport -bgjit- -loopinterpretcount:1 -bvt -oopjit-  -off:JITLoopBody
//noRepro switches4: -printsystemexception -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport -bgjit- -loopinterpretcount:1 -bvt -oopjit-  -off:SimpleJit
function test0() {
  var __loopvar0 = -4;
  while (true) {
    __loopvar0 += 2;
    if (__loopvar0 > 3) {
      break;
    }

    if ('caller') {
      var strvar9 = '';
    } else {
      var strvar9 = '';
      do {
        (strvar9 + protoObj0)();
      } while ('');

      return this;
    }
  }
}

test0();
test0();
WScript.Echo("Pass");
 
  // === Output ===
  // command: D:\\BinariesCache\\1808\00025.55398_180827.0925_sethb_4fd79162a0d5cbf937f5f47fa77d55a211ea9c2b\bin\x86_debug\jshost.exe -printsystemexception -maxinterpretcount:1 -maxsimplejitruncount:2 -werexceptionsupport -bgjit- -loopinterpretcount:1 -bvt -oopjit-  step.js
  // exitcode: 0xC0000005
  // stdout:
  // 
  // stderr:
  // FATAL ERROR: jshost.exe failed due to exception code c0000005
  // 
  // 
  // 
