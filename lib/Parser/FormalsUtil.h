//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once
template <class Fn, bool mapRest>
void MapFormalsImpl(ParseNodeFnc *pnodeFunc, Fn fn)
{
    for (ParseNode *pnode = pnodeFunc->pnodeParams; pnode != nullptr; pnode = pnode->GetFormalNext())
    {
        fn(pnode);
    }
    if (mapRest && pnodeFunc->pnodeRest != nullptr)
    {
        fn(pnodeFunc->pnodeRest);
    }
}

template <class Fn>
void MapFormalsWithoutRest(ParseNodeFnc *pnodeFunc, Fn fn)
{
    return MapFormalsImpl<Fn, false>(pnodeFunc, fn);
}

template <class Fn>
void MapFormals(ParseNodeFnc *pnodeFunc, Fn fn)
{
    return MapFormalsImpl<Fn, true>(pnodeFunc, fn);
}

template <class Fn>
void MapFormalsFromPattern(ParseNodeFnc *pnodeFunc, Fn fn)
{
    for (ParseNode *pnode = pnodeFunc->pnodeParams; pnode != nullptr; pnode = pnode->GetFormalNext())
    {
        if (pnode->nop == knopParamPattern)
        {
            Parser::MapBindIdentifier(pnode->AsParseNodeParamPattern()->pnode1, fn);
        }
    }
}

