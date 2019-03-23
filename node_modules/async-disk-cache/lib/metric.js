function Metric() {
  this.count = 0;
  this.time = 0;
  this.running = 0;
  this.startTime = undefined;
}

Metric.prototype.start = function() {
  this.startTime = process.hrtime();
  this.count++;
  this.running++;
};

Metric.prototype.stop = function() {
  this.running--;

  // Since we're only recording time once after all running locks are released,
  // we're only collecting "wall clock" time and not "cost" time.
  if (this.running > 0) {
    return;
  } else if (this.running < 0) {
    throw new Error('Called stop more times than start was called');
  }

  var change = process.hrtime(this.startTime);

  this.time += change[0] * 1e9 + change[1];
  this.startTime = undefined;
};

Metric.prototype.toJSON = function() {
  return {
    count: this.count,
    time: this.time
  };
};

module.exports = Metric;
