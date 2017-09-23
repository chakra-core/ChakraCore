//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

// JavascriptLibraryBase.h is used by static lib shared between Trident and Chakra. We need to keep
// the size consistent and try not to change its size. We need to have matching mshtml.dll
// if the size changed here.
#pragma once

namespace Js
{
    class EngineInterfaceObject;

    class JavascriptLibraryBase : public FinalizableObject
    {
        friend class JavascriptLibrary;
        friend class ScriptSite;
    public:
        JavascriptLibraryBase(GlobalObject* globalObject):
            globalObject(globalObject)
        {
        }
        Var GetPI() { return pi; }
        Var GetNaN() { return nan; }
        Var GetNegativeInfinite() { return negativeInfinite; }
        Var GetPositiveInfinite() { return positiveInfinite; }
        Var GetMaxValue() { return maxValue; }
        Var GetMinValue() { return minValue; }
        Var GetNegativeZero() { return negativeZero; }
        RecyclableObject* GetUndefined() { return undefinedValue; }
        RecyclableObject* GetNull() { return nullValue; }
        JavascriptBoolean* GetTrue() { return booleanTrue; }
        JavascriptBoolean* GetFalse() { return booleanFalse; }

        JavascriptSymbol* GetSymbolHasInstance() { return symbolHasInstance; }
        JavascriptSymbol* GetSymbolIsConcatSpreadable() { return symbolIsConcatSpreadable; }
        JavascriptSymbol* GetSymbolIterator() { return symbolIterator; }
        JavascriptSymbol* GetSymbolToPrimitive() { return symbolToPrimitive; }
        JavascriptSymbol* GetSymbolToStringTag() { return symbolToStringTag; }
        JavascriptSymbol* GetSymbolUnscopables() { return symbolUnscopables; }

        JavascriptFunction* GetObjectConstructor() { return objectConstructor; }
        JavascriptFunction* GetArrayConstructor() { return arrayConstructor; }
        JavascriptFunction* GetBooleanConstructor() { return booleanConstructor; }
        JavascriptFunction* GetDateConstructor() { return dateConstructor; }
        JavascriptFunction* GetFunctionConstructor() { return functionConstructor; }
        JavascriptFunction* GetNumberConstructor() { return numberConstructor; }
        JavascriptRegExpConstructor* GetRegExpConstructor() { return regexConstructor; }
        JavascriptFunction* GetStringConstructor() { return stringConstructor; }
        JavascriptFunction* GetArrayBufferConstructor() { return arrayBufferConstructor; }
        JavascriptFunction* GetPixelArrayConstructor() { return pixelArrayConstructor; }
        JavascriptFunction* GetTypedArrayConstructor() const { return typedArrayConstructor; }
        JavascriptFunction* GetInt8ArrayConstructor() { return Int8ArrayConstructor; }
        JavascriptFunction* GetUint8ArrayConstructor() { return Uint8ArrayConstructor; }
        JavascriptFunction* GetUint8ClampedArrayConstructor() { return Uint8ClampedArrayConstructor; }
        JavascriptFunction* GetInt16ArrayConstructor() { return Int16ArrayConstructor; }
        JavascriptFunction* GetUint16ArrayConstructor() { return Uint16ArrayConstructor; }
        JavascriptFunction* GetInt32ArrayConstructor() { return Int32ArrayConstructor; }
        JavascriptFunction* GetUint32ArrayConstructor() { return Uint32ArrayConstructor; }
        JavascriptFunction* GetFloat32ArrayConstructor() { return Float32ArrayConstructor; }
        JavascriptFunction* GetFloat64ArrayConstructor() { return Float64ArrayConstructor; }
        JavascriptFunction* GetMapConstructor() { return mapConstructor; }
        JavascriptFunction* GetSetConstructor() { return setConstructor; }
        JavascriptFunction* GetWeakMapConstructor() { return weakMapConstructor; }
        JavascriptFunction* GetWeakSetConstructor() { return weakSetConstructor; }
        JavascriptFunction* GetSymbolConstructor() { return symbolConstructor; }
        JavascriptFunction* GetProxyConstructor() const { return proxyConstructor; }
        JavascriptFunction* GetPromiseConstructor() const { return promiseConstructor; }
        JavascriptFunction* GetGeneratorFunctionConstructor() const { return generatorFunctionConstructor; }
        JavascriptFunction* GetAsyncFunctionConstructor() const { return asyncFunctionConstructor; }

