evaluate = WScript.LoadScript;

__defineSetter__("x", function () { });

evaluate(`
  let x = 'let';
  Object.defineProperty(this, "x", { value:
          0xdec0  })
  if (x === 'let' && this.x === 57024)
  {
    WScript.Echo('pass');
  }
`);
