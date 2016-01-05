//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeLibraryPch.h"

// Parser Includes
#include "DebugWriter.h"
#include "RegexPattern.h"

namespace Js
{
    JavascriptRegExp::JavascriptRegExp(UnifiedRegex::RegexPattern* pattern, DynamicType* type) :
        DynamicObject(type),
        pattern(pattern),
        splitPattern(nullptr),
        lastIndexVar(nullptr),
        lastIndexOrFlag(0)
    {
        Assert(type->GetTypeId() == TypeIds_RegEx);

        // See JavascriptRegExp::IsWritable for special non-writable properties
        // The JavascriptLibrary should have cleared the bits already
        Assert(!this->GetTypeHandler()->GetHasOnlyWritableDataProperties());
        Assert(!this->GetType()->AreThisAndPrototypesEnsuredToHaveOnlyWritableDataProperties());

#if ENABLE_REGEX_CONFIG_OPTIONS
        if (REGEX_CONFIG_FLAG(RegexTracing))
        {
            UnifiedRegex::DebugWriter* w = type->GetScriptContext()->GetRegexDebugWriter();
            if (pattern == 0)
                w->PrintEOL(L"// REGEX CREATE");
            else
            {
                w->Print(L"// REGEX CREATE ");
                pattern->Print(w);
                w->EOL();
            }
        }
#endif
    }

    JavascriptRegExp::JavascriptRegExp(DynamicType * type) :
        DynamicObject(type),
        pattern(nullptr),
        splitPattern(nullptr),
        lastIndexVar(nullptr),
        lastIndexOrFlag(0)
    {
        Assert(type->GetTypeId() == TypeIds_RegEx);

#if DBG
        if (REGEX_CONFIG_FLAG(RegexTracing))
        {
            UnifiedRegex::DebugWriter* w = type->GetScriptContext()->GetRegexDebugWriter();
            w->PrintEOL(L"REGEX CREATE");
        }
#endif
    }

     JavascriptRegExp::JavascriptRegExp(JavascriptRegExp * instance) :
        DynamicObject(instance),
        pattern(instance->GetPattern()),
        splitPattern(instance->GetSplitPattern()),
        lastIndexVar(instance->lastIndexVar),
        lastIndexOrFlag(instance->lastIndexOrFlag)
    {
        // For boxing stack instance
        Assert(ThreadContext::IsOnStack(instance));
    }

    bool JavascriptRegExp::Is(Var aValue)
    {
        return JavascriptOperators::GetTypeId(aValue) == TypeIds_RegEx;
    }

    JavascriptRegExp* JavascriptRegExp::FromVar(Var aValue)
    {
        AssertMsg(Is(aValue), "Ensure var is actually a 'JavascriptRegExp'");

        return static_cast<JavascriptRegExp *>(RecyclableObject::FromVar(aValue));
    }

    void JavascriptRegExp::SetPattern(UnifiedRegex::RegexPattern* pattern)
    {
        this->pattern = pattern;
    }

    void JavascriptRegExp::SetSplitPattern(UnifiedRegex::RegexPattern* splitPattern)
    {
        this->splitPattern = splitPattern;
    }

    InternalString JavascriptRegExp::GetSource() const
    {
        return GetPattern()->GetSource();
    }

    UnifiedRegex::RegexFlags JavascriptRegExp::GetFlags() const
    {
        return GetPattern()->GetFlags();
    }

    JavascriptRegExp* JavascriptRegExp::GetJavascriptRegExp(Var var, ScriptContext* scriptContext)
    {
        if (JavascriptRegExp::Is(var))
        {
            return JavascriptRegExp::FromVar(var);
        }

        if (JavascriptOperators::GetTypeId(var) == TypeIds_HostDispatch)
        {
            TypeId remoteTypeId;
            RecyclableObject* reclObj = RecyclableObject::FromVar(var);
            reclObj->GetRemoteTypeId(&remoteTypeId);
            if (remoteTypeId == TypeIds_RegEx)
            {
                return static_cast<JavascriptRegExp *>(reclObj->GetRemoteObject());
            }
        }

        return nullptr;
    }

    Var JavascriptRegExp::NewInstance(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);

        ScriptContext* scriptContext = function->GetScriptContext();

        AssertMsg(args.Info.Count > 0, "Should always have implicit 'this'");

        // SkipDefaultNewObject function flag should have prevented the default object from
        // being created, except when call true a host dispatch.
        Var newTarget = callInfo.Flags & CallFlags_NewTarget ? args.Values[args.Info.Count] : args[0];
        bool isCtorSuperCall = (callInfo.Flags & CallFlags_New) && newTarget != nullptr && RecyclableObject::Is(newTarget);
        Assert(isCtorSuperCall || !(callInfo.Flags & CallFlags_New) || args[0] == nullptr
            || JavascriptOperators::GetTypeId(args[0]) == TypeIds_HostDispatch);

