//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#if ENABLE_TTD

namespace TTD
{
    namespace NSTokens
    {
        void InitKeyNamesArray(const char16*** names, size_t** lengths)
        {
            const char16** nameArray = TT_HEAP_ALLOC_ARRAY(const char16*, (uint32)Key::Count);
            size_t* lengthArray = TT_HEAP_ALLOC_ARRAY(size_t, (uint32)Key::Count);

#define ENTRY_SERIALIZE_ENUM(K) { nameArray[(uint32)Key::##K] = _u(#K); lengthArray[(uint32)Key::##K] = wcslen(_u(#K)); }
#include "TTSerializeEnum.h"

            *names = nameArray;
            *lengths = lengthArray;
        }

        void CleanupKeyNamesArray(const char16*** names, size_t** lengths)
        {
            if(*names != nullptr)
            {
                TT_HEAP_FREE_ARRAY(char16*, *names, (uint32)NSTokens::Key::Count);
                *names = nullptr;
            }

            if(*lengths != nullptr)
            {
                TT_HEAP_FREE_ARRAY(size_t, *lengths, (uint32)NSTokens::Key::Count);
                *lengths = nullptr;
            }
        }
    }

    //////////////////

    void FileWriter::WriteBlock(const byte* buff, size_t bufflen)
    {
        TTDAssert(bufflen != 0, "Shouldn't be writing empty blocks");
        TTDAssert(this->m_hfile != nullptr, "Trying to write to closed file.");

        size_t bwp = 0;
        this->m_pfWrite(this->m_hfile, buff, bufflen, &bwp);
    }

    FileWriter::FileWriter(JsTTDStreamHandle handle, TTDWriteBytesToStreamCallback pfWrite, TTDFlushAndCloseStreamCallback pfClose)
        : m_hfile(handle), m_pfWrite(pfWrite), m_pfClose(pfClose), m_cursor(0), m_buffer(nullptr)
    {
        this->m_buffer = TT_HEAP_ALLOC_ARRAY(byte, TTD_SERIALIZATION_BUFFER_SIZE);
    }

    FileWriter::~FileWriter()
    {
        this->FlushAndClose();
    }

    void FileWriter::FlushAndClose()
    {
        if(this->m_hfile != nullptr)
        {
            if(this->m_cursor != 0)
            {
                this->WriteBlock(this->m_buffer, this->m_cursor);
                this->m_cursor = 0;
            }

            this->m_pfClose(this->m_hfile, false, true);
            this->m_hfile = nullptr;
        }

        if(this->m_buffer != nullptr)
        {
            TT_HEAP_FREE_ARRAY(byte, this->m_buffer, TTD_SERIALIZATION_BUFFER_SIZE);
            this->m_buffer = nullptr;
        }
    }

    void FileWriter::WriteLengthValue(uint32 length, NSTokens::Separator separator)
    {
        this->WriteKey(NSTokens::Key::count, separator);
        this->WriteNakedUInt32(length);
    }

    void FileWriter::WriteSequenceStart_DefaultKey(NSTokens::Separator separator)
    {
        this->WriteKey(NSTokens::Key::values, separator);
        this->WriteSequenceStart();
    }

    void FileWriter::WriteRecordStart_DefaultKey(NSTokens::Separator separator)
    {
        this->WriteKey(NSTokens::Key::entry, separator);
        this->WriteRecordStart();
    }

    void FileWriter::WriteNull(NSTokens::Key key, NSTokens::Separator separator)
    {
        this->WriteKey(key, separator);
        this->WriteNakedNull();
    }

    void FileWriter::WriteInt32(NSTokens::Key key, int32 val, NSTokens::Separator separator)
    {
        this->WriteKey(key, separator);
        this->WriteNakedInt32(val);
    }

    void FileWriter::WriteUInt32(NSTokens::Key key, uint32 val, NSTokens::Separator separator)
    {
        this->WriteKey(key, separator);
        this->WriteNakedUInt32(val);
    }

    void FileWriter::WriteInt64(NSTokens::Key key, int64 val, NSTokens::Separator separator)
    {
        this->WriteKey(key, separator);
        this->WriteNakedInt64(val);
    }

    void FileWriter::WriteUInt64(NSTokens::Key key, uint64 val, NSTokens::Separator separator)
    {
        this->WriteKey(key, separator);
        this->WriteNakedUInt64(val);
    }

    void FileWriter::WriteDouble(NSTokens::Key key, double val, NSTokens::Separator separator)
    {
        this->WriteKey(key, separator);
        this->WriteNakedDouble(val);
    }

    void FileWriter::WriteAddr(NSTokens::Key key, TTD_PTR_ID val, NSTokens::Separator separator)
    {
        this->WriteKey(key, separator);
        this->WriteNakedAddr(val);
    }

    void FileWriter::WriteLogTag(NSTokens::Key key, TTD_LOG_PTR_ID val, NSTokens::Separator separator)
    {
        this->WriteKey(key, separator);
        this->WriteNakedLogTag(val);
    }

    ////

    void FileWriter::WriteString(NSTokens::Key key, const TTString& val, NSTokens::Separator separator)
    {
        this->WriteKey(key, separator);
        this->WriteNakedString(val);
    }

    void FileWriter::WriteWellKnownToken(NSTokens::Key key, TTD_WELLKNOWN_TOKEN val, NSTokens::Separator separator)
    {
        this->WriteKey(key, separator);
        this->WriteNakedWellKnownToken(val);
    }

    //////////////////

    TextFormatWriter::TextFormatWriter(JsTTDStreamHandle handle, TTDWriteBytesToStreamCallback pfWrite, TTDFlushAndCloseStreamCallback pfClose)
        : FileWriter(handle, pfWrite, pfClose), m_keyNameArray(nullptr), m_keyNameLengthArray(nullptr), m_indentSize(0)
    {
        byte byteOrderMarker[2] = { 0xFF, 0xFE };
        this->WriteRawByteBuff(byteOrderMarker, 2);

        NSTokens::InitKeyNamesArray(&(this->m_keyNameArray), &(this->m_keyNameLengthArray));
    }

    TextFormatWriter::~TextFormatWriter()
    {
        NSTokens::CleanupKeyNamesArray(&(this->m_keyNameArray), &(this->m_keyNameLengthArray));
    }

    void TextFormatWriter::WriteSeperator(NSTokens::Separator separator)
    {
        if((separator & NSTokens::Separator::CommaSeparator) == NSTokens::Separator::CommaSeparator)
        {
            this->WriteRawChar(_u(','));

            if((separator & NSTokens::Separator::BigSpaceSeparator) == NSTokens::Separator::BigSpaceSeparator)
            {
                this->WriteRawChar(_u('\n'));
                for(uint32 i = 0; i < this->m_indentSize; ++i)
                {
                    this->WriteRawChar(_u(' '));
                    this->WriteRawChar(_u(' '));
                }
            }
            else
            {
                this->WriteRawChar(_u(' '));
            }
        }

        if(separator == NSTokens::Separator::BigSpaceSeparator)
        {
            this->WriteRawChar(_u('\n'));
            for(uint32 i = 0; i < this->m_indentSize; ++i)
            {
                this->WriteRawChar(_u(' '));
                this->WriteRawChar(_u(' '));
            }
        }
    }

