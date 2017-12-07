#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------
import xml.dom.minidom as DOM

lttngDataTypeMapping = {
    "win:null"          :" ",
    "win:Int64"         :"const __int64",
    "win:ULong"         :"const unsigned long",
    "win:count"         :"*",
    "win:Struct"        :"const char *",
    "win:GUID"          :"const int",
    "win:AnsiString"    :"const char*",
    "win:UnicodeString" :"const char*",
    "win:Double"        :"const double",
    "win:Int32"         :"const signed int",
    "win:HexInt32"      :"const signed int",
    "win:Boolean"       :"const bool",
    "win:UInt64"        :"const unsigned __int64",
    "win:UInt32"        :"const unsigned int",
    "win:UInt16"        :"const unsigned short",
    "win:UInt8"         :"const unsigned char",
    "win:Int8"          :"const char",
    "win:Pointer"       :"const uintptr_t",
    "win:Binary"        :"const char"
}

ctfDataTypeMapping = {
    "win:Int64"         :"ctf_integer",
    "win:HexInt64"      :"ctf_integer_hex",
    "win:ULong"         :"ctf_integer",
    "win:count"         :"ctf_sequence",
    "win:Struct"        :"ctf_sequence",
    "win:GUID"          :"ctf_sequence",
    "win:AnsiString"    :"ctf_string",
    "win:UnicodeString" :"ctf_string",
    "win:Double"        :"ctf_float",
    "win:Int32"         :"ctf_integer",
    "win:HexInt32"      :"ctf_integer_hex",
    "win:Boolean"       :"ctf_integer",
    "win:UInt64"        :"ctf_integer",
    "win:UInt32"        :"ctf_integer",
    "win:UInt16"        :"ctf_integer",
    "win:HexInt16"      :"ctf_integer_hex",
    "win:UInt8"         :"ctf_integer",  #actually a character
    "win:Int8"          :"ctf_integer",  #actually a character
    "win:Pointer"       :"ctf_integer",
    "win:Binary"        :"ctf_sequence",
    "xs:string"         :"ctf_string",
    "xs:unsignedLong"   :"ctf_integer",
    "xs:unsignedInt"    :"ctf_integer"
}

palDataTypeMapping ={
        "win:null"          :" ",
        "win:Int64"         :"const __int64",
        "win:ULong"         :"const unsigned long",
        "win:count"         :"*",
        "win:Struct"        :"const void",
        "win:GUID"          :"const GUID",
        "win:AnsiString"    :"LPCSTR",
        "win:UnicodeString" :"PCWSTR",
        "win:Double"        :"const double",
        "win:Int32"         :"const signed int",
        "win:HexInt32"      :"const signed int",
        "win:Boolean"       :"const bool",
        "win:UInt64"        :"const unsigned __int64",
        "win:UInt32"        :"const unsigned int",
        "win:UInt16"        :"const unsigned short",
        "win:UInt8"         :"const unsigned char",
        "win:Int8"          :"const char",
        "win:Pointer"       :"const void*",
        "win:Binary"        :"const char"
        }

MAX_LTTNG_ARGS = 10

def getParamSequenceSize(paramSequence, estimate):
    total = 0
    pointers =0
    for param in paramSequence:
        if param in ["win:Int64", "win:UInt64", "win:Double"]:
            total += 8
        elif param in ["win:ULong", "win:Int32", "win:Boolean",]:
            total += 4
        elif param == "GUID":
            total += 16
        elif param in ["win:UInt16"]:
            total += 2
        elif param in ["win:Uint8", "win:Binary"]:
            total += 1
        elif param == "win:Pointer":
            if estimate:
                total += 8
            else:
                pointers += 1
        elif estimate:
            if param in ["win:AnsiString", "win:Struct"]:
                total += 32
            elif param in ["win:UnicodeString"]:
                total += 64
        else:
            raise Exception ("Don't know size of " + param)

    if estimate:
        return total

    return total, pointers

