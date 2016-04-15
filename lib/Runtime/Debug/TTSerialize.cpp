//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeDebugPch.h"

#include <wchar.h>

#if ENABLE_TTD

namespace TTD
{
    namespace NSTokens
    {
        LPCWSTR* InitKeyNamesArray()
        {
            LPCWSTR* res = HeapNewArrayZ(LPCWSTR, (uint32)Key::Count);

#define ENTRY_SERIALIZE_ENUM(K) res[(uint32)Key::##K] = __TEXT(#K);
#include "TTSerializeEnum.h"

            return res;
        }
    }

    //////////////////

    void FileWriter::Flush()
    {
        if(this->m_cursor != 0)
        {
            AssertMsg(this->m_hfile != INVALID_HANDLE_VALUE, "Trying to write to closed file.");

            DWORD bwp = 0;
            this->m_pfWrite(this->m_hfile, this->m_buffer, this->m_cursor, &bwp);

            this->m_totalBytesWritten += this->m_cursor;
            this->m_cursor = 0;
        }
    }

    void FileWriter::WriteByte(byte b)
    {
        if(this->m_cursor == TTD_SERIALIZATION_BUFFER_SIZE)
        {
            this->Flush();
        }

        this->m_buffer[this->m_cursor] = b;
        this->m_cursor++;
    }

    void FileWriter::WriteBytes(byte* buff, uint32 bufflen)
    {
        if(this->m_cursor + bufflen >= TTD_SERIALIZATION_BUFFER_SIZE)
        {
            this->Flush();
        }

        if(bufflen >= TTD_SERIALIZATION_BUFFER_SIZE)
        {
            AssertMsg(this->m_buffer == 0, "Should always have been flushed in check above!");

            //explicitly write the buffer to disk
            DWORD bwp = 0;
            this->m_pfWrite(this->m_hfile, buff, bufflen, &bwp);
            AssertMsg(bwp == bufflen, "Write Failed");

            this->m_totalBytesWritten += bufflen;
        }
        else
        {
            memcpy(this->m_buffer + this->m_cursor, buff, bufflen);
            this->m_cursor += bufflen;
        }
    }

    FileWriter::FileWriter(HANDLE handle, TTDWriteBytesToStreamCallback pfWrite, TTDFlushAndCloseStreamCallback pfClose)
        : m_hfile(handle), m_pfWrite(pfWrite), m_pfClose(pfClose), m_totalBytesWritten(0), m_cursor(0), m_buffer(nullptr)
    {
        this->m_buffer = HeapNewArrayZ(byte, TTD_SERIALIZATION_BUFFER_SIZE);
    }

    FileWriter::~FileWriter()
    {
        this->FlushAndClose();
    }

    void FileWriter::FlushAndClose()
    {
        if(this->m_hfile != INVALID_HANDLE_VALUE)
        {
            this->Flush();
            HeapDeleteArray(TTD_SERIALIZATION_BUFFER_SIZE, this->m_buffer);

            this->m_pfClose(this->m_hfile, false, true);
            this->m_hfile = INVALID_HANDLE_VALUE;
        }
    }

    uint64 FileWriter::GetBytesWritten() const
    {
        return this->m_totalBytesWritten;
    }

