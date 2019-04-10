'use strict';

const findIndex = require('find-index');
const fs = require('fs');

function contentMapper(entry) {
  return entry.content;
}

function notUndefined(content) {
  return content !== undefined;
}

class Entry {
  constructor(file, contentLimit, baseDir) {
    this._content = undefined;
    this._contentLimit = contentLimit || 10000;
    this._baseDir = baseDir;
    this.file = file;
  }
  get content() {
    if (this._content !== undefined) {
      return this._content;
    }
    let content = fs.readFileSync(`${this._baseDir}${this.file}`, 'utf-8');
    if (content.length < this._contentLimit) {
      this._content = content;
    }
    return content;
  }

  set content(value) {
    if (value.length < this._contentLimit) {
      this._content = value;
    }
  }
}

class SimpleConcat {
  constructor(attrs) {
    this.separator = attrs.separator || '';

    this.header = attrs.header;
    this.headerFiles = attrs.headerFiles || [];
    this.footerFiles = attrs.footerFiles || [];
    this.footer = attrs.footer;
    this.contentLimit = attrs.contentLimit;
    this.baseDir = attrs.baseDir ? `${attrs.baseDir}/` : '';

    // Internally, we represent the concatenation as a series of entries. These
    // entries have a 'file' attribute for lookup/sorting and a 'content' property
    // which represents the value to be used in the concatenation.
    this._internal = [];

    // We represent the header/footer files as empty at first so that we don't
    // have to figure out order when patching
    this._internalHeaderFiles = this.headerFiles.map(file => { return { file: file }; });
    this._internalFooterFiles = this.footerFiles.map(file => { return { file: file }; });
  }

  /**
   * Finds the index of the given file in the internal data structure.
   */
  _findIndexOf(file) {
    return findIndex(this._internal, entry => entry.file === file);
  }

  /**
   * Updates the contents of a header file.
   */
  _updateHeaderFile(fileIndex, content) {
    this._internalHeaderFiles[fileIndex].content = content;
  }

  /**
   * Updates the contents of a footer file.
   */
  _updateFooterFile(fileIndex, content) {
    this._internalFooterFiles[fileIndex].content = content;
  }

  /**
   * Determines if the given file is a header or footer file, and if so updates
   * it with the given contents.
   */
  _handleHeaderOrFooterFile(file, content) {
    let headerFileIndex = this.headerFiles.indexOf(file);
    if (headerFileIndex !== -1) {
      this._updateHeaderFile(headerFileIndex, content);
      return true;
    }

    let footerFileIndex = this.footerFiles.indexOf(file);
    if (footerFileIndex !== -1) {
      this._updateFooterFile(footerFileIndex, content);
      return true;
    }

    return false;
  }

  addFile(file, content) {
    if (this._handleHeaderOrFooterFile(file, content)) {
      return;
    }
    
    let entry = new Entry(file, this.contentLimit, this.baseDir);

    let index = findIndex(this._internal, entry => entry.file > file);
    if (index === -1) {
      this._internal.push(entry);
    } else {
      this._internal.splice(index, 0, entry);
    }
  }

  updateFile(file, content) {
    if (this._handleHeaderOrFooterFile(file, content)) {
      return;
    }

    let index = this._findIndexOf(file);

    if (index === -1) {
      throw new Error('Trying to update ' + file + ' but it has not been read before');
    }

    this._internal[index].content = content;
  }

  removeFile(file) {
    if (this._handleHeaderOrFooterFile(file, undefined)) {
      return;
    }
    let index = this._findIndexOf(file);

    if (index === -1) {
      throw new Error('Trying to remove ' + file + ' but it did not previously exist');
    }

    this._internal.splice(index, 1);
  }

  fileSizes() {
    return [].concat(
      this._internalHeaderFiles,
      this._internal,
      this._internalFooterFiles
    ).reduce((sizes, entry) => {
      sizes[entry.file] = entry.content.length;
      return sizes;
    }, {});
  }

  result() {
    let fileContents = [].concat(
      this._internalHeaderFiles.map(contentMapper),
      this._internal.map(contentMapper),
      this._internalFooterFiles.map(contentMapper)
    ).filter(notUndefined);

    if (!fileContents.length) {
      return;
    }

    let content = [].concat(
      this.header,
      fileContents,
      this.footer
    ).filter(notUndefined).join(this.separator);

    if (this.footer) {
      content += '\n';
    }

    return content;
  }
}

SimpleConcat.isPatchBased = true;

module.exports = SimpleConcat;
