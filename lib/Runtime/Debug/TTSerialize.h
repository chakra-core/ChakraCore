//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#if ENABLE_TTD

#define TTD_SERIALIZATION_BUFFER_SIZE 2097152
#define TTD_SERIALIZATION_MAX_FORMATTED_DATA_SIZE 128

//forward decl
//
//TODO: This is not cool but we need it for the trace logger decls.
//      Split that class out into a seperate file and then include it later in Runtime.h
//
namespace Js
{
    class JavascriptFunction;
}

namespace TTD
{
    namespace NSTokens
    {
        //Seperator tokens for records
        enum class Separator : byte
        {
            NoSeparator = 0x0,
            CommaSeparator = 0x1,
            BigSpaceSeparator = 0x2,
            CommaAndBigSpaceSeparator = (CommaSeparator | BigSpaceSeparator)
        };
        DEFINE_ENUM_FLAG_OPERATORS(Separator);

        enum class ParseTokenKind
        {
            Error = 0x0,
            Comma,
            Colon,
            LBrack,
            RBrack,
            LCurly,
            RCurly,
            Null,
            True,
            False,
            NaN,
            PosInfty,
            NegInfty,
            UpperBound,
            LowerBound,
            Epsilon,
            Number,
            Address,
            LogTag,
            EnumTag,
            WellKnownToken,
            String
        };

        //Key values for records
        //WARNING - note the byte size on the enum type so be careful when adding new keys to the enumeration
        enum class Key : byte
        {
            Invalid = 0x0,
#define ENTRY_SERIALIZE_ENUM(X) X,
#include "TTSerializeEnum.h"
            Count
        };

        void InitKeyNamesArray(const char16*** names, size_t** lengths);
        void CleanupKeyNamesArray(const char16*** names, size_t** lengths);
    }

    ////

    //A virtual class that handles the actual write (and format) of a value to a stream
    class FileWriter
    {
    private:
        //The file that we are writing into
        JsTTDStreamHandle m_hfile;
        TTDWriteBytesToStreamCallback m_pfWrite;
        TTDFlushAndCloseStreamCallback m_pfClose;

        size_t m_cursor;
        byte* m_buffer;

        //flush the buffer contents to disk
        void WriteBlock(const byte* buff, size_t bufflen);

        template <size_t requestedSpace>
        byte* ReserveSpaceForSmallData()
        {
            TTDAssert(requestedSpace < TTD_SERIALIZATION_BUFFER_SIZE, "Must be small data element!");

            if(this->m_cursor + requestedSpace >= TTD_SERIALIZATION_BUFFER_SIZE)
            {
                this->WriteBlock(this->m_buffer, this->m_cursor);
                this->m_cursor = 0;
            }

            return (this->m_buffer + this->m_cursor);
        }

        void CommitSpaceForSmallData(size_t usedSpace)
        {
            TTDAssert(this->m_cursor + usedSpace < TTD_SERIALIZATION_BUFFER_SIZE, "Must have already reserved the space!");

            this->m_cursor += usedSpace;
        }

    protected:
        template <typename T>
        void WriteRawByteBuff_Fixed(const T& data)
        {
            byte* trgt = this->ReserveSpaceForSmallData<sizeof(T)>();

            js_memcpy_s(trgt, sizeof(T), (const byte*)(&data), sizeof(T));

            this->CommitSpaceForSmallData(sizeof(T));
        }