        JavascriptFunction* GetErrorConstructor() const { return errorConstructor; }
        JavascriptFunction* GetEvalErrorConstructor() const { return evalErrorConstructor; }
        JavascriptFunction* GetRangeErrorConstructor() const { return rangeErrorConstructor; }
        JavascriptFunction* GetReferenceErrorConstructor() const { return referenceErrorConstructor; }
        JavascriptFunction* GetSyntaxErrorConstructor() const { return syntaxErrorConstructor; }
        JavascriptFunction* GetTypeErrorConstructor() const { return typeErrorConstructor; }
        JavascriptFunction* GetURIErrorConstructor() const { return uriErrorConstructor; }
        JavascriptFunction* GetPromiseResolve() const { return promiseResolveFunction; }
        JavascriptFunction* GetPromiseThen() const { return promiseThenFunction; }
        JavascriptFunction* GetJSONStringify() const { return jsonStringifyFunction; }
        JavascriptFunction* GetObjectFreeze() const { return objectFreezeFunction; }
        JavascriptFunction* GetDebugEval() const { return debugEval; }
        JavascriptFunction* GetStackTraceFunction() const { return getStackTrace; }
#ifdef EDIT_AND_CONTINUE
        JavascriptFunction* GetEditSource() const { return editSource; }
#endif

        JavascriptFunction* GetArrayPrototypeForEachFunction() const { return arrayPrototypeForEachFunction; }
        JavascriptFunction* GetArrayPrototypeKeysFunction() const { return arrayPrototypeKeysFunction; }
        JavascriptFunction* GetArrayPrototypeValuesFunction() const { return arrayPrototypeValuesFunction; }
        JavascriptFunction* GetArrayPrototypeEntriesFunction() const { return arrayPrototypeEntriesFunction; }

        DynamicObject* GetMathObject() { return mathObject; }
        DynamicObject* GetJSONObject() { return JSONObject; }
#ifdef ENABLE_INTL_OBJECT
        DynamicObject* GetINTLObject() { return IntlObject; }
#endif
#if defined(ENABLE_INTL_OBJECT) || defined(ENABLE_PROJECTION)
        EngineInterfaceObject* GetEngineInterfaceObject() { return engineInterfaceObject; }
#endif

