'use strict';

class MockAnalytics {
  constructor() {
    this.tracks = [];
    this.trackTimings = [];
    this.trackErrors = [];
  }

  track(arg) {
    this.tracks.push(arg);
  }

  trackTiming(arg) {
    this.trackTimings.push(arg);
  }

  trackError(arg) {
    this.trackErrors.push(arg);
  }
}

module.exports = MockAnalytics;