        UnifiedRegex::RegexPattern* pattern = nullptr;
        UnifiedRegex::RegexPattern* splitPattern = nullptr;
        JavascriptRegExp* regex = nullptr;

        if (callInfo.Count < 2)
        {
            pattern = scriptContext->GetLibrary()->GetEmptyRegexPattern();
        }
        else if (JavascriptRegExp::Is(args[1]))
        {
            if (!(callInfo.Flags & CallFlags_New) &&
                (callInfo.Count == 2 || JavascriptOperators::IsUndefinedObject(args[2], scriptContext)) &&
                regex == nullptr)
            {
                // ES5 15.10.3.1 Called as a function: If pattern R is a regexp object and flags is undefined, then return R unchanged.
                // As per ES6 21.2.3.1: We should only return pattern when the this argument is not an uninitialized RegExp object.
                //                      If regex is null, we can be sure the this argument is not initialized.
                return args[1];
            }

            JavascriptRegExp* source = JavascriptRegExp::FromVar(args[1]);

            if (callInfo.Count > 2 )
            {
                // As per ES 2015 21.2.3.1: If 1st argument is RegExp and 2nd argument is flag then return regexp with same pattern as 1st
                // argument and flags supplied by the 2nd argument.
                if (!JavascriptOperators::IsUndefinedObject(args[2], scriptContext))
                {
                    InternalString str = source->GetSource();
                    pattern = CreatePattern(JavascriptString::NewCopyBuffer(str.GetBuffer(), str.GetLength(), scriptContext),
                        args[2], scriptContext);

                    // "splitPattern" is a version of "pattern" without the sticky flag. If other flags are the same, we can safely
                    // reuse "splitPattern".
                    UnifiedRegex::RegexFlags currentSplitFlags =
                        static_cast<UnifiedRegex::RegexFlags>(source->GetPattern()->GetFlags() & ~UnifiedRegex::StickyRegexFlag);
                    UnifiedRegex::RegexFlags newSplitFlags =
                        static_cast<UnifiedRegex::RegexFlags>(pattern->GetFlags() & ~UnifiedRegex::StickyRegexFlag);
                    if (newSplitFlags == currentSplitFlags)
                    {
                        splitPattern = source->GetSplitPattern();
                    }
                }
            }
            if (!pattern)
            {
                pattern = source->GetPattern();
                splitPattern = source->GetSplitPattern();
            }

        }
        else
        {
            pattern = CreatePattern(args[1], (callInfo.Count > 2) ? args[2] : nullptr, scriptContext);
        }

        if (regex == nullptr)
        {
            regex = scriptContext->GetLibrary()->CreateRegExp(nullptr);
        }

        regex->SetPattern(pattern);
        regex->SetSplitPattern(splitPattern);

