R"(

var $262 = {
  agent: {
    start: function (src) {
      WScript.LoadScript(        
        `
        $262 = {
          agent:{
            receiveBroadcast: function(callback){ WScript.RecieveBroadcast(callback); },
            report: function(value){ WScript.Report(value); },
            leaving: function(){ WScript.Leaving(); }
          }
        };
        ${src}
        `, 'crossthread');
    },
    broadcast: function (sab) { WScript.Broadcast(sab); },
    sleep: function (timeout) { WScript.Sleep(timeout); },
    getReport: function () { return WScript.GetReport(); },
  }
}

)"