    void TextFormatWriter::WriteKey(NSTokens::Key key, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        TTDAssert(1 <= (uint32)key && (uint32)key < (uint32)NSTokens::Key::Count, "Key not in valid range!");
        const char16* kname = this->m_keyNameArray[(uint32)key];
        size_t ksize = this->m_keyNameLengthArray[(uint32)key];

        this->WriteRawCharBuff(kname, ksize);
        this->WriteRawChar(_u(':'));
    }

    void TextFormatWriter::WriteSequenceStart(NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawChar(_u('['));
    }

    void TextFormatWriter::WriteSequenceEnd(NSTokens::Separator separator)
    {
        TTDAssert(separator == NSTokens::Separator::NoSeparator || separator == NSTokens::Separator::BigSpaceSeparator, "Shouldn't be anything else!!!");

        this->WriteSeperator(separator);
        this->WriteRawChar(_u(']'));
    }

    void TextFormatWriter::WriteRecordStart(NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawChar(_u('{'));
    }

    void TextFormatWriter::WriteRecordEnd(NSTokens::Separator separator)
    {
        TTDAssert(separator == NSTokens::Separator::NoSeparator || separator == NSTokens::Separator::BigSpaceSeparator, "Shouldn't be anything else!!!");

        this->WriteSeperator(separator);
        this->WriteRawChar(_u('}'));
    }

    void TextFormatWriter::AdjustIndent(int32 delta)
    {
        this->m_indentSize += delta;
    }

    void TextFormatWriter::SetIndent(uint32 depth)
    {
        this->m_indentSize = depth;
    }

    void TextFormatWriter::WriteNakedNull(NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        this->WriteRawCharBuff(_u("null"), 4);
    }

    void TextFormatWriter::WriteNakedByte(byte val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteFormattedCharData(_u("%I32u"), (uint32)val);
    }

    void TextFormatWriter::WriteBool(NSTokens::Key key, bool val, NSTokens::Separator separator)
    {
        this->WriteKey(key, separator);
        if(val)
        {
            this->WriteRawCharBuff(_u("true"), 4);
        }
        else
        {
            this->WriteRawCharBuff(_u("false"), 5);
        }
    }

    void TextFormatWriter::WriteNakedInt32(int32 val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteFormattedCharData(_u("%I32i"), val);
    }

    void TextFormatWriter::WriteNakedUInt32(uint32 val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteFormattedCharData(_u("%I32u"), val);
    }

    void TextFormatWriter::WriteNakedInt64(int64 val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteFormattedCharData(_u("%I64i"), val);
    }

    void TextFormatWriter::WriteNakedUInt64(uint64 val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteFormattedCharData(_u("%I64u"), val);
    }

    void TextFormatWriter::WriteNakedDouble(double val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        if(Js::JavascriptNumber::IsNan(val))
        {
            this->WriteRawCharBuff(_u("#nan"), 4);
        }
        else if(Js::JavascriptNumber::IsPosInf(val))
        {
            this->WriteRawCharBuff(_u("#+inf"), 5);
        }
        else if(Js::JavascriptNumber::IsNegInf(val))
        {
            this->WriteRawCharBuff(_u("#-inf"), 5);
        }
        else if(Js::JavascriptNumber::MAX_VALUE == val)
        {
            this->WriteRawCharBuff(_u("#ub"), 3);
        }
        else if(Js::JavascriptNumber::MIN_VALUE == val)
        {
            this->WriteRawCharBuff(_u("#lb"), 3);
        }
        else if(Js::Math::EPSILON == val)
        {
            this->WriteRawCharBuff(_u("#ep"), 3);
        }
        else
        {
            if(INT32_MAX <= val && val <= INT32_MAX && floor(val) == val)
            {
                this->WriteFormattedCharData(_u("%I64i"), (int64)val);
            }
            else
            {
                //
                //TODO: this is nice for visual debugging but we inherently lose precision
                //      will want to change this to a dump of the bit representation of the number
                //

                this->WriteFormattedCharData(_u("%.32f"), val);
            }
        }
    }

    void TextFormatWriter::WriteNakedAddr(TTD_PTR_ID val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteFormattedCharData(_u("*%I64u"), val);
    }

    void TextFormatWriter::WriteNakedLogTag(TTD_LOG_PTR_ID val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteFormattedCharData(_u("!%I64i"), val);
    }

    void TextFormatWriter::WriteNakedTag(uint32 tagvalue, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteFormattedCharData(_u("$%I32i"), tagvalue);
    }

    ////

    void TextFormatWriter::WriteNakedString(const TTString& val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        if(IsNullPtrTTString(val))
        {
            this->WriteNakedNull();
        }
        else
        {
            this->WriteFormattedCharData(_u("@%I32u"), val.Length);

            this->WriteRawChar(_u('\"'));
            this->WriteRawCharBuff(val.Contents, val.Length);
            this->WriteRawChar(_u('\"'));
        }
    }

    void TextFormatWriter::WriteNakedWellKnownToken(TTD_WELLKNOWN_TOKEN val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        this->WriteRawChar(_u('~'));
        this->WriteRawCharBuff(val, wcslen(val));
        this->WriteRawChar(_u('~'));
    }

    void TextFormatWriter::WriteInlineCode(_In_reads_(length) const char16* code, uint32 length, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        this->WriteFormattedCharData(_u("@%I32u"), length);

        this->WriteRawChar(_u('\"'));
        this->WriteRawCharBuff(code, length);
        this->WriteRawChar(_u('\"'));
    }

    void TextFormatWriter::WriteInlinePropertyRecordName(_In_reads_(length) const char16* pname, uint32 length, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        this->WriteFormattedCharData(_u("@%I32u"), length);

        this->WriteRawChar(_u('\"'));
        this->WriteRawCharBuff(pname, length);
        this->WriteRawChar(_u('\"'));
    }

    BinaryFormatWriter::BinaryFormatWriter(JsTTDStreamHandle handle, TTDWriteBytesToStreamCallback pfWrite, TTDFlushAndCloseStreamCallback pfClose)
        : FileWriter(handle, pfWrite, pfClose)
    {
        ;
    }

    BinaryFormatWriter::~BinaryFormatWriter()
    {
        ;
    }

    void BinaryFormatWriter::WriteSeperator(NSTokens::Separator separator)
    {
        if((separator & NSTokens::Separator::CommaSeparator) == NSTokens::Separator::CommaSeparator)
        {
            this->WriteRawByteBuff_Fixed<byte>((byte)NSTokens::Separator::CommaSeparator);
        }
    }

    void BinaryFormatWriter::WriteKey(NSTokens::Key key, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<byte>((byte)key);
    }

    void BinaryFormatWriter::WriteSequenceStart(NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<byte>('[');
    }

    void BinaryFormatWriter::WriteSequenceEnd(NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<byte>(']');
    }

    void BinaryFormatWriter::WriteRecordStart(NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<byte>('{');
    }

    void BinaryFormatWriter::WriteRecordEnd(NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<byte>('}');
    }

    void BinaryFormatWriter::AdjustIndent(int32 delta)
    {
        ;
    }

    void BinaryFormatWriter::SetIndent(uint32 depth)
    {
        ;
    }

    void BinaryFormatWriter::WriteNakedNull(NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<byte>((byte)0);
    }

    void BinaryFormatWriter::WriteNakedByte(byte val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<byte>(val);
    }

    void BinaryFormatWriter::WriteBool(NSTokens::Key key, bool val, NSTokens::Separator separator)
    {
        this->WriteKey(key, separator);
        this->WriteRawByteBuff_Fixed<byte>(val ? (byte)1 : (byte)0);
    }

