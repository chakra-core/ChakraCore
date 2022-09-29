//-------------------------------------------------------------------------------------------------------
// Copyright (C) Microsoft Corporation and contributors. All rights reserved.
// Copyright (c) 2021 ChakraCore Project Contributors. All rights reserved.
// Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#define IDS_COMPILATION_ERROR_SOURCE    4096
#define IDS_RUNTIME_ERROR_SOURCE        4097
#define IDS_UNKNOWN_RUNTIME_ERROR        4098

#define IDS_INFINITY                    6000
#define IDS_MINUSINFINITY               6001


#ifndef RT_ERROR_MSG
#define RT_ERROR_MSG(name, errnum, str1, str2, jst, errorNumSource)
#endif
#ifndef RT_PUBLICERROR_MSG
#define RT_PUBLICERROR_MSG(name, errnum, str1, str2, jst, errorNumSource)
#endif
#ifndef RT_ERROR_MSG_UNUSED_ENTRY
#define RT_ERROR_MSG_UNUSED_ENTRY
#endif

// These VBS macros were taken from VBScript and the names and definitions are
// written in stone. Other projects depend on these definitions as well.
// Keep the commented lines as documentation of values that should not be reused.

RT_ERROR_MSG(VBSERR_None, 0,    "", "", kjstError, 0)
//RT_ERROR_MSG(VBSERR_ReturnWOGoSub, 3,    "",   "Return without GoSub")
RT_ERROR_MSG(VBSERR_IllegalFuncCall, 5,    "", "Invalid procedure call or argument", kjstTypeError, 0)
RT_ERROR_MSG(VBSERR_Overflow, 6,    "",    "Overflow", kjstRangeError, 0)
RT_ERROR_MSG(VBSERR_OutOfMemory, 7,    "", "Out of memory", kjstError, 0)
RT_ERROR_MSG(VBSERR_OutOfBounds, 9,    "", "Subscript out of range", kjstRangeError, 0)
RT_ERROR_MSG(VBSERR_ArrayLocked, 10,    "",    "This array is fixed or temporarily locked", kjstError, 0)
//RT_ERROR_MSG(VBSERR_DivByZero, 11,    "",  "Division by zero", kjstError, 0)
RT_ERROR_MSG(VBSERR_TypeMismatch, 13,    "",   "Type mismatch", kjstTypeError, 0)
//RT_ERROR_MSG(VBSERR_OutOfStrSpace, 14,    "",  "Out of string space", kjstError, 0)
//RT_ERROR_MSG(VBSERR_ExprTooComplex, 16,    "", "Expression too complex")
//RT_ERROR_MSG(VBSERR_CantContinue, 17,    "",   "Can't perform requested operation", kjstError, 0)
//RT_ERROR_MSG(VBSERR_UserInterrupt, 18,    "",  "User interrupt occurred")
//RT_ERROR_MSG(VBSERR_ResumeWOErr, 20,    "",    "Resume without error")
RT_ERROR_MSG(VBSERR_OutOfStack, 28,    "", "Out of stack space", kjstError, 0)
//RT_ERROR_MSG(VBSERR_UndefinedProc, 35,    "",  "Sub or Function not defined", kjstError, 0)
//RT_ERROR_MSG(VBSERR_TooManyClients, 47,    "", "Too many DLL application clients")
RT_ERROR_MSG(VBSERR_DLLLoadErr, 48,    "", "Error in loading DLL", kjstError, 0)
//RT_ERROR_MSG(VBSERR_DLLBadCallingConv, 49,    "",  "Bad DLL calling convention")
RT_ERROR_MSG(VBSERR_InternalError, 51,    "",  "Internal error", kjstError, 0)
//RT_ERROR_MSG(VBSERR_BadFileNameOrNumber, 52,    "",    "Bad file name or number", kjstError, 0)
RT_ERROR_MSG(VBSERR_FileNotFound, 53,    "",   "File not found", kjstError, 0)
//RT_ERROR_MSG(VBSERR_BadFileMode, 54,    "",    "Bad file mode", kjstError, 0)
//RT_ERROR_MSG(VBSERR_FileAlreadyOpen, 55,    "",    "File already open", kjstError, 0)
RT_ERROR_MSG(VBSERR_IOError, 57,    "",    "Device I/O error", kjstError, 0)
RT_ERROR_MSG(VBSERR_FileAlreadyExists, 58,    "",  "File already exists", kjstError, 0)
//RT_ERROR_MSG(VBSERR_BadRecordLen, 59,    "",   "Bad record length")
RT_ERROR_MSG(VBSERR_DiskFull, 61,    "",   "Disk full", kjstError, 0)
//RT_ERROR_MSG(VBSERR_EndOfFile, 62,    "",  "Input past end of file", kjstError, 0)
//RT_ERROR_MSG(VBSERR_BadRecordNum, 63,    "",   "Bad record number")
RT_ERROR_MSG(VBSERR_TooManyFiles, 67,    "",   "Too many files", kjstError, 0)
//RT_ERROR_MSG(VBSERR_DevUnavailable, 68,    "", "Device unavailable", kjstError, 0)
RT_ERROR_MSG(VBSERR_PermissionDenied, 70,    "",   "Permission denied", kjstError, 0)
//RT_ERROR_MSG(VBSERR_DiskNotReady, 71,    "",   "Disk not ready", kjstError, 0)
//RT_ERROR_MSG(VBSERR_DifferentDrive, 74,    "", "Can't rename with different drive", kjstError, 0)
RT_ERROR_MSG(VBSERR_PathFileAccess, 75,    "", "Path/File access error", kjstError, 0)
RT_ERROR_MSG(VBSERR_PathNotFound, 76,    "",   "Path not found", kjstError, 0)
//RT_ERROR_MSG(VBSERR_ObjNotSet, 91,    "",  "Object variable or With block variable not set", kjstTypeError, 0)
//RT_ERROR_MSG(VBSERR_IllegalFor, 92,    "", "For loop not initialized", kjstError, 0)
//RT_ERROR_MSG(VBSERR_BadPatStr, 93,    "",  "Invalid pattern string")
//RT_ERROR_MSG(VBSERR_CantUseNull, 94,    "",    "Invalid use of Null", kjstError, 0)
//RT_ERROR_MSG(VBSERR_UserDefined, 95,    "",    "Application-defined or object-defined error")
RT_ERROR_MSG(VBSERR_CantCreateTmpFile, 322,    "", "Can't create necessary temporary file", kjstError, 0)
//RT_ERROR_MSG(VBSERR_InvalidResourceFormat, 325,    "", "Invalid format in resource file")
//RT_ERROR_MSG(VBSERR_InvalidPropertyValue, 380,    "",  "Invalid property value")
//RT_ERROR_MSG(VBSERR_NoSuchControlOrProperty, 423,    "",   "Property or method not found")
//RT_ERROR_MSG(VBSERR_NotObject, 424,    "", "Object required", kjstTypeError, 0)
RT_ERROR_MSG(VBSERR_CantCreateObject, 429,    "",  "Automation server can't create object", kjstError, 0)
RT_ERROR_MSG(VBSERR_OLENotSupported, 430,    "",   "Class doesn't support Automation", kjstError, 0)
RT_ERROR_MSG(VBSERR_OLEFileNotFound, 432,    "",   "File name or class name not found during Automation operation", kjstError, 0)
RT_ERROR_MSG(VBSERR_OLENoPropOrMethod, 438,    "Object doesn't support property or method '%s'",    "Object doesn't support this property or method", kjstTypeError, 0)
//RT_ERROR_MSG(VBSERR_OLEAutomationError, 440,    "",    "Automation error", kjstError, 0)
//RT_ERROR_MSG(VBSERR_LostTLB, 442,    "",   "Connection to type library or object library for remote process has been lost. Press OK for dialog to remove reference.")
//RT_ERROR_MSG(VBSERR_OLENoDefault, 443,    "",  "Automation object does not have a default value")
RT_ERROR_MSG(VBSERR_ActionNotSupported, 445,    "",    "Object doesn't support this action", kjstTypeError, 0)
RT_ERROR_MSG(VBSERR_NamedArgsNotSupported, 446,    "", "Object doesn't support named arguments", kjstError, 0)
RT_ERROR_MSG(VBSERR_LocaleSettingNotSupported, 447,    "", "Object doesn't support current locale setting", kjstError, 0)
RT_ERROR_MSG(VBSERR_NamedParamNotFound, 448,    "",    "Named argument not found", kjstError, 0)
RT_ERROR_MSG(VBSERR_ParameterNotOptional, 449,    "Argument to the function '%s' is not optional",  "Argument not optional", kjstTypeError, 0)
RT_ERROR_MSG(VBSERR_FuncArityMismatch, 450,    "", "Wrong number of arguments or invalid property assignment", kjstTypeError, 0)
RT_ERROR_MSG(VBSERR_NotEnum, 451,    "",   "Object not a collection", kjstTypeError, 0)
//RT_ERROR_MSG(VBSERR_InvalidOrdinal, 452,    "",    "Invalid ordinal")
RT_ERROR_MSG(VBSERR_InvalidDllFunctionName, 453,    "",    "Specified DLL function not found", kjstError, 0)
//RT_ERROR_MSG(VBSERR_CodeResourceNotFound, 454,    "",  "Code resource not found")
//RT_ERROR_MSG(VBSERR_CodeResourceLockError, 455,    "", "Code resource lock error")
//RT_ERROR_MSG(VBSERR_DuplicateKey, 457,    "",  "This key is already associated with an element of this collection")
RT_ERROR_MSG(VBSERR_InvalidTypeLibVariable, 458,    "",    "Variable uses an Automation type not supported in JavaScript", kjstTypeError, 0)
RT_ERROR_MSG(VBSERR_ServerNotFound, 462, "", "The remote server machine does not exist or is unavailable", kjstError, 0)
//RT_ERROR_MSG(VBSERR_InvalidPicture, 481,    "",    "Invalid picture")

