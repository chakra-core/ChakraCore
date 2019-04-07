'use strict';

module.exports = function echo() {
  //  discuss at: http://locutus.io/php/echo/
  // original by: Philip Peterson
  // improved by: echo is bad
  // improved by: Nate
  // improved by: Brett Zamir (http://brett-zamir.me)
  // improved by: Brett Zamir (http://brett-zamir.me)
  // improved by: Brett Zamir (http://brett-zamir.me)
  //  revised by: Der Simon (http://innerdom.sourceforge.net/)
  // bugfixed by: Eugene Bulkin (http://doubleaw.com/)
  // bugfixed by: Brett Zamir (http://brett-zamir.me)
  // bugfixed by: Brett Zamir (http://brett-zamir.me)
  // bugfixed by: EdorFaus
  //      note 1: In 1.3.2 and earlier, this function wrote to the body of the document when it
  //      note 1: was called in webbrowsers, in addition to supporting XUL.
  //      note 1: This involved >100 lines of boilerplate to do this in a safe way.
  //      note 1: Since I can't imageine a complelling use-case for this, and XUL is deprecated
  //      note 1: I have removed this behavior in favor of just calling `console.log`
  //      note 2: You'll see functions depends on `echo` instead of `console.log` as we'll want
  //      note 2: to have 1 contact point to interface with the outside world, so that it's easy
  //      note 2: to support other ways of printing output.
  //  revised by: Kevin van Zonneveld (http://kvz.io)
  //    input by: JB
  //   example 1: echo('Hello world')
  //   returns 1: undefined

  var args = Array.prototype.slice.call(arguments);
  return console.log(args.join(' '));
};
//# sourceMappingURL=echo.js.map