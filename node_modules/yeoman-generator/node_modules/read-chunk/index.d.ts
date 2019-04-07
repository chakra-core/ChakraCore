/// <reference types="node"/>

/**
 * Read a chunk from a file asyncronously.
 *
 * @param filePath - The path to the file.
 * @param startPosition - Position to start reading.
 * @param length - Number of bytes to read.
 * @returns The read chunk.
 */
declare const readChunk: {
	(filePath: string, startPosition: number, length: number): Promise<Buffer>;

	/**
	 * Read a chunk from a file synchronously.
	 *
	 * @param filePath - The path to the file.
	 * @param startPosition - Position to start reading.
	 * @param length - Number of bytes to read.
	 * @returns The read chunk.
	 */
	sync(filePath: string, startPosition: number, length: number): Buffer;
};

export default readChunk;