//RT_ERROR_MSG(VBSERR_CantAssignTo, 501,    "",    "Cannot assign to variable", kjstReferenceError, 0)

//RT_ERROR_MSG(VBSERR_NotSafeForScripting, 502,    "",    "Object not safe for scripting", kjstError, 0)
//RT_ERROR_MSG(VBSERR_NotSafeForInitializing, 503,    "",    "Object not safe for initializing", kjstError, 0)
//RT_ERROR_MSG(VBSERR_NotSafeForCreating, 504,    "",    "Object not safe for creating", kjstError, 0)
//RT_ERROR_MSG(VBSERR_InvalidReference, 505, "", "Invalid or unqualified reference")
//RT_ERROR_MSG(VBSERR_ClassNotDefined, 506, "", "Class not defined")
RT_ERROR_MSG(VBSERR_ComponentException, 507, "", "An exception occurred", kjstError, 0)

RT_ERROR_MSG(JSERR_CantAssignThis, 5000, "", "Cannot assign to 'this'", kjstError, 0)
RT_ERROR_MSG(JSERR_NeedNumber, 5001, "'%s' is not a number", "Number expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NeedFunction, 5002, "'%s' is not a function", "Function expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_CantAsgCall, 5003, "", "Cannot assign to a function result", kjstReferenceError, 0)
RT_ERROR_MSG(JSERR_NeedIndxObj, 5004, "'%s' is not an indexable object", "Cannot index object", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NeedString, 5005, "'%s' is not a string", "String expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NeedDate, 5006, "'%s' is not a date object", "Date object expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NeedObject, 5007, "'%s' is null or not an object", "Object expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_CantAssignTo, 5008, "", "Invalid left-hand side in assignment", kjstReferenceError, 0)
RT_ERROR_MSG(JSERR_UndefVariable, 5009, "'%s' is not defined", "Undefined identifier", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NeedBoolean, 5010, "'%s' is not a boolean", "Boolean expected", kjstTypeError, 0)

// This is the legacy error code for JScript.
RT_ERROR_MSG(JSERR_CantExecute, 5011, "", "Can't execute code from a freed script", kjstError, 0)

// JScript9 is to use the newer JSCRIPT_E_CANTEXECUTE public HResult.
RT_PUBLICERROR_MSG(JSPUBLICERR_CantExecute, 1, "", "Can't execute code from a freed script", kjstError, 0)

RT_ERROR_MSG(JSERR_CantDelete, 5012, "Cannot delete '%s'", "Object member expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NeedVBArray, 5013, "'%s' is not a VBArray", "VBArray expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NeedInternalObj, 5014, "'%s' is not a JavaScript object", "JavaScript object expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NeedEnumerator, 5015, "'%s' is not an enumerator object", "Enumerator object expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NeedRegExp, 5016, "'%s' is not a regular expression object", "Regular Expression object expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_RegExpSyntax, 5017, "", "Syntax error in regular expression", kjstSyntaxError, 0)
RT_ERROR_MSG(JSERR_RegExpBadQuant, 5018, "", "Unexpected quantifier", kjstSyntaxError, 0)
RT_ERROR_MSG(JSERR_RegExpNoBracket, 5019, "", "Expected ']' in regular expression", kjstSyntaxError, 0)
RT_ERROR_MSG(JSERR_RegExpNoParen, 5020, "", "Expected ')' in regular expression", kjstSyntaxError, 0)
RT_ERROR_MSG(JSERR_RegExpBadRange, 5021, "", "Invalid range in character set", kjstSyntaxError, 0)
RT_ERROR_MSG(JSERR_UncaughtException, 5022, "", "Exception thrown and not caught", kjstError, 0)
RT_ERROR_MSG(JSERR_InvalidPrototype, 5023, "", "Function does not have a valid prototype object", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_URIEncodeError, 5024, "", "The URI to be encoded contains an invalid character", kjstURIError, 0)
RT_ERROR_MSG(JSERR_URIDecodeError, 5025, "", "The URI to be decoded is not a valid encoding", kjstURIError, 0)
RT_ERROR_MSG(JSERR_FractionOutOfRange, 5026, "", "The number of fractional digits is out of range", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_PrecisionOutOfRange, 5027, "", "The precision is out of range", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_NeedArrayOrArg, 5028, "%s is not an Array or arguments object", "Array or arguments object expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_ArrayLengthConstructIncorrect, 5029, "", "Array length must be a finite positive integer", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_ArrayLengthAssignIncorrect, 5030, "", "Array length must be assigned a finite positive integer", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_NeedArrayObject, 5031, "%s is not an Array object", "Array object expected", kjstTypeError, 0)
// RETIRED ECMACP removed ;; RT_ERROR_MSG(JSERR_NoCPEval,     5032, "", "'eval' is not available in the ECMA 327 Compact Profile",               kjstEvalError, 0)
// RETIRED ECMACP removed ;; RT_ERROR_MSG(JSERR_NoCPFunction, 5033, "", "Function constructor is not available in the ECMA 327 Compact Profile", kjstEvalError, 0)
RT_ERROR_MSG(JSERR_JSONSerializeCircular, 5034, "", "Circular reference in value argument not supported", kjstError, 0)
RT_ERROR_MSG(JSERR_JSONInvalidReplacer, 5035, "", "Invalid replacer argument", kjstError, 0)
RT_ERROR_MSG(JSERR_InvalidAttributeTrue,5036,"'%s' attribute on the property descriptor cannot be set to 'true' on this object","",kjstTypeError, 0)
RT_ERROR_MSG(JSERR_InvalidAttributeFalse,5037,"'%s' attribute on the property descriptor cannot be set to 'false' on this object","",kjstTypeError, 0)
RT_ERROR_MSG(JSERR_ArgListTooLarge, 5038, "", "Argument list too large to apply", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_ConstRedeclaration, 5039, "Redeclaration of const '%s'", "Redeclaration of const property", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_CyclicProtoValue, 5040, "", "Cyclic __proto__ value", kjstError, 0)

RT_ERROR_MSG(JSERR_CantDeleteExpr, 5041, "Calling delete on '%s' is not allowed in strict mode", "Object member not configurable", kjstTypeError, 0) // string 4
RT_ERROR_MSG(JSERR_RefErrorUndefVariable, 5042, "", "Variable undefined in strict mode",  kjstReferenceError, 0) // string 10
RT_ERROR_MSG(JSERR_AccessRestrictedProperty, 5043, "", "'arguments', 'callee' and 'caller' are restricted function properties and cannot be accessed in this context", kjstTypeError, 0)
// 5044 - Removed
RT_ERROR_MSG(JSERR_CantAssignToReadOnly, 5045, "", "Assignment to read-only properties is not allowed in strict mode", kjstTypeError, 0) // string 5
RT_ERROR_MSG(JSERR_NonExtensibleObject, 5046, "", "Cannot create property for a non-extensible object", kjstTypeError, 0) // string 6

RT_ERROR_MSG(JSERR_Property_CannotSet_NullOrUndefined, 5047, "Unable to set property '%s' of undefined or null reference", "Object expected", kjstTypeError, JSERR_NeedObject)
RT_ERROR_MSG(JSERR_Property_CannotGet_NullOrUndefined, 5048, "Unable to get property '%s' of undefined or null reference", "Object expected", kjstTypeError, JSERR_NeedObject)
RT_ERROR_MSG(JSERR_Property_CannotDelete_NullOrUndefined, 5049, "Unable to delete property '%s' of undefined or null reference", "Object expected", kjstTypeError, JSERR_NeedObject)
// 5050 - Removed
RT_ERROR_MSG(JSERR_Property_NeedFunction, 5051, "The value of the property '%s' is not a Function object", "Function expected", kjstTypeError, JSERR_NeedFunction)
RT_ERROR_MSG(JSERR_Property_NeedFunction_NullOrUndefined, 5052, "The value of the property '%s' is null or undefined, not a Function object", "Function expected", kjstTypeError, JSERR_NeedObject)
RT_ERROR_MSG(JSERR_Property_CannotHaveAccessorsAndValue, 5053, "", "Invalid property descriptor: cannot both specify accessors and a 'value' attribute", kjstTypeError, VBSERR_ActionNotSupported)

RT_ERROR_MSG(JSERR_This_NullOrUndefined, 5054, "%s: 'this' is null or undefined", "'this' is null or undefined", kjstTypeError, JSERR_NeedObject) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedObject, 5055, "%s: 'this' is not an Object", "Object expected", kjstTypeError, JSERR_NeedObject) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedFunction, 5056,  "%s: 'this' is not a Function object", "Function expected", kjstTypeError, JSERR_NeedFunction) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedString, 5057,  "%s: 'this' is not a String object", "String expected", kjstTypeError, JSERR_NeedString) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedBoolean, 5058,  "%s: 'this' is not a Boolean object", "Boolean expected", kjstTypeError, JSERR_NeedBoolean) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedDate, 5059,  "%s: 'this' is not a Date object", "Date expected", kjstTypeError, JSERR_NeedDate) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedNumber, 5060,  "%s: 'this' is not a Number object", "Number expected", kjstTypeError, JSERR_NeedNumber) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedVBArray, 5061, "%s: 'this' is not a VBArray object", "VBArray expected", kjstTypeError, JSERR_NeedVBArray) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedInternalObject, 5062, "%s: 'this' is not a JavaScript object", "JavaScript object expected", kjstTypeError, JSERR_NeedInternalObj) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedEnumerator, 5063, "%s: 'this' is not an Enumerator object", "Enumerator object expected", kjstTypeError, JSERR_NeedEnumerator) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedRegExp, 5064, "%s: 'this' is not a RegExp object", "RegExp object expected", kjstTypeError, JSERR_NeedRegExp) // {Locked="\'this\'"}

