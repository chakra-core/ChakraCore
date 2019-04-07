module.exports = function list(args) {
  var res = [];
  var temp = [];
  for (var i = 0, max = args.length; i < max; i++) {
    var arg = args[i];
    if (arg[arg.length - 1] === ',') {
      temp.push(arg.slice(0, -1));
      res.push(temp);
      temp = [];
    } else {
      temp.push(arg);
    }
  }
  res.push(temp);
  return res;
};