    void BinaryFormatWriter::WriteNakedInt32(int32 val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<int32>(val);
    }

    void BinaryFormatWriter::WriteNakedUInt32(uint32 val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<uint32>(val);
    }

    void BinaryFormatWriter::WriteNakedInt64(int64 val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<int64>(val);
    }

    void BinaryFormatWriter::WriteNakedUInt64(uint64 val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<uint64>(val);
    }

    void BinaryFormatWriter::WriteNakedDouble(double val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<double>(val);
    }

    void BinaryFormatWriter::WriteNakedAddr(TTD_PTR_ID val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<TTD_PTR_ID>(val);
    }

    void BinaryFormatWriter::WriteNakedLogTag(TTD_LOG_PTR_ID val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<TTD_LOG_PTR_ID>(val);
    }

    void BinaryFormatWriter::WriteNakedTag(uint32 tagvalue, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteRawByteBuff_Fixed<uint32>(tagvalue);
    }

    void BinaryFormatWriter::WriteNakedString(const TTString& val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        if(IsNullPtrTTString(val))
        {
            this->WriteRawByteBuff_Fixed<uint32>(UINT32_MAX);
        }
        else
        {
            this->WriteRawByteBuff_Fixed<uint32>(val.Length);
            this->WriteRawByteBuff((const byte*)val.Contents, val.Length * sizeof(char16));
        }
    }

    void BinaryFormatWriter::WriteNakedWellKnownToken(TTD_WELLKNOWN_TOKEN val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        uint32 charLen = (uint32)wcslen(val);
        this->WriteRawByteBuff_Fixed<uint32>(charLen);
        this->WriteRawByteBuff((const byte*)val, charLen * sizeof(char16));
    }

    void BinaryFormatWriter::WriteInlineCode(_In_reads_(length) const char16* code, uint32 length, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        this->WriteRawByteBuff_Fixed<uint32>(length);
        this->WriteRawByteBuff((const byte*)code, length * sizeof(char16));
    }

    void BinaryFormatWriter::WriteInlinePropertyRecordName(_In_reads_(length) const char16* pname, uint32 length, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        this->WriteRawByteBuff_Fixed<uint32>(length);
        this->WriteRawByteBuff((const byte*)pname, length * sizeof(char16));
    }

    //////////////////

    void FileReader::ReadBlock(byte* buff, size_t* readSize)
    {
        TTDAssert(this->m_hfile != nullptr, "Trying to read a invalid file.");

        size_t bwp = 0;
        this->m_pfRead(this->m_hfile, buff, TTD_SERIALIZATION_BUFFER_SIZE, &bwp);

        *readSize = (size_t)bwp;
    }

    FileReader::FileReader(JsTTDStreamHandle handle, TTDReadBytesFromStreamCallback pfRead, TTDFlushAndCloseStreamCallback pfClose)
        : m_hfile(handle), m_pfRead(pfRead), m_pfClose(pfClose), m_peekChar(-1), m_cursor(0), m_buffCount(0), m_buffer(nullptr)
    {
        this->m_buffer = TT_HEAP_ALLOC_ARRAY(byte, TTD_SERIALIZATION_BUFFER_SIZE);
    }

    FileReader::~FileReader()
    {
        if(this->m_hfile != nullptr)
        {
            this->m_pfClose(this->m_hfile, true, false);
            this->m_hfile = nullptr;
        }

        if(this->m_buffer != nullptr)
        {
            TT_HEAP_FREE_ARRAY(byte, this->m_buffer, TTD_SERIALIZATION_BUFFER_SIZE);
            this->m_buffer = nullptr;
        }
    }

    uint32 FileReader::ReadLengthValue(bool readSeparator)
    {
        this->ReadKey(NSTokens::Key::count, readSeparator);
        return this->ReadNakedUInt32();
    }

    void FileReader::ReadSequenceStart_WDefaultKey(bool readSeparator)
    {
        this->ReadKey(NSTokens::Key::values, readSeparator);
        this->ReadSequenceStart();
    }

    void FileReader::ReadRecordStart_WDefaultKey(bool readSeparator)
    {
        this->ReadKey(NSTokens::Key::entry, readSeparator);
        this->ReadRecordStart();
    }

    void FileReader::ReadNull(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadKey(keyCheck, readSeparator);
        this->ReadNakedNull();
    }

    int32 FileReader::ReadInt32(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadKey(keyCheck, readSeparator);
        return this->ReadNakedInt32();
    }

    uint32 FileReader::ReadUInt32(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadKey(keyCheck, readSeparator);
        return this->ReadNakedUInt32();
    }

    int64 FileReader::ReadInt64(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadKey(keyCheck, readSeparator);
        return this->ReadNakedInt64();
    }

    uint64 FileReader::ReadUInt64(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadKey(keyCheck, readSeparator);
        return this->ReadNakedUInt64();
    }

    double FileReader::ReadDouble(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadKey(keyCheck, readSeparator);
        return this->ReadNakedDouble();
    }

    TTD_PTR_ID FileReader::ReadAddr(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadKey(keyCheck, readSeparator);
        return this->ReadNakedAddr();
    }

    TTD_LOG_PTR_ID FileReader::ReadLogTag(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadKey(keyCheck, readSeparator);
        return this->ReadNakedLogTag();
    }

    //////////////////

    NSTokens::ParseTokenKind TextFormatReader::Scan(JsUtil::List<char16, HeapAllocator>& charList)
    {
        char16 c = _u('\0');
        charList.Clear();

        while(this->ReadRawChar(&c))
        {
            switch(c)
            {
            case 0:
                return NSTokens::ParseTokenKind::Error; //we shouldn't hit EOF explicitly here
            case _u('\t'):
            case _u('\r'):
            case _u('\n'):
            case _u(' '):
                //WS - keep looping
                break;
            case _u(','):
                return NSTokens::ParseTokenKind::Comma;
            case _u(':'):
                return NSTokens::ParseTokenKind::Colon;
            case _u('['):
                return NSTokens::ParseTokenKind::LBrack;
            case _u(']'):
                return NSTokens::ParseTokenKind::RBrack;
            case _u('{'):
                return NSTokens::ParseTokenKind::LCurly;
            case _u('}'):
                return NSTokens::ParseTokenKind::RCurly;
            case _u('#'):
                //# starts special double/number value representation
                return this->ScanSpecialNumber();
            case _u('-'):
            case _u('+'):
            case _u('0'):
            case _u('1'):
            case _u('2'):
            case _u('3'):
            case _u('4'):
            case _u('5'):
            case _u('6'):
            case _u('7'):
            case _u('8'):
            case _u('9'):
                //decimal digit or (-,+) starts a number
                charList.Add(c);
                return this->ScanNumber(charList);
            case _u('*'):
                //address
                return this->ScanAddress(charList);
            case _u('!'):
                //log tag
                return this->ScanLogTag(charList);
            case _u('$'):
                //enumeration value tag
                return this->ScanEnumTag(charList);
            case _u('~'):
                //wellknown token
                return this->ScanWellKnownToken(charList);
            case _u('@'):
                //string
                return this->ScanString(charList);
            default:
                //it is a naked literal value (or an error)
                return this->ScanNakedString(c);
            }
        }

        return NSTokens::ParseTokenKind::Error;
    }