RT_ERROR_MSG(JSERR_FunctionArgument_Invalid, 5065, "%s: invalid argument", "Invalid function argument", kjstTypeError, VBSERR_IllegalFuncCall)
RT_ERROR_MSG(JSERR_FunctionArgument_NeedObject, 5066, "%s: argument is not an Object", "Object expected", kjstTypeError, JSERR_NeedObject)
RT_ERROR_MSG(JSERR_FunctionArgument_NeedInternalObject, 5067, "%s: argument is not a JavaScript object", "JavaScript object expected", kjstTypeError, JSERR_NeedInternalObj)
RT_ERROR_MSG(JSERR_FunctionArgument_NeedFunction, 5068, "%s: argument is not a Function object", "Function expected", kjstTypeError, JSERR_NeedFunction)
RT_ERROR_MSG(JSERR_FunctionArgument_NeedVBArray, 5069, "%s: argument is not a VBArray object", "VBArray expected", kjstTypeError, JSERR_NeedVBArray)
RT_ERROR_MSG(JSERR_FunctionArgument_NullOrUndefined, 5070, "%s: argument is null or undefined", "Object expected", kjstTypeError, JSERR_NeedObject)
RT_ERROR_MSG(JSERR_FunctionArgument_NotObjectOrNull, 5071, "%s: argument is not an Object and is not null", "Object expected", kjstTypeError, JSERR_NeedObject)
RT_ERROR_MSG(JSERR_FunctionArgument_InvalidLength, 5072, "%s: argument does not have a valid 'length' property", "Invalid 'length' property", kjstTypeError, VBSERR_ActionNotSupported)
RT_ERROR_MSG(JSERR_FunctionArgument_NeedArrayOrArguments, 5073, "%s: Array or arguments object expected", "Array or arguments object expected", kjstTypeError, JSERR_NeedArrayOrArg)

