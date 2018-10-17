//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    /***************************************************************************
        Big non-negative integer class.
    ***************************************************************************/
    class BigUInt
    {
        // Non-negative BigInt is stored as an array of 'digit' where each digit is unit32
    private:
        // Make this big enough that we rarely have to call malloc.
        enum { kcluMaxInit = 30 };// initilize 30 digits

        int32 m_cluMax; // current maximum length (or number of digits) it can contains
        int32 m_clu; // current length (or number of digits)
        uint32 *m_prglu; // pointer to array of digits
        uint32 m_rgluInit[kcluMaxInit]; // pre-defined space to store array

        inline BigUInt & operator= (BigUInt &bi);
        bool FResize(int32 clu);// allocate more space if length go over maximum

#if DBG
        #define AssertBi(pbi) Assert(pbi); (pbi)->AssertValid(true);
        #define AssertBiNoVal(pbi) Assert(pbi); (pbi)->AssertValid(false);
        inline void AssertValid(bool fCheckVal);
#else //!DBG
        #define AssertBi(pbi)
        #define AssertBiNoVal(pbi)
#endif //!DBG

    public:
        BigUInt(void);
        ~BigUInt(void);

        bool FInitFromRglu(uint32 *prglu, int32 clu); // init from array and length
        bool FInitFromBigint(BigUInt *pbiSrc); 
        template <typename EncodedChar>
        bool FInitFromDigits(const EncodedChar *prgch, int32 cch, int32 *pcchDec); // init from char of digits
        bool FMulAdd(uint32 luMul, uint32 luAdd);
        bool FMulPow5(int32 c5);
        bool FShiftLeft(int32 cbit);
        void ShiftLusRight(int32 clu);
        void ShiftRight(int32 cbit);
        int Compare(BigUInt *pbi);
        bool FAdd(BigUInt *pbi);
        void Subtract(BigUInt *pbi);
        int DivRem(BigUInt *pbi);

        int32 Clu(void); // return current length
        uint32 Lu(int32 ilu); // return digit at position ilu start from 0
        double GetDbl(void);
    };
}
