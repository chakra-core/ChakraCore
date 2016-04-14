
//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

#if ENABLE_TTD

#define TTD_SERIALIZATION_BUFFER_SIZE 262144

namespace TTD
{
    namespace NSTokens
    {
        //Seperator tokens for records
        enum class Separator
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
            Number,
            String,
            Null,
            True,
            False,
            LBrack,
            RBrack,
            LCurly,
            RCurly,
            Comma,
            Colon
        };

        //Key values for records
        enum class Key
        {
            Invalid = 0x0,
#define ENTRY_SERIALIZE_ENUM(X) X,
#include "TTSerializeEnum.h"
            Count
        };

        LPCWSTR* InitKeyNamesArray();
    }

    ////

    //A virtual class that handles the actual write (and format) of a value to a stream
    class FileWriter
    {
    private:
        //The file that we are writing into
        HANDLE m_hfile;
        TTDWriteBytesToStreamCallback m_pfWrite;
        TTDFlushAndCloseStreamCallback m_pfClose;

        //A count of the bytes written
        uint64 m_totalBytesWritten;

        uint32 m_cursor;
        byte* m_buffer;

        //flush the buffer contents to disk
        void Flush();

    protected:
        //write a given byte or buffer of bytes to the buffer/disk as needed
        void WriteByte(byte b);
        void WriteBytes(byte* buff, uint32 bufflen);

        //buffer for writing formatted numbers into and a buffer for writing wide chars into as \uxxxx format
        wchar m_numberFormatBuff[64];
        wchar m_unicodeBuff[16];

    public:
        FileWriter(HANDLE handle, TTDWriteBytesToStreamCallback pfWrite, TTDFlushAndCloseStreamCallback pfClose);
        virtual ~FileWriter();

        void FlushAndClose();

        uint64 GetBytesWritten() const;

        //Format a value (at U64) into a temp buffer for you to use (do not free this buffer)
        LPCWSTR FormatNumber(DWORD_PTR value);

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

        virtual void WriteNakedLogTag(TTD_LOG_TAG val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
        void WriteLogTag(NSTokens::Key key, TTD_LOG_TAG val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator);

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

        virtual void WriteFileNameForSourceLocation(LPCWSTR filename, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) = 0;
    };

    //A implements the writer for JSON formatted output to a file
    class JSONWriter : public FileWriter
    {
    private:
        //Array of key names 
        LPCWSTR* m_keyNameArray;

        //indent size for formatting
        uint32 m_indentSize;

        void WriteWCHAR(wchar c);
        void WriteString_InternalNoEscape(LPCWSTR str, size_t length);
        void WriteString_Internal(LPCWSTR str, size_t length);

    public:
        JSONWriter(HANDLE handle, TTDWriteBytesToStreamCallback pfWrite, TTDFlushAndCloseStreamCallback pfClose);
        virtual ~JSONWriter();

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
        virtual void WriteNakedLogTag(TTD_LOG_TAG val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteNakedTag(uint32 tagvalue, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        ////

        virtual void WriteNakedString(const TTString& val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteNakedWellKnownToken(TTD_WELLKNOWN_TOKEN val, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;

        virtual void WriteFileNameForSourceLocation(LPCWSTR filename, NSTokens::Separator separator = NSTokens::Separator::NoSeparator) override;
    };

    //////////////////

    //A virtual class that handles the actual read of values from a stream
    class FileReader
    {
    private:
        HANDLE m_hfile;
        TTDReadBytesFromStreamCallback m_pfRead;
        TTDFlushAndCloseStreamCallback m_pfClose;

        uint64 m_totalBytesRead;

        int32 m_peekByte;

        uint32 m_cursor;
        uint32 m_buffCount;
        byte* m_buffer;

    protected:
        void Fill();

        bool Peek(byte* b);
        bool ReadByte(byte* b);

        //The action we should take if we encounter an invalid token or unexpected state in the file
        void FileReadAssert(bool ok);

        //buffer for writing formatted numbers into and a buffer for writing wide chars into as \uxxxx format
        wchar m_numberFormatBuff[64];

    public:
        FileReader(HANDLE handle, TTDReadBytesFromStreamCallback pfRead, TTDFlushAndCloseStreamCallback pfClose);
        virtual ~FileReader();

        //Format a value (at U64) into a temp buffer for you to use (do not free this buffer)
        LPCWSTR FormatNumber(DWORD_PTR value);

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

        virtual TTD_LOG_TAG ReadNakedLogTag(bool readSeparator = false) = 0;
        TTD_LOG_TAG ReadLogTag(NSTokens::Key keyCheck, bool readSeparator = false);

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

        virtual LPCWSTR ReadFileNameForSourceLocation(bool readSeparator = false) = 0;
    };

    //////////////////

    //A serialization class that reads a JSON encoded representation of the heap snapshot
    class JSONReader : public FileReader
    {
    private:
        JsUtil::List<wchar, HeapAllocator> m_charListPrimary;
        JsUtil::List<wchar, HeapAllocator> m_charListOpt;
        JsUtil::List<wchar, HeapAllocator> m_charListDiscard;

        //Array of key names 
        LPCWSTR* m_keyNameArray;

        NSTokens::ParseTokenKind Scan(JsUtil::List<wchar, HeapAllocator>& charList);
        NSTokens::ParseTokenKind ScanString(JsUtil::List<wchar, HeapAllocator>& charList);
        NSTokens::ParseTokenKind ScanSpecialNumber(JsUtil::List<wchar, HeapAllocator>& charList);
        NSTokens::ParseTokenKind ScanNumber(JsUtil::List<wchar, HeapAllocator>& charList);
        NSTokens::ParseTokenKind ScanNakedString(wchar leadChar, JsUtil::List<wchar, HeapAllocator>& charList);

        uint64 ReadHexFromCharArray(const wchar* buff);
        int64 ReadIntFromCharArray(const wchar* buff);
        uint64 ReadUIntFromCharArray(const wchar* buff);
        double ReadDoubleFromCharArray(const wchar* buff);

    public:
        JSONReader(HANDLE handle, TTDReadBytesFromStreamCallback pfRead, TTDFlushAndCloseStreamCallback pfClose);
        virtual ~JSONReader();

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
        virtual TTD_LOG_TAG ReadNakedLogTag(bool readSeparator = false) override;

        virtual uint32 ReadNakedTag(bool readSeparator = false) override;

        ////

        virtual void ReadNakedString(SlabAllocator& alloc, TTString& into, bool readSeparator = false) override;
        virtual void ReadNakedString(UnlinkableSlabAllocator& alloc, TTString& into, bool readSeparator = false) override;

        virtual TTD_WELLKNOWN_TOKEN ReadNakedWellKnownToken(SlabAllocator& alloc, bool readSeparator = false) override;
        virtual TTD_WELLKNOWN_TOKEN ReadNakedWellKnownToken(UnlinkableSlabAllocator& alloc, bool readSeparator = false) override;

        virtual LPCWSTR ReadFileNameForSourceLocation(bool readSeparator = false) override;
    };

    //////////////////

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
#define TRACE_LOGGER_BUFFER_SIZE 4096
#define TRACE_LOGGER_INDENT_BUFFER_SIZE 64

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
                AssertMsg(this->m_currLength + length < TRACE_LOGGER_BUFFER_SIZE, "We are going to overrun!");

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
                AssertMsg(this->m_currLength + length < TRACE_LOGGER_BUFFER_SIZE, "We are going to overrun!");

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
        TraceLogger(FILE* outfile = stdout);
        ~TraceLogger();

        void ForceFlush();

        template<size_t N>
        void WriteLiteralMsg(const char(&str)[N])
        {
            this->AppendIndent();
            this->AppendLiteral(str);

            this->ForceFlush();
        }

        void WriteVar(Js::Var var);

        void WriteCall(Js::JavascriptFunction* function, bool isExternal, uint32 argc, Js::Var* argv);
        void WriteReturn(Js::JavascriptFunction* function, Js::Var res);
        void WriteReturnException(Js::JavascriptFunction* function);

        void WriteStmtIndex(uint32 line, uint32 column);
    };
#endif
}

#endif