    LPCWSTR FileWriter::FormatNumber(DWORD_PTR value)
    {
        swprintf_s(this->m_numberFormatBuff, _u("%u"), (uint32)value);

        return this->m_numberFormatBuff;
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

    void FileWriter::WriteLogTag(NSTokens::Key key, TTD_LOG_TAG val, NSTokens::Separator separator)
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

    void JSONWriter::WriteWCHAR(wchar c)
    {
        if(c <= CHAR_MAX)
        {
            this->WriteByte((byte)c);
        }
        else
        {
            swprintf_s(this->m_unicodeBuff, _u("\\u%.4x"), c);

            for(wchar* curr = this->m_unicodeBuff; *curr != _u('\0'); ++curr)
            {
                this->WriteByte((byte)(*curr));
            }
        }
    }

    void JSONWriter::WriteString_InternalNoEscape(LPCWSTR str, size_t length)
    {
        for(size_t i = 0; i < length; ++i)
        {
            this->WriteWCHAR(str[i]);
        }
    }

    void JSONWriter::WriteString_Internal(LPCWSTR str, size_t length)
    {
        for(size_t i = 0; i < length; ++i)
        {
            wchar c = str[i];

            //JSON escape sequence in a string \0 \", \/, \\, \b, \f, \n, \r, \t, unicode seq
            switch(c)
            {
            case _u('\0'):
                this->WriteWCHAR(_u('\\'));
                this->WriteWCHAR(_u('0'));
                break;
            case _u('\"'):
                this->WriteWCHAR(_u('\\'));
                this->WriteWCHAR(_u('\"'));
                break;
            case _u('/'):
                this->WriteWCHAR(_u('\\'));
                this->WriteWCHAR(_u('/'));
                break;
            case _u('\\'):
                this->WriteWCHAR(_u('\\'));
                this->WriteWCHAR(_u('\\'));
                break;
            case _u('\b'):
                this->WriteWCHAR(_u('\\'));
                this->WriteWCHAR(_u('b'));
                break;
            case _u('\f'):
                this->WriteWCHAR(_u('\\'));
                this->WriteWCHAR(_u('f'));
                break;
            case _u('\n'):
                this->WriteWCHAR(_u('\\'));
                this->WriteWCHAR(_u('n'));
                break;
            case _u('\r'):
                this->WriteWCHAR(_u('\\'));
                this->WriteWCHAR(_u('r'));
                break;
            case _u('\t'):
                this->WriteWCHAR(_u('\\'));
                this->WriteWCHAR(_u('t'));
                break;
            default:
                this->WriteWCHAR(c);
                break;
            }
        }
    }

    JSONWriter::JSONWriter(HANDLE handle, TTDWriteBytesToStreamCallback pfWrite, TTDFlushAndCloseStreamCallback pfClose)
        : FileWriter(handle, pfWrite, pfClose), m_keyNameArray(nullptr), m_indentSize(0)
    {
        this->m_keyNameArray = NSTokens::InitKeyNamesArray();
    }

    JSONWriter::~JSONWriter()
    {
        if(this->m_keyNameArray != nullptr)
        {
            HeapDeleteArray((uint32)NSTokens::Key::Count, this->m_keyNameArray);
            this->m_keyNameArray = nullptr;
        }
    }

    void JSONWriter::WriteSeperator(NSTokens::Separator separator)
    {
        if((separator & NSTokens::Separator::CommaSeparator) == NSTokens::Separator::CommaSeparator)
        {
            this->WriteWCHAR(_u(','));

            if((separator & NSTokens::Separator::BigSpaceSeparator) == NSTokens::Separator::BigSpaceSeparator)
            {
                this->WriteWCHAR(_u('\n'));
                for(uint32 i = 0; i < this->m_indentSize; ++i)
                {
                    this->WriteWCHAR(_u(' '));
                    this->WriteWCHAR(_u(' '));
                }
            }
            else
            {
                this->WriteWCHAR(_u(' '));
            }
        }

        if(separator == NSTokens::Separator::BigSpaceSeparator)
        {
            this->WriteWCHAR(_u('\n'));
            for(uint32 i = 0; i < this->m_indentSize; ++i)
            {
                this->WriteWCHAR(_u(' '));
                this->WriteWCHAR(_u(' '));
            }
        }
    }

    void JSONWriter::WriteKey(NSTokens::Key key, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        AssertMsg(1 <= (uint32)key && (uint32)key < (uint32)NSTokens::Key::Count, "Key not in valid range!");
        LPCWSTR kname = this->m_keyNameArray[(uint32)key];
        AssertMsg(kname != nullptr, "Key not registered!");

        this->WriteWCHAR(_u('\"'));
        this->WriteString_InternalNoEscape(kname, wcslen(kname));
        this->WriteWCHAR(_u('\"'));

        this->WriteWCHAR(_u(':'));
        this->WriteWCHAR(_u(' '));
    }

    void JSONWriter::WriteSequenceStart(NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteWCHAR(_u('['));
    }

    void JSONWriter::WriteSequenceEnd(NSTokens::Separator separator)
    {
        AssertMsg(separator == NSTokens::Separator::NoSeparator || separator == NSTokens::Separator::BigSpaceSeparator, "Shouldn't be anything else!!!");

        this->WriteSeperator(separator);
        this->WriteWCHAR(_u(']'));
    }

    void JSONWriter::WriteRecordStart(NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);
        this->WriteWCHAR(_u('{'));
    }

    void JSONWriter::WriteRecordEnd(NSTokens::Separator separator)
    {
        AssertMsg(separator == NSTokens::Separator::NoSeparator || separator == NSTokens::Separator::BigSpaceSeparator, "Shouldn't be anything else!!!");

        this->WriteSeperator(separator);
        this->WriteWCHAR(_u('}'));
    }

    void JSONWriter::AdjustIndent(int32 delta)
    {
        this->m_indentSize += delta;
    }

    void JSONWriter::SetIndent(uint32 depth)
    {
        this->m_indentSize = depth;
    }

    void JSONWriter::WriteNakedNull(NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        this->WriteString_InternalNoEscape(_u("null"), 4);
    }

    void JSONWriter::WriteNakedByte(byte val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        swprintf_s(this->m_numberFormatBuff, _u("%I32u"), (uint32)val);
        this->WriteString_InternalNoEscape(this->m_numberFormatBuff, wcslen(this->m_numberFormatBuff));
    }

    void JSONWriter::WriteBool(NSTokens::Key key, bool val, NSTokens::Separator separator)
    {
        this->WriteKey(key, separator);
        if(val)
        {
            this->WriteString_InternalNoEscape(_u("true"), 4);
        }
        else
        {
            this->WriteString_InternalNoEscape(_u("false"), 5);
        }
    }