        DynamicObject* GetArrayPrototype() { return arrayPrototype; }
        DynamicObject* GetBooleanPrototype() { return booleanPrototype; }
        DynamicObject* GetDatePrototype() { return datePrototype; }
        DynamicObject* GetFunctionPrototype() { return functionPrototype; }
        DynamicObject* GetNumberPrototype() { return numberPrototype; }
        DynamicObject* GetSIMDBool8x16Prototype()  { return simdBool8x16Prototype;  }
        DynamicObject* GetSIMDBool16x8Prototype()  { return simdBool16x8Prototype;  }
        DynamicObject* GetSIMDBool32x4Prototype()  { return simdBool32x4Prototype;  }
        DynamicObject* GetSIMDInt8x16Prototype()   { return simdInt8x16Prototype;   }
        DynamicObject* GetSIMDInt16x8Prototype()   { return simdInt16x8Prototype;   }
        DynamicObject* GetSIMDInt32x4Prototype()   { return simdInt32x4Prototype;   }
        DynamicObject* GetSIMDUint8x16Prototype()  { return simdUint8x16Prototype;  }
        DynamicObject* GetSIMDUint16x8Prototype()  { return simdUint16x8Prototype;  }
        DynamicObject* GetSIMDUint32x4Prototype()  { return simdUint32x4Prototype;  }
        DynamicObject* GetSIMDFloat32x4Prototype() { return simdFloat32x4Prototype; }
        DynamicObject* GetSIMDFloat64x2Prototype() { return simdFloat64x2Prototype; }
        ObjectPrototypeObject* GetObjectPrototypeObject() { return objectPrototype; }
        DynamicObject* GetObjectPrototype();
        DynamicObject* GetRegExpPrototype() { return regexPrototype; }
        DynamicObject* GetStringPrototype() { return stringPrototype; }
        DynamicObject* GetMapPrototype() { return mapPrototype; }
        DynamicObject* GetSetPrototype() { return setPrototype; }
        DynamicObject* GetWeakMapPrototype() { return weakMapPrototype; }
        DynamicObject* GetWeakSetPrototype() { return weakSetPrototype; }
        DynamicObject* GetSymbolPrototype() { return symbolPrototype; }
        DynamicObject* GetArrayIteratorPrototype() const { return arrayIteratorPrototype; }
        DynamicObject* GetMapIteratorPrototype() const { return mapIteratorPrototype; }
        DynamicObject* GetSetIteratorPrototype() const { return setIteratorPrototype; }
        DynamicObject* GetStringIteratorPrototype() const { return stringIteratorPrototype; }
        DynamicObject* GetIteratorPrototype() const { return iteratorPrototype; }
        DynamicObject* GetPromisePrototype() const { return promisePrototype; }
        DynamicObject* GetGeneratorFunctionPrototype() const { return generatorFunctionPrototype; }
        DynamicObject* GetGeneratorPrototype() const { return generatorPrototype; }
        DynamicObject* GetAsyncFunctionPrototype() const { return asyncFunctionPrototype; }

        DynamicObject* GetErrorPrototype() const { return errorPrototype; }
        DynamicObject* GetEvalErrorPrototype() const { return evalErrorPrototype; }
        DynamicObject* GetRangeErrorPrototype() const { return rangeErrorPrototype; }
        DynamicObject* GetReferenceErrorPrototype() const { return referenceErrorPrototype; }
        DynamicObject* GetSyntaxErrorPrototype() const { return syntaxErrorPrototype; }
        DynamicObject* GetTypeErrorPrototype() const { return typeErrorPrototype; }
        DynamicObject* GetURIErrorPrototype() const { return uriErrorPrototype; }
        PropertyId GetPropertyIdSymbolIterator() { return PropertyIds::_symbolIterator; };
        PropertyId GetPropertyIdSymbolToStringTag() { return PropertyIds::_symbolToStringTag; };

    protected:
        Field(GlobalObject*) globalObject;
        Field(RuntimeFunction*) mapConstructor;
        Field(RuntimeFunction*) setConstructor;
        Field(RuntimeFunction*) weakMapConstructor;
        Field(RuntimeFunction*) weakSetConstructor;
        Field(RuntimeFunction*) arrayConstructor;
        Field(RuntimeFunction*) typedArrayConstructor;
        Field(RuntimeFunction*) Int8ArrayConstructor;
        Field(RuntimeFunction*) Uint8ArrayConstructor;
        Field(RuntimeFunction*) Uint8ClampedArrayConstructor;
        Field(RuntimeFunction*) Int16ArrayConstructor;
        Field(RuntimeFunction*) Uint16ArrayConstructor;
        Field(RuntimeFunction*) Int32ArrayConstructor;
        Field(RuntimeFunction*) Uint32ArrayConstructor;
        Field(RuntimeFunction*) Float32ArrayConstructor;
        Field(RuntimeFunction*) Float64ArrayConstructor;
        Field(RuntimeFunction*) arrayBufferConstructor;
        Field(RuntimeFunction*) dataViewConstructor;
        Field(RuntimeFunction*) booleanConstructor;
        Field(RuntimeFunction*) dateConstructor;
        Field(RuntimeFunction*) functionConstructor;
        Field(RuntimeFunction*) numberConstructor;
        Field(RuntimeFunction*) objectConstructor;
        Field(RuntimeFunction*) symbolConstructor;
        Field(JavascriptRegExpConstructor*) regexConstructor;
        Field(RuntimeFunction*) stringConstructor;
        Field(RuntimeFunction*) pixelArrayConstructor;

