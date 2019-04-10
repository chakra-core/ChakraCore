// Describes the package to typescript users.

import {Readable, Writable} from 'stream';

/** A stream that reads from a string given at creation time. */
export declare class ReadableStream extends Readable {
  constructor(contents: string);
}

/** A stream that writes into an in-memory buffer. */
export declare class WritableStream extends Writable {
  /** Returns the written contents as a string. */
  toString(): string;

  /** Returns the written contents as a buffer. */
  toBuffer(): Buffer;
}
