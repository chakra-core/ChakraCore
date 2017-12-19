//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

namespace Js
{
    template <typename TStatementMapList>
    class StatementReader
    {
    private:
        const byte* m_startLocation = nullptr;
        SmallSpanSequence* m_statementMap = nullptr;
        SmallSpanSequenceIter m_statementMapIter;

        TStatementMapList * m_fullstatementMap = nullptr;
        const byte* m_nextStatementBoundary = nullptr;
        int m_statementIndex = 0;
        bool m_startOfStatement = true;

    public:
        void Create(FunctionBody* functionRead, uint startOffset = 0);
        void Create(FunctionBody* functionRead, uint startOffset, bool useOriginalByteCode);

        void Create(
            _In_ const byte * byteCodeStart,
            uint startOffset,
            Js::SmallSpanSequence * statementMap,
            TStatementMapList* fullstatementMap);

        inline bool AtStatementBoundary(ByteCodeReader * reader) { return m_nextStatementBoundary == reader->GetIP(); }
        inline uint32 MoveNextStatementBoundary();
        inline uint32 GetStatementIndex() const { return m_statementIndex; }
    };
} // namespace Js
