'use strict';

const queue = require('./');

// the example worker
const worker = queue.async.asyncify(function(work) {
  console.log('work', work.file);
  return new Promise(resolve => {
    if (work.file === '/path-2') { throw new Error('/path-2'); }
    if (work.file === '/path-3') { throw new Error('/path-3'); }
    setTimeout(resolve, work.duration);
  });
});

// the work
const work = [
  { file:'/path-1',  duration: 1000 },
  { file:'/path-2',  duration: 50 },
  { file:'/path-3',  duration: 100 },
  { file:'/path-4',  duration: 50 },
];

// calling our queue helper
queue(worker, work, 3)
  .catch(reason => console.error(reason))
  .then(value   => console.log('complete!!', value))