class Template:
    def __repr__(self):
        return "<Template " + self.name + " />"
    
    def __init__(self, name, prototypes, dependencies, structCounts, arrayCounts):
        self.name = name
        self.signature = FunctionSignature()
        self.structCounts = structCounts
        self.arrayCounts = arrayCounts

        for variable in prototypes.paramList:
            for dependency in dependencies[variable]:
                if not self.signature.getParam(dependency):
                    self.signature.append(dependency, prototypes.getParam(dependency))
    @property
    def num_params(self):
        return len(self.signature.paramList)

    def getParam(self, name):
        return self.signature.getParam(name)

    @property
    def estimatedSize(self):
        total = getParamSequenceSize((self.getParam(paramName).winType for paramName in self.signature.paramList), True)

        if total < 32:
            return 32
        elif total > 1024:
            return 1024
        return total

class FunctionSignature:
    def __repr__(self):
        return ', '.join(self.paramList)
        
    def __init__(self):
        self.LUT = {}
        self.paramList = []

    def append(self, variable, param):
        self.LUT[variable] = param
        self.paramList.append(variable)

    def getParam(self, variable):
        return self.LUT.get(variable)

    def getLength(self):
        return len(self.paramList)

class FunctionParameter:
    def __repr__(self):
        return self.name

    def __init__(self, winType, name, count, outType, length):
        self.winType = winType
        self.outType = outType
        self.name = name
        self.length = length
        self.count = "win:null"
        if winType == "win:GUID" or count == "win:count":
            self.count = "win:count"

ignoredXmlAttributes = frozenset(["map"])
usedXmlAttributes    = frozenset(["name", "inType", "count", "length", "outType"])
knownXmlAttributes   = ignoredXmlAttributes | usedXmlAttributes
            
def checkKnownAttributes(nodes, templateName):
    for node in nodes:
        nodeMap = node.attributes
        for attribute in nodeMap.values():
            if attribute.name not in knownXmlAttributes:
                raise ValueError('Unknown attribute: ' + attribute.name + ' in template ' + templateName)

def getTopLevelElementsByTagName(node, tag):
    return [e for e in node.getElementsByTagName(tag) if e.parentNode == node]
            
def parseTemplateNodes(templateNodes):
    templates = {}

    for templateNode in templateNodes:
        templateName = templateNode.getAttribute('tid')
        dataNodes = getTopLevelElementsByTagName(templateNode, 'data')

        checkKnownAttributes(dataNodes, templateName)

        functionPrototypes = FunctionSignature()
        
        arrayCounts = {}
        structCounts = {}
        var_Dependencies = {}

        for dataNode in dataNodes:
            variable = dataNode.getAttribute('name')
            wintype = dataNode.getAttribute('inType')
            outType = dataNode.getAttribute('outType')

            wincount = dataNode.getAttribute('count')
            winLength = dataNode.getAttribute('length')

            var_dependency = [variable]
            if winLength:
                if wincount:
                    raise Exception("Both count and length properties found on " + variable + " in template " + templateName)

            if wincount.isdigit() and int(wincount) == 1:
                wincount = ''

            if wincount:
                if wincount.isdigit():
                    raise Exception("Expect constant count to be length")
                elif functionPrototypes.getParam(wincount):
                    var_dependency.insert(0, wincount)
                    arrayCounts[variable] = wincount


            var_Dependencies[variable] = var_dependency
            functionParameter = FunctionParameter(wintype, variable, wincount, outType, winLength)
            functionPrototypes.append(variable, functionParameter)

        structNodes = getTopLevelElementsByTagName(templateNode, 'struct')

        for structNode in structNodes:
            structName = structNode.getAttribute('name')
            countName  = structNode.getAttribute('count')

            assert(countName in functionPrototypes.paramList)

            #childData = structNode.getElementsByTagName("data")
            #names = [x.attributes['name'].value for x in childData]
            #types = [x.attributes['inType'].value for x in childData]

            structCounts[structName] = countName
            var_Dependencies[structName] = [countName, structName]
            functionParameterPointer = FunctionParameter("win:Struct", structName, "win:count", None, None)
            functionPrototypes.append(structName, functionParameterPointer)

        templates[templateName] = Template(templateName, functionPrototypes, var_Dependencies, structCounts, arrayCounts)

    return templates
            
