
function test() {
  var line = '"Value1InQuotes",Value2,Value 3 ,0.33,,,Last Value';
  var inQuotes = false;
  var quoted = false;
  for (var i = 0; i < line.length; i++) {
    if (inQuotes) {
      if (line[i] === '"') {
          inQuotes = false;
      }
    } else {
      if (line[i] === '"') {
        inQuotes = true;
        quoted = true;
      } else if (line[i] === ',') {
        if (line[i - 1] === '"' && !quoted) {
          WScript.Echo('Read from wrong var');
          return false;
        }
      }
    }
  }
  return true;
}

if (test() && test() && test()) {
  WScript.Echo('Passed');
}