    void JSONWriter::WriteNakedInt32(int32 val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        swprintf_s(this->m_numberFormatBuff, _u("%I32i"), val);
        this->WriteString_InternalNoEscape(this->m_numberFormatBuff, wcslen(this->m_numberFormatBuff));
    }

    void JSONWriter::WriteNakedUInt32(uint32 val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        swprintf_s(this->m_numberFormatBuff, _u("%I32u"), val);
        this->WriteString_InternalNoEscape(this->m_numberFormatBuff, wcslen(this->m_numberFormatBuff));
    }

    void JSONWriter::WriteNakedInt64(int64 val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        swprintf_s(this->m_numberFormatBuff, _u("%I64i"), val);
        this->WriteString_InternalNoEscape(this->m_numberFormatBuff, wcslen(this->m_numberFormatBuff));
    }

    void JSONWriter::WriteNakedUInt64(uint64 val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        swprintf_s(this->m_numberFormatBuff, _u("%I64u"), val);
        this->WriteString_InternalNoEscape(this->m_numberFormatBuff, wcslen(this->m_numberFormatBuff));
    }

    void JSONWriter::WriteNakedDouble(double val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        if(Js::JavascriptNumber::IsNan(val))
        {
            this->WriteString_InternalNoEscape(_u("#nan"), 4);
        }
        else if(Js::JavascriptNumber::IsPosInf(val))
        {
            this->WriteString_InternalNoEscape(_u("#+inf"), 5);
        }
        else if(Js::JavascriptNumber::IsNegInf(val))
        {
            this->WriteString_InternalNoEscape(_u("#-inf"), 5);
        }
        else if(Js::JavascriptNumber::MAX_VALUE == val)
        {
            this->WriteString_InternalNoEscape(_u("#ub"), 3);
        }
        else if(Js::JavascriptNumber::MIN_VALUE == val)
        {
            this->WriteString_InternalNoEscape(_u("#lb"), 3);
        }
        else if(Js::Math::EPSILON == val)
        {
            this->WriteString_InternalNoEscape(_u("#ep"), 3);
        }
        else
        {
            if(floor(val) == val)
            {
                swprintf_s(this->m_numberFormatBuff, _u("%I64i"), (int64)val);
            }
            else
            {
                //
                //TODO: this is nice for visual debugging but we inherently lose precision
                //      will want to change this to a dump of the bit representation of the number
                //

                swprintf_s(this->m_numberFormatBuff, _u("%.22f"), val);
            }

            this->WriteString_InternalNoEscape(this->m_numberFormatBuff, wcslen(this->m_numberFormatBuff));
        }
    }

    void JSONWriter::WriteNakedAddr(TTD_PTR_ID val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        swprintf_s(this->m_numberFormatBuff, _u("\"*0x%I64x\""), val);
        this->WriteString_InternalNoEscape(this->m_numberFormatBuff, wcslen(this->m_numberFormatBuff));
    }

    void JSONWriter::WriteNakedLogTag(TTD_LOG_TAG val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        swprintf_s(this->m_numberFormatBuff, _u("\"!%I64i\""), val);
        this->WriteString_InternalNoEscape(this->m_numberFormatBuff, wcslen(this->m_numberFormatBuff));
    }

    void JSONWriter::WriteNakedTag(uint32 tagvalue, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        swprintf_s(this->m_numberFormatBuff, _u("\"$%I32i\""), tagvalue);
        this->WriteString_InternalNoEscape(this->m_numberFormatBuff, wcslen(this->m_numberFormatBuff));
    }

    ////

    void JSONWriter::WriteNakedString(const TTString& val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        if(IsNullPtrTTString(val))
        {
            this->WriteNakedNull();
        }
        else
        {
            this->WriteWCHAR('\"');
            this->WriteString_Internal(val.Contents, val.Length);
            this->WriteWCHAR('\"');
        }
    }

    void JSONWriter::WriteNakedWellKnownToken(TTD_WELLKNOWN_TOKEN val, NSTokens::Separator separator)
    {
        this->WriteSeperator(separator);

        this->WriteWCHAR('\"');
        this->WriteString_InternalNoEscape(val, wcslen(val));
        this->WriteWCHAR('\"');
    }

    void JSONWriter::WriteFileNameForSourceLocation(LPCWSTR filename, NSTokens::Separator separator)
    {
        this->WriteKey(NSTokens::Key::uri, separator);

        this->WriteWCHAR('\"');
        this->WriteString_Internal(filename, wcslen(filename));
        this->WriteWCHAR('\"');
    }

    //////////////////

    void FileReader::Fill()
    {
        if(this->m_cursor == this->m_buffCount)
        {
            AssertMsg(this->m_hfile != INVALID_HANDLE_VALUE, "Trying to read a invalid file.");

            DWORD bwp = 0;
            BOOL ok = this->m_pfRead(this->m_hfile, this->m_buffer, TTD_SERIALIZATION_BUFFER_SIZE, &bwp);
            AssertMsg(ok, "Read failed.");

            this->m_buffCount = bwp;
            this->m_cursor = 0;

            this->m_totalBytesRead += bwp;
        }
    }

