//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{

class JavascriptAsyncGeneratorFunction : public JavascriptGeneratorFunction
{
  private:
      static FunctionInfo functionInfo;

      DEFINE_VTABLE_CTOR(JavascriptAsyncGeneratorFunction, JavascriptGeneratorFunction);
      DEFINE_MARSHAL_OBJECT_TO_SCRIPT_CONTEXT(JavascriptAsyncGeneratorFunction);

  protected:
      JavascriptAsyncGeneratorFunction(DynamicType* type);

  public:
      JavascriptAsyncGeneratorFunction(
          DynamicType* type,
          GeneratorVirtualScriptFunction* scriptFunction);

      static JavascriptAsyncGeneratorFunction* New(
          ScriptContext* scriptContext,
          GeneratorVirtualScriptFunction* scriptFunction);

      static Var EntryAsyncGeneratorFunctionImplementation(
          RecyclableObject* function,
          CallInfo callInfo, ...);

      inline static bool Test(JavascriptFunction *obj)
      {
          return
              VirtualTableInfo<JavascriptAsyncGeneratorFunction>::HasVirtualTable(obj) ||
              VirtualTableInfo<CrossSiteObject<JavascriptAsyncGeneratorFunction>>::HasVirtualTable(obj);
      }

#if ENABLE_TTD
      /*
      virtual TTD::NSSnapObjects::SnapObjectType GetSnapTag_TTD() const override;
      virtual void ExtractSnapObjectDataInto(TTD::NSSnapObjects::SnapObject* objData, TTD::SlabAllocator& alloc) override;
      */
#endif

  public:
      virtual VTableValue DummyVirtualFunctionToHinderLinkerICF()
      {
          return VTableValue::VtableJavascriptAsyncGeneratorFunction;
      }
};

template<>
bool VarIsImpl<JavascriptAsyncGeneratorFunction>(RecyclableObject* obj);

}
