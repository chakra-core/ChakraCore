module.exports = function reduceFunnels(data) {
  var reduced = data.reduce(function(reduced, entry) {
    var key = entry.src + '!☃!!' + entry.dest;
    var current = reduced[key] = reduced[key] || {
      srcDir: entry.src,
      destDir: entry.dest,
      include: []
    };

    current.include.push(entry.file);

    return reduced;
  }, {});

  return Object.keys(reduced).map(function(key) {
    return reduced[key];
  });
}
