import fs from 'fs';
import {
  loadFromNode as loadFromNode$1,
  loadFromJSON as loadFromJSON$1,
} from '../index.js';

export function loadFromFile(path) {
  let json = JSON.parse(fs.readFileSync(path, 'UTF8'));

  return loadFromJSON(json);
}

export function loadFromNode(...args) {
  return loadFromNode$1(...args);
}

export function loadFromJSON(...args) {
  return loadFromJSON$1(...args);
}