    bool FileReader::Peek(byte* b)
    {
        if(this->m_peekByte != -1)
        {
            *b = (byte)this->m_peekByte;
            return true;
        }
        else
        {
            bool success = this->ReadByte(b);
            if(success)
            {
                this->m_peekByte = *b;
            }
            return success;
        }
    }

    bool FileReader::ReadByte(byte* b)
    {
        if(this->m_peekByte != -1)
        {
            *b = (byte)this->m_peekByte;
            this->m_peekByte = -1;

            return true;
        }
        else
        {
            this->Fill();

            if(this->m_cursor == this->m_buffCount)
            {
                return false;
            }
            else
            {
                *b = this->m_buffer[this->m_cursor];
                this->m_cursor++;

                return true;
            }
        }
    }

    void FileReader::FileReadAssert(bool ok)
    {
        //
        //TODO: we probably want to make this a special exception so we can abort cleanly when reading an invalid data
        //
        AssertMsg(ok, "Unexpected event in file reading!");
    }

    FileReader::FileReader(HANDLE handle, TTDReadBytesFromStreamCallback pfRead, TTDFlushAndCloseStreamCallback pfClose)
        : m_hfile(handle), m_pfRead(pfRead), m_pfClose(pfClose), m_totalBytesRead(0), m_peekByte(-1), m_cursor(0), m_buffCount(0), m_buffer(nullptr)
    {
        this->m_buffer = HeapNewArray(byte, TTD_SERIALIZATION_BUFFER_SIZE);
    }

    FileReader::~FileReader()
    {
        if(this->m_hfile != INVALID_HANDLE_VALUE)
        {
            HeapDeleteArray(TTD_SERIALIZATION_BUFFER_SIZE, this->m_buffer);

            this->m_pfClose(this->m_hfile, true, false);
            this->m_hfile = INVALID_HANDLE_VALUE;
        }
    }

