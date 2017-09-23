//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#pragma once
namespace JsUtil
{

// Sorting Networks - BEGIN
#define CCQ_SORT2(a, b)                 \
    if (comparer(context,               \
        base + (b * size),              \
        base + (a * size)) < 0)         \
    {                                   \
        CCQ_SWAP(base + (a * size),     \
          base + (b * size), size);     \
    }

#define CCQ_SORT3() \
    CCQ_SORT2(0, 1) \
    CCQ_SORT2(1, 2) \
    CCQ_SORT2(0, 1)

#define CCQ_SORT4() \
    CCQ_SORT2(0, 1) \
    CCQ_SORT2(2, 3) \
    CCQ_SORT2(0, 2) \
    CCQ_SORT2(1, 3) \
    CCQ_SORT2(1, 2)

#define CCQ_SORT5(a, b, c, d, e) \
    CCQ_SORT2(a, b) \
    CCQ_SORT2(d, e) \
    CCQ_SORT2(c, e) \
    CCQ_SORT2(c, d) \
    CCQ_SORT2(a, d) \
    CCQ_SORT2(a, c) \
    CCQ_SORT2(b, e) \
    CCQ_SORT2(b, d) \
    CCQ_SORT2(b, c)

#define CCQ_SORT6()  \
    CCQ_SORT2(1, 2)  \
    CCQ_SORT2(0, 2)  \
    CCQ_SORT2(0, 1)  \
    CCQ_SORT2(4, 5)  \
    CCQ_SORT2(3, 5)  \
    CCQ_SORT2(3, 4)  \
    CCQ_SORT2(0, 3)  \
    CCQ_SORT2(1, 4)  \
    CCQ_SORT2(2, 5)  \
    CCQ_SORT2(2, 4)  \
    CCQ_SORT2(1, 3)  \
    CCQ_SORT2(2, 3)

#define CCQ_SORT7() \
    CCQ_SORT2(1, 2) \
    CCQ_SORT2(0, 2) \
    CCQ_SORT2(0, 1) \
    CCQ_SORT2(3, 4) \
    CCQ_SORT2(5, 6) \
    CCQ_SORT2(3, 5) \
    CCQ_SORT2(4, 6) \
    CCQ_SORT2(4, 5) \
    CCQ_SORT2(0, 4) \
    CCQ_SORT2(0, 3) \
    CCQ_SORT2(1, 5) \
    CCQ_SORT2(2, 6) \
    CCQ_SORT2(2, 5) \
    CCQ_SORT2(1, 3) \
    CCQ_SORT2(2, 4) \
    CCQ_SORT2(2, 3)

#define CCQ_SORT8() \
    CCQ_SORT2(0, 1) \
    CCQ_SORT2(2, 3) \
    CCQ_SORT2(0, 2) \
    CCQ_SORT2(1, 3) \
    CCQ_SORT2(1, 2) \
    CCQ_SORT2(4, 5) \
    CCQ_SORT2(6, 7) \
    CCQ_SORT2(4, 6) \
    CCQ_SORT2(5, 7) \
    CCQ_SORT2(5, 6) \
    CCQ_SORT2(0, 4) \
    CCQ_SORT2(1, 5) \
    CCQ_SORT2(1, 4) \
    CCQ_SORT2(2, 6) \
    CCQ_SORT2(3, 7) \
    CCQ_SORT2(3, 6) \
    CCQ_SORT2(2, 4) \
    CCQ_SORT2(3, 5) \
    CCQ_SORT2(3, 4)

#define CCQ_SORT9() \
    CCQ_SORT2(0, 1) \
    CCQ_SORT2(3, 4) \
    CCQ_SORT2(6, 7) \
    CCQ_SORT2(1, 2) \
    CCQ_SORT2(4, 5) \
    CCQ_SORT2(7, 8) \
    CCQ_SORT2(0, 1) \
    CCQ_SORT2(3, 4) \
    CCQ_SORT2(6, 7) \
    CCQ_SORT2(0, 3) \
    CCQ_SORT2(3, 6) \
    CCQ_SORT2(0, 3) \
    CCQ_SORT2(1, 4) \
    CCQ_SORT2(4, 7) \
    CCQ_SORT2(1, 4) \
    CCQ_SORT2(2, 5) \
    CCQ_SORT2(5, 8) \
    CCQ_SORT2(2, 5) \
    CCQ_SORT2(1, 3) \
    CCQ_SORT2(5, 7) \
    CCQ_SORT2(2, 6) \
    CCQ_SORT2(4, 6) \
    CCQ_SORT2(2, 4) \
    CCQ_SORT2(2, 3) \
    CCQ_SORT2(5, 6)
// Sorting Networks - END

