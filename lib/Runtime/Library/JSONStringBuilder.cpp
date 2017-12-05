//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "RuntimeLibraryPch.h"

namespace Js
{

void
JSONStringBuilder::AppendCharacter(char16 character)
{
    AssertOrFailFast(this->currentLocation < endLocation);
    *this->currentLocation = character;
    ++this->currentLocation;
}

void
JSONStringBuilder::AppendBuffer(_In_ const char16* buffer, charcount_t length)
{
    AssertOrFailFast(this->currentLocation + length <= endLocation);
    wmemcpy_s(this->currentLocation, length, buffer, length);
    this->currentLocation += length;
}

void
JSONStringBuilder::AppendString(_In_ JavascriptString* str)
{
    AppendBuffer(str->GetString(), str->GetLength());
}

void
JSONStringBuilder::EscapeAndAppendString(_In_ JavascriptString* str)
{
    const charcount_t strLength = str->GetLength();

    // Strings should be surrounded by double quotes
    this->AppendCharacter(_u('"'));
    const char16* bufferStart = str->GetString();
    for (const char16* index = bufferStart; index < bufferStart + strLength; ++index)
    {
        char16 currentCharacter = *index;
        switch (currentCharacter)
        {
        case _u('"'):
        case _u('\\'):
            // Special characters are escaped with a backslash
            this->AppendCharacter(_u('\\'));
            this->AppendCharacter(currentCharacter);
            break;
        case _u('\b'):
            this->AppendCharacter(_u('\\'));
            this->AppendCharacter(_u('b'));
            break;
        case _u('\f'):
            this->AppendCharacter(_u('\\'));
            this->AppendCharacter(_u('f'));
            break;
        case _u('\n'):
            this->AppendCharacter(_u('\\'));
            this->AppendCharacter(_u('n'));
            break;
        case _u('\r'):
            this->AppendCharacter(_u('\\'));
            this->AppendCharacter(_u('r'));
            break;
        case _u('\t'):
            this->AppendCharacter(_u('\\'));
            this->AppendCharacter(_u('t'));
            break;
        default:
            if (currentCharacter < _u(' '))
            {
                // If character is less than SPACE, it is converted into a 4 digit hex code (e.g. \u0010)
                this->AppendCharacter(_u('\\'));
                this->AppendCharacter(_u('u'));
                {
                    char16 buf[5];
                    // Get hex value
                    _ltow_s(currentCharacter, buf, _countof(buf), 16);

                    // Append leading zeros if necessary before the hex value
                    charcount_t count = static_cast<charcount_t>(wcslen(buf));
                    switch (count)
                    {
                    case 1:
                        this->AppendCharacter(_u('0'));
                    case 2:
                        this->AppendCharacter(_u('0'));
                    case 3:
                        this->AppendCharacter(_u('0'));
                    default:
                        this->AppendBuffer(buf, count);
                        break;
                    }
                }
            }
            else
            {
                this->AppendCharacter(currentCharacter);
            }
            break;
        }
    }

    this->AppendCharacter(_u('"'));
}

void
JSONStringBuilder::AppendGap(uint32 count)
{
    for (uint i = 0; i < count; ++i)
    {
        this->AppendBuffer(this->gap, this->gapLength);
    }
}

void
JSONStringBuilder::AppendObjectString(_In_ JSONObject* valueList)
{
    const uint elementCount = valueList->Count();
    if (elementCount == 0)
    {
        this->AppendCharacter(_u('{'));
        this->AppendCharacter(_u('}'));
        return;
    }

    const uint32 stepbackLevel = this->indentLevel;
    ++this->indentLevel;

    this->AppendCharacter(_u('{'));
    if (this->gap != nullptr)
    {
        this->AppendCharacter(_u('\n'));
        this->AppendGap(indentLevel);
    }

    bool isFirstMember = true;
    FOREACH_SLISTCOUNTED_ENTRY(JSONObjectProperty, entry, valueList)
    {
        if (!isFirstMember)
        {
            if (this->gap == nullptr)
            {
                this->AppendCharacter(_u(','));
            }
            else
            {
                this->AppendCharacter(_u(','));
                this->AppendCharacter(_u('\n'));
                this->AppendGap(indentLevel);
            }
        }
        this->EscapeAndAppendString(entry.propertyName);
        this->AppendCharacter(_u(':'));
        if (this->gap != nullptr)
        {
            this->AppendCharacter(_u(' '));
        }

        this->AppendJSONPropertyString(&entry.propertyValue);

        isFirstMember = false;
    }
    NEXT_SLISTCOUNTED_ENTRY;

    if (this->gap != nullptr)
    {
        this->AppendCharacter(_u('\n'));
        this->AppendGap(stepbackLevel);
    }

    this->AppendCharacter(_u('}'));

    this->indentLevel = stepbackLevel;
}

void
JSONStringBuilder::AppendArrayString(_In_ JSONArray* valueArray)
{
    uint32 length = valueArray->length;
    if (length == 0)
    {
        this->AppendCharacter(_u('['));
        this->AppendCharacter(_u(']'));
        return;
    }

    const uint32 stepbackLevel = this->indentLevel;
    ++this->indentLevel;

    JSONProperty* arr = valueArray->arr;

    this->AppendCharacter(_u('['));

    if (this->gap != nullptr)
    {
        this->AppendCharacter(_u('\n'));
        this->AppendGap(indentLevel);
    }

    this->AppendJSONPropertyString(&arr[0]);

    for (uint32 i = 1; i < length; ++i)
    {
        if (this->gap == nullptr)
        {
            this->AppendCharacter(_u(','));
        }
        else
        {
            this->AppendCharacter(_u(','));
            this->AppendCharacter(_u('\n'));
            this->AppendGap(indentLevel);
        }
        AppendJSONPropertyString(&arr[i]);
    }

    if (this->gap != nullptr)
    {
        this->AppendCharacter(_u('\n'));
        this->AppendGap(stepbackLevel);
    }
    this->AppendCharacter(_u(']'));

    this->indentLevel = stepbackLevel;
}

void
JSONStringBuilder::AppendJSONPropertyString(_In_ JSONProperty* prop)
{
    switch (prop->type)
    {
    case JSONContentType::False:
        this->AppendString(this->scriptContext->GetLibrary()->GetFalseDisplayString());
        return;
    case JSONContentType::True:
        this->AppendString(this->scriptContext->GetLibrary()->GetTrueDisplayString());
        return;
    case JSONContentType::Null:
        this->AppendString(this->scriptContext->GetLibrary()->GetNullDisplayString());
        return;
    case JSONContentType::Number:
        this->AppendString(prop->numericValue.string);
        return;
    case JSONContentType::Object:
        this->AppendObjectString(prop->obj);
        return;
    case JSONContentType::Array:
        this->AppendArrayString(prop->arr);
        return;
    case JSONContentType::String:
        this->EscapeAndAppendString(prop->stringValue);
        return;
    default:
        Assume(UNREACHED);
    }
}

void
JSONStringBuilder::Build()
{
    this->AppendJSONPropertyString(this->jsonContent);
    // Null terminate the string
    AssertOrFailFast(this->currentLocation == endLocation);
    *this->currentLocation = _u('\0');
}

JSONStringBuilder::JSONStringBuilder(
    _In_ ScriptContext* scriptContext,
    _In_ JSONProperty* jsonContent,
    _In_ char16* buffer,
    charcount_t bufferLength,
    _In_opt_ const char16* gap,
    charcount_t gapLength) :
        scriptContext(scriptContext),
        endLocation(buffer + bufferLength - 1),
        currentLocation(buffer),
        jsonContent(jsonContent),
        gap(gap),
        gapLength(gapLength),
        indentLevel(0)
{
}

} //namespace Js
