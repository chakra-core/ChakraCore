//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#pragma once

#if defined(__ANDROID__)
#include <pthread.h>
#endif

#include "Runtime.h"
#include "Library/JavascriptRegularExpression.h"
#include "Library/JavascriptProxy.h"
#include "Library/SameValueComparer.h"
#include "Library/MapOrSetDataList.h"
#include "Library/JavascriptMap.h"
#include "Library/JavascriptSet.h"
#include "Library/JavascriptWeakMap.h"
#include "Library/JavascriptWeakSet.h"
// =================

#include "SCATypes.h"
#include "SCAEngine.h"
#include "SCAPropBag.h"
#include "StreamHelper.h"
#include "StreamReader.h"
#include "StreamWriter.h"
#include "SCADeserialization.h"
#include "SCASerialization.h"
#include "SCACore.h"
