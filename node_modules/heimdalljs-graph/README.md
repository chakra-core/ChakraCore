# heimdalljs-graph

`heimdalljs-graph` is intended to be the primary entry point for doing visualizations with data
gathered by [Heimdall](https://github.com/heimdalljs/heimdalljs-lib). 

## Loading Data

### Loading from JSON File

Example:

```js
const heimdallGraph = require('heimdalljs-graph');

let graph = heimdallGraph.loadFromFile('some/path/on/disk.json');
```

### Loading from a Heimdall Node

Example:

```js
const heimdallGraph = require('heimdalljs-graph');

let graph = heimdallGraph.loadFromNode(heimdallNode);
```

## Interacting with the Graph

Once data is loaded, the resulting object is called a "node". Each node in the graph provides 
an API for iterating its subgraph as well as iterating its own stats. 

The API that each node supports is:

* `label`
* `toJSON`
* `adjacentIterator`
* `dfsIterator`
* `bfsIterator`

### label

A POJO property that describes the node.  It will always include a `name`
property and for broccoli nodes will include a `broccoliNode` property.

Example:

```js
node.label === {
  name: 'TreeMerger (allTrees)',
  broccoliNode: true,
}
```

### toJSON()

Returns a POJO that represents the serialized subgraph rooted at this node (the
entire tree if called on the root node).

There is no particular guarantee about the format (as it will change over time),
but a general example might be:

```js
// for a graph
//  TreeMerger
//    |- Babel_1
//    |- Babel_2
//    |--|- Funnel
console.log(JSON.stringify(node.toJSON(), null, 2));
// might print
//
{
  nodes: [{
    id: 1,
    children: [2,3],
    stats: {
      time: {
        self: 5000000,
      },
      fs: {
        lstat: {
          count: 2,
          time: 2000000
        }
      },
      own: {
      }
    }
  }, {
    // ...
  }]
}
```

### dfsIterator(until)

Returns an iterator that yields every node in the subgraph sourced at this node.
Nodes are yielded in depth-first order.  If the optional parameter `until` is
passed, nodes for which `until` returns `true` will not be yielded, nor will
nodes in their subgraph, unless those nodes are reachable by some other path.

Example:

```js
// for a graph
//  TreeMerger
//    |- Babel_1
//    |--|- Funnel
//    |- Babel_2
for (n of node.dfsIterator()) {
  console.log(n.label.name);
}
// prints
//
// TreeMerger
// Babel_1
// Funnel
// Babel_2
```

### bfsIterator()

Returns an iterator that yields every node in the subgraph sourced at this node.
Nodes are yielded in breadth-first order.  If the optional parameter `until` is
passed, nodes for which `until` returns `true` will not be yielded, nor will
nodes in their subgraph, unless those nodes are reachable by some other path.


Example:
```js
// for a tree
//  TreeMerger
//    |- Babel_1
//    |--|- Funnel
//    |- Babel_2
for (n of node.bfsIterator()) {
  console.log(n.label.name);
}
// prints
//
// TreeMerger
// Babel_1
// Babel_2
// Funnel
```

### adjacentIterator()

Returns an iterator that yields each adjacent outbound node. There is no guarantee
about the order in which they are yielded.

Example:

```js
// for a graph
//  TreeMerger
//    |- Babel_1
//    |--|- Funnel
//    |- Babel_2
for (n of node.childIterator()) {
  console.log(n.label.name);
}
// prints
//
// Babel_1
// Babel_2
```

### statsIterator()

Returns an iterator that yields `[name, value]` pairs of stat names and values.

Example:
```js
//  for a typical broccoli node 
for ([statName, statValue] of node.statsIterator()) {
  console.log(statName, statValue);
}
// prints
//
// "time.self" 64232794
// "fs.statSync.count" 40
// "fs.statSync.time" 401232123
// ...
```