RT_ERROR_MSG(JSERR_Operand_Invalid_NeedObject, 5074, "Invalid operand to '%s': Object expected", "Invalid Operand", kjstTypeError, JSERR_NeedObject)
RT_ERROR_MSG(JSERR_Operand_Invalid_NeedFunction, 5075, "Invalid operand to '%s': Function expected", "Invalid Operand", kjstTypeError, JSERR_NeedFunction)
RT_ERROR_MSG(JSERR_PropertyDescriptor_Invalid, 5076, "Invalid descriptor for property '%s'", "Invalid property descriptor", kjstTypeError, JSERR_NeedObject)

RT_ERROR_MSG(JSERR_DefineProperty_NotExtensible, 5077, "Cannot define property '%s': object is not extensible", "Cannot define property: object is not extensible", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_DefineProperty_NotConfigurable, 5078, "Cannot redefine non-configurable property '%s'", "Cannot redefine non-configurable property", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_DefineProperty_NotWritable, 5079, "Cannot modify non-writable property '%s'", "Cannot modify non-writable property", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_DefineProperty_LengthNotWritable, 5080, "Cannot modify property '%s': 'length' is not writable", "Cannot modify property: 'length' is not writable", kjstTypeError, 0) // {Locked="\'length\'"}
RT_ERROR_MSG(JSERR_DefineProperty_Default, 5081, "Cannot define property '%s'", "Cannot define property", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_InvalidTypedArray_Constructor, 5082, "", "Typed array constructor argument is invalid", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_This_NeedTypedArray, 5083, "", "'this' is not a typed array object", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_InvalidTypedArrayLength, 5084, "", "Invalid offset/length when creating typed array", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_InvalidTypedArraySubarrayLength, 5085, "", "Invalid begin/end value in typed array subarray method", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_TypedArray_NeedSource, 5086, "", "Invalid source in typed array set", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_This_NeedDataView, 5087, "", "'this' is not a DataView object", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_DataView_NeedArgument, 5088, "Required argument %s in DataView method is not specified", "Invalid arguments in DataView", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_DataView_InvalidOffset, 5089, "", "DataView operation access beyond specified buffer length", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_DataView_InvalidArgument, 5090, "DataView constructor argument %s is invalid", "Invalid arguments in DataView", kjstRangeError, 0)

