//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    struct PropertyDescriptor
    {
    public:
        PropertyDescriptor();
        PropertyDescriptor(const PropertyDescriptor& other)
            :Value(other.Value),
            Getter(other.Getter),
            Setter(other.Setter),
            originalVar(other.originalVar),
            writableSpecified(other.writableSpecified),
            enumerableSpecified(other.enumerableSpecified),
            configurableSpecified(other.configurableSpecified),
            valueSpecified(other.valueSpecified),
            getterSpecified(other.getterSpecified),
            setterSpecified(other.setterSpecified),
            Writable(other.Writable),
            Enumerable(other.Enumerable),
            Configurable(other.Configurable),
            fromProxy(other.fromProxy)
        {
        }

    private:
        Field(Var)  Value;
        Field(Var)  Getter;
        Field(Var)  Setter;
        Field(Var)  originalVar;

        Field(bool) writableSpecified;
        Field(bool) enumerableSpecified;
        Field(bool) configurableSpecified;
        Field(bool) valueSpecified;
        Field(bool) getterSpecified;
        Field(bool) setterSpecified;

        Field(bool) Writable;
        Field(bool) Enumerable;
        Field(bool) Configurable;
        Field(bool) fromProxy;

    public:
        bool IsDataDescriptor() const { return writableSpecified | valueSpecified;}
        bool IsAccessorDescriptor() const { return getterSpecified | setterSpecified;}
        bool IsGenericDescriptor() const { return !IsAccessorDescriptor() && !IsDataDescriptor(); }
        void SetEnumerable(bool value);
        void SetWritable(bool value);
        void SetConfigurable(bool value);

        void SetValue(Var value);
        Var GetValue() const { return Value; }

        void SetGetter(Var getter);
        Var GetGetter() const { Assert(getterSpecified || Getter == nullptr);  return Getter; }
        void SetSetter(Var setter);
        Var GetSetter() const { Assert(setterSpecified || Setter == nullptr); return Setter; }

        PropertyAttributes GetAttributes() const;

        bool IsFromProxy() const { return fromProxy; }
        void SetFromProxy(bool value) { fromProxy = value; }

        void SetOriginal(Var original) { originalVar = original; }
        Var GetOriginal() const { return originalVar; }

        bool ValueSpecified() const { return valueSpecified; }
        bool WritableSpecified() const { return writableSpecified; };
        bool ConfigurableSpecified() const { return configurableSpecified; }
        bool EnumerableSpecified() const { return enumerableSpecified; }
        bool GetterSpecified() const { return getterSpecified; }
        bool SetterSpecified() const { return setterSpecified; }

        bool IsWritable() const { Assert(writableSpecified);  return Writable; }
        bool IsEnumerable() const { Assert(enumerableSpecified); return Enumerable; }
        bool IsConfigurable() const { Assert(configurableSpecified);  return Configurable; }

        // Set configurable/enumerable/writable.
        // attributes: attribute values.
        // mask: specified which attributes to set. If an attribute is not in the mask.
        void SetAttributes(PropertyAttributes attributes, PropertyAttributes mask = ~PropertyNone);

        // Merge from descriptor parameter into this but only fields specified by descriptor parameter.
        void MergeFrom(const PropertyDescriptor& descriptor);
    };
}
