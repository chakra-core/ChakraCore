module.exports = function sortObjectByKeyNameList(object, sortWith) {
  var keys;
  var sortFn;

  if (typeof sortWith === 'function') {
    sortFn = sortWith;
  } else {
    keys = sortWith;
  }
  return (keys || []).concat(Object.keys(object).sort(sortFn)).reduce(function(total, key) {
    total[key] = object[key];
    return total;
  }, Object.create(null));
}