RT_ERROR_MSG(JSERR_InvalidFunctionSignature, 5091, "The function '%s' has an invalid signature and cannot be called", "invalid function signature", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_InvalidPropertySignature, 5092, "The property '%s' has an invalid signature and cannot be accessed", "invalid property signature", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_InvalidRTCPropertyValueIn, 5093, "The runtimeclass %s that has Windows.Foundation.IPropertyValue as default interface is not supported as an input parameter type", "invalid input parameter type", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_RTCInvalidRTCPropertyValueOut, 5094, "The object with interface Windows.Foundation.IPropertyValue that has runtimeclass name %s is not supported as an output parameter", "invalid output parameter", kjstTypeError, 0)
// 5095 - Removed
RT_ERROR_MSG(JSERR_This_NeedInspectableObject, 5096, "%s: 'this' is not an Inspectable Object", "Inspectable Object expected", kjstTypeError, JSERR_NeedObject) // {Locked="\'this\'"}

RT_ERROR_MSG(JSERR_FunctionArgument_NeedWinRTChar, 5097, "%s: could not convert argument to type 'char'", "Could not convert argument to type 'char'", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_FunctionArgument_NeedWinRTGUID, 5098, "%s: could not convert argument to type 'GUID'", "Could not convert argument to type 'GUID'", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_ReturnValue_NeedInspectable, 5099, "%s: could not convert return value to IInspectable", "IInspectable expected", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_MissingStructProperty, 5100, "Could not convert object to struct: object missing expected property '%s'", "Could not convert object to struct: object missing expected property", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_UnknownType, 5101, "Type '%s' not found", "Unknown type", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_WinRTFunction_TooFewArguments, 5102, "%s: function called with too few arguments", "Function called with too few arguments", kjstError, 0)
RT_ERROR_MSG(JSERR_UnconstructableClass, 5103, "%s: type is not constructible", "Type is not constructible", kjstError, 0)
RT_ERROR_MSG(JSERR_InvalidPropertyValue, 5104, "Could not convert value to PropertyValue: %s not supported by PropertyValue", "Could not convert value to PropertyValue: Type not supported by PropertyValue", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_InvalidIInspectable, 5105, "Could not convert value to IInspectable: %s not supported by IInspectable", "Could not convert value to IInspectable: Type not supported by IInspectable", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_OutOfDateTimeRange, 5106, "", "Could not convert Date to Windows.Foundation.DateTime: value outside of valid range", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_OutOfTimeSpanRange, 5107, "", "Could not convert value to Windows.Foundation.TimeSpan: value outside of valid range", kjstRangeError, 0)

RT_ERROR_MSG(JSERR_This_ReleasedInspectableObject, 5108, "%s: The Inspectable object 'this' is released and cannot be accessed", "Invalid access to already released Inspectable Object", kjstReferenceError, JSERR_NeedObject) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_AlreadyReleasedInspectableObject, 5109, "", "Cannot release already released Inspectable Object", kjstReferenceError, JSERR_NeedObject) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedWinRTType, 5110, "'this' is not of expected type: %s", "'this' is not of the expected type", kjstTypeError, JSERR_NeedObject) // {Locked="\'this\'"}

RT_ERROR_MSG(JSERR_IllegalArraySizeAndLength, 5111, "", "Illegal length and size specified for the array", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_UnexpectedMetadataFailure, 5112, "%s: an unexpected failure occurred while trying to obtain metadata information", "An unexpected failure occurred while trying to obtain metadata information", kjstError, 0)
RT_ERROR_MSG(JSERR_UseBeforeDeclaration, 5113, "", "Use before declaration", kjstReferenceError, 0)

RT_ERROR_MSG(JSERR_ObjectIsAlreadyInitialized, 5114, "Cannot initialize '%s' object: 'this' is already initialized as '%s' object", "Cannot re-initialize 'this', object already initialized", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_ObjectIsNonExtensible, 5115, "Cannot initialize '%s' object: 'this' is not extensible", "Cannot initialize 'this' because it is a non-extensible object", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NeedObjectOfType, 5116, "%s: 'this' is not a %s object", "'this' is not of the expected type", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_WeakMapSetKeyNotAnObject, 5117, "%s: 'key' is not an object", "'key' is not an object", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_OptionValueOutOfRange, 5118, "Option value '%s' for '%s' is outside of valid range. Expected: %s", "Option value is outside of valid range", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_NeedObjectOrString, 5119, "%s is not an object or a string", "Object or string expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NotAConstructor, 5120, "Function '%s' is not a constructor", "Function is not a constructor", kjstTypeError, 0)

//Intl Specific
RT_ERROR_MSG(JSERR_LocaleNotWellFormed, 5121, "Locale '%s' is not well-formed", "Locale is not well-formed", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_InvalidCurrencyCode, 5122, "Currency code '%s' is invalid", "Currency code is invalid", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_MissingCurrencyCode, 5123, "", "Currency code was not specified", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_InvalidDate, 5124, "", "Invalid Date", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_IntlNotAvailable, 5125, "", "Intl is not available.", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_IntlNotImplemented, 5126, "", "Intl operation '%s' is not implemented.", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_ArgumentOutOfRange, 5130, "%s: argument out of range", "argument out of range", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_ErrorOnNew, 5131, "", "Function is not a constructor", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_This_NeedArrayIterator, 5132, "%s: 'this' is not a Array Iterator object", "Array Iterator expected", kjstTypeError, 0) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedMapIterator, 5133, "%s: 'this' is not a Map Iterator object", "Map Iterator expected", kjstTypeError, 0) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedSetIterator, 5134, "%s: 'this' is not a Set Iterator object", "Set Iterator expected", kjstTypeError, 0) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedStringIterator, 5135, "%s: 'this' is not a String Iterator object", "String Iterator expected", kjstTypeError, 0) // {Locked="\'this\'"}

RT_ERROR_MSG(JSERR_InvalidSpreadArgument, 5140, "%s: argument cannot be spread; expected Array or Object with a 'length' property", "Argument cannot be spread; expected Array or Object with a 'length' property", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_InvalidSpreadLength, 5141, "%s: argument cannot be spread; the 'length' property must be a number or convert to a number", "Argument cannot be spread; the 'length' property must be a number or convert to a number", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_BadSuperReference, 5145, "", "Missing or invalid 'super' binding", kjstReferenceError, 0)
RT_ERROR_MSG(JSERR_DeletePropertyWithSuper, 5146, "Unable to delete property '%s' which has a super reference", "Unable to delete property with a super reference", kjstReferenceError, 0)

