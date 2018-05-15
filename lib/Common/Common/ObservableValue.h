//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once

template<class T>
class ObservableValueObserver
{
public:
    virtual void ValueChanged(const T& newVal, const T& oldVal) = 0;
};


template<class T>
class ObservableValue final
{
private:
    T value;
    ObservableValueObserver<T>* observer;

    void SetValue(const T newValue)
    {
        if (observer != nullptr && newValue != this->value)
        {
            observer->ValueChanged(newValue, this->value);
        }
        this->value = newValue;
    }

public:
    ObservableValue(T val, ObservableValueObserver<T>* observer) :
        observer(observer),
        value(val)
    {
    }

    ObservableValue(ObservableValue<T>& other) = delete;

    void operator= (const T value)
    {
        this->SetValue(value);
    }

    operator T() const
    {
        return this->value;
    }
};

