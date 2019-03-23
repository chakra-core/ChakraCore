module.exports = calculateSummary;

function byTotalSelfTime(a, b) {
  return b.totalSelfTime - a.totalSelfTime;
}

function bySelfTime(a, b) {
  return b.selfTime - a.selfTime;
}

function normalizeSelfTime(n) {
  n.selfTime = n.selfTime / 1e6;
}

function normalizeTimes(n) {
  n.averageSelfTime = n.averageSelfTime / 1e6;
  n.totalSelfTime = n.totalSelfTime / 1e6;
}

function calculateSummary(tree) {
  var totalTime = 0;
  var nodes = [];
  var groupedNodes = [];
  var nodesGroupedByName = {};

  // calculate times
  tree.visitPostOrder(function (node) {
    var nonbroccoliChildrenTime = 0;
    node.forEachChild(function (childNode) {
      // subsume non-broccoli nodes as their ancestor broccoli nodes'
      // broccoliSelfTime
      if (!childNode.id.broccoliNode) {
        nonbroccoliChildrenTime += childNode._slowTrees.broccoliSelfTime;
      }
    });

    var time = nonbroccoliChildrenTime + node.stats.time.self;

    node._slowTrees = { broccoliSelfTime: time };
    totalTime += node.stats.time.self;

    if (node.id.broccoliNode) {
      nodes.push({
        name: node.id.name,
        selfTime: time,
      });

      if (!nodesGroupedByName[node.id.name]) {
        nodesGroupedByName[node.id.name] = {
          name: node.id.name,
          count: 0,
          averageSelfTime: 0,
          totalSelfTime: 0,
        };
        groupedNodes.push(nodesGroupedByName[node.id.name]);
      }

      var group = nodesGroupedByName[node.id.name];
      group.count++;
      group.totalSelfTime += time;
      group.averageSelfTime = group.totalSelfTime / group.count;
    }
  });

  // sort nodes
  nodes = nodes.sort(bySelfTime);

  groupedNodes = groupedNodes.sort(byTotalSelfTime);
  nodes.forEach(normalizeSelfTime);
  groupedNodes.forEach(normalizeTimes);

  // normalize times (nanosec to ms)
  totalTime = totalTime / 1e6;

  return {
    totalTime: totalTime,
    nodes: nodes,
    groupedNodes: groupedNodes,
  };
}
