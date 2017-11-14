//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

function XLFuzzTest() {
  try {
    do {
      function apInitTest() {
        do {
          function apEndTest() {
            do {
              apInitTest:
                if (false) {
                  return;
                }
              return;
            } while (false);
          }
        } while (false);
      }
    } while (false);
  } catch (e) {}
}
for (var i = 0; i < 1; i++) {
  XLFuzzTest();
}
print("PASSED");
