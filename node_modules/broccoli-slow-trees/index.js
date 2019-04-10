var calculateSummary = require('./calculate-summary');

function ellipsize(string, desiredLength) {
  if (string.length > desiredLength) {
    return string.slice(0, desiredLength - 3) + '...';
  } else {
    return string;
  }
}

module.exports = function printSlowNodes(tree, factor) {
  try {
    var summary = calculateSummary(tree);
    var pcThreshold = factor || 0.05;
    var msThreshold = pcThreshold * summary.totalTime;
    var logLines = [];
    var cumulativeLogLines = [];

    var MAX_NAME_CELL_LENGTH = 45;
    var MAX_VALUE_CELL_LENGTH = 20;


    if (logLines.length > 0) {
      logLines.unshift(pad('', MAX_NAME_CELL_LENGTH, '-') + '-+-' + pad('', MAX_VALUE_CELL_LENGTH, '-'));
      logLines.unshift(pad('Slowest Nodes', MAX_NAME_CELL_LENGTH) + ' | ' + pad('Total', MAX_VALUE_CELL_LENGTH));
    }

    for (var i = 0; i < summary.groupedNodes.length; i++) {
      var group = summary.groupedNodes[i];
      var averageStr;

      if (group.totalSelfTime > msThreshold) {
        if (group.count > 1) {
          averageStr = ' (' + Math.floor(group.averageSelfTime) + ' ms)';
        } else {
          averageStr = '';
        }

        var countStr = ' (' + group.count + ')'
        var nameStr = ellipsize(group.name, MAX_NAME_CELL_LENGTH - countStr.length)

        cumulativeLogLines.push(pad(nameStr + countStr, MAX_NAME_CELL_LENGTH) + ' | ' + pad(Math.floor(group.totalSelfTime) + 'ms' + averageStr, MAX_VALUE_CELL_LENGTH))
      }
    }

    cumulativeLogLines.unshift(pad('', MAX_NAME_CELL_LENGTH, '-') + '-+-' + pad('', MAX_VALUE_CELL_LENGTH, '-'))
    cumulativeLogLines.unshift(pad('Slowest Nodes (totalTime => ' + (pcThreshold * 100) +'% )', MAX_NAME_CELL_LENGTH) + ' | ' + pad('Total (avg)', MAX_VALUE_CELL_LENGTH))
    cumulativeLogLines.unshift('\n')

    console.log('\n' + logLines.join('\n') + cumulativeLogLines.join('\n') + '\n')
  } catch (e) {
    console.error('Error when printing slow nodes:', e);
    console.error(e.stack)
  }
}


function pad(str, len, char, dir) {
  if (!char) { char = ' '}

  if (len + 1 >= str.length)
    switch (dir){
      case 'left':
        str = Array(len + 1 - str.length).join(char) + str
        break

      case 'both':
        var padlen = len - str.length
        var right = Math.ceil(padlen / 2)
        var left = padlen - right
        str = Array(left + 1).join(char) + str + Array(right + 1).join(char)
        break

      default:
        str = str + Array(len + 1 - str.length).join(char)
    }

  return str
}