        Field(RuntimeFunction*) errorConstructor;
        Field(RuntimeFunction*) evalErrorConstructor;
        Field(RuntimeFunction*) rangeErrorConstructor;
        Field(RuntimeFunction*) referenceErrorConstructor;
        Field(RuntimeFunction*) syntaxErrorConstructor;
        Field(RuntimeFunction*) typeErrorConstructor;
        Field(RuntimeFunction*) uriErrorConstructor;
        Field(RuntimeFunction*) proxyConstructor;
        Field(RuntimeFunction*) promiseConstructor;
        Field(RuntimeFunction*) generatorFunctionConstructor;
        Field(RuntimeFunction*) asyncFunctionConstructor;

        Field(JavascriptFunction*) defaultAccessorFunction;
        Field(JavascriptFunction*) stackTraceAccessorFunction;
        Field(JavascriptFunction*) throwTypeErrorRestrictedPropertyAccessorFunction;
        Field(JavascriptFunction*) debugObjectNonUserGetterFunction;
        Field(JavascriptFunction*) debugObjectNonUserSetterFunction;
        Field(JavascriptFunction*) debugObjectDebugModeGetterFunction;
        Field(JavascriptFunction*) __proto__getterFunction;
        Field(JavascriptFunction*) __proto__setterFunction;
        Field(JavascriptFunction*) arrayIteratorPrototypeBuiltinNextFunction;
        Field(JavascriptFunction*) promiseResolveFunction;
        Field(JavascriptFunction*) promiseThenFunction;
        Field(JavascriptFunction*) jsonStringifyFunction;
        Field(JavascriptFunction*) objectFreezeFunction;

        Field(DynamicObject*) mathObject;
        // SIMD_JS
        Field(DynamicObject*) simdObject;

        Field(DynamicObject*) debugObject;
        Field(DynamicObject*) JSONObject;
#ifdef ENABLE_INTL_OBJECT
        Field(DynamicObject*) IntlObject;
#endif
#if defined(ENABLE_INTL_OBJECT) || defined(ENABLE_PROJECTION)
        Field(EngineInterfaceObject*) engineInterfaceObject;
#endif
        Field(DynamicObject*) reflectObject;

        Field(DynamicObject*) arrayPrototype;

        Field(DynamicObject*) typedArrayPrototype;
        Field(DynamicObject*) Int8ArrayPrototype;
        Field(DynamicObject*) Uint8ArrayPrototype;
        Field(DynamicObject*) Uint8ClampedArrayPrototype;
        Field(DynamicObject*) Int16ArrayPrototype;
        Field(DynamicObject*) Uint16ArrayPrototype;
        Field(DynamicObject*) Int32ArrayPrototype;
        Field(DynamicObject*) Uint32ArrayPrototype;
        Field(DynamicObject*) Float32ArrayPrototype;
        Field(DynamicObject*) Float64ArrayPrototype;
        Field(DynamicObject*) Int64ArrayPrototype;
        Field(DynamicObject*) Uint64ArrayPrototype;
        Field(DynamicObject*) BoolArrayPrototype;
        Field(DynamicObject*) CharArrayPrototype;
        Field(DynamicObject*) arrayBufferPrototype;
        Field(DynamicObject*) dataViewPrototype;
        Field(DynamicObject*) pixelArrayPrototype;
        Field(DynamicObject*) booleanPrototype;
        Field(DynamicObject*) datePrototype;
        Field(DynamicObject*) functionPrototype;
        Field(DynamicObject*) numberPrototype;
        Field(ObjectPrototypeObject*) objectPrototype;
        Field(DynamicObject*) regexPrototype;
        Field(DynamicObject*) stringPrototype;
        Field(DynamicObject*) mapPrototype;
        Field(DynamicObject*) setPrototype;
        Field(DynamicObject*) weakMapPrototype;
        Field(DynamicObject*) weakSetPrototype;
        Field(DynamicObject*) symbolPrototype;
        Field(DynamicObject*) iteratorPrototype;           // aka %IteratorPrototype%
        Field(DynamicObject*) arrayIteratorPrototype;
        Field(DynamicObject*) mapIteratorPrototype;
        Field(DynamicObject*) setIteratorPrototype;
        Field(DynamicObject*) stringIteratorPrototype;
        Field(DynamicObject*) promisePrototype;
        Field(DynamicObject*) generatorFunctionPrototype;  // aka %Generator%
        Field(DynamicObject*) generatorPrototype;          // aka %GeneratorPrototype%
        Field(DynamicObject*) asyncFunctionPrototype;      // aka %AsyncFunctionPrototype%