        void WriteRawByteBuff(const byte* buff, size_t bufflen)
        {
            if(this->m_cursor + bufflen < TTD_SERIALIZATION_BUFFER_SIZE)
            {
                size_t sizeAvailable = (TTD_SERIALIZATION_BUFFER_SIZE - this->m_cursor);
                TTDAssert(sizeAvailable >= bufflen, "Our size computation is off somewhere.");

                js_memcpy_s(this->m_buffer + this->m_cursor, sizeAvailable, buff, bufflen);
                this->m_cursor += bufflen;
            }
            else
            {
                this->WriteBlock(this->m_buffer, this->m_cursor);
                this->m_cursor = 0;

                const byte* remainingBuff = buff;
                size_t remainingBytes = bufflen;
                while(remainingBytes > TTD_SERIALIZATION_BUFFER_SIZE)
                {
                    TTDAssert(this->m_cursor == 0, "Should be empty.");

                    this->WriteBlock(remainingBuff, TTD_SERIALIZATION_BUFFER_SIZE);
                    remainingBuff += TTD_SERIALIZATION_BUFFER_SIZE;
                    remainingBytes -= TTD_SERIALIZATION_BUFFER_SIZE;
                }

                if(remainingBytes > 0)
                {
                    js_memcpy_s(this->m_buffer, TTD_SERIALIZATION_BUFFER_SIZE, remainingBuff, remainingBytes);
                    this->m_cursor += remainingBytes;
                }
            }
        }

        void WriteRawCharBuff(const char16* buff, size_t bufflen)
        {
            this->WriteRawByteBuff((const byte*)buff, bufflen * sizeof(char16));
        }

        void WriteRawChar(char16 c)
        {
            this->WriteRawByteBuff_Fixed<char16>(c);
        }

        template <size_t N, typename T>
        void WriteFormattedCharData(const char16(&formatString)[N], T data)
        {
            byte* trgtBuff = this->ReserveSpaceForSmallData<TTD_SERIALIZATION_MAX_FORMATTED_DATA_SIZE>();

            int addedChars = swprintf_s((char16*)trgtBuff, (TTD_SERIALIZATION_MAX_FORMATTED_DATA_SIZE / sizeof(char16)), formatString, data);
            TTDAssert(addedChars != -1 && addedChars < (TTD_SERIALIZATION_MAX_FORMATTED_DATA_SIZE / sizeof(char16)), "Formatting failed or result is too big.");

            int addedBytes = (addedChars != -1) ? (addedChars * sizeof(char16)) : 0;
            this->CommitSpaceForSmallData(addedBytes);
        }

    public:
        FileWriter(JsTTDStreamHandle handle, TTDWriteBytesToStreamCallback pfWrite, TTDFlushAndCloseStreamCallback pfClose);
        virtual ~FileWriter();

        void FlushAndClose();

        ////