RT_ERROR_MSG(JSERR_DetachedTypedArray, 5147, "%s: The ArrayBuffer is detached.", "The ArrayBuffer is detached.", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_AsmJsCompileError, 5148, "%s: Compiling asm.js failed.", "Compiling asm.js failed.", kjstError, 0)
RT_ERROR_MSG(JSERR_ImmutablePrototypeSlot, 5149, "%s: Can't set the prototype of this object.", "Can't set the prototype of this object.", kjstTypeError, 0)

/* Error messages for misbehaved Async Operations for use in Promise.js */
RT_ERROR_MSG(ASYNCERR_NoErrorInErrorState, 5200, "", "Status is 'error', but getResults did not return an error", kjstError, 0)
RT_ERROR_MSG(ASYNCERR_InvalidStatusArg, 5201, "", "Missing or invalid status parameter passed to completed handler", kjstError, 0)
RT_ERROR_MSG(ASYNCERR_InvalidSenderArg, 5202, "", "Missing or invalid sender parameter passed to completed handler", kjstError, 0)

RT_ERROR_MSG(JSERR_InvalidCodePoint, 5600, "Invalid code point %s", "Invalid code point", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_InvalidNormalizationForm, 5601, "Normalization form '%s' is invalid. Expected one of: ['NFC', 'NFD', 'NFKC', 'NFKD'].", "Invalid normalization form. Expected one of: ['NFC', 'NFD', 'NFKC', 'NFKD']", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_InvalidUnicodeCharacter, 5602, "Failed to normalize: invalid or missing unicode character at index %d.", "Failed to normalize: invalid or missing unicode character.", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_FailedToNormalize, 5603, "Failed to normalize string.", "Failed to normalize string.", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_NeedArrayBufferObject, 5604, "%s is not an ArrayBuffer", "ArrayBuffer object expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NeedSymbol, 5605, "'%s' is not a symbol", "Symbol expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_This_NeedSymbol, 5606, "%s: 'this' is not a Symbol object", "Symbol expected", kjstTypeError, JSERR_NeedSymbol) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_RegExpNoCurlyBracket, 5607, "", "Expected '}' in regular expression", kjstSyntaxError, 0)