    LPCWSTR FileReader::FormatNumber(DWORD_PTR value)
    {
        swprintf_s(this->m_numberFormatBuff, _u("%u"), (uint32)value);

        return this->m_numberFormatBuff;
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

    TTD_LOG_TAG FileReader::ReadLogTag(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadKey(keyCheck, readSeparator);
        return this->ReadNakedLogTag();
    }

    //////////////////

    NSTokens::ParseTokenKind JSONReader::Scan(JsUtil::List<wchar, HeapAllocator>& charList)
    {
        byte b;
        charList.Clear();

        while(this->ReadByte(&b))
        {
            switch(b)
            {
            case 0:
                return NSTokens::ParseTokenKind::Error; //we shouldn't hit EOF explicitly here
            case '\t':
            case '\r':
            case '\n':
            case ' ':
                //WS - keep looping
                break;
            case '"':
                //check for string
                charList.Add(_u('"'));
                return this->ScanString(charList);
            case '#':
                //# starts special double/number value representation
                charList.Add(_u('#'));
                return this->ScanSpecialNumber(charList);
            case '-':
            case '+':
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9':
            {
                //decimal digit or (-,+) starts a number
                charList.Add((wchar)b);
                return this->ScanNumber(charList);
            }
            case ',':
                return NSTokens::ParseTokenKind::Comma;
            case ':':
                return NSTokens::ParseTokenKind::Colon;
            case '[':
                return NSTokens::ParseTokenKind::LBrack;
            case ']':
                return NSTokens::ParseTokenKind::RBrack;
            case '{':
                return NSTokens::ParseTokenKind::LCurly;
            case '}':
                return NSTokens::ParseTokenKind::RCurly;
            default:
                //it is a naked string (or an error)
                charList.Add((wchar)b);
                return this->ScanNakedString((wchar)b, charList);
            }
        }

        return NSTokens::ParseTokenKind::Error;
    }

    NSTokens::ParseTokenKind JSONReader::ScanString(JsUtil::List<wchar, HeapAllocator>& charList)
    {
        byte b;
        bool endFound = false;

        while(this->ReadByte(&b))
        {
            if(b == 0)
            {
                return NSTokens::ParseTokenKind::Error;
            }

            if(b == '"')
            {
                //end of the string
                charList.Add(_u('"'));
                endFound = true;
                break;
            }
            else if(b == '\\')
            {
                //JSON escape sequence in a string \", \/, \\, \b, \f, \n, \r, \t, unicode seq
                // unlikely V5.8 regular chars are not escaped, i.e '\g'' in a string is illegal not 'g'
                bool ok = this->ReadByte(&b);
                if(!ok)
                {
                    return NSTokens::ParseTokenKind::Error;
                }

                switch(b)
                {
                case 0:
                    return NSTokens::ParseTokenKind::Error; //we shouldn't hit EOF explicitly here
                case '0':
                    charList.Add(0x0);
                    break;
                case '"':
                case '/':
                case '\\':
                    charList.Add((wchar)b); //keep ch
                    break;
                case 'b':
                    charList.Add(0x08);
                    break;
                case 'f':
                    charList.Add(0x0C);
                    break;
                case 'n':
                    charList.Add(0x0A);
                    break;
                case 'r':
                    charList.Add(0x0D);
                    break;
                case 't':
                    charList.Add(0x09);
                    break;
                case 'u':
                {
                    int tempHex = 0;
                    int chcode = 0; // 4 hex digits are needed

                    ok = this->ReadByte(&b);
                    if(!ok || !Js::NumberUtilities::FHexDigit(b, &tempHex))
                    {
                        return NSTokens::ParseTokenKind::Error;
                    }
                    chcode = tempHex * 0x1000;

                    ok = this->ReadByte(&b);
                    if(!ok || !Js::NumberUtilities::FHexDigit(b, &tempHex))
                    {
                        return NSTokens::ParseTokenKind::Error;
                    }
                    chcode += tempHex * 0x0100;

                    ok = this->ReadByte(&b);
                    if(!ok || !Js::NumberUtilities::FHexDigit(b, &tempHex))
                    {
                        return NSTokens::ParseTokenKind::Error;
                    }
                    chcode += tempHex * 0x0010;

                    ok = this->ReadByte(&b);
                    if(!ok || !Js::NumberUtilities::FHexDigit(b, &tempHex))
                    {
                        return NSTokens::ParseTokenKind::Error;
                    }
                    chcode += tempHex;

                    AssertMsg(chcode == (chcode & 0xFFFF), "Bad unicode code");
                    charList.Add((wchar)chcode);
                }
                break;
                default:
                    // Any other '\o' is an error
                    return NSTokens::ParseTokenKind::Error;
                }
            }
            else
            {
                charList.Add((wchar)b);
            }
        }

        if(!endFound)
        {
            // no ending '"' found 
            return NSTokens::ParseTokenKind::Error;
        }

        return NSTokens::ParseTokenKind::String;
    }

    NSTokens::ParseTokenKind JSONReader::ScanSpecialNumber(JsUtil::List<wchar, HeapAllocator>& charList)
    {
        byte b;
        bool ok = this->ReadByte(&b);
        charList.Add((wchar)b);

        if(ok && b == 'n')
        {
            ok = this->ReadByte(&b);
            if(!ok || b != 'a')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            ok = this->ReadByte(&b);
            if(!ok || b != 'n')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            return NSTokens::ParseTokenKind::Number;
        }
        else if(ok && (b == '+' || b == '-'))
        {
            ok = this->ReadByte(&b);
            if(!ok || b != 'i')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            ok = this->ReadByte(&b);
            if(!ok || b != 'n')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            ok = this->ReadByte(&b);
            if(!ok || b != 'f')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            return NSTokens::ParseTokenKind::Number;
        }
        else if(ok && (b == 'u' || b == 'l'))
        {
            ok = this->ReadByte(&b);
            if(!ok || b != 'b')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            return NSTokens::ParseTokenKind::Number;
        }
        else if(ok && b == 'e')
        {
            ok = this->ReadByte(&b);
            if(!ok || b != 'p')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            return NSTokens::ParseTokenKind::Number;
        }
        else
        {
            return NSTokens::ParseTokenKind::Error;
        }
    }

    NSTokens::ParseTokenKind JSONReader::ScanNumber(JsUtil::List<wchar, HeapAllocator>& charList)
    {
        byte b;
        while(this->Peek(&b) && (('0' <= b && b <= '9') || (b == '.')))
        {
            this->ReadByte(&b);
            charList.Add((wchar)b);
        }

        bool likelyint; //we don't care about this just want to know that it is convertable to a number
        const wchar* end;
        const wchar* start = charList.GetBuffer();
        double val = Js::NumberUtilities::StrToDbl<wchar>(start, &end, likelyint);
        if(start == end)
        {
            return NSTokens::ParseTokenKind::Error;
        }
        AssertMsg(!Js::JavascriptNumber::IsNan(val), "Bad result from string to double conversion");

        return NSTokens::ParseTokenKind::Number;
    }

    NSTokens::ParseTokenKind JSONReader::ScanNakedString(wchar leadChar, JsUtil::List<wchar, HeapAllocator>& charList)
    {
        bool ok;
        byte b;

        if(leadChar == 'n')
        {
            //check for "null"

            ok = this->ReadByte(&b);
            if(!ok || b != 'u')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            ok = this->ReadByte(&b);
            if(!ok || b != 'l')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            ok = this->ReadByte(&b);
            if(!ok || b != 'l')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            return NSTokens::ParseTokenKind::Null;
        }
        else if(leadChar == 't')
        {
            //check for "true"

            ok = this->ReadByte(&b);
            if(!ok || b != 'r')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            ok = this->ReadByte(&b);
            if(!ok || b != 'u')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            ok = this->ReadByte(&b);
            if(!ok || b != 'e')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            return NSTokens::ParseTokenKind::True;
        }
        else if(leadChar == 'f')
        {
            //check for "false"

            ok = this->ReadByte(&b);
            if(!ok || b != 'a')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            ok = this->ReadByte(&b);
            if(!ok || b != 'l')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            ok = this->ReadByte(&b);
            if(!ok || b != 's')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            ok = this->ReadByte(&b);
            if(!ok || b != 'e')
            {
                return NSTokens::ParseTokenKind::Error;
            }
            charList.Add((wchar)b);

            return NSTokens::ParseTokenKind::False;
        }
        else
        {
            return NSTokens::ParseTokenKind::Error;
        }
    }

    uint64 JSONReader::ReadHexFromCharArray(const wchar* buff)
    {
        uint64 value = 0;
        uint64 multiplier = 1;

        int32 digitCount = (int32)wcslen(buff);
        for(int32 i = digitCount - 1; i >= 0; --i)
        {
            wchar digit = buff[i];

            uint32 digitValue;
            if((L'a' <= digit) & (digit <= 'f'))
            {
                digitValue = (digit - _u('a')) + 10;
            }
            else
            {
                digitValue = (digit - _u('0'));
            }

            value += (multiplier * digitValue);
            multiplier *= 16;
        }

        return value;
    }

    int64 JSONReader::ReadIntFromCharArray(const wchar* buff)
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
            wchar digit = buff[i];
            uint32 digitValue = (digit - _u('0'));

            value += (multiplier * digitValue);
            multiplier *= 10;
        }