def shouldPackTemplate(template):
    return template.num_params > MAX_LTTNG_ARGS or len(template.structCounts) > 0 or len(template.arrayCounts) > 0

def generateArgList(template):
    # Construct a TP_ARGS macro call, as defined in another macro, e.g.
    #
    # TP_ARGS(                \
    #    int, my_integer_arg, \
    #    char*, my_string_arg \
    # )
    header = "TP_ARGS( \\\n"
    footer = "\\\n)"

    args = []

    if shouldPackTemplate(template):
        args.append("        const unsigned int, length")
        args.append("        const char *, __data__")
    else:
        signature = template.signature
        for param in signature.paramList:
            functionParam = signature.getParam(param)
            wintypeName   = functionParam.winType
            mappedType    = lttngDataTypeMapping[wintypeName]
            winCount      = functionParam.count
            mappedCount   = lttngDataTypeMapping[winCount]

            arg = "        " + mappedType
            if mappedCount != " ":
                arg += mappedCount
            elif functionParam.length:
                arg += "*"
            arg += ", " + functionParam.name
            
            args.append(arg)
        
    return header + ", \\\n".join(args) + footer

def generateFieldList(template):
    # Construct a TP_FIELDS macro call, e.g.
    # TP_FIELDS(
    #     ctf_string(my_string_field, my_string_arg)
    #     ctf_integer(int, my_integer_field, my_integer_arg)
    # )
    header = "    " + " TP_FIELDS(\n"
    footer = "\n    )"

    fieldList = []
    
    if shouldPackTemplate(template):
        fieldList.append("      ctf_integer(unsigned long, length, length)")
        fieldList.append("      ctf_sequence(char, __data__, __data__, unsigned long, length)")
    else:
        signature = template.signature
        for param in signature.paramList:
            functionParam = signature.getParam(param)
            wintypeName   = functionParam.winType
            winCount      = functionParam.count
            mappedCount   = lttngDataTypeMapping[winCount]
            mappedType    = lttngDataTypeMapping[wintypeName].replace("const ", "")

            if functionParam.outType:
                wintypeName = functionParam.outType

            ctf_type = None
            field_body = None
            varname = functionParam.name

            if param in template.structCounts or param in template.arrayCounts:
                # This is a struct, treat as a sequence
                countVar = template.structCounts.get(param, template.arrayCounts.get(param))
                ctf_type = "ctf_sequence"
                field_body = ", ".join((mappedType, varname, varname, "size_t", functionParam.prop))
            elif functionParam.length:
                ctf_type = "ctf_sequence"
                field_body = ", ".join((mappedType, varname, varname, "size_t", functionParam.length))
            else:
                ctf_type = ctfDataTypeMapping[wintypeName]
                if ctf_type == "ctf_string":
                    field_body = ", ".join((varname, varname))
                elif ctf_type == "ctf_integer" or ctf_type == "ctf_integer_hex" or ctf_type == "ctf_float":
                    field_body = ", ".join((mappedType, varname, varname))
                elif ctf_type == "ctf_sequence":
                    raise Exception("ctf_sequence needs special handling: " + template.name + " " + param)
                else:
                    raise Exception("Unhandled ctf intrinsic: " + ctf_type)

#            fieldList.append("//    " + wintypeName)
            fieldList.append("      %s(%s)" % (ctf_type, field_body))
    
    return header + "\n".join(fieldList) + footer

