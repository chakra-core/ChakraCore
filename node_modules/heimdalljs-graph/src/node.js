import keyValueIterator from './utils/key-value-iterator';

export default class Node {
  constructor(id, label, stats, children = []) {
    this._id = id;
    this._label = label;
    this._stats = stats;

    this._parent = null;
    this._children = children;
  }

  get label() {
    return this._label;
  }

  *dfsIterator(until = (() => false)) {
    yield this;

    for (let child of this._children) {
      if (until && until(child)) {
        continue;
      }

      yield* child.dfsIterator(until);
    }
  }

  *[Symbol.iterator]() {
    yield* this.dfsIterator();
  }

  *bfsIterator(until = (() => false)) {
    let queue = [this];

    while (queue.length > 0) {
      let node = queue.shift();
      yield node;

      for (let child of node.adjacentIterator()) {
        if (until && until(child)) {
          continue;
        }

        queue.push(child);
      }
    }
  }

  *adjacentIterator() {
    for (let child of this._children) {
      yield child;
    }
  }

  *ancestorsIterator() {
    if (this._parent) {
      yield this._parent;
      yield* this._parent.ancestorsIterator();
    }
  }

  *statsIterator() {
    yield* keyValueIterator(this._stats);
  }

  toJSON() {
    let nodes = [];

    for (let node of this.dfsIterator()) {
      nodes.push({
        id: node._id,
        label: node._label,
        stats: node._stats,
        children: node._children.map(x => x._id),
      });
    }

    return { nodes };
  }
}