    union CC_QSORT_SWAP8
    {
        char chr[8]; // optimized for Js::Var
    };

    union CC_QSORT_SWAP4
    {
        char chr[4]; // optimized for TypedArray
    };

    union CC_QSORT_SWAP2
    {
        char chr[2]; // others
    };

    #define CC_QSORT_SWAP_LOOP(T, a, b, nsize)        \
    {                                       \
        for (size_t i = 0; i < nsize; i++)  \
        {                                   \
            T c = (a)[i];                   \
            (a)[i] = (b)[i];                \
            (b)[i] = c;                     \
        }                                   \
    }

    template <class Policy, class T>
    class QuickSortSwap
    {
    public:

        static inline void swap(T* a, T* b, size_t size)
        {
            if (a == b) return;

            size_t mod;
            if (size >= 8)
            {
                mod = size & 7;
                CC_QSORT_SWAP_LOOP(CC_QSORT_SWAP8, (CC_QSORT_SWAP8*) a, (CC_QSORT_SWAP8*) b, size / 8)
            }
            else if (size >= 4)
            {
                mod = size & 3;
                CC_QSORT_SWAP_LOOP(CC_QSORT_SWAP4, (CC_QSORT_SWAP4*) a, (CC_QSORT_SWAP4*) b, size / 4)
            }
            else
            {
                mod = size & 1;
                CC_QSORT_SWAP_LOOP(CC_QSORT_SWAP2, (CC_QSORT_SWAP2*) a, (CC_QSORT_SWAP2*) b, size / 2)
            }

            CC_QSORT_SWAP_LOOP(char, (char*) a, (char*) b, mod);
        }
    };

#ifdef RECYCLER_WRITE_BARRIER
    template <class T>
    class QuickSortSwap<_write_barrier_policy, T>
    {
    public:
        static inline void swap(T* a, T* b, size_t _)
        {
            AssertMsg(_ == 1, "No byte to byte copies when a type has SWB is enabled. size should be 1!");
            T temp = *a;
            *a = *b;
            *b = temp;
        }
    };
#endif

    template <class Policy, class T, class Comparer>
    class QuickSort
    {
    public:
        #define CCQ_SWAP QuickSortSwap<Policy, T>::swap
        static void Sort(T* base, size_t nmemb, size_t size, Comparer comparer, void* context)
        {
            if (!base)
            {
                return;
            }

            if (nmemb <= 9) // use sorting networks for members <= 2^3
            {
                switch(nmemb)
                {
                    case 9:
                        CCQ_SORT9()
                        break;
                    case 8:
                        CCQ_SORT8()
                        break;
                    case 7:
                        CCQ_SORT7()
                        break;
                    case 6:
                        CCQ_SORT6()
                        break;
                    case 5:
                        CCQ_SORT5(0, 1, 2, 3, 4)
                        break;
                    case 4:
                        CCQ_SORT4()
                        break;
                    case 3:
                        CCQ_SORT3()
                        break;
                    case 2:
                        CCQ_SORT2(0, 1)
                        break;
                    default:
                        break; // nothing to sort
                }

                return;
            }

            CCQ_SORT5(0, (nmemb / 4), (nmemb / 2), ((3 * nmemb) / 4), (nmemb - 1));
            // guess the median is in the middle now.
            size_t pos = 0;
            const size_t pivot = (nmemb - 1) * size;
            // make last element the median(pivot)
            CCQ_SWAP(base + pivot, base + ((nmemb / 2) * size), size);
            // standard qsort pt. below
            for (size_t i = 0; i < pivot; i+= size)
            {
                if (comparer(context, base + i, base + pivot) <= 0)
                {
                    CCQ_SWAP(base + i, base + (pos * size), size);
                    pos++;
                }
            }

            // issue the last change
            CCQ_SWAP(base + (pos * size), base + pivot, size);

            if (pos >= nmemb - 1)
            {
                return; // looks like it was either all sorted OR nothing to sort
            }

            Sort(base, pos++, size, comparer, context);
            Sort(base + (pos * size), nmemb - pos, size, comparer, context);
        }
        #undef CCQ_SWAP
    };

#undef CCQ_SORT2
#undef CCQ_SORT3
#undef CCQ_SORT4
#undef CCQ_SORT5
#undef CCQ_SORT6
#undef CCQ_SORT7
#undef CCQ_SORT8
#undef CCQ_SORT9
#undef CC_QSORT_SWAP_LOOP
}
