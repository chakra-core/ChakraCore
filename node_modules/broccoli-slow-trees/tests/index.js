var chai = require('chai'), expect = chai.expect;
var chaiAsPromised = require('chai-as-promised');

var Heimdall = require('heimdalljs/heimdall');
var calculateSummary = require('../calculate-summary');

chai.use(chaiAsPromised);

function stubTime(ms) {
  process.hrtime = function () {
    return [0, ms * 1e6];
  };
}

var originalHrtime = process.hrtime;

function restoreTime() {
  process.hrtime = originalHrtime;
}

describe('calculateSummary', function() {
  afterEach(restoreTime);

  it('summarizes simple graphs', function() {
    stubTime(100);
    var heimdall = new Heimdall();

    return expect(heimdall.node({ name: 'babel', broccoliNode: true, }, function () {
      stubTime(200);
      return heimdall.node({ name: 'merge-trees', broccoliNode: true }, function () {
        stubTime(350);
      });
    }).then(function () {
      return heimdall.node({ name: 'merge-trees', broccoliNode: true }, function () {
        stubTime(600);
      });
    }).then(function () {
      return calculateSummary(heimdall);
    })).to.eventually.deep.equal({
      totalTime: 500,
      nodes: [
        {
          name: 'merge-trees',
          selfTime: 250,
        },
        {
          name: 'merge-trees',
          selfTime: 150,
        },
        {
          name: 'babel',
          selfTime: 100,
        }
      ],
      groupedNodes: [
        {
          name: 'merge-trees',
          count: 2,
          averageSelfTime: 200,
          totalSelfTime: 400,
        },
        {
          name: 'babel',
          count: 1,
          averageSelfTime: 100,
          totalSelfTime: 100,
        }
      ],
    });
  });

  it("counts non-broccoli nodes' time as part of their ancestor broccoli node's time", function() {
    stubTime(100);
    var heimdall = new Heimdall();

    return expect(heimdall.node({ name: 'merge-trees', broccoliNode: true, }, function () {

      stubTime(200);
      return heimdall.node({ name: 'babel', broccoliNode: true }, function () {
        stubTime(300);
      }).then(function () {
        return heimdall.node({ name: 'fs-tree-diff' }, function () {
          return heimdall.node({ name: 'calculatePatch' }, function () {
            stubTime(550);
          }).then(function () {
            return heimdall.node({ name: 'sortAndExpand' }, function () {
              stubTime(600);
            });
          });
        });
      });

    }).then(function () {
      return calculateSummary(heimdall);
    })).to.eventually.deep.equal({
      totalTime: 500,
      nodes: [
        {
          name: 'merge-trees',
          selfTime: 400,
        },
        {
          name: 'babel',
          selfTime: 100,
        }
      ],
      groupedNodes: [
        {
          averageSelfTime: 400,
          count: 1,
          name: "merge-trees",
          totalSelfTime: 400,
        },
        {
          averageSelfTime: 100,
          count: 1,
          name: "babel",
          totalSelfTime: 100,
        }
      ],
    });
  });
});