        Field(DynamicObject*) errorPrototype;
        Field(DynamicObject*) evalErrorPrototype;
        Field(DynamicObject*) rangeErrorPrototype;
        Field(DynamicObject*) referenceErrorPrototype;
        Field(DynamicObject*) syntaxErrorPrototype;
        Field(DynamicObject*) typeErrorPrototype;
        Field(DynamicObject*) uriErrorPrototype;

        //SIMD Prototypes
        Field(DynamicObject*) simdBool8x16Prototype;
        Field(DynamicObject*) simdBool16x8Prototype;
        Field(DynamicObject*) simdBool32x4Prototype;
        Field(DynamicObject*) simdInt8x16Prototype;
        Field(DynamicObject*) simdInt16x8Prototype;
        Field(DynamicObject*) simdInt32x4Prototype;
        Field(DynamicObject*) simdUint8x16Prototype;
        Field(DynamicObject*) simdUint16x8Prototype;
        Field(DynamicObject*) simdUint32x4Prototype;
        Field(DynamicObject*) simdFloat32x4Prototype;
        Field(DynamicObject*) simdFloat64x2Prototype;

        Field(JavascriptBoolean*) booleanTrue;
        Field(JavascriptBoolean*) booleanFalse;

        Field(Var) nan;
        Field(Var) negativeInfinite;
        Field(Var) positiveInfinite;
        Field(Var) pi;
        Field(Var) minValue;
        Field(Var) maxValue;
        Field(Var) negativeZero;
        Field(RecyclableObject*) undefinedValue;
        Field(RecyclableObject*) nullValue;

        Field(JavascriptSymbol*) symbolHasInstance;
        Field(JavascriptSymbol*) symbolIsConcatSpreadable;
        Field(JavascriptSymbol*) symbolIterator;
        Field(JavascriptSymbol*) symbolSpecies;
        Field(JavascriptSymbol*) symbolToPrimitive;
        Field(JavascriptSymbol*) symbolToStringTag;
        Field(JavascriptSymbol*) symbolUnscopables;
        Field(JavascriptFunction*) arrayPrototypeForEachFunction;
        Field(JavascriptFunction*) arrayPrototypeKeysFunction;
        Field(JavascriptFunction*) arrayPrototypeValuesFunction;
        Field(JavascriptFunction*) arrayPrototypeEntriesFunction;

    public:
        Field(ScriptContext*) scriptContext;

    private:
        virtual void Dispose(bool isShutdown) override;
        virtual void Finalize(bool isShutdown) override;
        virtual void Mark(Recycler *recycler) override { AssertMsg(false, "Mark called on object that isn't TrackableObject"); }

    protected:
        Field(JavascriptFunction*) debugEval;
        Field(JavascriptFunction*) getStackTrace;
#ifdef EDIT_AND_CONTINUE
        Field(JavascriptFunction*) editSource;
#endif
    };
}