def generateLttngHeader(providerName, lttngEventHeaderShortName, templates, events):
    headerLines = []

    headerLines.append("")
    headerLines.append("#ifdef __int64")
    headerLines.append("#if TARGET_64")
    headerLines.append("#undef __int64")
    headerLines.append("#else")
    headerLines.append("#error \"Linux and OSX builds only support 64bit platforms\"")
    headerLines.append("#endif // TARGET_64")
    headerLines.append("#endif // __int64")
    headerLines.append("#undef TRACEPOINT_PROVIDER")
    headerLines.append("#undef TRACEPOINT_INCLUDE")
    headerLines.append("")
    headerLines.append("#define TRACEPOINT_PROVIDER " + providerName + "\n")
    headerLines.append("#define TRACEPOINT_INCLUDE \"./" + lttngEventHeaderShortName + "\"\n\n")

    headerLines.append("#if !defined(LTTNG_CHAKRA_H" + providerName + ") || defined(TRACEPOINT_HEADER_MULTI_READ)\n\n")
    headerLines.append("#define LTTNG_CHAKRA_H" + providerName +"\n")

    headerLines.append("\n#include <lttng/tracepoint.h>\n\n")

    
    for templateName in templates:
        template = templates[templateName]
        functionSignature = template.signature

        headerLines.append("")
        headerLines.append("#define " + templateName + "_TRACEPOINT_ARGS \\")

        tracepointArgs = generateArgList(template)
        headerLines.append(tracepointArgs)

        headerLines.append("TRACEPOINT_EVENT_CLASS(")
        headerLines.append("    " + providerName + ",")
        headerLines.append("    " + templateName + ",")
        headerLines.append("    " + templateName + "_TRACEPOINT_ARGS,")
        tracepointFields = generateFieldList(template)
        headerLines.append(tracepointFields)
        headerLines.append(")")
                        
        headerLines.append("#define " + templateName + "T_TRACEPOINT_INSTANCE(name) \\")
        headerLines.append("TRACEPOINT_EVENT_INSTANCE(\\")
        headerLines.append("    " + providerName + ",\\")
        headerLines.append("    " + templateName + ",\\")
        headerLines.append("    name,\\")
        headerLines.append("    " + templateName + "_TRACEPOINT_ARGS \\")
        headerLines.append(")")

    headerLines.append("")
    headerLines.append("")
    headerLines.append("TRACEPOINT_EVENT_CLASS(")
    headerLines.append("    " + providerName + ",")
    headerLines.append("    emptyTemplate,")
    headerLines.append("    TP_ARGS(),")
    headerLines.append("    TP_FIELDS()")
    headerLines.append(")")
    headerLines.append("#define T_TRACEPOINT_INSTANCE(name) \\")
    headerLines.append("TRACEPOINT_EVENT_INSTANCE(\\")
    headerLines.append("    " + providerName + ",\\")
    headerLines.append("    emptyTemplate,\\")
    headerLines.append("    name,\\")
    headerLines.append("    TP_ARGS()\\")
    headerLines.append(")")

    headerLines.append("")

    for eventNode in events:
        eventName    = eventNode.getAttribute('symbol')
        templateName = eventNode.getAttribute('template')

        if not eventName:
            raise Exception(eventNode + " event does not have a symbol")
        if not templateName:
            headerLines.append("T_TRACEPOINT_INSTANCE(" + eventName + ")")
            continue

        headerLines.append(templateName + "T_TRACEPOINT_INSTANCE(" + eventName + ")")

    headerLines.append("#endif /* LTTNG_CHAKRA_H" + providerName + " */")
    headerLines.append("#include <lttng/tracepoint-event.h>")

    return "\n".join(headerLines)

def generateMethodBody(template, providerName, eventName):
    # Convert from ETW's windows types to LTTng compatiable types

    methodBody = [""]
    
    functionSignature = template.signature

    if not shouldPackTemplate(template):
        invocation = ["do_tracepoint(" + providerName, eventName]

        for paramName in functionSignature.paramList:
            functionParam = functionSignature.getParam(paramName)
            wintypeName   = functionParam.winType
            winCount      = functionParam.count
            
            ctf_type      = None

            if functionParam.outType:
                ctf_type = ctfDataTypeMapping.get(functionParam.outType)
            else:
                ctf_Type = ctfDataTypeMapping.get(winCount)

            if not ctf_type:
                ctf_type = ctfDataTypeMapping[wintypeName]

            if ctf_type == "ctf_string" and wintypeName == "win:UnicodeString":
                # Convert wchar unicode string to utf8
                if functionParam.length:
                    methodBody.append("utf8::WideToNarrow " + paramName + "_converter(" + paramName + ", " + functionParam.length + ");")
                else:
                    methodBody.append("utf8::WideToNarrow " + paramName + "_converter(" + paramName + ");")
                invocation.append(paramName + "_converter")