        virtual void WriteSeperator(NSTokens::Separator separator) = 0;
        virtual void WriteKey(NSTokens::Key key, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;

        void WriteLengthValue(uint32 length, NSTokens::Separator separator = NSTokens::Separator::NoSeparator);

        void WriteSequenceStart_DefaultKey(NSTokens::Separator separator = NSTokens::Separator::NoSeparator);
        virtual void WriteSequenceStart(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        virtual void WriteSequenceEnd(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;

        void WriteRecordStart_DefaultKey(NSTokens::Separator separator = NSTokens::Separator::NoSeparator);
        virtual void WriteRecordStart(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        virtual void WriteRecordEnd(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;

        virtual void AdjustIndent(int32 delta) = 0;
        virtual void SetIndent(uint32 depth) = 0;

        ////

        virtual void WriteNakedNull(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        void WriteNull(NSTokens::Key key, NSTokens::Separator separator = NSTokens::Separator::NoSeparator);

        virtual void WriteNakedByte(byte val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        virtual void WriteBool(NSTokens::Key key, bool val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;

        virtual void WriteNakedInt32(int32 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        void WriteInt32(NSTokens::Key key, int32 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator);

        virtual void WriteNakedUInt32(uint32 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        void WriteUInt32(NSTokens::Key key, uint32 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator);

        virtual void WriteNakedInt64(int64 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        void WriteInt64(NSTokens::Key key, int64 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator);

        virtual void WriteNakedUInt64(uint64 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        void WriteUInt64(NSTokens::Key key, uint64 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator);

        virtual void WriteNakedDouble(double val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        void WriteDouble(NSTokens::Key key, double val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator);

        virtual void WriteNakedAddr(TTD_PTR_ID val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        void WriteAddr(NSTokens::Key key, TTD_PTR_ID val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator);

        virtual void WriteNakedLogTag(TTD_LOG_PTR_ID val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        void WriteLogTag(NSTokens::Key key, TTD_LOG_PTR_ID val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator);

        virtual void WriteNakedTag(uint32 tagvalue, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;

        template <typename T>
        void WriteTag(NSTokens::Key key, T tag, NSTokens::Separator separator = NSTokens::Separator::NoSeparator)
        {
            this->WriteKey(key, separator);
            this->WriteNakedTag((uint32)tag);
        }

        ////

        virtual void WriteNakedString(const TTString& val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        void WriteString(NSTokens::Key key, const TTString& val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator);

        virtual void WriteNakedWellKnownToken(TTD_WELLKNOWN_TOKEN val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        void WriteWellKnownToken(NSTokens::Key key, TTD_WELLKNOWN_TOKEN val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator);

        virtual void WriteInlineCode(_In_reads_(length) const char16* code, uint32 length, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        virtual void WriteInlinePropertyRecordName(_In_reads_(length) const char16* pname, uint32 length, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
    };

    //A implements the writer for verbose text formatted output
    class TextFormatWriter : public FileWriter
    {
    private:
        //Array of key names and their lengths
        const char16** m_keyNameArray;
        size_t* m_keyNameLengthArray;

        //indent size for formatting
        uint32 m_indentSize;

    public:
        TextFormatWriter(JsTTDStreamHandle handle, TTDWriteBytesToStreamCallback pfWrite, TTDFlushAndCloseStreamCallback pfClose);
        virtual ~TextFormatWriter();

        ////

        virtual void WriteSeperator(NSTokens::Separator separator) override;
        virtual void WriteKey(NSTokens::Key key, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteSequenceStart(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteSequenceEnd(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteRecordStart(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteRecordEnd(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void AdjustIndent(int32 delta) override;
        virtual void SetIndent(uint32 depth) override;

        ////

        virtual void WriteNakedNull(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteNakedByte(byte val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteBool(NSTokens::Key key, bool val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteNakedInt32(int32 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteNakedUInt32(uint32 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteNakedInt64(int64 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteNakedUInt64(uint64 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteNakedDouble(double val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteNakedAddr(TTD_PTR_ID val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteNakedLogTag(TTD_LOG_PTR_ID val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteNakedTag(uint32 tagvalue, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        ////

        virtual void WriteNakedString(const TTString& val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteNakedWellKnownToken(TTD_WELLKNOWN_TOKEN val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteInlineCode(_In_reads_(length) const char16* code, uint32 length, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteInlinePropertyRecordName(_In_reads_(length) const char16* pname, uint32 length, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
    };

    //A implements the writer for a compact binary formatted output
    class BinaryFormatWriter : public FileWriter
    {
    public:
        BinaryFormatWriter(JsTTDStreamHandle handle, TTDWriteBytesToStreamCallback pfWrite, TTDFlushAndCloseStreamCallback pfClose);
        virtual ~BinaryFormatWriter();

        ////

        virtual void WriteSeperator(NSTokens::Separator separator) override;
        virtual void WriteKey(NSTokens::Key key, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteSequenceStart(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteSequenceEnd(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteRecordStart(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteRecordEnd(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void AdjustIndent(int32 delta) override;
        virtual void SetIndent(uint32 depth) override;

        ////

        virtual void WriteNakedNull(NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteNakedByte(byte val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteBool(NSTokens::Key key, bool val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteNakedInt32(int32 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteNakedUInt32(uint32 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteNakedInt64(int64 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteNakedUInt64(uint64 val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteNakedDouble(double val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteNakedAddr(TTD_PTR_ID val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteNakedLogTag(TTD_LOG_PTR_ID val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteNakedTag(uint32 tagvalue, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        ////

        virtual void WriteNakedString(const TTString& val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteNakedWellKnownToken(TTD_WELLKNOWN_TOKEN val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteInlineCode(_In_reads_(length) const char16* code, uint32 length, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
        virtual void WriteInlinePropertyRecordName(_In_reads_(length) const char16* pname, uint32 length, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
    };

    //////////////////

    //A virtual class that handles the actual read of values from a stream
    class FileReader
    {
    private:
        JsTTDStreamHandle m_hfile;
        TTDReadBytesFromStreamCallback m_pfRead;
        TTDFlushAndCloseStreamCallback m_pfClose;

        int32 m_peekChar;

        size_t m_cursor;
        size_t m_buffCount;
        byte* m_buffer;

        void ReadBlock(byte* buff, size_t* readSize);

    protected:
        template <typename T>
        void ReadBytesInto_Fixed(T& data)
        {
            size_t sizeAvailable = (this->m_buffCount - this->m_cursor);
            byte* buff = (byte*)&data;

            if(sizeAvailable >= sizeof(T))
            {
                js_memcpy_s(buff, sizeAvailable, this->m_buffer + this->m_cursor, sizeof(T));
                this->m_cursor += sizeof(T);
            }
            else
            {
                if(sizeAvailable > 0)
                {
                    js_memcpy_s(buff, sizeAvailable, this->m_buffer + this->m_cursor, sizeAvailable);
                    this->m_cursor += sizeAvailable;
                }

                byte* remainingBuff = (buff + sizeAvailable);
                size_t remainingBytes = (sizeof(T) - sizeAvailable);

                if(remainingBytes > 0)
                {
                    this->ReadBlock(this->m_buffer, &this->m_buffCount);
                    this->m_cursor = 0;

                    TTDAssert(this->m_buffCount >= remainingBytes, "Not sure what happened");
                    js_memcpy_s(remainingBuff, this->m_buffCount, this->m_buffer, remainingBytes);
                    this->m_cursor += remainingBytes;
                }
            }
        }

        void ReadBytesInto(byte* buff, size_t requiredBytes)
        {
            size_t sizeAvailable = (this->m_buffCount - this->m_cursor);

            if(sizeAvailable >= requiredBytes)
            {
                js_memcpy_s(buff, sizeAvailable, this->m_buffer + this->m_cursor, requiredBytes);
                this->m_cursor += requiredBytes;
            }
            else
            {
                if(sizeAvailable > 0)
                {
                    js_memcpy_s(buff, sizeAvailable, this->m_buffer + this->m_cursor, sizeAvailable);
                    this->m_cursor += sizeAvailable;
                }

                byte* remainingBuff = (buff + sizeAvailable);
                size_t remainingBytes = (requiredBytes - sizeAvailable);

                while(remainingBytes > TTD_SERIALIZATION_BUFFER_SIZE)
                {
                    size_t readCount = 0;
                    this->ReadBlock(remainingBuff, &readCount);

                    TTDAssert(readCount > 0, "We are out of data but still need more");
                    remainingBuff += readCount;
                    remainingBytes -= readCount;
                }

                if(remainingBytes > 0)
                {
                    this->ReadBlock(this->m_buffer, &this->m_buffCount);
                    this->m_cursor = 0;

                    TTDAssert(this->m_buffCount >= remainingBytes, "Not sure what happened");
                    js_memcpy_s(remainingBuff, this->m_buffCount, this->m_buffer, remainingBytes);
                    this->m_cursor += remainingBytes;
                }
            }
        }

        bool PeekRawChar(_Out_ char16* c)
        {
            if(this->m_peekChar != -1)
            {
                *c = (char16)this->m_peekChar;
                return true;
            }
            else
            {
                bool success = this->ReadRawChar(c);
                if(success)
                {
                    this->m_peekChar = *c;
                }
                return success;
            }
        }

        bool ReadRawChar(_Out_ char16* c)
        {
            if(this->m_peekChar != -1)
            {
                *c = (char16)this->m_peekChar;
                this->m_peekChar = -1;

                return true;
            }
            else
            {
                if(this->m_cursor == this->m_buffCount)
                {
                    this->ReadBlock(this->m_buffer, &this->m_buffCount);
                    this->m_cursor = 0;
                }

                if(this->m_cursor == this->m_buffCount)
                {
                    // Make sure to set a value before we return.
                    *c = _u('\0');

                    return false;
                }
                else
                {
                    *c = *((char16*)(this->m_buffer + this->m_cursor));
                    this->m_cursor += sizeof(char16);

                    return true;
                }
            }
        }

    public:
        FileReader(JsTTDStreamHandle handle, TTDReadBytesFromStreamCallback pfRead, TTDFlushAndCloseStreamCallback pfClose);
        virtual ~FileReader();

        virtual void ReadSeperator(bool readSeparator) = 0;
        virtual void ReadKey(NSTokens::Key keyCheck, bool readSeparator = false) = 0;

        uint32 ReadLengthValue(bool readSeparator = false);

        void ReadSequenceStart_WDefaultKey(bool readSeparator = false);
        virtual void ReadSequenceStart(bool readSeparator = false) = 0;
        virtual void ReadSequenceEnd() = 0;

        void ReadRecordStart_WDefaultKey(bool readSeparator = false);
        virtual void ReadRecordStart(bool readSeparator = false) = 0;
        virtual void ReadRecordEnd() = 0;

        ////

        virtual void ReadNakedNull(bool readSeparator = false) = 0;
        void ReadNull(NSTokens::Key keyCheck, bool readSeparator = false);

        virtual byte ReadNakedByte(bool readSeparator = false) = 0;
        virtual bool ReadBool(NSTokens::Key keyCheck, bool readSeparator = false) = 0;

        virtual int32 ReadNakedInt32(bool readSeparator = false) = 0;
        int32 ReadInt32(NSTokens::Key keyCheck, bool readSeparator = false);

        virtual uint32 ReadNakedUInt32(bool readSeparator = false) = 0;
        uint32 ReadUInt32(NSTokens::Key keyCheck, bool readSeparator = false);

        virtual int64 ReadNakedInt64(bool readSeparator = false) = 0;
        int64 ReadInt64(NSTokens::Key keyCheck, bool readSeparator = false);

        virtual uint64 ReadNakedUInt64(bool readSeparator = false) = 0;
        uint64 ReadUInt64(NSTokens::Key keyCheck, bool readSeparator = false);

        virtual double ReadNakedDouble(bool readSeparator = false) = 0;
        double ReadDouble(NSTokens::Key keyCheck, bool readSeparator = false);

        virtual TTD_PTR_ID ReadNakedAddr(bool readSeparator = false) = 0;
        TTD_PTR_ID ReadAddr(NSTokens::Key keyCheck, bool readSeparator = false);

        virtual TTD_LOG_PTR_ID ReadNakedLogTag(bool readSeparator = false) = 0;
        TTD_LOG_PTR_ID ReadLogTag(NSTokens::Key keyCheck, bool readSeparator = false);

        virtual uint32 ReadNakedTag(bool readSeparator = false) = 0;

        template <typename T>
        T ReadTag(NSTokens::Key keyCheck, bool readSeparator = false)
        {
            this->ReadKey(keyCheck, readSeparator);
            uint32 tval = this->ReadNakedTag();

            return (T)tval;
        }

        ////

        virtual void ReadNakedString(SlabAllocator& alloc, TTString& into, bool readSeparator = false) = 0;
        virtual void ReadNakedString(UnlinkableSlabAllocator& alloc, TTString& into, bool readSeparator = false) = 0;

        template <typename Allocator>
        void ReadString(NSTokens::Key keyCheck, Allocator& alloc, TTString& into, bool readSeparator = false)
        {
            this->ReadKey(keyCheck, readSeparator);
            return this->ReadNakedString(alloc, into);
        }

        virtual TTD_WELLKNOWN_TOKEN ReadNakedWellKnownToken(SlabAllocator& alloc, bool readSeparator = false) = 0;
        virtual TTD_WELLKNOWN_TOKEN ReadNakedWellKnownToken(UnlinkableSlabAllocator& alloc, bool readSeparator = false) = 0;

        template <typename Allocator>
        TTD_WELLKNOWN_TOKEN ReadWellKnownToken(NSTokens::Key keyCheck, Allocator& alloc, bool readSeparator = false)
        {
            this->ReadKey(keyCheck, readSeparator);
            return this->ReadNakedWellKnownToken(alloc);
        }

        virtual void ReadInlineCode(_Out_writes_(length) char16* code, uint32 length, bool readSeparator = false) = 0;
    };

    //////////////////

    //A serialization class that reads a verbose text data format
    class TextFormatReader : public FileReader
    {
    private:
        JsUtil::List<char16, HeapAllocator> m_charListPrimary;
        JsUtil::List<char16, HeapAllocator> m_charListOpt;
        JsUtil::List<char16, HeapAllocator> m_charListDiscard;

        //Array of key names and their lengths
        const char16** m_keyNameArray;
        size_t* m_keyNameLengthArray;

        NSTokens::ParseTokenKind Scan(JsUtil::List<char16, HeapAllocator>& charList);

        NSTokens::ParseTokenKind ScanKey(JsUtil::List<char16, HeapAllocator>& charList);

        NSTokens::ParseTokenKind ScanSpecialNumber();
        NSTokens::ParseTokenKind ScanNumber(JsUtil::List<char16, HeapAllocator>& charList);
        NSTokens::ParseTokenKind ScanAddress(JsUtil::List<char16, HeapAllocator>& charList);
        NSTokens::ParseTokenKind ScanLogTag(JsUtil::List<char16, HeapAllocator>& charList);
        NSTokens::ParseTokenKind ScanEnumTag(JsUtil::List<char16, HeapAllocator>& charList);
        NSTokens::ParseTokenKind ScanWellKnownToken(JsUtil::List<char16, HeapAllocator>& charList);

        NSTokens::ParseTokenKind ScanString(JsUtil::List<char16, HeapAllocator>& charList);
        NSTokens::ParseTokenKind ScanNakedString(char16 leadChar);

        int64 ReadIntFromCharArray(const char16* buff);
        uint64 ReadUIntFromCharArray(const char16* buff);
        double ReadDoubleFromCharArray(const char16* buff);

    public:
        TextFormatReader(JsTTDStreamHandle handle, TTDReadBytesFromStreamCallback pfRead, TTDFlushAndCloseStreamCallback pfClose);
        virtual ~TextFormatReader();

        virtual void ReadSeperator(bool readSeparator) override;
        virtual void ReadKey(NSTokens::Key keyCheck, bool readSeparator = false) override;

        virtual void ReadSequenceStart(bool readSeparator = false) override;
        virtual void ReadSequenceEnd() override;
        virtual void ReadRecordStart(bool readSeparator = false) override;
        virtual void ReadRecordEnd() override;

        ////

        virtual void ReadNakedNull(bool readSeparator = false) override;
        virtual byte ReadNakedByte(bool readSeparator = false) override;
        virtual bool ReadBool(NSTokens::Key keyCheck, bool readSeparator = false) override;

        virtual int32 ReadNakedInt32(bool readSeparator = false) override;
        virtual uint32 ReadNakedUInt32(bool readSeparator = false) override;
        virtual int64 ReadNakedInt64(bool readSeparator = false) override;
        virtual uint64 ReadNakedUInt64(bool readSeparator = false) override;
        virtual double ReadNakedDouble(bool readSeparator = false) override;
        virtual TTD_PTR_ID ReadNakedAddr(bool readSeparator = false) override;
        virtual TTD_LOG_PTR_ID ReadNakedLogTag(bool readSeparator = false) override;

        virtual uint32 ReadNakedTag(bool readSeparator = false) override;

        ////

        virtual void ReadNakedString(SlabAllocator& alloc, TTString& into, bool readSeparator = false) override;
        virtual void ReadNakedString(UnlinkableSlabAllocator& alloc, TTString& into, bool readSeparator = false) override;

        virtual TTD_WELLKNOWN_TOKEN ReadNakedWellKnownToken(SlabAllocator& alloc, bool readSeparator = false) override;
        virtual TTD_WELLKNOWN_TOKEN ReadNakedWellKnownToken(UnlinkableSlabAllocator& alloc, bool readSeparator = false) override;

        virtual void ReadInlineCode(_Out_writes_(length) char16* code, uint32 length, bool readSeparator = false) override;
    };

    //A serialization class that reads a compact binary format
    class BinaryFormatReader : public FileReader
    {
    public:
        BinaryFormatReader(JsTTDStreamHandle handle, TTDReadBytesFromStreamCallback pfRead, TTDFlushAndCloseStreamCallback pfClose);
        virtual ~BinaryFormatReader();

        virtual void ReadSeperator(bool readSeparator) override;
        virtual void ReadKey(NSTokens::Key keyCheck, bool readSeparator = false) override;

        virtual void ReadSequenceStart(bool readSeparator = false) override;
        virtual void ReadSequenceEnd() override;
        virtual void ReadRecordStart(bool readSeparator = false) override;
        virtual void ReadRecordEnd() override;

        ////

        virtual void ReadNakedNull(bool readSeparator = false) override;
        virtual byte ReadNakedByte(bool readSeparator = false) override;
        virtual bool ReadBool(NSTokens::Key keyCheck, bool readSeparator = false) override;

        virtual int32 ReadNakedInt32(bool readSeparator = false) override;
        virtual uint32 ReadNakedUInt32(bool readSeparator = false) override;
        virtual int64 ReadNakedInt64(bool readSeparator = false) override;
        virtual uint64 ReadNakedUInt64(bool readSeparator = false) override;
        virtual double ReadNakedDouble(bool readSeparator = false) override;
        virtual TTD_PTR_ID ReadNakedAddr(bool readSeparator = false) override;
        virtual TTD_LOG_PTR_ID ReadNakedLogTag(bool readSeparator = false) override;

        virtual uint32 ReadNakedTag(bool readSeparator = false) override;

        ////

        virtual void ReadNakedString(SlabAllocator& alloc, TTString& into, bool readSeparator = false) override;
        virtual void ReadNakedString(UnlinkableSlabAllocator& alloc, TTString& into, bool readSeparator = false) override;

        virtual TTD_WELLKNOWN_TOKEN ReadNakedWellKnownToken(SlabAllocator& alloc, bool readSeparator = false) override;
        virtual TTD_WELLKNOWN_TOKEN ReadNakedWellKnownToken(UnlinkableSlabAllocator& alloc, bool readSeparator = false) override;

        virtual void ReadInlineCode(_Out_writes_(length) char16* code, uint32 length, bool readSeparator = false) override;
    };

    //////////////////

#if ENABLE_OBJECT_SOURCE_TRACKING
    //A struct that we use for tracking where objects have been allocated
    struct DiagnosticOrigin
    {
        int32 SourceLine;
        uint32 EventTime;
        uint64 TimeHash;
    };

    bool IsDiagnosticOriginInformationValid(const DiagnosticOrigin& info);

    void InitializeDiagnosticOriginInformation(DiagnosticOrigin& info);
    void CopyDiagnosticOriginInformation(DiagnosticOrigin& infoInto, const DiagnosticOrigin& infoFrom);
    void SetDiagnosticOriginInformation(DiagnosticOrigin& info, uint32 sourceLine, uint64 eTime, uint64 fTime, uint64 lTime);

    void EmitDiagnosticOriginInformation(const DiagnosticOrigin& info, FileWriter* writer, NSTokens::Separator separator);
    void ParseDiagnosticOriginInformation(DiagnosticOrigin& info, bool readSeperator, FileReader* reader);
#endif

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
#define TRACE_LOGGER_BUFFER_SIZE 4096
#define TRACE_LOGGER_INDENT_BUFFER_SIZE 64

    //Class that provides all of the trace output functionality we want
    class TraceLogger
    {
    private:
        char* m_buffer;
        char* m_indentBuffer;

        int32 m_currLength;
        int32 m_indentSize;
        FILE* m_outfile;

        void EnsureSpace(uint32 length)
        {
            if(this->m_currLength + length >= TRACE_LOGGER_BUFFER_SIZE)
            {
                fwrite(this->m_buffer, sizeof(char), this->m_currLength, this->m_outfile);
                fflush(this->m_outfile);

                this->m_currLength = 0;
            }
        }

        void AppendRaw(const char* str, uint32 length)
        {
            if(length >= TRACE_LOGGER_BUFFER_SIZE)
            {
                const char* msg = "Oversize string ... omitting from output";
                fwrite(msg, sizeof(char), strlen(msg), this->m_outfile);
            }
            else
            {
                TTDAssert(this->m_currLength + length < TRACE_LOGGER_BUFFER_SIZE, "We are going to overrun!");

                memcpy(this->m_buffer + this->m_currLength, str, length);
                this->m_currLength += length;
            }
        }

        void AppendRaw(const char16* str, uint32 length)
        {
            if(length >= TRACE_LOGGER_BUFFER_SIZE)
            {
                const char* msg = "Oversize string ... omitting from output";
                fwrite(msg, sizeof(char), strlen(msg), this->m_outfile);
            }
            else
            {
                TTDAssert(this->m_currLength + length < TRACE_LOGGER_BUFFER_SIZE, "We are going to overrun!");

                char* currs = (this->m_buffer + this->m_currLength);
                const char16* currw = str;

                for(uint32 i = 0; i < length; ++i)
                {
                    *currs = (char)(*currw);
                    ++currs;
                    ++currw;
                }

                this->m_currLength += length;
            }
        }

        template<size_t N>
        void AppendLiteral(const char(&str)[N])
        {
            this->EnsureSpace(N - 1);
            this->AppendRaw(str, N - 1);
        }

        void AppendText(char* text, uint32 length);
        void AppendText(const char16* text, uint32 length);
        void AppendIndent();
        void AppendString(char* text);
        void AppendBool(bool bval);
        void AppendInteger(int64 ival);
        void AppendUnsignedInteger(uint64 ival);
        void AppendIntegerHex(int64 ival);
        void AppendDouble(double dval);

    public:
        TraceLogger(FILE* outfile = stderr);
        ~TraceLogger();

        void ForceFlush();

        template<size_t N>
        void WriteLiteralMsg(const char(&str)[N])
        {
            this->AppendIndent();
            this->AppendLiteral(str);

            this->ForceFlush();
        }

        void WriteEnumAction(int64 eTime, BOOL returnCode, Js::PropertyId pid, Js::PropertyAttributes attrib, Js::JavascriptString* pname);

        void WriteVar(Js::Var var, bool skipStringContents=false);

        void WriteCall(Js::JavascriptFunction* function, bool isExternal, uint32 argc, Js::Var* argv, int64 etime);
        void WriteReturn(Js::JavascriptFunction* function, Js::Var res, int64 etime);
        void WriteReturnException(Js::JavascriptFunction* function, int64 etime);

        void WriteStmtIndex(uint32 line, uint32 column);

        void WriteTraceValue(Js::Var var);
    };
#endif
}

#endif
