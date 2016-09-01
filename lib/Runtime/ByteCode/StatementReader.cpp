//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include "RuntimeByteCodePch.h"

namespace Js
{
    template <typename TStatementMapList>
    void StatementReader<TStatementMapList>::Create(FunctionBody * functionRead, uint startOffset /* = 0 */)
    {
        Assert(functionRead);
        StatementReader::Create(functionRead, startOffset, false);
    }

    template <typename TStatementMapList>
    void StatementReader<TStatementMapList>::Create(
        _In_ const byte * byteCodeStart,
        uint startOffset,
        Js::SmallSpanSequence * statementMap,
        TStatementMapList* fullstatementMap)
    {
        m_startLocation = byteCodeStart;

        const byte * currentLocation = m_startLocation + startOffset;

        m_statementMap = statementMap;
        m_fullstatementMap = fullstatementMap;

        if (m_statementMap && m_statementMap->Count())
        {
            m_statementMap->Reset(m_statementMapIter);

            m_statementIndex = 0;
            m_startOfStatement = true;

            StatementData data;
            if (!m_statementMap->Seek(m_statementIndex, data))
            {
                Assert(FALSE);
            }

            m_nextStatementBoundary = m_startLocation + data.bytecodeBegin;

            // If we starting in the middle of the function (e.g., loop body), find out where the next statement is.
            while (m_nextStatementBoundary < currentLocation)
            {
                this->MoveNextStatementBoundary();
            }
        }
        else if (m_fullstatementMap && m_fullstatementMap->Count())
        {
            m_statementIndex = 0;
            m_startOfStatement = true;
            FunctionBody::StatementMap *nextMap = Js::FunctionBody::GetNextNonSubexpressionStatementMap(m_fullstatementMap, m_statementIndex);
            if (!nextMap)
            {
                // set to a location that will never match
                m_nextStatementBoundary = currentLocation - 1;
            }
            else
            {
                m_nextStatementBoundary = m_startLocation + m_fullstatementMap->Item(m_statementIndex)->byteCodeSpan.begin;

                // If we starting in the middle of the function (e.g., loop body), find out where the next statement is.
                while (m_nextStatementBoundary < currentLocation)
                {
                    this->MoveNextStatementBoundary();
                }
            }
        }
        else
        {
            // set to a location that will never match
            m_nextStatementBoundary = currentLocation - 1;
        }
    }
    template <>
    void StatementReader<FunctionBody::ArenaStatementMapList>::Create(FunctionBody* functionRead, uint startOffset, bool useOriginalByteCode)
    {
        Assert(UNREACHED);
    }

    template <>
    void StatementReader<FunctionBody::StatementMapList>::Create(FunctionBody* functionRead, uint startOffset, bool useOriginalByteCode)
    {
        AssertMsg(functionRead != nullptr, "Must provide valid function to execute");

        ByteBlock * pblkByteCode = useOriginalByteCode ?
            functionRead->GetOriginalByteCode() :
            functionRead->GetByteCode();

        AssertMsg(pblkByteCode != nullptr, "Must have valid byte-code to read");

        SmallSpanSequence* statementMap = functionRead->GetStatementMapSpanSequence();
        FunctionBody::StatementMapList* fullMap = nullptr;
        if (statementMap == nullptr && functionRead->IsInDebugMode())
        {
            fullMap = functionRead->GetStatementMaps();
        }
        Create(pblkByteCode->GetBuffer(), startOffset, statementMap, fullMap);
    }

    template <typename TStatementMapList>
    uint32 StatementReader<TStatementMapList>::MoveNextStatementBoundary()
    {
        StatementData data;
        uint32 retStatement = Js::Constants::NoStatementIndex;

        if (m_startOfStatement)
        {
            m_statementIndex++;
            if (m_statementMap && (uint32)m_statementIndex < m_statementMap->Count() && m_statementMap->Item(m_statementIndex, m_statementMapIter, data))
            {
                // The end boundary is the last byte of the last instruction in the previous range.
                // We want to track the beginning of the next instruction for AtStatementBoundary.
                m_nextStatementBoundary = m_startLocation + data.bytecodeBegin;

                // The next user statement is adjacent in the bytecode
                retStatement = m_statementIndex;
            }
            else if (m_fullstatementMap && m_statementIndex < m_fullstatementMap->Count())
            {
                int nextInstrStart = m_fullstatementMap->Item(m_statementIndex - 1)->byteCodeSpan.end + 1;
                m_nextStatementBoundary = m_startLocation + nextInstrStart;
                Js::FunctionBody::GetNextNonSubexpressionStatementMap(m_fullstatementMap, m_statementIndex);

                if (nextInstrStart == m_fullstatementMap->Item(m_statementIndex)->byteCodeSpan.begin)
                {
                    retStatement = m_statementIndex;
                }
                else
                {
                    m_startOfStatement = false;
                }
            }
            else
            {
                m_startOfStatement = false;
            }
        }
        else
        {
            m_startOfStatement = true;
            if (m_statementMap && (uint32)m_statementIndex < m_statementMap->Count() && m_statementMap->Item(m_statementIndex, m_statementMapIter, data))
            {
                // Start a range of bytecode that maps to a user statement
                m_nextStatementBoundary = m_startLocation + data.bytecodeBegin;
                retStatement = m_statementIndex;
            }
            else if (m_fullstatementMap && m_statementIndex < m_fullstatementMap->Count())
            {
                FunctionBody::StatementMap *nextMap = Js::FunctionBody::GetNextNonSubexpressionStatementMap(m_fullstatementMap, m_statementIndex);
                if (!nextMap)
                {
                    // set to a location that will never match
                    m_nextStatementBoundary = m_startLocation - 1;
                }
                else
                {
                    // Start a range of bytecode that maps to a user statement
                    m_nextStatementBoundary = m_startLocation + m_fullstatementMap->Item(m_statementIndex)->byteCodeSpan.begin;
                    retStatement = m_statementIndex;
                }
            }
            else
            {
                // The remaining bytecode instructions do not map to a user statement, set a statementBoundary that cannot match
                m_nextStatementBoundary = m_startLocation - 1;
            }
        }

        return retStatement;
    }

    // explicit instantiations
    template class StatementReader<FunctionBody::ArenaStatementMapList>;
    template class StatementReader<FunctionBody::StatementMapList>;
} // namespace Js

