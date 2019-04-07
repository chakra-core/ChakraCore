import _ from 'lodash';
const mergeArrays = (a, b) => (_.isArray(a) ? _.uniq(a.concat(b)) : undefined);

function mergeAndConcat(...objects) {
  if (objects.length === 0) {
    return {};
  }
  return _.mergeWith(...objects, mergeArrays);
}

export default mergeAndConcat;
