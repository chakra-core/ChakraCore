#-------------------------------------------------------------------------------------------------------
# Copyright (C) Microsoft. All rights reserved.
# Copyright (c) ChakraCore Project Contributors. All rights reserved.
# Licensed under the MIT license. See LICENSE.txt file in the project root for full license information.
#-------------------------------------------------------------------------------------------------------

using namespace System.Collections
using namespace System.IO
using namespace System.Security
using namespace System.Text.RegularExpressions
using namespace System.Xml

Set-StrictMode -Version 5.1

class PrimitiveMustacheRenderer {
    hidden static [string]$mustacheTagPatternFormat = (
        '(?<startDelimiter>\{{{{{0}}})\s*' + 
        '(?<tokenName>[\w-]+)' + 
        '\s*(?<endDelimiter>\}}{{{0}}})'
    )
    hidden static [regex]$mustacheTagsRegex = [regex]::new((
        # Tag that is replaced with an unescaped value (e.g. `{{{name}}}`).
        ([PrimitiveMustacheRenderer]::mustacheTagPatternFormat -f 3) +
        '|' +
        # Tag that is replaced with an escaped value (e.g. `{{name}}`).
        ([PrimitiveMustacheRenderer]::mustacheTagPatternFormat -f 2)
        
    ))

    static [string]RenderTemplate([string]$template, [Hashtable]$data) {
        return [PrimitiveMustacheRenderer]::mustacheTagsRegex.Replace(
            $template,
            {
                param([Match]$match)

                $startDelimiter = $match.Groups['startDelimiter'].Value
                $endDelimiter = $match.Groups['endDelimiter'].Value
                $tokenName = $match.Groups['tokenName'].Value
                $tokenValue = [string]::Empty

                if ($data.Contains($tokenName)) {
                    $tokenValue = $data[$tokenName]
                    if ($startDelimiter -eq '{{{' -and $endDelimiter -eq '}}}') {
                        # Skip escaping of the token value.
                        return $tokenValue
                    }

                    # Converts a token value into an XML-encoded string.
                    $tokenValue = [SecurityElement]::Escape($tokenValue)
                }

                return $tokenValue
            }
        )
    }

    static [bool]ContainsTag([string]$content) {
        return $content.Contains('{{') -and $content.Contains('}}')
    }
}

class Package {
    [ValidateNotNullOrEmpty()][string]$Id
    [ValidateNotNullOrEmpty()][string]$NuspecFile
    [Hashtable]$Properties
    [PreprocessableFile[]]$PreprocessableFiles

    Package([string]$id, [string]$nuspecFile, [Hashtable]$properties,
        [PreprocessableFile[]]$preprocessableFiles
    ) {
        $this.Id = $id
        $this.NuspecFile = $nuspecFile
        $this.Properties = $properties
        $this.PreprocessableFiles = $preprocessableFiles
    }

    static [Package[]]GetPackages([string]$packageDataFile) {
        $packageDataXml = [xml](Get-Content $packageDataFile -Encoding utf8)
        $packageDataElem = $packageDataXml.DocumentElement
        $commonPropertiesElem = $packageDataElem.commonProperties
        $packageElems = $packageDataElem.packages.SelectNodes('child::*')

        $packageDataDir = Split-Path -Parent $packageDataFile
        $commonProperties = [Package]::GetPackageCommonProperties($commonPropertiesElem)
        $packageCount = $packageElems.Count
        $packages = [Package[]]::new($packageCount)

        for ($packageIndex = 0; $packageIndex -lt $packageCount; $packageIndex++) {
            $packageElem = $packageElems[$packageIndex]
            $propertyElems = $null
            if ($packageElem['properties']) {
                $propertyElems = $packageElem.properties.SelectNodes('child::*')
            }
            $preprocessableFileElems = $null
            if ($packageElem['preprocessableFiles']) {
                $preprocessableFileElems = $packageElem.preprocessableFiles.SelectNodes('child::*')
            }

            $packageId = $packageElem.id
            $packageNuspecFile = (Join-Path $packageDataDir $packageElem.nuspecFile)
            $packageProperties = [Package]::ConvertXmlElementsToHashtable($propertyElems)
            $packageProperties = [Package]::MergePackageProperties($commonProperties,
                $packageProperties)
            $packagePreprocessableFiles = [Package]::GetPreprocessableFiles($preprocessableFileElems,
                $packageDataDir)

            $packages[$packageIndex] = [Package]::new($packageId, $packageNuspecFile,
                $packageProperties, $packagePreprocessableFiles)
        }

        return $packages
    }

