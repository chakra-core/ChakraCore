(function() {
  var CAPACITY_READ_ONLY, PAYLOAD_UNIT, Pickle, PickleIterator, SIZE_DOUBLE, SIZE_FLOAT, SIZE_INT32, SIZE_INT64, SIZE_UINT32, SIZE_UINT64, alignInt;

  SIZE_INT32 = 4;

  SIZE_UINT32 = 4;

  SIZE_INT64 = 8;

  SIZE_UINT64 = 8;

  SIZE_FLOAT = 4;

  SIZE_DOUBLE = 8;

  PAYLOAD_UNIT = 64;

  CAPACITY_READ_ONLY = 9007199254740992;

  alignInt = function(i, alignment) {
    return i + (alignment - (i % alignment)) % alignment;
  };

  PickleIterator = (function() {
    function PickleIterator(pickle) {
      this.payload = pickle.header;
      this.payloadOffset = pickle.headerSize;
      this.readIndex = 0;
      this.endIndex = pickle.getPayloadSize();
    }

    PickleIterator.prototype.readBool = function() {
      return this.readInt() !== 0;
    };

    PickleIterator.prototype.readInt = function() {
      return this.readBytes(SIZE_INT32, Buffer.prototype.readInt32LE);
    };

    PickleIterator.prototype.readUInt32 = function() {
      return this.readBytes(SIZE_UINT32, Buffer.prototype.readUInt32LE);
    };

    PickleIterator.prototype.readInt64 = function() {
      return this.readBytes(SIZE_INT64, Buffer.prototype.readInt64LE);
    };

    PickleIterator.prototype.readUInt64 = function() {
      return this.readBytes(SIZE_UINT64, Buffer.prototype.readUInt64LE);
    };

    PickleIterator.prototype.readFloat = function() {
      return this.readBytes(SIZE_FLOAT, Buffer.prototype.readFloatLE);
    };

    PickleIterator.prototype.readDouble = function() {
      return this.readBytes(SIZE_DOUBLE, Buffer.prototype.readDoubleLE);
    };

    PickleIterator.prototype.readString = function() {
      return this.readBytes(this.readInt()).toString();
    };

    PickleIterator.prototype.readBytes = function(length, method) {
      var readPayloadOffset;
      readPayloadOffset = this.getReadPayloadOffsetAndAdvance(length);
      if (method != null) {
        return method.call(this.payload, readPayloadOffset, length);
      } else {
        return this.payload.slice(readPayloadOffset, readPayloadOffset + length);
      }
    };

    PickleIterator.prototype.getReadPayloadOffsetAndAdvance = function(length) {
      var readPayloadOffset;
      if (length > this.endIndex - this.readIndex) {
        this.readIndex = this.endIndex;
        throw new Error("Failed to read data with length of " + length);
      }
      readPayloadOffset = this.payloadOffset + this.readIndex;
      this.advance(length);
      return readPayloadOffset;
    };

    PickleIterator.prototype.advance = function(size) {
      var alignedSize;
      alignedSize = alignInt(size, SIZE_UINT32);
      if (this.endIndex - this.readIndex < alignedSize) {
        return this.readIndex = this.endIndex;
      } else {
        return this.readIndex += alignedSize;
      }
    };

    return PickleIterator;

  })();

  Pickle = (function() {
    function Pickle(buffer) {
      if (buffer) {
        this.initFromBuffer(buffer);
      } else {
        this.initEmpty();
      }
    }

    Pickle.prototype.initEmpty = function() {
      this.header = new Buffer(0);
      this.headerSize = SIZE_UINT32;
      this.capacityAfterHeader = 0;
      this.writeOffset = 0;
      this.resize(PAYLOAD_UNIT);
      return this.setPayloadSize(0);
    };

    Pickle.prototype.initFromBuffer = function(buffer) {
      this.header = buffer;
      this.headerSize = buffer.length - this.getPayloadSize();
      this.capacityAfterHeader = CAPACITY_READ_ONLY;
      this.writeOffset = 0;
      if (this.headerSize > buffer.length) {
        this.headerSize = 0;
      }
      if (this.headerSize !== alignInt(this.headerSize, SIZE_UINT32)) {
        this.headerSize = 0;
      }
      if (this.headerSize === 0) {
        return this.header = new Buffer(0);
      }
    };

    Pickle.prototype.createIterator = function() {
      return new PickleIterator(this);
    };

    Pickle.prototype.toBuffer = function() {
      return this.header.slice(0, this.headerSize + this.getPayloadSize());
    };

    Pickle.prototype.writeBool = function(value) {
      return this.writeInt(value ? 1 : 0);
    };

    Pickle.prototype.writeInt = function(value) {
      return this.writeBytes(value, SIZE_INT32, Buffer.prototype.writeInt32LE);
    };

    Pickle.prototype.writeUInt32 = function(value) {
      return this.writeBytes(value, SIZE_UINT32, Buffer.prototype.writeUInt32LE);
    };

    Pickle.prototype.writeInt64 = function(value) {
      return this.writeBytes(value, SIZE_INT64, Buffer.prototype.writeInt64LE);
    };

    Pickle.prototype.writeUInt64 = function(value) {
      return this.writeBytes(value, SIZE_UINT64, Buffer.prototype.writeUInt64LE);
    };

    Pickle.prototype.writeFloat = function(value) {
      return this.writeBytes(value, SIZE_FLOAT, Buffer.prototype.writeFloatLE);
    };

    Pickle.prototype.writeDouble = function(value) {
      return this.writeBytes(value, SIZE_DOUBLE, Buffer.prototype.writeDoubleLE);
    };

    Pickle.prototype.writeString = function(value) {
      if (!this.writeInt(value.length)) {
        return false;
      }
      return this.writeBytes(value, value.length);
    };

    Pickle.prototype.setPayloadSize = function(payloadSize) {
      return this.header.writeUInt32LE(payloadSize, 0);
    };

    Pickle.prototype.getPayloadSize = function() {
      return this.header.readUInt32LE(0);
    };

    Pickle.prototype.writeBytes = function(data, length, method) {
      var dataLength, endOffset, newSize;
      dataLength = alignInt(length, SIZE_UINT32);
      newSize = this.writeOffset + dataLength;
      if (newSize > this.capacityAfterHeader) {
        this.resize(Math.max(this.capacityAfterHeader * 2, newSize));
      }
      if (method != null) {
        method.call(this.header, data, this.headerSize + this.writeOffset);
      } else {
        this.header.write(data, this.headerSize + this.writeOffset, length);
      }
      endOffset = this.headerSize + this.writeOffset + length;
      this.header.fill(0, endOffset, endOffset + dataLength - length);
      this.setPayloadSize(newSize);
      this.writeOffset = newSize;
      return true;
    };

    Pickle.prototype.resize = function(newCapacity) {
      newCapacity = alignInt(newCapacity, PAYLOAD_UNIT);
      this.header = Buffer.concat([this.header, new Buffer(newCapacity)]);
      return this.capacityAfterHeader = newCapacity;
    };

    return Pickle;

  })();

  module.exports = Pickle;

}).call(this);
