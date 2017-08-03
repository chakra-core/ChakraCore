//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    class RuntimeFunction : public JavascriptFunction
    {
    protected:
        DEFINE_VTABLE_CTOR(RuntimeFunction, JavascriptFunction);
        DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(RuntimeFunction);
        RuntimeFunction(DynamicType * type);
    public:
        RuntimeFunction(DynamicType * type, FunctionInfo * functionInfo);
        RuntimeFunction(DynamicType * type, FunctionInfo * functionInfo, ConstructorCache* cache);

        void SetFunctionNameId(Var nameId);

        // This is for cached source string for the function. Possible values are:
        // NULL; initialized for anonymous methods.
        // propertyId in Int31 format; this is used for fastDOM function as well as library function
        // JavascriptString: composed using functionname from the propertyId, or fixed string for anonymous functions.
        // NOTE: This has a side-effect that after toString() is called for the first time on a built-in function the functionNameId gets replaced with a string like "function foo() { native code }".
        // As a result any code like debugger(F12) that shows the functionNameId to the user will need to pre-process this string as it may not be desirable to use this as-is in some cases.
        // See RuntimeFunction::EnsureSourceString() for details.
        Field(Var) functionNameId;
        virtual Var GetSourceString() const { return functionNameId; }
        virtual Var EnsureSourceString();

#if ENABLE_TTD
    public:
        virtual void MarkVisitKindSpecificPtrs(TTD::SnapshotExtractor* extractor) override;

        virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
        virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
#endif
    };
};