    NSTokens::ParseTokenKind TextFormatReader::ScanKey(JsUtil::List<char16, HeapAllocator>& charList)
    {
        charList.Clear();

        char16 c = _u('\0');
        bool endFound = false;

        //Read off any whitespace
        while(this->PeekRawChar(&c))
        {
            if((c != _u('\t')) & (c != _u('\r')) & (c != _u('\n')) & (c != _u(' ')))
            {
                break;
            }

            this->ReadRawChar(&c);
        }

        while(this->PeekRawChar(&c))
        {
            if(c == 0 || charList.Count() > 256)
            {
                //we reached the end of the file or the "key" is much longer than it should be
                return NSTokens::ParseTokenKind::Error;
            }

            if(c == _u(':'))
            {
                //end of the string
                endFound = true;
                break;
            }
            else
            {
                this->ReadRawChar(&c);
                charList.Add(c);
            }
        }

        if(!endFound)
        {
            // no ending found 
            return NSTokens::ParseTokenKind::Error;
        }

        return NSTokens::ParseTokenKind::String;
    }

    NSTokens::ParseTokenKind TextFormatReader::ScanSpecialNumber()
    {
        char16 c = _u('\0');
        bool ok = this->ReadRawChar(&c);

        if(ok && c == _u('n'))
        {
            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('a'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('n'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            return NSTokens::ParseTokenKind::NaN;
        }
        else if(ok && (c == _u('+') || c == _u('-')))
        {
            char16 signc = c;

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('i'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('n'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('f'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            return (signc == _u('+')) ? NSTokens::ParseTokenKind::PosInfty : NSTokens::ParseTokenKind::NegInfty;
        }
        else if(ok && (c == _u('u') || c == _u('l')))
        {
            char16 limitc = c;

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('b'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            return (limitc == _u('u')) ? NSTokens::ParseTokenKind::UpperBound : NSTokens::ParseTokenKind::LowerBound;
        }
        else if(ok && c == _u('e'))
        {
            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('p'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            return NSTokens::ParseTokenKind::Epsilon;
        }
        else
        {
            return NSTokens::ParseTokenKind::Error;
        }
    }

    NSTokens::ParseTokenKind TextFormatReader::ScanNumber(JsUtil::List<char16, HeapAllocator>& charList)
    {
        char16 c = _u('\0');
        while(this->PeekRawChar(&c) && ((_u('0') <= c && c <= _u('9')) || (c == _u('.'))))
        {
            this->ReadRawChar(&c);
            charList.Add(c);
        }

        // Null-terminate the list before we try to use the buffer as a string.
        charList.Add(_u('\0'));

        bool likelyint; //we don't care about this just want to know that it is convertable to a number
        const char16* end;
        const char16* start = charList.GetBuffer();
        double val = Js::NumberUtilities::StrToDbl<char16>(start, &end, likelyint);
        if(start == end)
        {
            return NSTokens::ParseTokenKind::Error;
        }
        TTDAssert(!Js::JavascriptNumber::IsNan(val), "Bad result from string to double conversion");

        return NSTokens::ParseTokenKind::Number;
    }

    NSTokens::ParseTokenKind TextFormatReader::ScanAddress(JsUtil::List<char16, HeapAllocator>& charList)
    {
        NSTokens::ParseTokenKind okNumber = this->ScanNumber(charList);
        if(okNumber != NSTokens::ParseTokenKind::Number)
        {
            return NSTokens::ParseTokenKind::Error;
        }

        return NSTokens::ParseTokenKind::Address;
    }

    NSTokens::ParseTokenKind TextFormatReader::ScanLogTag(JsUtil::List<char16, HeapAllocator>& charList)
    {
        NSTokens::ParseTokenKind okNumber = this->ScanNumber(charList);
        if(okNumber != NSTokens::ParseTokenKind::Number)
        {
            return NSTokens::ParseTokenKind::Error;
        }

        return NSTokens::ParseTokenKind::LogTag;
    }

    NSTokens::ParseTokenKind TextFormatReader::ScanEnumTag(JsUtil::List<char16, HeapAllocator>& charList)
    {
        NSTokens::ParseTokenKind okNumber = this->ScanNumber(charList);
        if(okNumber != NSTokens::ParseTokenKind::Number)
        {
            return NSTokens::ParseTokenKind::Error;
        }

        return NSTokens::ParseTokenKind::EnumTag;
    }

    NSTokens::ParseTokenKind TextFormatReader::ScanWellKnownToken(JsUtil::List<char16, HeapAllocator>& charList)
    {
        char16 c = _u('\0');
        bool endFound = false;

        while(this->ReadRawChar(&c))
        {
            if(c == 0)
            {
                return NSTokens::ParseTokenKind::Error;
            }

            if(c == _u('~'))
            {
                //end of the string
                endFound = true;
                break;
            }
            else
            {
                charList.Add(c);
            }
        }

        if(!endFound)
        {
            // no ending found 
            return NSTokens::ParseTokenKind::Error;
        }

        return NSTokens::ParseTokenKind::WellKnownToken;
    }

    NSTokens::ParseTokenKind TextFormatReader::ScanString(JsUtil::List<char16, HeapAllocator>& charList)
    {
        bool ok = false;
        char16 c = _u('\0');

        //first we should find a number
        NSTokens::ParseTokenKind okNumber = this->ScanNumber(charList);
        if(okNumber != NSTokens::ParseTokenKind::Number)
        {
            return NSTokens::ParseTokenKind::Error;
        }

        // Convert this number to get the length of the string (not including ""),
        // charList is already null-terminated by the call to ScanNumber.
        uint32 length = (uint32)this->ReadUIntFromCharArray(charList.GetBuffer());

        //read the lead "\""
        ok = this->ReadRawChar(&c);
        if(!ok || c != _u('\"'))
        {
            return NSTokens::ParseTokenKind::Error;
        }

        //read that many chars and check for the terminating "\""
        charList.Clear();
        for(uint32 i = 0; i < length; ++i)
        {
            ok = this->ReadRawChar(&c);
            if(!ok)
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add(c);
        }

        ok = this->ReadRawChar(&c);
        if(!ok || c != _u('\"'))
        {
            return NSTokens::ParseTokenKind::Error;
        }

        return NSTokens::ParseTokenKind::String;
    }

    NSTokens::ParseTokenKind TextFormatReader::ScanNakedString(char16 leadChar)
    {
        bool ok = false;
        char16 c = _u('\0');

        if(leadChar == _u('n'))
        {
            //check for "null"

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('u'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('l'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('l'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            return NSTokens::ParseTokenKind::Null;
        }
        else if(leadChar == _u('t'))
        {
            //check for "true"

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('r'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('u'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('e'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            return NSTokens::ParseTokenKind::True;
        }
        else if(leadChar == _u('f'))
        {
            //check for "false"

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('a'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('l'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('s'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            ok = this->ReadRawChar(&c);
            if(!ok || c != _u('e'))
            {
                return NSTokens::ParseTokenKind::Error;
            }

            return NSTokens::ParseTokenKind::False;
        }
        else
        {
            return NSTokens::ParseTokenKind::Error;
        }
    }

    int64 TextFormatReader::ReadIntFromCharArray(const char16* buff)
    {
        int64 value = 0;
        int64 multiplier = 1;

        int64 sign = 1;
        int32 lastIdx = 0;
        if(buff[0] == _u('-'))
        {
            sign = -1;
            lastIdx = 1;
        }

        int32 digitCount = (int32)wcslen(buff);
        for(int32 i = digitCount - 1; i >= lastIdx; --i)
        {
            char16 digit = buff[i];
            uint32 digitValue = (digit - _u('0'));

            value += (multiplier * digitValue);
            multiplier *= 10;
        }

        return value * sign;
    }

    uint64 TextFormatReader::ReadUIntFromCharArray(const char16* buff)
    {
        uint64 value = 0;
        uint64 multiplier = 1;

        int32 digitCount = (int32)wcslen(buff);
        for(int32 i = digitCount - 1; i >= 0; --i)
        {
            char16 digit = buff[i];
            uint32 digitValue = (digit - _u('0'));

            value += (multiplier * digitValue);
            multiplier *= 10;
        }

        return value;
    }

    double TextFormatReader::ReadDoubleFromCharArray(const char16* buff)
    {
        bool likelytInt; //we don't care about this as we already know it is a double
        const char16* end;
        double val = Js::NumberUtilities::StrToDbl<char16>(buff, &end, likelytInt);
        TTDAssert((buff != end) && !Js::JavascriptNumber::IsNan(val), "Error in parse.");

        return val;
    }

    TextFormatReader::TextFormatReader(JsTTDStreamHandle handle, TTDReadBytesFromStreamCallback pfRead, TTDFlushAndCloseStreamCallback pfClose)
        : FileReader(handle, pfRead, pfClose), m_charListPrimary(&HeapAllocator::Instance), m_charListOpt(&HeapAllocator::Instance), m_charListDiscard(&HeapAllocator::Instance), m_keyNameArray(nullptr), m_keyNameLengthArray(nullptr)
    {
        byte byteOrderMarker[2] = { 0x0, 0x0 };
        this->ReadBytesInto(byteOrderMarker, 2);
        TTDAssert(byteOrderMarker[0] == 0xFF && byteOrderMarker[1] == 0xFE, "Byte Order Marker is incorrect!");

        NSTokens::InitKeyNamesArray(&(this->m_keyNameArray), &(this->m_keyNameLengthArray));
    }

    TextFormatReader::~TextFormatReader()
    {
        NSTokens::CleanupKeyNamesArray(&(this->m_keyNameArray), &(this->m_keyNameLengthArray));
    }

    void TextFormatReader::ReadSeperator(bool readSeparator)
    {
        if(readSeparator)
        {
            NSTokens::ParseTokenKind tok = this->Scan(this->m_charListDiscard);
            TTDAssert(tok == NSTokens::ParseTokenKind::Comma, "Error in parse.");
        }
    }

    void TextFormatReader::ReadKey(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        //We do a special scan here for a key (instead of the more general scan we call elsewhere)
        NSTokens::ParseTokenKind tok = this->ScanKey(this->m_charListPrimary);
        TTDAssert(tok == NSTokens::ParseTokenKind::String, "Error in parse.");

        this->m_charListPrimary.Add(_u('\0'));
        const char16* keystr = this->m_charListPrimary.GetBuffer();

        //check key strings are the same
        TTDAssert(1 <= (uint32)keyCheck && (uint32)keyCheck < (uint32)NSTokens::Key::Count, "Error in parse.");
        const char16* kname = this->m_keyNameArray[(uint32)keyCheck];
        TTDAssert(kname != nullptr, "Error in parse.");
        TTDAssert(wcscmp(keystr, kname) == 0, "Error in parse.");

        NSTokens::ParseTokenKind toksep = this->Scan(this->m_charListDiscard);
        TTDAssert(toksep == NSTokens::ParseTokenKind::Colon, "Error in parse.");
    }

    void TextFormatReader::ReadSequenceStart(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListDiscard);
        TTDAssert(tok == NSTokens::ParseTokenKind::LBrack, "Error in parse.");
    }

    void TextFormatReader::ReadSequenceEnd()
    {
        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListDiscard);
        TTDAssert(tok == NSTokens::ParseTokenKind::RBrack, "Error in parse.");
    }

    void TextFormatReader::ReadRecordStart(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListDiscard);
        TTDAssert(tok == NSTokens::ParseTokenKind::LCurly, "Error in parse.");
    }

    void TextFormatReader::ReadRecordEnd()
    {
        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListDiscard);
        TTDAssert(tok == NSTokens::ParseTokenKind::RCurly, "Error in parse.");
    }

    void TextFormatReader::ReadNakedNull(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListDiscard);
        TTDAssert(tok == NSTokens::ParseTokenKind::Null, "Error in parse.");
    }

    byte TextFormatReader::ReadNakedByte(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        TTDAssert(tok == NSTokens::ParseTokenKind::Number, "Error in parse.");

        uint64 uval = this->ReadUIntFromCharArray(this->m_charListOpt.GetBuffer());
        TTDAssert(uval <= BYTE_MAX, "Error in parse.");

        return (byte)uval;
    }

    bool TextFormatReader::ReadBool(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadKey(keyCheck, readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        TTDAssert(tok == NSTokens::ParseTokenKind::True || tok == NSTokens::ParseTokenKind::False, "Error in parse.");

        return (tok == NSTokens::ParseTokenKind::True);
    }

    int32 TextFormatReader::ReadNakedInt32(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        TTDAssert(tok == NSTokens::ParseTokenKind::Number, "Error in parse.");

        int64 ival = this->ReadIntFromCharArray(this->m_charListOpt.GetBuffer());
        TTDAssert(INT32_MIN <= ival && ival <= INT32_MAX, "Error in parse.");

        return (int32)ival;
    }

    uint32 TextFormatReader::ReadNakedUInt32(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        TTDAssert(tok == NSTokens::ParseTokenKind::Number, "Error in parse.");

        uint64 uval = this->ReadUIntFromCharArray(this->m_charListOpt.GetBuffer());
        TTDAssert(uval <= UINT32_MAX, "Error in parse.");

        return (uint32)uval;
    }

    int64 TextFormatReader::ReadNakedInt64(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        TTDAssert(tok == NSTokens::ParseTokenKind::Number, "Error in parse.");

        return this->ReadIntFromCharArray(this->m_charListOpt.GetBuffer());
    }

    uint64 TextFormatReader::ReadNakedUInt64(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        TTDAssert(tok == NSTokens::ParseTokenKind::Number, "Error in parse.");

        return this->ReadUIntFromCharArray(this->m_charListOpt.GetBuffer());
    }

    double TextFormatReader::ReadNakedDouble(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);

        double res = -1.0;
        switch(tok)
        {
        case TTD::NSTokens::ParseTokenKind::NaN:
            res = Js::JavascriptNumber::NaN;
            break;
        case TTD::NSTokens::ParseTokenKind::PosInfty:
            res = Js::JavascriptNumber::POSITIVE_INFINITY;
            break;
        case TTD::NSTokens::ParseTokenKind::NegInfty:
            res = Js::JavascriptNumber::NEGATIVE_INFINITY;
            break;
        case TTD::NSTokens::ParseTokenKind::UpperBound:
            res = Js::JavascriptNumber::MAX_VALUE;
            break;
        case TTD::NSTokens::ParseTokenKind::LowerBound:
            res = Js::JavascriptNumber::MIN_VALUE;
            break;
        case TTD::NSTokens::ParseTokenKind::Epsilon:
            res = Js::Math::EPSILON;
            break;
        default:
        {
            TTDAssert(tok == NSTokens::ParseTokenKind::Number, "Error in parse.");

            res = this->ReadDoubleFromCharArray(this->m_charListOpt.GetBuffer());

            break;
        }
        }

        return res;
    }

    TTD_PTR_ID TextFormatReader::ReadNakedAddr(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        TTDAssert(tok == NSTokens::ParseTokenKind::Address, "Error in parse.");

        return (TTD_PTR_ID)this->ReadUIntFromCharArray(this->m_charListOpt.GetBuffer());
    }

    TTD_LOG_PTR_ID TextFormatReader::ReadNakedLogTag(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        TTDAssert(tok == NSTokens::ParseTokenKind::LogTag, "Error in parse.");

        return (TTD_LOG_PTR_ID)this->ReadUIntFromCharArray(this->m_charListOpt.GetBuffer());
    }

    uint32 TextFormatReader::ReadNakedTag(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        TTDAssert(tok == NSTokens::ParseTokenKind::EnumTag, "Error in parse.");

        uint64 tval = this->ReadUIntFromCharArray(this->m_charListOpt.GetBuffer());
        TTDAssert(tval <= UINT32_MAX, "Error in parse.");

        return (uint32)tval;
    }

    ////

    void TextFormatReader::ReadNakedString(SlabAllocator& alloc, TTString& into, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        TTDAssert(tok == NSTokens::ParseTokenKind::String || tok == NSTokens::ParseTokenKind::Null, "Error in parse.");

        if(tok == NSTokens::ParseTokenKind::Null)
        {
            alloc.CopyNullTermStringInto(nullptr, into);
        }
        else
        {
            alloc.CopyStringIntoWLength(this->m_charListOpt.GetBuffer(), this->m_charListOpt.Count(), into);
        }
    }

    void TextFormatReader::ReadNakedString(UnlinkableSlabAllocator& alloc, TTString& into, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        TTDAssert(tok == NSTokens::ParseTokenKind::String || tok == NSTokens::ParseTokenKind::Null, "Error in parse.");

        if(tok == NSTokens::ParseTokenKind::Null)
        {
            alloc.CopyNullTermStringInto(nullptr, into);
        }
        else
        {
            alloc.CopyStringIntoWLength(this->m_charListOpt.GetBuffer(), this->m_charListOpt.Count(), into);
        }
    }

    TTD_WELLKNOWN_TOKEN TextFormatReader::ReadNakedWellKnownToken(SlabAllocator& alloc, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        TTDAssert(tok == NSTokens::ParseTokenKind::WellKnownToken, "Error in parse.");

        this->m_charListOpt.Add(_u('\0')); //add null terminator
        return alloc.CopyRawNullTerminatedStringInto(this->m_charListOpt.GetBuffer());
    }

    TTD_WELLKNOWN_TOKEN TextFormatReader::ReadNakedWellKnownToken(UnlinkableSlabAllocator& alloc, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        TTDAssert(tok == NSTokens::ParseTokenKind::WellKnownToken, "Error in parse.");

        this->m_charListOpt.Add(_u('\0')); //add null terminator
        return alloc.CopyRawNullTerminatedStringInto(this->m_charListOpt.GetBuffer() + 1);
    }

    void TextFormatReader::ReadInlineCode(_Out_writes_(length) char16* code, uint32 length, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        TTDAssert(tok == NSTokens::ParseTokenKind::String, "Error in parse.");

        js_memcpy_s(code, length * sizeof(char16), this->m_charListOpt.GetBuffer(), this->m_charListOpt.Count() * sizeof(char16));
    }

    BinaryFormatReader::BinaryFormatReader(JsTTDStreamHandle handle, TTDReadBytesFromStreamCallback pfRead, TTDFlushAndCloseStreamCallback pfClose)
        : FileReader(handle, pfRead, pfClose)
    {
        ;
    }

    BinaryFormatReader::~BinaryFormatReader()
    {
        ;
    }

    void BinaryFormatReader::ReadSeperator(bool readSeparator)
    {
        if(readSeparator)
        {
            byte sep = 0;
            this->ReadBytesInto_Fixed<byte>(sep);

            TTDAssert((NSTokens::Separator)sep == NSTokens::Separator::CommaSeparator, "Error in parse.");
        }
    }

    void BinaryFormatReader::ReadKey(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        byte key = 0;
        this->ReadBytesInto_Fixed<byte>(key);

        TTDAssert((NSTokens::Key)key == keyCheck, "Error in parse.");
    }

    void BinaryFormatReader::ReadSequenceStart(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        byte tok = 0;
        this->ReadBytesInto_Fixed<byte>(tok);

        TTDAssert(tok == '[', "Error in parse.");
    }

    void BinaryFormatReader::ReadSequenceEnd()
    {
        byte tok = 0;
        this->ReadBytesInto_Fixed<byte>(tok);

        TTDAssert(tok == ']', "Error in parse.");
    }

    void BinaryFormatReader::ReadRecordStart(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        byte tok = 0;
        this->ReadBytesInto_Fixed<byte>(tok);

        TTDAssert(tok == '{', "Error in parse.");
    }

    void BinaryFormatReader::ReadRecordEnd()
    {
        byte tok = 0;
        this->ReadBytesInto_Fixed<byte>(tok);

        TTDAssert(tok == '}', "Error in parse.");
    }

    void BinaryFormatReader::ReadNakedNull(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        byte tok = 0;
        this->ReadBytesInto_Fixed<byte>(tok);

        TTDAssert(tok == 0, "Error in parse.");
    }

    byte BinaryFormatReader::ReadNakedByte(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        byte b = 0;
        this->ReadBytesInto_Fixed<byte>(b);

        return b;
    }

    bool BinaryFormatReader::ReadBool(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadKey(keyCheck, readSeparator);

        byte b = 0;
        this->ReadBytesInto_Fixed<byte>(b);

        return !!b;
    }

    int32 BinaryFormatReader::ReadNakedInt32(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        int32 i = 0;
        this->ReadBytesInto_Fixed<int32>(i);

        return i;
    }

    uint32 BinaryFormatReader::ReadNakedUInt32(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        uint32 i = 0;
        this->ReadBytesInto_Fixed<uint32>(i);

        return i;
    }

    int64 BinaryFormatReader::ReadNakedInt64(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        int64 i = 0;
        this->ReadBytesInto_Fixed<int64>(i);

        return i;
    }

    uint64 BinaryFormatReader::ReadNakedUInt64(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        uint64 i = 0;
        this->ReadBytesInto_Fixed<uint64>(i);

        return i;
    }

    double BinaryFormatReader::ReadNakedDouble(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        double d = 0.0;
        this->ReadBytesInto_Fixed<double>(d);

        return d;
    }

    TTD_PTR_ID BinaryFormatReader::ReadNakedAddr(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        TTD_PTR_ID addr = TTD_INVALID_PTR_ID;
        this->ReadBytesInto_Fixed<TTD_PTR_ID>(addr);

        return addr;
    }

    TTD_LOG_PTR_ID BinaryFormatReader::ReadNakedLogTag(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        TTD_LOG_PTR_ID tag = TTD_INVALID_LOG_PTR_ID;
        this->ReadBytesInto_Fixed<TTD_LOG_PTR_ID>(tag);

        return tag;
    }

    uint32 BinaryFormatReader::ReadNakedTag(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        uint32 tag = 0;
        this->ReadBytesInto_Fixed<uint32>(tag);

        return tag;
    }

    void BinaryFormatReader::ReadNakedString(SlabAllocator& alloc, TTString& into, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        uint32 sizeField = 0;
        this->ReadBytesInto_Fixed<uint32>(sizeField);

        if(sizeField == UINT32_MAX)
        {
            alloc.CopyNullTermStringInto(nullptr, into);
        }
        else
        {
            alloc.InitializeAndAllocateWLength(sizeField, into);
            this->ReadBytesInto((byte*)into.Contents, into.Length * sizeof(char16));
            into.Contents[into.Length] = '\0';
        }
    }

    void BinaryFormatReader::ReadNakedString(UnlinkableSlabAllocator& alloc, TTString& into, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        uint32 sizeField = 0;
        this->ReadBytesInto_Fixed<uint32>(sizeField);

        if(sizeField == UINT32_MAX)
        {
            alloc.CopyNullTermStringInto(nullptr, into);
        }
        else
        {
            alloc.InitializeAndAllocateWLength(sizeField, into);
            this->ReadBytesInto((byte*)into.Contents, into.Length * sizeof(char16));
            into.Contents[into.Length] = '\0';
        }
    }

    TTD_WELLKNOWN_TOKEN BinaryFormatReader::ReadNakedWellKnownToken(SlabAllocator& alloc, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        uint32 charLen = 0;
        this->ReadBytesInto_Fixed<uint32>(charLen);

        char16* cbuff = alloc.SlabAllocateArray<char16>(charLen + 1);
        this->ReadBytesInto((byte*)cbuff, charLen * sizeof(char16));
        cbuff[charLen] = _u('\0');

        return cbuff;
    }

    TTD_WELLKNOWN_TOKEN BinaryFormatReader::ReadNakedWellKnownToken(UnlinkableSlabAllocator& alloc, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        uint32 charLen = 0;
        this->ReadBytesInto_Fixed<uint32>(charLen);

        char16* cbuff = alloc.SlabAllocateArray<char16>(charLen + 1);
        this->ReadBytesInto((byte*)cbuff, charLen * sizeof(char16));
        cbuff[charLen] = _u('\0');

        return cbuff;
    }

    void BinaryFormatReader::ReadInlineCode(_Out_writes_(length) char16* code, uint32 length, bool readSeparator)
    {
        uint32 wlen = 0;
        this->ReadBytesInto_Fixed<uint32>(wlen);
        TTDAssert(wlen == length, "Not exepcted string length!!!");

        this->ReadBytesInto((byte*)code, length * sizeof(char16));
    }

    //////////////////

#if ENABLE_OBJECT_SOURCE_TRACKING
    bool IsDiagnosticOriginInformationValid(const DiagnosticOrigin& info)
    {
        return info.SourceLine != -1;
    }

    void InitializeDiagnosticOriginInformation(DiagnosticOrigin& info)
    {
        info.SourceLine = -1;
        info.EventTime = 0;
        info.TimeHash = 0;
    }

    void CopyDiagnosticOriginInformation(DiagnosticOrigin& infoInto, const DiagnosticOrigin& infoFrom)
    {
        infoInto.SourceLine = infoFrom.SourceLine;
        infoInto.EventTime = infoFrom.EventTime;
        infoInto.TimeHash = infoFrom.TimeHash;
    }

    void SetDiagnosticOriginInformation(DiagnosticOrigin& info, uint32 sourceLine, uint64 eTime, uint64 fTime, uint64 lTime)
    {
        info.SourceLine = sourceLine;
        info.EventTime = (uint32)eTime;
        info.TimeHash = ((uint32)(lTime << 32)) | ((uint32)fTime);
    }

    void EmitDiagnosticOriginInformation(const DiagnosticOrigin& info, FileWriter* writer, NSTokens::Separator separator)
    {
        writer->WriteRecordStart(separator);
        writer->WriteInt32(NSTokens::Key::line, info.SourceLine);
        writer->WriteUInt32(NSTokens::Key::eventTime, info.EventTime, NSTokens::Separator::CommaSeparator);
        writer->WriteUInt64(NSTokens::Key::u64Val, info.TimeHash, NSTokens::Separator::CommaSeparator);
        writer->WriteRecordEnd();
    }

    void ParseDiagnosticOriginInformation(DiagnosticOrigin& info, bool readSeperator, FileReader* reader)
    {
        reader->ReadRecordStart(readSeperator);
        info.SourceLine = reader->ReadInt32(NSTokens::Key::line);
        info.EventTime = reader->ReadUInt32(NSTokens::Key::eventTime, true);
        info.TimeHash = reader->ReadUInt64(NSTokens::Key::u64Val, true);
        reader->ReadRecordEnd();
    }
#endif

#if ENABLE_BASIC_TRACE || ENABLE_FULL_BC_TRACE
    void TraceLogger::AppendText(char* text, uint32 length)
    {
        this->EnsureSpace(length);
        this->AppendRaw(text, length);
    }

    void TraceLogger::AppendText(const char16* text, uint32 length)
    {
        this->EnsureSpace(length);
        this->AppendRaw(text, length);
    }

    void TraceLogger::AppendIndent()
    {
        uint32 totalIndent = this->m_indentSize * 2;
        while(totalIndent > TRACE_LOGGER_INDENT_BUFFER_SIZE)
        {
            this->EnsureSpace(TRACE_LOGGER_INDENT_BUFFER_SIZE);
            this->AppendRaw(this->m_indentBuffer, TRACE_LOGGER_INDENT_BUFFER_SIZE);

            totalIndent -= TRACE_LOGGER_INDENT_BUFFER_SIZE;
        }

        this->EnsureSpace(totalIndent);
        this->AppendRaw(this->m_indentBuffer, totalIndent);
    }

    void TraceLogger::AppendString(char* text)
    {
        uint32 length = (uint32)strlen(text);
        this->AppendText(text, length);
    }

    void TraceLogger::AppendBool(bool bval)
    {
        if(bval)
        {
            this->AppendLiteral("true");
        }
        else
        {
            this->AppendLiteral("false");
        }
    }

    void TraceLogger::AppendInteger(int64 ival)
    {
        this->EnsureSpace(64);
        this->m_currLength += sprintf_s(this->m_buffer + this->m_currLength, 64, "%I64i", ival);
    }

    void TraceLogger::AppendUnsignedInteger(uint64 ival)
    {
        this->EnsureSpace(64);
        this->m_currLength += sprintf_s(this->m_buffer + this->m_currLength, 64, "%I64u", ival);
    }

    void TraceLogger::AppendIntegerHex(int64 ival)
    {
        this->EnsureSpace(64);
        this->m_currLength += sprintf_s(this->m_buffer + this->m_currLength, 64, "0x%I64x", ival);
    }

    void TraceLogger::AppendDouble(double dval)
    {
        this->EnsureSpace(64);

        if(INT32_MIN <= dval && dval <= INT32_MAX &&  floor(dval) == dval)
        {
            this->m_currLength += sprintf_s(this->m_buffer + this->m_currLength, 64, "%I64i", (int64)dval);
        }
        else
        {
            this->m_currLength += sprintf_s(this->m_buffer + this->m_currLength, 64, "%.32f", dval);
        }
    }

    TraceLogger::TraceLogger(FILE* outfile)
        : m_currLength(0), m_indentSize(0), m_outfile(outfile)
    {
        this->m_buffer = (char*)malloc(TRACE_LOGGER_BUFFER_SIZE);
        TTDAssert(this->m_buffer != nullptr, "Malloc failure in tracing code.");

        this->m_indentBuffer = (char*)malloc(TRACE_LOGGER_INDENT_BUFFER_SIZE);
        TTDAssert(this->m_indentBuffer != nullptr, "Malloc failure in tracing code.");

        memset(this->m_buffer, 0, TRACE_LOGGER_BUFFER_SIZE);
        memset(this->m_indentBuffer, 0, TRACE_LOGGER_INDENT_BUFFER_SIZE);
    }

    TraceLogger::~TraceLogger()
    {
        this->ForceFlush();

        if(this->m_outfile != stdout && this->m_outfile != stderr)
        {
            fclose(this->m_outfile);
        }

        free(this->m_buffer);
        free(this->m_indentBuffer);
    }

    void TraceLogger::ForceFlush()
    {
        if(this->m_currLength != 0)
        {
            fwrite(this->m_buffer, sizeof(char), this->m_currLength, this->m_outfile);

            this->m_currLength = 0;
        }

        fflush(this->m_outfile);
    }

    void TraceLogger::WriteEnumAction(int64 eTime, BOOL returnCode, Js::PropertyId pid, Js::PropertyAttributes attrib, Js::JavascriptString* pname)
    {
        this->AppendLiteral("EnumAction(time: ");
        this->AppendInteger(eTime);
        this->AppendLiteral(", rCode: ");
        this->AppendInteger(returnCode);
        this->AppendLiteral(", pid: ");
        this->AppendInteger(pid);

        if(returnCode)
        {
            this->AppendLiteral(", attrib: ");
            this->AppendInteger(attrib);
            this->AppendLiteral(", name: ");
            this->AppendText(pname->GetSz(), (uint32)pname->GetLength());
        }

        this->AppendLiteral(")\n");
    }

    void TraceLogger::WriteVar(Js::Var var, bool skipStringContents)
    {
        if(var == nullptr)
        {
            this->AppendLiteral("nullptr");
        }
        else
        {
            Js::TypeId tid = Js::JavascriptOperators::GetTypeId(var);
            switch(tid)
            {
            case Js::TypeIds_Undefined:
                this->AppendLiteral("undefined");
                break;
            case Js::TypeIds_Null:
                this->AppendLiteral("null");
                break;
            case Js::TypeIds_Boolean:
                this->AppendBool(!!Js::JavascriptBoolean::FromVar(var)->GetValue());
                break;
            case Js::TypeIds_Integer:
                this->AppendInteger(Js::TaggedInt::ToInt64(var));
                break;
            case Js::TypeIds_Number:
                this->AppendDouble(Js::JavascriptNumber::GetValue(var));
                break;
            case Js::TypeIds_Int64Number:
                this->AppendInteger(Js::JavascriptInt64Number::FromVar(var)->GetValue());
                break;
            case Js::TypeIds_UInt64Number:
                this->AppendUnsignedInteger(Js::JavascriptUInt64Number::FromVar(var)->GetValue());
                break;
            case Js::TypeIds_String:
                this->AppendLiteral("'");
                if(!skipStringContents)
                {
                    if(Js::JavascriptString::FromVar(var)->GetLength() <= 40)
                    {
                        this->AppendText(Js::JavascriptString::FromVar(var)->GetSz(), Js::JavascriptString::FromVar(var)->GetLength());
                    }
                    else
                    {
                        this->AppendText(Js::JavascriptString::FromVar(var)->GetSz(), 40);
                        this->AppendLiteral("...");
                        this->AppendInteger(Js::JavascriptString::FromVar(var)->GetLength());
                    }
                }
                else
                {
                    this->AppendLiteral("string@length=");
                    this->AppendInteger(Js::JavascriptString::FromVar(var)->GetLength());
                    this->AppendLiteral("...");
                }
                this->AppendLiteral("'");
                break;
            default:
            {
#if ENABLE_OBJECT_SOURCE_TRACKING
                if(tid > Js::TypeIds_LastStaticType)
                {
                    const Js::DynamicObject* dynObj = Js::DynamicObject::FromVar(var);
                    if(!IsDiagnosticOriginInformationValid(dynObj->TTDDiagOriginInfo))
                    {
                        this->AppendLiteral("*");
                    }
                    else
                    {
                        this->AppendLiteral("obj(");
                        this->AppendInteger((int64)dynObj->TTDDiagOriginInfo.SourceLine);
                        this->AppendLiteral(", ");
                        this->AppendInteger((int64)dynObj->TTDDiagOriginInfo.EventTime);
                        this->AppendLiteral(", ");
                        this->AppendInteger((int64)dynObj->TTDDiagOriginInfo.TimeHash);
                        this->AppendLiteral(")");
                    }
                }
                else
                {
#endif
                    this->AppendLiteral("Unspecialized object kind: ");
                    this->AppendInteger((int64)tid);
#if ENABLE_OBJECT_SOURCE_TRACKING
                }
#endif
                break;
            }
            }
        }
    }

    void TraceLogger::WriteCall(Js::JavascriptFunction* function, bool isExternal, uint32 argc, Js::Var* argv, int64 etime)
    {
        Js::JavascriptString* displayName = function->GetDisplayName();

        this->AppendIndent();
        const char16* nameStr = displayName->GetSz();
        uint32 nameLength = displayName->GetLength();
        this->AppendText(nameStr, nameLength);

        if(isExternal)
        {
            this->AppendLiteral("^(");
        }
        else
        {
            this->AppendLiteral("(");
        }

        for(uint32 i = 0; i < argc; ++i)
        {
            if(i != 0)
            {
                this->AppendLiteral(", ");
            }

            this->WriteVar(argv[i]);
        }

        this->AppendLiteral(")");

        this->AppendLiteral(" @ ");
        this->AppendInteger(etime);

        this->AppendLiteral("\n");

        this->m_indentSize++;
    }

    void TraceLogger::WriteReturn(Js::JavascriptFunction* function, Js::Var res, int64 etime)
    {
        this->m_indentSize--;

        Js::JavascriptString* displayName = function->GetDisplayName();

        this->AppendIndent();
        this->AppendLiteral("return(");
        this->AppendText(displayName->GetSz(), displayName->GetLength());
        this->AppendLiteral(") -> ");
        this->WriteVar(res);

        this->AppendLiteral(" @ ");
        this->AppendInteger(etime);

        this->AppendLiteral("\n");
    }

    void TraceLogger::WriteReturnException(Js::JavascriptFunction* function, int64 etime)
    {
        this->m_indentSize--;

        Js::JavascriptString* displayName = function->GetDisplayName();

        this->AppendIndent();
        this->AppendLiteral("return(");
        this->AppendText(displayName->GetSz(), displayName->GetLength());
        this->AppendLiteral(") -> !!exception");

        this->AppendLiteral(" @ ");
        this->AppendInteger(etime);

        this->AppendLiteral("\n");
    }

    void TraceLogger::WriteStmtIndex(uint32 line, uint32 column)
    {
        this->AppendIndent();

        this->EnsureSpace(128);
        this->m_currLength += sprintf_s(this->m_buffer + this->m_currLength, 128, "(l:%I32u, c:%I32u)\n", line + 1, column);

        ////
        //Temp debugging help if needed 
        this->ForceFlush();
        //
        ////
    }

    void TraceLogger::WriteTraceValue(Js::Var var)
    {
        this->WriteVar(var, true);
        this->WriteLiteralMsg("\n");
    }
#endif
}

#endif