        return value * sign;
    }

    uint64 JSONReader::ReadUIntFromCharArray(const wchar* buff)
    {
        uint64 value = 0;
        uint64 multiplier = 1;

        int32 digitCount = (int32)wcslen(buff);
        for(int32 i = digitCount - 1; i >= 0; --i)
        {
            wchar digit = buff[i];
            uint32 digitValue = (digit - _u('0'));

            value += (multiplier * digitValue);
            multiplier *= 10;
        }

        return value;
    }

    double JSONReader::ReadDoubleFromCharArray(const wchar* buff)
    {
        bool likelytInt; //we don't care about this as we already know it is a double
        const wchar* end;
        double val = Js::NumberUtilities::StrToDbl<wchar>(buff, &end, likelytInt);
        FileReader::FileReadAssert((buff != end) && !Js::JavascriptNumber::IsNan(val));

        return val;
    }

    JSONReader::JSONReader(HANDLE handle, TTDReadBytesFromStreamCallback pfRead, TTDFlushAndCloseStreamCallback pfClose)
        : FileReader(handle, pfRead, pfClose), m_charListPrimary(&HeapAllocator::Instance), m_charListOpt(&HeapAllocator::Instance), m_charListDiscard(&HeapAllocator::Instance), m_keyNameArray(nullptr)
    {
        this->m_keyNameArray = NSTokens::InitKeyNamesArray();
    }

    JSONReader::~JSONReader()
    {
        if(this->m_keyNameArray != nullptr)
        {
            HeapDeleteArray((uint32)NSTokens::Key::Count, this->m_keyNameArray);
            this->m_keyNameArray = nullptr;
        }
    }

    void JSONReader::ReadSeperator(bool readSeparator)
    {
        if(readSeparator)
        {
            NSTokens::ParseTokenKind tok = this->Scan(this->m_charListDiscard);
            FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::Comma);
        }
    }

    void JSONReader::ReadKey(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListPrimary);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::String);

        this->m_charListPrimary.SetItem(this->m_charListPrimary.Count() - 1, _u('\0'));
        LPCWSTR keystr = this->m_charListPrimary.GetBuffer() + 1;

        //check key strings are the same
        FileReader::FileReadAssert(1 <= (uint32)keyCheck && (uint32)keyCheck < (uint32)NSTokens::Key::Count);
        LPCWSTR kname = this->m_keyNameArray[(uint32)keyCheck];
        FileReader::FileReadAssert(kname != nullptr);

        FileReader::FileReadAssert(wcscmp(keystr, kname) == 0);

        NSTokens::ParseTokenKind toksep = this->Scan(this->m_charListDiscard);
        FileReader::FileReadAssert(toksep == NSTokens::ParseTokenKind::Colon);
    }

    void JSONReader::ReadSequenceStart(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListDiscard);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::LBrack);
    }

    void JSONReader::ReadSequenceEnd()
    {
        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListDiscard);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::RBrack);
    }

    void JSONReader::ReadRecordStart(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListDiscard);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::LCurly);
    }

    void JSONReader::ReadRecordEnd()
    {
        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListDiscard);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::RCurly);
    }

    void JSONReader::ReadNakedNull(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListDiscard);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::Null);
    }

    byte JSONReader::ReadNakedByte(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::Number);

        this->m_charListOpt.Add(_u('\0'));
        uint64 uval = this->ReadUIntFromCharArray(m_charListOpt.GetBuffer());
        FileReader::FileReadAssert(uval <= BYTE_MAX);
        return (byte)uval;
    }

    bool JSONReader::ReadBool(NSTokens::Key keyCheck, bool readSeparator)
    {
        this->ReadKey(keyCheck, readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::True || tok == NSTokens::ParseTokenKind::False);

        return (tok == NSTokens::ParseTokenKind::True);
    }

    int32 JSONReader::ReadNakedInt32(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::Number);

        this->m_charListOpt.Add(_u('\0'));
        int64 ival = this->ReadIntFromCharArray(this->m_charListOpt.GetBuffer());
        FileReader::FileReadAssert(INT32_MIN <= ival && ival <= INT32_MAX);

        return (int32)ival;
    }

    uint32 JSONReader::ReadNakedUInt32(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::Number);

        this->m_charListOpt.Add(_u('\0'));
        uint64 uval = this->ReadUIntFromCharArray(m_charListOpt.GetBuffer());
        FileReader::FileReadAssert(uval <= UINT32_MAX);

        return (uint32)uval;
    }

    int64 JSONReader::ReadNakedInt64(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::Number);

        this->m_charListOpt.Add(_u('\0'));
        return this->ReadIntFromCharArray(this->m_charListOpt.GetBuffer());
    }

    uint64 JSONReader::ReadNakedUInt64(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::Number);

        this->m_charListOpt.Add(_u('\0'));
        return this->ReadUIntFromCharArray(this->m_charListOpt.GetBuffer());
    }

    double JSONReader::ReadNakedDouble(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::Number);

        double res = -1.0;
        if(this->m_charListOpt.Item(0) == _u('#'))
        {
            if(this->m_charListOpt.Item(1) == _u('n'))
            {
                res = Js::JavascriptNumber::NaN;
            }
            else if(this->m_charListOpt.Item(1) == _u('+'))
            {
                res = Js::JavascriptNumber::POSITIVE_INFINITY;
            }
            else if(this->m_charListOpt.Item(1) == _u('-'))
            {
                res = Js::JavascriptNumber::NEGATIVE_INFINITY;
            }
            else if(this->m_charListOpt.Item(1) == _u('u'))
            {
                res = Js::JavascriptNumber::MAX_VALUE;
            }
            else if(this->m_charListOpt.Item(1) == _u('l'))
            {
                res = Js::JavascriptNumber::MIN_VALUE;
            }
            else if(this->m_charListOpt.Item(1) == _u('e'))
            {
                res = Js::Math::EPSILON;
            }
            else
            {
                FileReader::FileReadAssert(false);
            }
        }
        else
        {
            this->m_charListOpt.Add(_u('\0'));
            res = this->ReadDoubleFromCharArray(this->m_charListOpt.GetBuffer());
        }

        return res;
    }

    TTD_PTR_ID JSONReader::ReadNakedAddr(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::String); //Addrs are strings \"*0x...\" are always strings.

        this->m_charListOpt.SetItem(this->m_charListOpt.Count() - 1, _u('\0')); //remove last "
        return (TTD_PTR_ID)this->ReadHexFromCharArray(this->m_charListOpt.GetBuffer() + 4); //skip off the first "*0x
    }

    TTD_LOG_TAG JSONReader::ReadNakedLogTag(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::String); //Log tags are strings \"!...\" are always strings.

        this->m_charListOpt.SetItem(this->m_charListOpt.Count() - 1, _u('\0')); //remove last "
        return (TTD_LOG_TAG)this->ReadUIntFromCharArray(this->m_charListOpt.GetBuffer() + 2); //skip off the first "!
    }

    uint32 JSONReader::ReadNakedTag(bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::String); //Tags are strings \"$...\" are always strings.

        this->m_charListOpt.SetItem(this->m_charListOpt.Count() - 1, _u('\0')); //remove last "
        uint64 tval = this->ReadUIntFromCharArray(this->m_charListOpt.GetBuffer() + 2); //skip off the "$
        FileReader::FileReadAssert(tval <= UINT32_MAX);

        return (uint32)tval;
    }

    ////

    void JSONReader::ReadNakedString(SlabAllocator& alloc, TTString& into, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::String || tok == NSTokens::ParseTokenKind::Null);

        if(tok == NSTokens::ParseTokenKind::Null)
        {
            alloc.CopyNullTermStringInto(nullptr, into);
        }
        else
        {
            const wchar* spos = (this->m_charListOpt.GetBuffer() + 1);
            alloc.CopyStringIntoWLength(spos, this->m_charListOpt.Count() - 2, into); //don't include the "..." marks
        }
    }

    void JSONReader::ReadNakedString(UnlinkableSlabAllocator& alloc, TTString& into, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::String || tok == NSTokens::ParseTokenKind::Null);

        if(tok == NSTokens::ParseTokenKind::Null)
        {
            alloc.CopyNullTermStringInto(nullptr, into);
        }
        else
        {
            const wchar* spos = (this->m_charListOpt.GetBuffer() + 1);
            alloc.CopyStringIntoWLength(spos, this->m_charListOpt.Count() - 2, into); //don't include the "..." marks
        }
    }

    TTD_WELLKNOWN_TOKEN JSONReader::ReadNakedWellKnownToken(SlabAllocator& alloc, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::String);

        this->m_charListOpt.SetItem(this->m_charListOpt.Count() - 1, _u('\0')); //remove last "
        LPCWSTR res = alloc.CopyRawNullTerminatedStringInto(this->m_charListOpt.GetBuffer() + 1); //remove first "

        return res;
    }

    TTD_WELLKNOWN_TOKEN JSONReader::ReadNakedWellKnownToken(UnlinkableSlabAllocator& alloc, bool readSeparator)
    {
        this->ReadSeperator(readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::String);

        this->m_charListOpt.SetItem(this->m_charListOpt.Count() - 1, _u('\0')); //remove last "
        LPCWSTR res = alloc.CopyRawNullTerminatedStringInto(this->m_charListOpt.GetBuffer() + 1); //remove first "

        return res;
    }

    LPCWSTR JSONReader::ReadFileNameForSourceLocation(bool readSeparator)
    {
        this->ReadKey(NSTokens::Key::uri, readSeparator);

        NSTokens::ParseTokenKind tok = this->Scan(this->m_charListOpt);
        FileReader::FileReadAssert(tok == NSTokens::ParseTokenKind::String);

        this->m_charListOpt.SetItem(this->m_charListOpt.Count() - 1, _u('\0')); //remove last "
        LPCWSTR res = this->m_charListOpt.GetBuffer() + 1; //remove first "

        return res;
    }

    //////////////////

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

        if(floor(dval) == dval)
        {
            this->m_currLength += sprintf_s(this->m_buffer + this->m_currLength, 64, "%I64i", (int64)dval);
        }
        else
        {
            this->m_currLength += sprintf_s(this->m_buffer + this->m_currLength, 64, "%.22f", dval);
        }
    }

    TraceLogger::TraceLogger(FILE* outfile)
        : m_currLength(0), m_indentSize(0), m_outfile(outfile)
    {
        this->m_buffer = (char*)CoTaskMemAlloc(TRACE_LOGGER_BUFFER_SIZE);
        this->m_indentBuffer = (char*)CoTaskMemAlloc(TRACE_LOGGER_INDENT_BUFFER_SIZE);

        memset(this->m_buffer, 0, TRACE_LOGGER_BUFFER_SIZE);
        memset(this->m_indentBuffer, 0, TRACE_LOGGER_INDENT_BUFFER_SIZE);
    }

    TraceLogger::~TraceLogger()
    {
        this->ForceFlush();

        if(this->m_outfile != stdout)
        {
            fclose(this->m_outfile);
        }

        CoTaskMemFree(this->m_buffer);
        CoTaskMemFree(this->m_indentBuffer);
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

    void TraceLogger::WriteVar(Js::Var var)
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
                this->AppendBool(Js::JavascriptBoolean::FromVar(var)->GetValue() ? true : false);
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
                this->AppendText(Js::JavascriptString::FromVar(var)->GetSz(), Js::JavascriptString::FromVar(var)->GetLength());
                this->AppendLiteral("'");
                break;
            default:
            {
                this->AppendLiteral("JsType:");
                this->AppendInteger((uint64)tid);
                break;
            }
            }
        }
    }

    void TraceLogger::WriteCall(Js::JavascriptFunction* function, bool isExternal, uint32 argc, Js::Var* argv)
    {
        Js::JavascriptString* displayName = function->GetDisplayName();

        this->AppendIndent();
        this->AppendText(displayName->GetSz(), displayName->GetLength());

        if(isExternal)
        {
            this->AppendLiteral("^(");
        }
        else
        {
            this->AppendLiteral("(");
        }

        for(uint32 i = 1; i < argc; ++i)
        {
            if(i != 1)
            {
                this->AppendLiteral(", ");
            }

            this->WriteVar(argv[i]);
        }

        this->AppendLiteral(")\n");

        this->m_indentSize++;
    }

    void TraceLogger::WriteReturn(Js::JavascriptFunction* function, Js::Var res)
    {
        this->m_indentSize--;

        Js::JavascriptString* displayName = function->GetDisplayName();

        this->AppendIndent();
        this->AppendLiteral("return(");
        this->AppendText(displayName->GetSz(), displayName->GetLength());
        this->AppendLiteral(") -> ");
        this->WriteVar(res);
        this->AppendLiteral("\n");
    }

    void TraceLogger::WriteReturnException(Js::JavascriptFunction* function)
    {
        this->m_indentSize--;

        Js::JavascriptString* displayName = function->GetDisplayName();

        this->AppendIndent();
        this->AppendLiteral("return(");
        this->AppendText(displayName->GetSz(), displayName->GetLength());
        this->AppendLiteral(") -> !!exception");
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
#endif
}

#endif