#            elif ctf_type == "ctf_sequence" or wintypeName == "win:Pointer":
            elif wintypeName == "win:Pointer":
                invocation.append("(" + lttngDataTypeMapping[wintypeName] + lttngDataTypeMapping[winCount] + ")" + paramName)
            else:
                invocation.append(paramName)

        methodBody.append(",\n        ".join(invocation) + ");")
    else:
        # Packing results into buffer
        methodBody.append("char stackBuffer[" + str(template.estimatedSize) + "];")
        methodBody.append("char *buffer = stackBuffer;")
        methodBody.append("int offset = 0;")
        methodBody.append("int size = " + str(template.estimatedSize) + ";")
        methodBody.append("bool fixedBuffer = true;")
        methodBody.append("bool success = true;")

        for paramName in functionSignature.paramList:
            functionParameter = functionSignature.getParam(paramName)

            if paramName in template.structCounts:
                size = "(unsigned int)" + paramName + "_ElementSize * (unsigned int)" + template.structCounts[paramName]
                methodBody.append("success &= WriteToBuffer((const char *)" + paramName + ", " + size + ", buffer, offset, size, fixedBuffer);")
            elif paramName in template.arrayCounts:
                size = "sizeof(" + lttngDataTypeMapping[functionParameter.winType] + ") * (unsigned int)" + template.arrayCounts[paramName]
                methodBody.append("success &= WriteToBuffer((const char *)" + paramName + ", " + size + ", buffer, offset, size, fixedBuffer);")
            elif functionParameter.winType == "win:GUID":
                methodBody.append("success &= WriteToBuffer(*" + paramName + ", buffer, offset, size, fixedBuffer);")
            else:
                methodBody.append("success &= WriteToBuffer(" + paramName + ", buffer, offset, size, fixedBuffer);")

        methodBody.append("if (!success)")
        methodBody.append("{")
        methodBody.append("    if (!fixedBuffer) delete[] buffer;")
        methodBody.append("    return ERROR_WRITE_FAULT;")
        methodBody.append("}")
        methodBody.append("do_tracepoint(" + providerName + ", " + eventName + ", offset, buffer);")
        methodBody.append("if (!fixedBuffer) delete[] buffer;")

    return "\n    ".join(methodBody) + "\n"

def generateMethodSignature(template):
    if not template:
        return ""
    
    functionSignature = template.signature
    lineFunctionPrototype = []
    for paramName in functionSignature.paramList:
        functionParameter = functionSignature.getParam(paramName)
        wintypeName = functionParameter.winType
        mappedType = palDataTypeMapping[wintypeName]
        winCount = functionParameter.count
        mappedCount = palDataTypeMapping[winCount]

        if paramName in template.structCounts:
            lineFunctionPrototype.append("    int " + paramName + "_ElementSize")
        # lineFunctionPrototype.append("//    " + wintypeName + " " + str(functionParameter.length))
        lineFunctionPrototype.append(
            "    " + mappedType
            + (mappedCount if mappedCount != " " else "*" if functionParameter.length and not wintypeName in ["win:UnicodeString", "win:AnsiString"] else "")
            + " "
            + functionParameter.name)
    return ",\n".join(lineFunctionPrototype)


