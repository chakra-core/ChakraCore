//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
#ifdef ENABLE_INTL_OBJECT
namespace Js
{
    enum IntlInitializationType : USHORT
    {
        Intl = 0,
        StringPrototype,
        DatePrototype,
        NumberPrototype,
        Classifier
    };

    class IntlEngineInterfaceExtensionObject : public EngineExtensionObjectBase
    {
    public:
        IntlEngineInterfaceExtensionObject(ScriptContext* scriptContext);
        void Initialize();
        void InjectIntlLibraryCode(_In_ ScriptContext * scriptContext, DynamicObject* intlObject, IntlInitializationType intlInitializationType);

        JavascriptFunction* GetDateToLocaleString() { return dateToLocaleString; }
        JavascriptFunction* GetDateToLocaleTimeString() { return dateToLocaleTimeString; }
        JavascriptFunction* GetDateToLocaleDateString() { return dateToLocaleDateString; }
        JavascriptFunction* GetNumberToLocaleString() { return numberToLocaleString; }
        JavascriptFunction* GetStringLocaleCompare() { return stringLocaleCompare; }
        static bool __cdecl InitializeIntlNativeInterfaces(DynamicObject* intlNativeInterfaces, DeferredTypeHandlerBase * typeHandler, DeferredInitializeMode mode);

#if DBG
        void DumpByteCode() override;
#endif
        class EntryInfo
        {
        public:
#ifdef INTL_ENTRY
#undef INTL_ENTRY
#endif
#define INTL_ENTRY(id, func) \
    static NoProfileFunctionInfo Intl_##func##;
#include "IntlExtensionObjectBuiltIns.h"
#undef INTL_ENTRY
        };

#ifdef INTL_ENTRY
#undef INTL_ENTRY
#endif
#define INTL_ENTRY(id, func) \
    static Var EntryIntl_##func##(RecyclableObject* function, CallInfo callInfo, ...);
#include "IntlExtensionObjectBuiltIns.h"
#undef INTL_ENTRY

    private:
        Field(JavascriptFunction*) dateToLocaleTimeString;
        Field(JavascriptFunction*) dateToLocaleDateString;
        Field(JavascriptFunction*) numberToLocaleString;
        Field(JavascriptFunction*) stringLocaleCompare;
        Field(JavascriptFunction*) dateToLocaleString;

        Field(DynamicObject*) intlNativeInterfaces;
        Field(FunctionBody*) intlByteCode;

        Field(bool) wasInitialized;
        void EnsureIntlByteCode(_In_ ScriptContext * scriptContext);
        static void deletePrototypePropertyHelper(ScriptContext* scriptContext, DynamicObject* intlObject, Js::PropertyId objectPropertyId, Js::PropertyId getterFunctionId);

#ifdef INTL_WINGLOB
        static WindowsGlobalizationAdapter* GetWindowsGlobalizationAdapter(_In_ ScriptContext*);
        static void prepareWithFractionIntegerDigits(ScriptContext* scriptContext, Windows::Globalization::NumberFormatting::INumberRounderOption* rounderOptions,
            Windows::Globalization::NumberFormatting::INumberFormatterOptions* formatterOptions, uint16 minFractionDigits, uint16 maxFractionDigits, uint16 minIntegerDigits);
        static void prepareWithSignificantDigits(ScriptContext* scriptContext, Windows::Globalization::NumberFormatting::INumberRounderOption* rounderOptions, Windows::Globalization::NumberFormatting::INumberFormatter *numberFormatter,
            Windows::Globalization::NumberFormatting::INumberFormatterOptions* formatterOptions, uint16 minSignificantDigits, uint16 maxSignificantDigits);
#endif

        void cleanUpIntl(ScriptContext* scriptContext, DynamicObject* intlObject);

    };
}
#endif
