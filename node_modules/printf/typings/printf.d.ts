// Type definitions for printf 0.2.5 and above
// Project: http://www.adaltas.com/projects/node-printf
// Definitions by: Aluísio Augusto Silva Gonçalves <https://github.com/AluisioASG>

/// <reference types="node" />

export = printf;
declare function printf(format: string, ...args: any[]): string;
declare function printf(writeStream: NodeJS.WritableStream, format: string, ...args: any[]): void;