def generateLttngTracepointProvider(providerName, lttngHeader, templates, events):
    providerLines = [];

    providerLines.append("#define TRACEPOINT_DEFINE")
    providerLines.append("#ifndef CHAKRA_STATIC_LIBRARY")
    providerLines.append("#define TRACEPOINT_PROBE_DYNAMIC_LINKAGE")
    providerLines.append("#endif")
    providerLines.append("#include \"stdlib.h\"")
    providerLines.append("#include \"Common.h\"")
    providerLines.append("#include \"Codex/Utf8Helper.h\"")
    
    providerLines.append("#include \"" + lttngHeader + "\"\n\n")
    providerLines.append("#ifndef tracepoint_enabled")
    providerLines.append("#define tracepoint_enabled(provider, name) 1")
    providerLines.append("#define do_tracepoint tracepoint")
    providerLines.append("#endif")

    providerLines.append("""
bool ResizeBuffer(char *&buffer, int&size, int currentLength, int newSize, bool &fixedBuffer)
{
    newSize *= 1.5;
    _ASSERTE(newSize > size); // Check for overflow

    if (newSize < 32)
    {
        newSize = 32;
    }

    char *newBuffer = new char[newSize];
    memcpy(newBuffer, buffer, currentLength);

    if (!fixedBuffer)
    {
        delete[] buffer;
    }

    buffer = newBuffer;
    size = newSize;
    fixedBuffer = false;
    return true;
}

bool WriteToBuffer(const char * src, int len, char *&buffer, int &offset, int &size, bool &fixedBuffer)
{
    if (!src)
    {
        return true;
    }
    if (offset + len > size)
    {
        if (!ResizeBuffer(buffer, size, offset, size+len, fixedBuffer))
        {
            return false;
        }
    }

    memcpy(buffer + offset, src, len);
    offset += len;
    return true;
}

template <typename T>
bool WriteToBuffer(const T &value, char *&buffer, int&offset, int&size, bool &fixedBuffer)
{
    if (sizeof(T) + offset > size)
    {
        if (!ResizeBuffer(buffer, size, offset, size + sizeof(T), fixedBuffer))
        {
            return false;
        }
    }

    *(T *)(buffer + offset) = value;
    offset += sizeof(T);
    return true;
}
""")

    for eventNode in events:
        eventName    = eventNode.getAttribute('symbol')
        templateName = eventNode.getAttribute('template')

        providerLines.append("extern \"C\" bool EventXplatEnabled%s(){ return tracepoint_enabled(%s, %s);}"
                             % (eventName, providerName, eventName))
        providerLines.append("")

        template = None
        if templateName:
            template = templates[templateName]

        providerLines.append("extern \"C\" unsigned long FireEtXplat" + eventName + "(")
        providerLines.append(generateMethodSignature(template))
        providerLines.append(")")
        providerLines.append("{")
        providerLines.append("    if (!EventXplatEnabled" + eventName + "())")
        providerLines.append("        return ERROR_SUCCESS;")

        if template:
            providerLines.append(generateMethodBody(template, providerName, eventName))
        else:
            providerLines.append("    do_tracepoint(" + providerName + ", " + eventName +");")

        providerLines.append("")
        providerLines.append("    return ERROR_SUCCESS;")
        providerLines.append("}")
        providerLines.append("")

    return "\n".join(providerLines)

def generateEtwHeader(templates, events):
    headerLines = []

    headerLines.append("#include \"pal.h\"")
    headerLines.append("")
    
    for event in events:
        eventName = event.getAttribute('symbol')
        templateName = event.getAttribute('template')

        template = None
        if templateName:
            template = templates[templateName]

        callArgs = []
        if template:
            functionSignature = template.signature
            for param in functionSignature.paramList:
                if param in template.structCounts:
                    callArgs.append(param + "_ElementSize")
                callArgs.append(param)

        headerLines.append("extern \"C\" bool EventXplatEnabled" + eventName +"();")
        headerLines.append("inline bool EventEnabled" + eventName +"() { return EventXplatEnabled" + eventName + "();}")
        headerLines.append("")
        headerLines.append("extern \"C\" unsigned long FireEtXplat" + eventName +" (")
        headerLines.append(generateMethodSignature(template))
        headerLines.append(");")
        headerLines.append("inline unsigned long EventWrite" + eventName + "(")
        headerLines.append(generateMethodSignature(template))
        headerLines.append(")")
        headerLines.append("{")
        headerLines.append("    return FireEtXplat" + eventName + "(" + ", ".join(callArgs) + ");")
        headerLines.append("}")
        headerLines.append("")
        

    return "\n".join(headerLines)

