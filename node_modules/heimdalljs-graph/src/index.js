// TODO: load from heimdall live events
// TODO: load from serialized events
// TODO: load from serialized graph (broccoli-viz-x.json)
// TODO: maybe lazy load

import Node from './node';

export function loadFromNode(heimdallNode) {
  // TODO: we are only doing toJSONSubgraph here b/c we're about to throw away
  // the node so the build doesn't grow unbounded; but we could just pluck off
  // this subtree and query against the live tree instead; may not matter since
  // we want to upgrade to v0.3 anyway
  let nodesJSON = heimdallNode.toJSONSubgraph();
  return loadFromV02Nodes(nodesJSON);
}

export function loadFromJSON(json) {
  let nodesJSON = json.nodes;

  return loadFromV02Nodes(nodesJSON);
}


function loadFromV02Nodes(nodesJSON) {

  let nodesById = {};
  let root = null;

  for (let jsonNode of nodesJSON) {
    let id, label;

    if (jsonNode._id) {
      [id, label] = [jsonNode._id, jsonNode.id];
    } else {
      [id, label] = [jsonNode.id, jsonNode.label];
    }

    nodesById[id] = new Node(id, label, jsonNode.stats);

    if (root === null) {
      root = nodesById[id];
    }
  }

  for (let jsonNode of nodesJSON) {
    let id = jsonNode._id || jsonNode.id;

    let node = nodesById[id];
    let children = jsonNode.children.map(childId => {
      let child = nodesById[childId];

      if (!child) {
        throw new Error(`No child with id '${childId}' found.`);
      }

      child._parent = node;
      return child;
    });
    node._children = children;
  }

  return root;
}
