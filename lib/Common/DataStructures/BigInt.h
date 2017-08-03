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
    class BigInt
    {
    private:
        // Make this big enough that we rarely have to call malloc.
        enum { kcluMaxInit = 30 };

        int32 m_cluMax;
        int32 m_clu;
        uint32 *m_prglu;
        uint32 m_rgluInit[kcluMaxInit];

        inline BigInt & operator= (BigInt &bi);
        bool FResize(int32 clu);

#if DBG
        #define AssertBi(pbi) Assert(pbi); (pbi)->AssertValid(true);
        #define AssertBiNoVal(pbi) Assert(pbi); (pbi)->AssertValid(false);
        inline void AssertValid(bool fCheckVal);
#else //!DBG
        #define AssertBi(pbi)
        #define AssertBiNoVal(pbi)
#endif //!DBG

    public:
        BigInt(void);
        ~BigInt(void);

        bool FInitFromRglu(uint32 *prglu, int32 clu);
        bool FInitFromBigint(BigInt *pbiSrc);
        template <typename EncodedChar>
        bool FInitFromDigits(const EncodedChar *prgch, int32 cch, int32 *pcchDec);
        bool FMulAdd(uint32 luMul, uint32 luAdd);
        bool FMulPow5(int32 c5);
        bool FShiftLeft(int32 cbit);
        void ShiftLusRight(int32 clu);
        void ShiftRight(int32 cbit);
        int Compare(BigInt *pbi);
        bool FAdd(BigInt *pbi);
        void Subtract(BigInt *pbi);
        int DivRem(BigInt *pbi);

        int32 Clu(void);
        uint32 Lu(int32 ilu);
        double GetDbl(void);
    };
}