def generateCmakeFile(providerName):
    cmakeLines = []
    cmakeLines.append("project(Chakra.LTTng)")
    cmakeLines.append("")
    cmakeLines.append("add_compile_options(-fPIC)")
    cmakeLines.append("")
    cmakeLines.append("add_library (Chakra.LTTng OBJECT")
    cmakeLines.append("  eventprovider" + providerName + ".cpp")
    cmakeLines.append("  tracepointprovider" + providerName + ".cpp")
    cmakeLines.append(")")

    return "\n".join(cmakeLines)

def generateLttngFiles(manifest, providerDirectory):
    import os
    tree = DOM.parse(manifest)

    if not os.path.exists(providerDirectory):
        os.makedirs(providerDirectory)

    if not os.path.exists(providerDirectory + "/lttng"):
        os.makedirs(providerDirectory + "/lttng")

    for providerNode in tree.getElementsByTagName("provider"):
        providerName = providerNode.getAttribute("name")
        providerName = providerName.replace("Microsoft-", "")

        providerNameFile = providerName.lower()

        lttngEventHeaderShortName = "tp" + providerNameFile + ".h"
        lttngEventHeaderPath      = providerDirectory + "/lttng/" + lttngEventHeaderShortName
        lttngEventProvider        = providerDirectory + "/lttng/eventprovider" + providerNameFile + ".cpp"
        lttngEventProviderTrace   = providerDirectory + "/lttng/tracepointprovider" + providerNameFile + ".cpp"
        lttngEtwHeaderFile        = providerDirectory + "/lttng/" + providerNameFile + "Etw.h"
        lttngCmakeFile            = providerDirectory + "/lttng/CMakeLists.txt"

        lttngHeader              = open(lttngEventHeaderPath, "w")
        lttngImplementation      = open(lttngEventProvider, "w")
        lttngTraceImplementation = open(lttngEventProviderTrace, "w")
        lttngEtwHeader           = open(lttngEtwHeaderFile, "w")
        lttngCmake               = open(lttngCmakeFile, "w")

        # Create the lttng implementation
        lttngTraceImplementation.write("#define TRACEPOINT_CREATE_PROBES\n")
        lttngTraceImplementation.write("#include \"./"+lttngEventHeaderShortName+"\"\n")
        lttngTraceImplementation.close()

        # Create the lttng header

        templateNodes = providerNode.getElementsByTagName('template')
        eventNodes = providerNode.getElementsByTagName('event')

        allTemplates = parseTemplateNodes(templateNodes)

        lttngHeader.write(generateLttngHeader(providerName, lttngEventHeaderShortName, allTemplates, eventNodes))
        lttngHeader.close();

        lttngImplementation.write(generateLttngTracepointProvider(providerName, lttngEventHeaderShortName, allTemplates, eventNodes))
        lttngImplementation.close();

        lttngEtwHeader.write(generateEtwHeader(allTemplates, eventNodes))
        lttngEtwHeader.close()

        # Note: This in particular assumes that there is only one ETW provider
        lttngCmake.write(generateCmakeFile(providerNameFile))
        lttngCmake.close()

if __name__ == '__main__':
    import argparse
    import sys

    parser = argparse.ArgumentParser(description="Generates the Code required to instrument LTTtng logging mechanism")

    required = parser.add_argument_group('required arguments')
    required.add_argument('--man',  type=str, required=True,
                                    help='full path to manifest containig the description of events')
    required.add_argument('--intermediate', type=str, required=True,
                                    help='full path to eventprovider intermediate directory')
    args, unknown = parser.parse_known_args(sys.argv[1:])
    if unknown:
        print('Unknown argument(s): ', ', '.join(unknown))
        sys.exit(1)

    generateLttngFiles(args.man, args.intermediate)
    sys.exit(0)
