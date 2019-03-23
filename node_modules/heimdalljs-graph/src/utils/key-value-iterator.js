export default function *keyValueIterator(obj, prefix = '') {
  for (let key in obj) {
    let value = obj[key];

    if (typeof value === 'object') {
      yield* keyValueIterator(value, `${prefix}${key}.`);
    } else {
      yield [`${prefix}${key}`, value];
    }
  }
}
