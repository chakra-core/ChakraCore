//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include "stdafx.h"
#include "catch.hpp"
#include "PlatformAgnostic/UnicodeText.h"


namespace UnicodeTextTest
{
	void Test(const WCHAR* str1, const WCHAR* str2, int expected)
	{
		// there are two tests, one to validate the expected value and validate the result of the call
		int compareStringResult = CompareStringW(LOCALE_USER_DEFAULT, NORM_IGNORECASE | SORT_DIGITSASNUMBERS, str1, -1, str2, -1);
		CHECK(compareStringResult != 0);
		compareStringResult = compareStringResult - CSTR_EQUAL;

		int res = PlatformAgnostic::UnicodeText::LogicalStringCompare(str1, str2);

		//int res = Chakra::PlatformAgnostic::UnicodeText::Internal::LogicalStringCompareImpl(str1, str2);
		
		bool passed = res == expected;

		//test to check that expected value passed is correct
		CHECK(compareStringResult == expected);
		
		//test to check that the result from call to LogicalStringCompareImpl is the expected value
		CHECK(passed);    		
	}

	TEST_CASE(){
        Test(_u("a"), _u("a"), 0);
    }
	TEST_CASE(){
		Test(_u("a"), _u("b"), -1);
    }
	TEST_CASE(){
        Test(_u("b"), _u("a"), 1);
    }

    TEST_CASE(){
        Test(_u("1"), _u("2"), -1);
    }
    TEST_CASE(){
        Test(_u("2"), _u("1"), 1);
    }


    TEST_CASE(){
        Test(_u("10"), _u("01"), 1);
    }
    TEST_CASE(){
        Test(_u("01"), _u("10"), -1);
    }


    TEST_CASE(){
        Test(_u("01"), _u("1"), -1);
    }
    TEST_CASE(){
        Test(_u("1"), _u("01"), 1);
    }


    TEST_CASE(){
        Test(_u("1a"), _u("a1"), -1);
    }
    TEST_CASE(){
        Test(_u("aa1"), _u("a1"), 1);
    }
    TEST_CASE(){
        Test(_u("a1"), _u("a1"), 0);
    }
    TEST_CASE(){
        Test(_u("a1"), _u("b1"), -1);
    }
    TEST_CASE(){
        Test(_u("b1"), _u("a1"), 1);
    }
    TEST_CASE(){
       Test(_u("a1"), _u("a2"), -1);
    }
    TEST_CASE(){
        Test(_u("a10"), _u("a2"), 1);
    }
    TEST_CASE(){
        Test(_u("a2"), _u("a10"), -1);
    }
    


    TEST_CASE(){
        Test(_u("A1"), _u("a1"), 0);
    }
    TEST_CASE(){
        Test(_u("A1"), _u("b1"), -1);
    }
    TEST_CASE(){
        Test(_u("B1"), _u("a1"), 1);
    }
    TEST_CASE(){
        Test(_u("A1"), _u("a2"), -1);
    }
    TEST_CASE(){
        Test(_u("A10"), _u("a2"), 1);
    }
    TEST_CASE(){
        Test(_u("A2"), _u("a10"), -1);
    }



    TEST_CASE(){
        Test(_u("123"), _u("456"), -1);
    }
    TEST_CASE(){
       Test(_u("456"), _u("123"), 1);
    }
    TEST_CASE(){
        Test(_u("abc123"), _u("def123"), -1);
    }
    TEST_CASE(){
        Test(_u("abc123"), _u("abc123"), 0);
    }
    TEST_CASE(){
        Test(_u("abc123"), _u("abc0123"), 1);
    }
    TEST_CASE(){
        Test(_u("abc123"), _u("abc124"), -1);
    }
    TEST_CASE(){
        Test(_u("abc124"), _u("abc123"), 1);
    }


    
    TEST_CASE(){
        Test(_u("abc123def"), _u("abc123def"), 0);
    }
    TEST_CASE(){
        Test(_u("abc123def"), _u("abc123eef"), -1);
    }
    TEST_CASE(){
        Test(_u("abc123eef"), _u("abc123def"), 1);
    }




    TEST_CASE(){
        Test(_u("abc1def"), _u("abc10def"), -1);
    }
    TEST_CASE(){
        Test(_u("abc1def1"), _u("abc1def12"), -1);
    }
    


    TEST_CASE(){
        Test(_u("2string"), _u("3string"), -1);
    }
    TEST_CASE(){
        Test(_u("20string"), _u("3string"), 1);
    }
    TEST_CASE(){
        Test(_u("20string"), _u("st2ring"), -1);
    }
    TEST_CASE(){
        Test(_u("st3ring"), _u("st2ring"), 1);
    }

    
    
    TEST_CASE(){
        Test(_u("2String"), _u("3string"), -1);
    }
    TEST_CASE(){
        Test(_u("20String"), _u("3string"), 1);
    }
    TEST_CASE(){
        Test(_u("20sTRing"), _u("st2ring"), -1);
    }
    TEST_CASE(){
        Test(_u("st3rING"), _u("st2riNG"), 1);
    }	
}