        return isCtorSuperCall ?
            JavascriptOperators::OrdinaryCreateFromConstructor(RecyclableObject::FromVar(newTarget), regex, nullptr, scriptContext) :
            regex;
    }

    UnifiedRegex::RegexPattern* JavascriptRegExp::CreatePattern(Var aValue, Var options, ScriptContext *scriptContext)
    {
        JavascriptString * strBody;

        if (JavascriptString::Is(aValue))
        {
            strBody = JavascriptString::FromVar(aValue);
        }
        else if (JavascriptOperators::GetTypeId(aValue) == TypeIds_Undefined)
        {
            strBody = scriptContext->GetLibrary()->GetEmptyString();
        }
        else
        {
            strBody = JavascriptConversion::ToString(aValue, scriptContext); // must be null terminated!
        }

        int cBody = strBody->GetLength();
        const wchar_t *szRegex = strBody->GetSz();
        int cOpts = 0;
        const wchar_t *szOptions = nullptr;

        JavascriptString * strOptions = nullptr;
        if (options != nullptr && !JavascriptOperators::IsUndefinedObject(options, scriptContext))
        {
            if (JavascriptString::Is(options))
            {
                strOptions = JavascriptString::FromVar(options);
            }
            else
            {
                strOptions = JavascriptConversion::ToString(options, scriptContext);
            }

            szOptions = strOptions->GetSz(); // must be null terminated!
            cOpts = strOptions->GetLength();
        }

        UnifiedRegex::RegexPattern* pattern = RegexHelper::CompileDynamic(scriptContext, szRegex, cBody, szOptions, cOpts, false);

        return pattern;
    }

    JavascriptRegExp* JavascriptRegExp::CreateRegEx(const wchar_t* pSource, CharCount sourceLen, UnifiedRegex::RegexFlags flags, ScriptContext *scriptContext)
    {
        UnifiedRegex::RegexPattern* pattern = RegexHelper::CompileDynamic(scriptContext, pSource, sourceLen, flags, false);

        return scriptContext->GetLibrary()->CreateRegExp(pattern);
    }

    JavascriptRegExp* JavascriptRegExp::CreateRegEx(Var aValue, Var options, ScriptContext *scriptContext)
    {
        // This is called as helper from OpCode::CoerseRegEx. If aValue is regex pattern /a/, CreatePattern converts
        // it to pattern "/a/" instead of "a". So if we know that aValue is regex, then just return the same object
        if (JavascriptRegExp::Is(aValue))
        {
            return JavascriptRegExp::FromVar(aValue);
        }
        else
        {
            UnifiedRegex::RegexPattern* pattern = CreatePattern(aValue, options, scriptContext);

            return scriptContext->GetLibrary()->CreateRegExp(pattern);
        }
    }

    void JavascriptRegExp::CacheLastIndex()
    {
        if (lastIndexVar == nullptr)
            lastIndexOrFlag = 0;
        else
        {
            // Does ToInteger(lastIndex) yield an integer in [0, MaxCharCount]?
            double v = JavascriptConversion::ToInteger(lastIndexVar, GetScriptContext());
            if (JavascriptNumber::IsNan(v))
                lastIndexOrFlag = 0;
            else if (JavascriptNumber::IsPosInf(v) ||
                JavascriptNumber::IsNegInf(v) ||
                v < 0.0 ||
                v > (double)MaxCharCount)
                lastIndexOrFlag = InvalidValue;
            else
                lastIndexOrFlag = (CharCount)v;
        }
    }

    JavascriptString *JavascriptRegExp::ToString(bool sourceOnly)
    {
        Js::InternalString str = pattern->GetSource();
        CompoundString *const builder = CompoundString::NewWithCharCapacity(str.GetLength() + 5, GetLibrary());

        if (!sourceOnly)
        {
            builder->AppendChars(L'/');
        }
        if (pattern->IsLiteral())
        {
            builder->AppendChars(str.GetBuffer(), str.GetLength());
        }
        else
        {
            // Need to ensure that the resulting static regex is functionally equivalent (as written) to 'this' regex. This
            // involves the following:
            //   - Empty regex should result in /(?:)/ rather than //, which is a comment
            //   - Unescaped '/' needs to be escaped so that it doesn't end the static regex prematurely
            //   - Line terminators need to be escaped since they're not allowed in a static regex
            if (str.GetLength() == 0)
            {
                builder->AppendChars(L"(?:)");
            }
            else
            {
                bool escape = false;
                for (charcount_t i = 0; i < str.GetLength(); ++i)
                {
                    const wchar_t c = str.GetBuffer()[i];

                    if(!escape)
                    {
                        switch(c)
                        {
                            case L'/':
                            case L'\n':
                            case L'\r':
                            case L'\x2028':
                            case L'\x2029':
                                // Unescaped '/' or line terminator needs to be escaped
                                break;

                            case L'\\':
                                // Escape sequence; the next character is escaped and shouldn't be escaped further
                                escape = true;
                                Assert(i + 1 < str.GetLength()); // cannot end in a '\'
                                // '\' is appended on the next iteration as 'escape' is true. This handles the case where we
                                // have an escaped line terminator (\<lineTerminator>), where \\n has a different meaning and we
                                // need to use \n instead.
                                continue;

                            default:
                                builder->AppendChars(c);
                                continue;
                        }
                    }
                    else
                    {
                        escape = false;
                    }

                    builder->AppendChars(L'\\');
                    switch(c)
                    {
                        // Line terminators need to be escaped. \<lineTerminator> is a special case, where \\n doesn't work
                        // since that means a '\' followed by an 'n'. We need to use \n instead.
                        case L'\n':
                            builder->AppendChars(L'n');
                            break;
                        case L'\r':
                            builder->AppendChars(L'r');
                            break;
                        case L'\x2028':
                            builder->AppendChars(L"u2028");
                            break;
                        case L'\x2029':
                            builder->AppendChars(L"u2029");
                            break;

                        default:
                            builder->AppendChars(c);
                    }
                }
            }
        }

        if (!sourceOnly)
        {
            builder->AppendChars(L'/');

            // Cross-browser compatibility - flags are listed in alphabetical order in the spec and by other browsers
            if (pattern->IsGlobal())
            {
                builder->AppendChars(L'g');
            }
            if (pattern->IsIgnoreCase())
            {
                builder->AppendChars(L'i');
            }
            if (pattern->IsMultiline())
            {
                builder->AppendChars(L'm');
            }
            if (pattern->IsUnicode())
            {
                builder->AppendChars(L'u');
            }
            if (pattern->IsSticky())
            {
                builder->AppendChars(L'y');
            }
        }

        return builder;
    }

    Var JavascriptRegExp::EntryCompile(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        // enforce 'this' arg generic
        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedRegExp, L"RegExp.prototype.compile");
        }

        JavascriptRegExp* thisRegularExpression = GetJavascriptRegExp(args[0], scriptContext);
        if (!thisRegularExpression)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedRegExp, L"RegExp.prototype.compile");
        }

        UnifiedRegex::RegexPattern* pattern;
        UnifiedRegex::RegexPattern* splitPattern = nullptr;

        if (callInfo.Count == 1 )
        {
            pattern = scriptContext->GetLibrary()->GetEmptyRegexPattern();
        }
        else if (JavascriptRegExp::Is(args[1]))
        {
            JavascriptRegExp* source = JavascriptRegExp::FromVar(args[1]);
            //compile with a regular expression
            pattern = source->GetPattern();
            splitPattern = source->GetSplitPattern();
            // second arg must be undefined if a reg expression is passed
            if(callInfo.Count > 2 &&  JavascriptOperators::GetTypeId(args[2]) != TypeIds_Undefined)
            {
                JavascriptError::ThrowSyntaxError(scriptContext, JSERR_RegExpSyntax);
            }
        }
        else
        {
            //compile with a string
            JavascriptString * strBody;
            if (JavascriptString::Is(args[1]))
            {
                strBody = JavascriptString::FromVar(args[1]);
            }
            else if(JavascriptOperators::GetTypeId(args[1]) == TypeIds_Undefined)
            {
                strBody = scriptContext->GetLibrary()->GetEmptyString();
            }
            else
            {
                strBody = JavascriptConversion::ToString(args[1], scriptContext);
            }

            int cBody = strBody->GetLength();
            const wchar_t *szRegex = strBody->GetSz(); // must be null terminated!
            int cOpts = 0;
            const wchar_t *szOptions = nullptr;

            JavascriptString * strOptions = nullptr;
            if (callInfo.Count > 2 && !JavascriptOperators::IsUndefinedObject(args[2], scriptContext))
            {
                if (JavascriptString::Is(args[2]))
                {
                    strOptions = JavascriptString::FromVar(args[2]);
                }
                else
                {
                    strOptions = JavascriptConversion::ToString(args[2], scriptContext);
                }

                szOptions = strOptions->GetSz(); // must be null terminated!
                cOpts = strOptions->GetLength();
            }
            pattern = RegexHelper::CompileDynamic(scriptContext, szRegex, cBody, szOptions, cOpts, false);
        }

        thisRegularExpression->SetPattern(pattern);
        thisRegularExpression->SetSplitPattern(splitPattern);
        thisRegularExpression->SetLastIndex(0);
        return thisRegularExpression;
    }

    Var JavascriptRegExp::OP_NewRegEx(Var aCompiledRegex, ScriptContext* scriptContext)
    {
        JavascriptRegExp * pNewInstance =
            RecyclerNew(scriptContext->GetRecycler(),JavascriptRegExp,((UnifiedRegex::RegexPattern*)aCompiledRegex),
            scriptContext->GetLibrary()->GetRegexType());
        return pNewInstance;
    }

    Var JavascriptRegExp::EntryExec(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        // enforce 'this' arg generic
        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedRegExp, L"RegExp.prototype.exec");
        }

        JavascriptRegExp * pRegEx = GetJavascriptRegExp(args[0], scriptContext);
        if (!pRegEx)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedRegExp, L"RegExp.prototype.exec");
        }

        JavascriptString * pStr;
        if(args.Info.Count == 1)
        {
            pStr = scriptContext->GetLibrary()->GetUndefinedDisplayString();
        }
        else if (JavascriptString::Is(args[1]))
        {
            pStr = JavascriptString::FromVar(args[1]);
        }
        else
        {
            pStr = JavascriptConversion::ToString(args[1], scriptContext);
        }

        return RegexHelper::RegexExec(scriptContext, pRegEx, pStr, RegexHelper::IsResultNotUsed(callInfo.Flags));
    }

    Var JavascriptRegExp::EntryTest(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();
        Assert(!(callInfo.Flags & CallFlags_New));

        // enforce 'this' arg generic
        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedRegExp, L"RegExp.prototype.test");
        }

        JavascriptRegExp* pRegEx = GetJavascriptRegExp(args[0], scriptContext);
        if (!pRegEx)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedRegExp, L"RegExp.prototype.test");
        }
        JavascriptString * pStr;

        if(args.Info.Count == 1)
        {
            pStr = scriptContext->GetLibrary()->GetUndefinedDisplayString();
        }
        else if (JavascriptString::Is(args[1]))
        {
            pStr = JavascriptString::FromVar(args[1]);
        }
        else
        {
            pStr = JavascriptConversion::ToString(args[1], scriptContext);
        }

        BOOL result = RegexHelper::RegexTest(scriptContext, pRegEx, pStr);

        return JavascriptBoolean::ToVar(result, scriptContext);
    }

    Var JavascriptRegExp::EntryToString(RecyclableObject* function, CallInfo callInfo, ...)
    {
        PROBE_STACK(function->GetScriptContext(), Js::Constants::MinStackDefault);

        ARGUMENTS(args, callInfo);
        ScriptContext* scriptContext = function->GetScriptContext();

        Assert(!(callInfo.Flags & CallFlags_New));

        // enforce 'this' arg generic
        if (args.Info.Count == 0)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedRegExp, L"RegExp.prototype.toString");
        }

        JavascriptRegExp* obj = GetJavascriptRegExp(args[0], scriptContext);
        if (!obj)
        {
            JavascriptError::ThrowTypeError(scriptContext, JSERR_This_NeedRegExp, L"RegExp.prototype.toString");
        }

        return obj->ToString();
    }

    Var JavascriptRegExp::EntryGetterSymbolSpecies(RecyclableObject* function, CallInfo callInfo, ...)
    {
        ARGUMENTS(args, callInfo);

        Assert(args.Info.Count > 0);

        return args[0];
    }

    JavascriptRegExp * JavascriptRegExp::BoxStackInstance(JavascriptRegExp * instance)
    {
        Assert(ThreadContext::IsOnStack(instance));
        // On the stack, the we reserved a pointer before the object as to store the boxed value
        JavascriptRegExp ** boxedInstanceRef = ((JavascriptRegExp **)instance) - 1;
        JavascriptRegExp * boxedInstance = *boxedInstanceRef;
        if (boxedInstance)
        {
            return boxedInstance;
        }
        Assert(instance->GetTypeHandler()->GetInlineSlotsSize() == 0);
        boxedInstance = RecyclerNew(instance->GetRecycler(), JavascriptRegExp, instance);
        *boxedInstanceRef = boxedInstance;
        return boxedInstance;
    }

    // Both 'unicode' and 'sticky' special properties could be controlled via config
    // flags. Below, 'sticky' is listed after 'unicode' (for no special reason).
    // When the 'unicode' config flag is disabled, we want 'unicode' to be excluded
    // from the list, but we still want 'sticky' to be included depending on its
    // config flag. That's the reason why we have two lists for the special property
    // IDs.
    #define DEFAULT_SPECIAL_PROPERTY_IDS \
        PropertyIds::lastIndex, \
        PropertyIds::global, \
        PropertyIds::multiline, \
        PropertyIds::ignoreCase, \
        PropertyIds::source, \
        PropertyIds::options
    PropertyId const JavascriptRegExp::specialPropertyIdsAll[] =
    {
        DEFAULT_SPECIAL_PROPERTY_IDS,
        PropertyIds::unicode,
        PropertyIds::sticky
    };
    PropertyId const JavascriptRegExp::specialPropertyIdsWithoutUnicode[] =
    {
        DEFAULT_SPECIAL_PROPERTY_IDS,
        PropertyIds::sticky
    };

    BOOL JavascriptRegExp::HasProperty(PropertyId propertyId)
    {
        switch (propertyId)
        {
        case PropertyIds::lastIndex:
        case PropertyIds::global:
        case PropertyIds::multiline:
        case PropertyIds::ignoreCase:
        case PropertyIds::source:
        case PropertyIds::options:
            return true;
        case PropertyIds::unicode:
            if (this->GetScriptContext()->GetConfig()->IsES6UnicodeExtensionsEnabled())
            {
                return true;
            }
            return DynamicObject::HasProperty(propertyId);
        case PropertyIds::sticky:
            if (this->GetScriptContext()->GetConfig()->IsES6RegExStickyEnabled())
            {
                return true;
            }
            return DynamicObject::HasProperty(propertyId);
        default:
            return DynamicObject::HasProperty(propertyId);
        }
    }

    BOOL JavascriptRegExp::GetPropertyReference(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        return JavascriptRegExp::GetProperty(originalInstance, propertyId, value, info, requestContext);
    }

    BOOL JavascriptRegExp::GetProperty(Var originalInstance, PropertyId propertyId, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        BOOL result;
        if (GetPropertyBuiltIns(propertyId, value, &result))
        {
            return result;
        }

        return DynamicObject::GetProperty(originalInstance, propertyId, value, info, requestContext);
    }

    BOOL JavascriptRegExp::GetProperty(Var originalInstance, JavascriptString* propertyNameString, Var* value, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        BOOL result;
        PropertyRecord const* propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, &result))
        {
            return result;
        }

        return DynamicObject::GetProperty(originalInstance, propertyNameString, value, info, requestContext);
    }

    bool JavascriptRegExp::GetPropertyBuiltIns(PropertyId propertyId, Var* value, BOOL* result)
    {
        switch (propertyId)
        {
        case PropertyIds::lastIndex:
            if (this->lastIndexVar == nullptr)
            {
                Assert(lastIndexOrFlag <= MaxCharCount);
                this->lastIndexVar = JavascriptNumber::ToVar(lastIndexOrFlag, GetScriptContext());
            }
            *value = this->lastIndexVar;
            *result = true;
            return true;
        case PropertyIds::global:
            *value = this->GetLibrary()->CreateBoolean(this->GetPattern()->IsGlobal());
            *result = true;
            return true;
        case PropertyIds::multiline:
            *value = this->GetLibrary()->CreateBoolean(this->GetPattern()->IsMultiline());
            *result = true;
            return true;
        case PropertyIds::ignoreCase:
            *value = this->GetLibrary()->CreateBoolean(this->GetPattern()->IsIgnoreCase());
            *result = true;
            return true;
        case PropertyIds::unicode:
            if (GetScriptContext()->GetConfig()->IsES6UnicodeExtensionsEnabled())
            {
                *value = this->GetLibrary()->CreateBoolean(this->GetPattern()->IsUnicode());
                *result = true;
                return true;
            }
            else
            {
                return false;
            }
        case PropertyIds::sticky:
            if (GetScriptContext()->GetConfig()->IsES6RegExStickyEnabled())
            {
                *value = this->GetLibrary()->CreateBoolean(this->GetPattern()->IsSticky());
                *result = true;
                return true;
            }
            else
            {
                return false;
            }
        case PropertyIds::source:
            {
                *value = this->ToString(true);
                *result = true;
                return true;
            }
        case PropertyIds::options:
        {
            ScriptContext* scriptContext = this->GetLibrary()->GetScriptContext();
            BEGIN_TEMP_ALLOCATOR(tempAlloc, scriptContext, L"JavascriptRegExp")
            {
                StringBuilder<ArenaAllocator> bs(tempAlloc, 4);

                if(GetPattern()->IsGlobal())
                {
                    bs.Append(L'g');
                }
                if(GetPattern()->IsIgnoreCase())
                {
                    bs.Append(L'i');
                }
                if(GetPattern()->IsMultiline())
                {
                    bs.Append(L'm');
                }
                if (GetPattern()->IsUnicode())
                {
                    bs.Append(L'u');
                }
                if (GetPattern()->IsSticky())
                {
                    bs.Append(L'y');
                }
                *value = Js::JavascriptString::NewCopyBuffer(bs.Detach(), bs.Count(), scriptContext);
            }
            END_TEMP_ALLOCATOR(tempAlloc, scriptContext);
            *result = true;
            return true;
        }
        default:
            return false;
        }
    }

    BOOL JavascriptRegExp::SetProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        BOOL result;
        if (SetPropertyBuiltIns(propertyId, value, flags, &result))
        {
            return result;
        }

        return DynamicObject::SetProperty(propertyId, value, flags, info);
    }

    BOOL JavascriptRegExp::SetProperty(JavascriptString* propertyNameString, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        BOOL result;
        PropertyRecord const * propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && SetPropertyBuiltIns(propertyRecord->GetPropertyId(), value, flags, &result))
        {
            return result;
        }

        return DynamicObject::SetProperty(propertyNameString, value, flags, info);
    }

    bool JavascriptRegExp::SetPropertyBuiltIns(PropertyId propertyId, Var value, PropertyOperationFlags flags, BOOL* result)
    {
        switch (propertyId)
        {
        case PropertyIds::lastIndex:
            this->lastIndexVar = value;
            lastIndexOrFlag = NotCachedValue;
            *result = true;
            return true;
        case PropertyIds::global:
        case PropertyIds::multiline:
        case PropertyIds::ignoreCase:
        case PropertyIds::source:
        case PropertyIds::options:
            JavascriptError::ThrowCantAssignIfStrictMode(flags, this->GetScriptContext());
            *result = false;
            return true;
        case PropertyIds::unicode:
            if (this->GetScriptContext()->GetConfig()->IsES6UnicodeExtensionsEnabled())
            {
                JavascriptError::ThrowCantAssignIfStrictMode(flags, this->GetScriptContext());
                *result = false;
                return true;
            }
            return false;
        case PropertyIds::sticky:
            if (this->GetScriptContext()->GetConfig()->IsES6RegExStickyEnabled())
            {
                JavascriptError::ThrowCantAssignIfStrictMode(flags, this->GetScriptContext());
                *result = false;
                return true;
            }
            return false;
        default:
            return false;
        }
    }

    BOOL JavascriptRegExp::InitProperty(PropertyId propertyId, Var value, PropertyOperationFlags flags, PropertyValueInfo* info)
    {
        return SetProperty(propertyId, value, flags, info);
    }

    BOOL JavascriptRegExp::DeleteProperty(PropertyId propertyId, PropertyOperationFlags propertyOperationFlags)
    {
        switch (propertyId)
        {
        case PropertyIds::lastIndex:
        case PropertyIds::global:
        case PropertyIds::multiline:
        case PropertyIds::ignoreCase:
        case PropertyIds::source:
        case PropertyIds::options:
            JavascriptError::ThrowCantDeleteIfStrictMode(propertyOperationFlags, this->GetScriptContext(), this->GetScriptContext()->GetPropertyName(propertyId)->GetBuffer());
            return false;
        case PropertyIds::unicode:
            if (this->GetScriptContext()->GetConfig()->IsES6UnicodeExtensionsEnabled())
            {
                JavascriptError::ThrowCantDeleteIfStrictMode(propertyOperationFlags, this->GetScriptContext(), this->GetScriptContext()->GetPropertyName(propertyId)->GetBuffer());
                return false;
            }
            return DynamicObject::DeleteProperty(propertyId, propertyOperationFlags);
        case PropertyIds::sticky:
            if (this->GetScriptContext()->GetConfig()->IsES6RegExStickyEnabled())
            {
                JavascriptError::ThrowCantDeleteIfStrictMode(propertyOperationFlags, this->GetScriptContext(), this->GetScriptContext()->GetPropertyName(propertyId)->GetBuffer());
                return false;
            }
            return DynamicObject::DeleteProperty(propertyId, propertyOperationFlags);
        default:
            return DynamicObject::DeleteProperty(propertyId, propertyOperationFlags);
        }
    }

    DescriptorFlags JavascriptRegExp::GetSetter(PropertyId propertyId, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        DescriptorFlags result;
        if (GetSetterBuiltIns(propertyId, info, &result))
        {
            return result;
        }

        return DynamicObject::GetSetter(propertyId, setterValue, info, requestContext);
    }

    DescriptorFlags JavascriptRegExp::GetSetter(JavascriptString* propertyNameString, Var* setterValue, PropertyValueInfo* info, ScriptContext* requestContext)
    {
        DescriptorFlags result;
        PropertyRecord const * propertyRecord;
        this->GetScriptContext()->FindPropertyRecord(propertyNameString, &propertyRecord);

        if (propertyRecord != nullptr && GetSetterBuiltIns(propertyRecord->GetPropertyId(), info, &result))
        {
            return result;
        }

        return DynamicObject::GetSetter(propertyNameString, setterValue, info, requestContext);
    }

    bool JavascriptRegExp::GetSetterBuiltIns(PropertyId propertyId, PropertyValueInfo* info, DescriptorFlags* result)
    {
        switch (propertyId)
        {
        case PropertyIds::lastIndex:
        case PropertyIds::global:
        case PropertyIds::multiline:
        case PropertyIds::ignoreCase:
        case PropertyIds::source:
        case PropertyIds::options:
            PropertyValueInfo::SetNoCache(info, this);
            *result = JavascriptRegExp::IsWritable(propertyId) ? WritableData : Data;
            return true;
        case PropertyIds::unicode:
            if (this->GetScriptContext()->GetConfig()->IsES6UnicodeExtensionsEnabled())
            {
                PropertyValueInfo::SetNoCache(info, this);
                *result = JavascriptRegExp::IsWritable(propertyId) ? WritableData : Data;
                return true;
            }
            return false;
        case PropertyIds::sticky:
            if (this->GetScriptContext()->GetConfig()->IsES6RegExStickyEnabled())
            {
                PropertyValueInfo::SetNoCache(info, this);
                *result = JavascriptRegExp::IsWritable(propertyId) ? WritableData : Data;
                return true;
            }
            return false;
        default:
            return false;
        }
    }

    BOOL JavascriptRegExp::GetDiagValueString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        Js::InternalString str = pattern->GetSource();
        stringBuilder->Append(str.GetBuffer(), str.GetLength());
        return TRUE;
    }

    BOOL JavascriptRegExp::GetDiagTypeString(StringBuilder<ArenaAllocator>* stringBuilder, ScriptContext* requestContext)
    {
        stringBuilder->AppendCppLiteral(JS_DIAG_TYPE_JavascriptRegExp);
        return TRUE;
    }

    BOOL JavascriptRegExp::IsEnumerable(PropertyId propertyId)
    {
        switch (propertyId)
        {
        case PropertyIds::lastIndex:
        case PropertyIds::global:
        case PropertyIds::multiline:
        case PropertyIds::ignoreCase:
        case PropertyIds::source:
        case PropertyIds::options:
            return false;
        case PropertyIds::unicode:
            if (this->GetScriptContext()->GetConfig()->IsES6UnicodeExtensionsEnabled())
            {
                return false;
            }
            return DynamicObject::IsEnumerable(propertyId);
        case PropertyIds::sticky:
            if (this->GetScriptContext()->GetConfig()->IsES6RegExStickyEnabled())
            {
                return false;
            }
            return DynamicObject::IsEnumerable(propertyId);
        default:
            return DynamicObject::IsEnumerable(propertyId);
        }
    }

    BOOL JavascriptRegExp::IsConfigurable(PropertyId propertyId)
    {
        switch (propertyId)
        {
        case PropertyIds::lastIndex:
        case PropertyIds::global:
        case PropertyIds::multiline:
        case PropertyIds::ignoreCase:
        case PropertyIds::source:
        case PropertyIds::options:
            return false;
        case PropertyIds::unicode:
            if (this->GetScriptContext()->GetConfig()->IsES6UnicodeExtensionsEnabled())
            {
                return false;
            }
            return DynamicObject::IsConfigurable(propertyId);
        case PropertyIds::sticky:
            if (this->GetScriptContext()->GetConfig()->IsES6RegExStickyEnabled())
            {
                return false;
            }
            return DynamicObject::IsConfigurable(propertyId);
        default:
            return DynamicObject::IsConfigurable(propertyId);
        }
    }

    BOOL JavascriptRegExp::IsWritable(PropertyId propertyId)
    {
        switch (propertyId)
        {
        case PropertyIds::lastIndex:
            return true;
        case PropertyIds::global:
        case PropertyIds::multiline:
        case PropertyIds::ignoreCase:
        case PropertyIds::source:
        case PropertyIds::options:
            return false;
        case PropertyIds::unicode:
            if (this->GetScriptContext()->GetConfig()->IsES6UnicodeExtensionsEnabled())
            {
                return false;
            }
            return DynamicObject::IsWritable(propertyId);
        case PropertyIds::sticky:
            if (this->GetScriptContext()->GetConfig()->IsES6RegExStickyEnabled())
            {
                return false;
            }
            return DynamicObject::IsWritable(propertyId);
        default:
            return DynamicObject::IsWritable(propertyId);
        }
    }
    BOOL JavascriptRegExp::GetSpecialPropertyName(uint32 index, Var *propertyName, ScriptContext * requestContext)
    {
        uint length = GetSpecialPropertyCount();

        if (index < length)
        {
            *propertyName = requestContext->GetPropertyString(GetSpecialPropertyIdsInlined()[index]);
            return true;
        }
        return false;
    }

    // Returns the number of special non-enumerable properties this type has.
    uint JavascriptRegExp::GetSpecialPropertyCount() const
    {
        uint specialPropertyCount = defaultSpecialPropertyIdsCount;

        if (GetScriptContext()->GetConfig()->IsES6UnicodeExtensionsEnabled())
        {
            specialPropertyCount += 1;
        }

        if (GetScriptContext()->GetConfig()->IsES6RegExStickyEnabled())
        {
            specialPropertyCount += 1;
        }

        return specialPropertyCount;
    }

    // Returns the list of special non-enumerable properties for the type.
    PropertyId const * JavascriptRegExp::GetSpecialPropertyIds() const
    {
        return GetSpecialPropertyIdsInlined();
    }

    inline PropertyId const * JavascriptRegExp::GetSpecialPropertyIdsInlined() const
    {
        return GetScriptContext()->GetConfig()->IsES6UnicodeExtensionsEnabled()
            ? specialPropertyIdsAll
            : specialPropertyIdsWithoutUnicode;
    }
} // namespace Js