    hidden static [Hashtable]GetPackageCommonProperties([XmlElement]$commonPropertiesElem) {
        if (!$commonPropertiesElem) {
            return @{}
        }

        $defaultPropertyElems = $null
        if ($commonPropertiesElem['defaultProperties']) {
            $defaultPropertyElems = $commonPropertiesElem.defaultProperties.SelectNodes('child::*')
        }

        $commonProperties = @{
            CommonMetadataElements = $commonPropertiesElem.commonMetadataElements.InnerXml
            CommonFileElements = $commonPropertiesElem.commonFileElements.InnerXml
        }
        $commonProperties += [Package]::ConvertXmlElementsToHashtable($defaultPropertyElems)

        return $commonProperties
    }

    hidden static [PreprocessableFile[]]GetPreprocessableFiles(
        [XmlNodeList]$fileElems,
        [string]$baseDir
    ) {
        if (!$fileElems) {
            return [PreprocessableFile[]]::new(0)
        }

        $fileCount = $fileElems.Count
        $files = [PreprocessableFile[]]::new($fileCount)

        for ($fileIndex = 0; $fileIndex -lt $fileCount; $fileIndex++) {
            $fileElem = $fileElems[$fileIndex]
            $srcFile = Join-Path $baseDir $fileElem.src
            $targetFile = Join-Path $baseDir $fileElem.target

            $files[$fileIndex] = [PreprocessableFile]::new($srcFile, $targetFile)
        }

        return $files
    }

    hidden static [Hashtable]MergePackageProperties(
        [Hashtable]$commonProperties,
        [Hashtable]$properties
    ) {
        $mergedProperties = $commonProperties.Clone()

        foreach ($propertyName in $properties.Keys) {
            $propertyValue = $properties[$propertyName]
            if ($mergedProperties.ContainsKey($propertyName) `
                -and [PrimitiveMustacheRenderer]::ContainsTag($propertyValue)
            ) {
                $basePropertyValue = $mergedProperties[$propertyName]
                $propertyValue = [PrimitiveMustacheRenderer]::RenderTemplate($propertyValue,
                    @{ base = $basePropertyValue })
            }
            $mergedProperties[$propertyName] = $propertyValue
        }

        return $mergedProperties
    }

    hidden static [Hashtable]ConvertXmlElementsToHashtable([XmlNodeList]$elems) {
        if (!$elems) {
            return @{}
        }

        $hashtable = @{}

        foreach ($elem in $elems) {
            $hashtable.Add($elem.Name, $elem.'#text')
        }

        return $hashtable
    }

    [void]PreprocessFiles() {
        foreach ($file in $this.PreprocessableFiles) {
            $file.Preprocess($this.Properties)
        }
    }

    [void]RemovePreprocessedFiles() {
        foreach ($file in $this.PreprocessableFiles) {
            if ($file.IsPreprocessed) {
                Remove-Item $file.Target
            }
        }
    }
}

class PreprocessableFile {
    [ValidateNotNullOrEmpty()][string]$Src
    [ValidateNotNullOrEmpty()][string]$Target
    [bool]$IsPreprocessed

    PreprocessableFile([string]$src, [string]$target) {
        $this.Src = $src
        $this.Target = $target
        $this.IsPreprocessed = $false
    }

    [void]Preprocess([Hashtable]$properties) {
        $content = Get-Content $this.Src -Raw -Encoding utf8
        $preprocessedContent = [PrimitiveMustacheRenderer]::RenderTemplate($content, $properties)
        $targetFile = [PrimitiveMustacheRenderer]::RenderTemplate($this.Target, $properties)

        Set-Content $targetFile $preprocessedContent -Encoding utf8 -NoNewline

        $this.Target = $targetFile
        $this.IsPreprocessed = $true
    }
}