RT_ERROR_MSG(JSERR_NeedProxyArgument, 5608, "", "Proxy requires more than 1 arguments", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_InvalidProxyArgument, 5609, "Proxy argument %s is not a valid object", "Invalid Proxy argument", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_InvalidProxyObject, 5610, "Revocable method requires Proxy object", "Revocable method requires Proxy object", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_ErrorOnRevokedProxy, 5611, "method %s is called on a revoked Proxy object", "trap called on a revoked Proxy object", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_InconsistentTrapResult, 5612, "Invariant check failed for %s proxy trap", "Invariant check failed for proxy trap", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_PromiseSelfResolution, 5613, "", "Object used to resolve a promise creates a circular resolution", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NeedPromise, 5614, "'%s' is not a promise", "Promise expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_This_NeedPromise, 5615, "%s: 'this' is not a Promise object", "Promise expected", kjstTypeError, JSERR_NeedPromise) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERROR_SetPrototypeOf, 5616, "Failed to set prototype", "Failed to set prototype", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_ObjectIsNotInitialized, 5617, "%s: Object internal state is not initialized", "Object internal state is not initialized", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_GeneratorAlreadyExecuting, 5618, "%s: Cannot execute generator function because it is currently executing", "", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_LengthIsTooBig, 5619, "Length property would exceed maximum value in output from '%s'", "", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NonObjectFromIterable, 5620, "Iterable provided to %s must not return non-object or null value.", "", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_EmptyArrayAndInitValueNotPresent, 5621, "%s: Array contains no elements and initialValue is not provided", "", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_PromiseAllRejected, 5622, "", "Promise.any all promises rejected.", kjstAggregateError, 0)

// 5623-5626 Unused
RT_ERROR_MSG(JSERR_NeedConstructor, 5627, "'%s' is not a constructor", "Constructor expected", kjstTypeError, 0)

RT_ERROR_MSG(VBSERR_CantDisplayDate, 32812, "", "The specified date is not available in the current locale's calendar", kjstRangeError, 0)

RT_ERROR_MSG(JSERR_ClassThisAlreadyAssigned, 5628, "", "Multiple calls to 'super' in a class constructor are not allowed", kjstReferenceError, 0)
RT_ERROR_MSG(JSERR_ClassSuperInBaseClass, 5629, "", "Unexpected call to 'super' in a base class constructor", kjstReferenceError, 0)
RT_ERROR_MSG(JSERR_ClassDerivedConstructorInvalidReturnType, 5630, "", "Derived class constructor can return only object or undefined", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_ClassStaticMethodCannotBePrototype, 5631, "", "Class static member cannot be named 'prototype'", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_ClassConstructorCannotBeCalledWithoutNew, 5632, "%s: cannot be called without the new keyword", "Class constructor cannot be called without the new keyword", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_SimdBool32x4TypeMismatch, 5633, "SIMD.Bool32x4.%s: Invalid SIMD types for operation", "Expecting Bool32x4 values", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_SimdBool16x8TypeMismatch, 5634, "SIMD.Bool16x8.%s: Invalid SIMD types for operation", "Expecting Bool16x8 values", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_SimdBool8x16TypeMismatch, 5635, "SIMD.Bool8x16.%s: Invalid SIMD types for operation", "Expecting Bool8x16 values", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_SimdInt16x8TypeMismatch, 5636, "SIMD.Int16x8.%s: Invalid SIMD types for operation", "Expecting Int16x8 values", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_SimdUint32x4TypeMismatch, 5637, "SIMD.Uint32x4.%s: Invalid SIMD types for operation", "Expecting UInt32x4 values", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_SimdUint16x8TypeMismatch, 5638, "SIMD.Uint16x8.%s: Invalid SIMD types for operation", "Expecting UInt16x8 values", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_SimdUint8x16TypeMismatch, 5639, "SIMD.Uint8x16.%s: Invalid SIMD types for operation", "Expecting UInt8x16 values", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_FunctionArgument_FirstCannotBeRegExp, 5640, "%s: first argument cannot be a RegExp", "First argument cannot be a RegExp", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_RegExpExecInvalidReturnType, 5641, "%s: Return value of RegExp 'exec' is not an Object and is not null", "Return value of RegExp 'exec' is not an Object and is not null", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_ProxyTrapReturnedFalse, 5642, "Proxy trap `%s` returned false", "Proxy trap returned false", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_ModuleResolveExport, 5643, "Module export %s cannot be resolved", "Module export cannot be resolved", kjstSyntaxError, 0)
RT_ERROR_MSG(JSERR_ModuleResolveImport, 5644, "Module import %s cannot be resolved", "Module import cannot be resolved", kjstSyntaxError, 0)
RT_ERROR_MSG(JSERR_TooManyImportExports, 5645, "Module has too many import/export definitions", "Module has too many import/export definitions", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_CannotResolveModule, 5646, "HostResolveImportedModule failed to resolve module with specifier %s", "HostResolveImportedModule failed to resolve module", kjstReferenceError, 0)
RT_ERROR_MSG(JSERR_ResolveExportFailed, 5647, "Resolve export %s failed due to circular reference or resolved exports", "Resolve export failed due to circular reference or resolved exports", kjstSyntaxError, 0)

RT_ERROR_MSG(JSERR_ObjectCoercible, 5648, "", "Cannot convert null or undefined to object", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_SIMDConversion, 5649, "%s: cannot be converted to a number", "Cannot be converted to a number", kjstTypeError, 0)


// JSON.parse errors. When this happens we want to make it explicitly clear the issue is in JSON.parse and not in the code
RT_ERROR_MSG(JSERR_JsonSyntax, 5650, "JSON.parse Error: Unexpected input at position:%s", "JSON.parse syntax error", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_JsonNoColon, 5651,"JSON.parse Error: Expected ':' at position:%s", "JSON.parse syntax error", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_JsonNoRbrack, 5652, "JSON.parse Error: Expected ']' at position:%s", "JSON.parse syntax error", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_JsonNoRcurly, 5653, "JSON.parse Error: Expected '}' at position:%s", "JSON.parse syntax error", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_JsonBadNumber, 5654, "JSON.parse Error: Invalid number at position:%s", "JSON.parse syntax error", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_JsonIllegalChar, 5655, "JSON.parse Error: Invalid character at position:%s", "JSON.parse syntax error", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_JsonBadHexDigit, 5656, "JSON.parse Error: Expected hexadecimal digit at position:%s", "JSON.parse syntax error", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_JsonNoStrEnd, 5657, "JSON.parse Error: Unterminated string constant at position:%s", "JSON.parse syntax error", kjstTypeError, 0)

// Date.prototype[@@toPrimitive] invalid hint
RT_ERROR_MSG(JSERR_InvalidHint, 5658, "%s: invalid hint", "invalid hint", kjstTypeError, 0)

RT_ERROR_MSG(JSERR_This_NeedNamespace, 5659, "%s: 'this' is not a Module Namespace object", "Module Namespace object expected", kjstTypeError, JSERR_This_NeedNamespace) // {Locked="\'this\'"}
RT_ERROR_MSG(JSERR_This_NeedListIterator, 5660, "%s: 'this' is not a List Iterator object", "List Iterator expected", kjstTypeError, 0) 
RT_ERROR_MSG(JSERR_NeedSharedArrayBufferObject, 5661, "%s is not a SharedArrayBuffer", "SharedArrayBuffer object expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NeedTypedArrayObject, 5662, "", "Atomics function called with invalid typed array object", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_InvalidTypedArrayIndex, 5663, "", "Access index is out of range", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_InvalidOperationOnTypedArray, 5664, "", "The operation is not supported on this typed array type", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_CannotSuspendBuffer, 5665, "", "Current agent cannot be suspended", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_CantDeleteNonConfigProp, 5666, "Cannot delete non-configurable property '%s'", "Cannot delete non-configurable property", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_CantRedefineProp, 5667, "Cannot redefine property '%s'", "Cannot redefine property", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_FunctionArgument_NeedArrayLike, 5668, "%s: argument is not an array or array-like object", "Array or array-like object expected", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_FatalMemoryExhaustion, 5669, "", "Encountered a non-recoverable OOM", kjstError, 0)
RT_ERROR_MSG(JSERR_OutOfBoundString, 5670, "", "String length is out of bound", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_InvalidIterableObject, 5671, "%s : Invalid iterable object", "Invalid iterable object", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_InvalidIteratorObject, 5672, "%s : Invalid iterator object", "Invalid iterator object", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_NoAccessors, 5673, "Invalid property descriptor: accessors not supported on this object", "", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_RegExpInvalidEscape, 5674, "", "Invalid regular expression: invalid escape in unicode pattern", kjstSyntaxError, 0)
RT_ERROR_MSG(JSERR_RegExpTooManyCapturingGroups, 5675, "", "Regular expression cannot have more than 32,767 capturing groups", kjstRangeError, 0)
RT_ERROR_MSG(JSERR_ProxyHandlerReturnedFalse, 5676, "Proxy %s handler returned false", "Proxy handler returned false", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_UnicodeRegExpRangeContainsCharClass, 5677, "%s", "Character classes not allowed in a RegExp class range.", kjstSyntaxError, 0)
RT_ERROR_MSG(JSERR_DuplicateKeysFromOwnPropertyKeys, 5678, "%s", "Proxy's ownKeys trap returned duplicate keys", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_InvalidGloFuncDecl, 5679, "The global property %s is not configurable, writable, nor enumerable, therefore cannot be declared as a function", "", kjstTypeError, 0)
RT_ERROR_MSG(JSERR_YieldStarThrowMissing, 5680, "", "Yielded iterator does not have a 'throw' method", kjstTypeError, 0)

//Host errors
RT_ERROR_MSG(JSERR_HostMaybeMissingPromiseContinuationCallback, 5700, "", "Host may not have set any promise continuation callback. Promises may not be executed.", kjstTypeError, 0)

// Built In functions Errors
RT_ERROR_MSG(JSERR_BuiltInNotAvailable, 5800, "", "Built In functions are not available.", kjstTypeError, 0)

// WebAssembly Errors
RT_ERROR_MSG(WASMERR_WasmCompileError, 7000, "%s", "Compilation failed.", kjstWebAssemblyCompileError, 0)
RT_ERROR_MSG(WASMERR_Unreachable, 7001, "", "Unreachable Code", kjstWebAssemblyRuntimeError, 0)
RT_ERROR_MSG(WASMERR_NeedBufferSource, 7002, "%s is not a BufferSource", "BufferSource expected", kjstTypeError, 0)
RT_ERROR_MSG(WASMERR_NeedModule, 7003, "%s is not a WebAssembly.Module", "WebAssembly.Module expected", kjstTypeError, 0)
RT_ERROR_MSG(WASMERR_DataSegOutOfRange, 7004, "", "Data segment is out of range", kjstWebAssemblyLinkError, 0)
RT_ERROR_MSG(WASMERR_MutableGlobal, 7005, "", "Cannot export mutable global", kjstTypeError, 0)
 // 7006 free
RT_ERROR_MSG(WASMERR_InvalidGlobalRef, 7007, "", "Global initialization does not support forward reference", kjstTypeError, 0)
RT_ERROR_MSG(WASMERR_NeedMemoryObject, 7008, "%s is not a WebAssembly.Memory", "WebAssembly.Memory object expected", kjstWebAssemblyLinkError, 0)
RT_ERROR_MSG(WASMERR_InvalidTypeConversion, 7009, "Invalid WebAssembly type conversion %s to %s", "Invalid WebAssembly type conversion", kjstTypeError, 0)
RT_ERROR_MSG(WASMERR_DivideByZero, 7010, "", "Division by zero", kjstWebAssemblyRuntimeError, 0)
RT_ERROR_MSG(WASMERR_ExpectedAnyFunc, 7011, "%s is not AnyFunc", "AnyFunc expected", kjstTypeError, 0)
RT_ERROR_MSG(WASMERR_NeedTableObject, 7012, "%s is not a WebAssembly.Table", "WebAssembly.Table object expected", kjstWebAssemblyLinkError, 0)
RT_ERROR_MSG(WASMERR_NeedWebAssemblyFunc, 7013, "%s is not a WebAssembly exported function", "WebAssembly exported function expected", kjstTypeError, 0)
RT_ERROR_MSG(WASMERR_SignatureMismatch, 7014, "%s called with invalid signature", "Function called with invalid signature", kjstWebAssemblyRuntimeError, 0)
RT_ERROR_MSG(WASMERR_ElementSegOutOfRange, 7015, "", "Element segment is out of range", kjstWebAssemblyLinkError, 0)
RT_ERROR_MSG(WASMERR_TableIndexOutOfRange, 7016, "", "Table index is out of range", kjstWebAssemblyRuntimeError, 0)
RT_ERROR_MSG(WASMERR_ArrayIndexOutOfRange, 7017, "", "Memory index is out of range", kjstWebAssemblyRuntimeError, 0)
RT_ERROR_MSG(WASMERR_InvalidInstantiateArgument, 7018, "", "Invalid arguments to instantiate", kjstTypeError, 0)
RT_ERROR_MSG(WASMERR_WasmLinkError, 7019, "%s", "Linking failed.", kjstWebAssemblyLinkError, 0)
RT_ERROR_MSG(WASMERR_NeedResponse, 7020, "%s is not a Response", "Response expected", kjstTypeError, 0)
RT_ERROR_MSG(WASMERR_CantDetach, 7021, "", "Not allowed to detach WebAssembly.Memory buffer", kjstTypeError, 0)
RT_ERROR_MSG(WASMERR_BufferGrowOnly, 7022, "", "WebAssembly.Memory can only grow", kjstTypeError, 0)
RT_ERROR_MSG(WASMERR_LinkSignatureMismatch, 7023, "Cannot link import %s in link table due to a signature mismatch", "Function called with invalid signature", kjstWebAssemblyRuntimeError, 0)
RT_ERROR_MSG(WASMERR_MemoryCreateFailed, 7024, "", "Failed to create WebAssembly.Memory", kjstTypeError, 0)
RT_ERROR_MSG(WASMERR_NeedInstanceObject, 7025, "%s is not a WebAssembly.Instance", "WebAssembly.Instance object expected", kjstWebAssemblyRuntimeError, 0)

RT_ERROR_MSG(WASMERR_InvalidImportModule, 7026, "Import module '%s' is invalid", "Import module is invalid", kjstTypeError, 0)
RT_ERROR_MSG(WASMERR_InvalidImport, 7027, "Import '%s.%s' is invalid. Expected type %s", "Import is invalid", kjstTypeError, 0)
RT_ERROR_MSG(WASMERR_InvalidInitialSize, 7028, "Imported %s initial size (%u) is smaller than declared (%u)", "Invalid initial size", kjstWebAssemblyLinkError, 0)
RT_ERROR_MSG(WASMERR_InvalidMaximumSize, 7029, "Imported %s maximum size (%u) is larger than declared (%u)", "Invalid maximum size", kjstWebAssemblyLinkError, 0)
RT_ERROR_MSG(WASMERR_InvalidMemoryType, 7030, "Imported %s type is %s, expected %s", "Invalid memory type", kjstWebAssemblyLinkError, 0)
RT_ERROR_MSG(WASMERR_UnalignedAtomicAccess, 7031, "", "Atomic memory access is unaligned", kjstWebAssemblyRuntimeError, 0)
RT_ERROR_MSG(WASMERR_SharedNoMaximum, 7032, "", "Shared memory must have a maximum size", kjstTypeError, 0)

// Wabt Errors
RT_ERROR_MSG(WABTERR_WabtError, 7200, "%s", "Wabt Error.", kjstTypeError, 0)

// Implicit Conversion Errors
RT_ERROR_MSG(JSERR_ImplicitStrConv, 7300, "No implicit conversion of %s to String", "Cannot implicitly convert to String", kjstTypeError, 0)
