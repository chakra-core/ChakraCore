//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
namespace JsUtil
{
    template <class T, typename Comparer>
    class QuickSort
    {
    public:
        static void Sort(T* low, T* high, Comparer comparer, void* context)
        {
            if ((low == NULL)||(high == NULL))
            {
                return;
            }
            if (high > low)
            {
                T* pivot = Partition(low, high, comparer, context);
                Sort(low, pivot-1, comparer, context);
                Sort(pivot+1, high, comparer, context);
            }
        }

    private:
        static T* Partition(T* l, T* r, Comparer comparer, void* context)
        {
            T* i = l-1;
            T* j = r;
            for (;;)
            {
                while (comparer(context, ++i, r) < 0) ;
                while (comparer(context, r, --j) < 0) if (j == l) break;
                if (i >= j) break;
                swap(*i, *j);
            }
            swap(*i, *r);
            return i;
        }

        inline static void swap(T& x, T& y)
        {
            T temp = x;
            x = y;
            y = temp;
        }
    };
}



