//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

namespace Js
{
    typedef void(*InitIteratorFunction)(Var, Var);
    typedef bool(*NextFunction)(Var, Var *, Var *);

    enum class ExternalIteratorKind
    {
        External_Keys,
        External_Values,
        External_KeyAndValue,
    };

    class ExternalIteratorCreatorFunction : public RuntimeFunction
    {
    protected:
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(ExternalIteratorCreatorFunction);
        DEFINE_VTABLE_CTOR(ExternalIteratorCreatorFunction, RuntimeFunction);

    public:
        ExternalIteratorCreatorFunction(DynamicType* type,
            FunctionInfo* functionInfo,
            JavascriptTypeId typeId,
            uint byteCount,
            Var prototypeForIterator, InitIteratorFunction initFunction, NextFunction nextFunction);

        virtual BOOL IsExternalFunction() override { return TRUE; }

        static Var EntryExternalEntries(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryExternalKeys(RecyclableObject* function, CallInfo callInfo, ...);
        static Var EntryExternalValues(RecyclableObject* function, CallInfo callInfo, ...);

        void ThrowIfNotValidObject(Var instance);
        static Var CreateCustomExternalIterator(Var instance, ExternalIteratorCreatorFunction* function, ExternalIteratorKind kind);

        static Var CreateFunction(JavascriptLibrary *library,
            JavascriptTypeId typeId,
            JavascriptMethod entryPoint,
            uint byteCount,
            Var prototypeForIterator, InitIteratorFunction initFunction, NextFunction nextFunction);

    public:
        Field(JavascriptTypeId) m_externalTypeId;
        Field(uint) m_extraByteCount;
        Field(InitIteratorFunction) m_initFunction;
        Field(NextFunction) m_nextFunction;
        Field(Var) m_prototypeForIterator;

        friend class JavascriptLibrary;
    };

    class JavascriptExternalIteratorNextFunction : public RuntimeFunction
    {
    protected:
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptExternalIteratorNextFunction);
        DEFINE_VTABLE_CTOR(JavascriptExternalIteratorNextFunction, RuntimeFunction);

        Field(JavascriptTypeId) m_externalTypeId;

        JavascriptExternalIteratorNextFunction(DynamicType* type, FunctionInfo* functionInfo, JavascriptTypeId typeId);
    public:

        virtual BOOL IsExternalFunction() override { return TRUE; }
        JavascriptTypeId GetExternalTypeId() const { return m_externalTypeId; }

        static JavascriptExternalIteratorNextFunction* CreateFunction(JavascriptLibrary *library, JavascriptTypeId typeId, JavascriptMethod entryPoint);

        friend class JavascriptLibrary;
    };

    class CustomExternalIterator : public DynamicObject
    {
    private:
        Field(ExternalIteratorKind) m_kind;
        Field(JavascriptTypeId) m_externalTypeId;
        Field(NextFunction) m_nextFunction;

    protected:
        DEFINE_VTABLE_CTOR(CustomExternalIterator, DynamicObject);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(CustomExternalIterator);

    public:
        CustomExternalIterator(DynamicType* type, ExternalIteratorKind kind, JavascriptTypeId typeId, NextFunction nextFunction);

        static bool Is(Var aValue);
        static CustomExternalIterator* FromVar(Var aValue);
        static Var CreateNextFunction(JavascriptLibrary *library, JavascriptTypeId typeId);
        static Var EntryNext(RecyclableObject* function, CallInfo callInfo, ...);
    };

}
