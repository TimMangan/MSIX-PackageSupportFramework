# Copyright TMurgent Technologies, LLP 2026
# This script is intended to be used as a post-build event in a project. 
# It performs the following acts:
#    * It sets the version number of the output file.
#    * It generates a Software Bill of Materials (SBOM) for the project output directory.

param (
	[string]$PathToStampVer,   # Path to the version stamping tool (e.g., verpatch.exe)
	[string]$FilePathOutputComponent,      # Path to the output DLL file whose version is to be set

	[string]$ProjectOutputDirectory, # Directory where the project build output is located (e.g., bin\x64Release)
	[string]$OutputSbomFilePath = "SBOM.xml",  # output file path (not name)
    [string]$ProjectFile = "main.csproj" # Specify the full path for the main project file so we can locate Nuget references.

)

# Function to generate CycloneDX SBOM
function Generate-CycloneDX-SBOM {
    param (
        [string]$Directory,
        [string]$FilePathOutputComponent,  # file path to the single file exe/dll output for this project
        [string]$ProjectFile,
        [string]$OutfileFilePath   # file path to the output SBOM file
    )

    # Parameter validation
    if (-not (Test-Path -Path $Directory -PathType Container)) {
        Write-Error "Directory '$Directory' does not exist."
        exit 4
    }
    if (-not (Test-Path -Path $FilePathOutputComponent -PathType Leaf)) {
        Write-Error "Output component file '$FilePathOutputComponent' does not exist."
        exit 5
    }
    if (-not (Test-Path -Path $ProjectFile -PathType Leaf)) {
        Write-Error "Project file '$ProjectFile' does not exist."
        exit 6
    }


    # Create the XML document
    [System.Xml.XmlDocument] $xmlDocument = New-Object System.Xml.XmlDocument

    # Create the root BOM element
    $root = $xmlDocument.CreateElement("bom")
    $root.SetAttribute("xmlns", "http://cyclonedx.org/schema/bom/1.7")
    $root.SetAttribute("xmlns.xsi", "http://www.w3.org/2001/XMLSchema-instance")
    $root.SetAttribute("xmlns.xsd", "http://www.w3.org/2001/XMLSchema")
    $root.SetAttribute("version", "urn:uuid:$($guid)")
    $guid = [System.Guid]::NewGuid().ToString()
    $root.SetAttribute("serialNumber", "1")
    $xmlDocument.AppendChild($root)

    # Add metadata
    $metadata = $xmlDocument.CreateElement("metadata")
    $timestamp = $xmlDocument.CreateElement("timestamp")
    $timestamp.InnerText = (Get-Date -Format "yyyy-MM-ddTHH:mm:ssZ")
    $metadata.AppendChild($timestamp)

    $tools = $xmlDocument.CreateElement("tools")
    $tool = $xmlDocument.CreateElement("tool")
    $vendor = $xmlDocument.CreateElement("vendor")
    $vendor.InnerText = "TMurgent Technologies"
    $tool.AppendChild($vendor)
    $name = $xmlDocument.CreateElement("name")
    $name.InnerText = "CreateProjectSBOM"
    $tool.AppendChild($name)
    $version = $xmlDocument.CreateElement("version")
    $version.InnerText = "1.0"
    $tool.AppendChild($version)
    $tools.AppendChild($tool)
    $metadata.AppendChild($tools)

    $metaComponent = $xmlDocument.CreateElement("component")
    $metaComponent.SetAttribute("type", "application")
    $metaComponentName = $xmlDocument.CreateElement("name")
    $metaComponentName.InnerText = (Split-Path -Leaf $Directory)
    $metaComponent.AppendChild($metaComponentName)
    $metadata.AppendChild($metaComponent)

    $root.AppendChild($metadata)

    # Add components for each file in the directory
    $components = $xmlDocument.CreateElement("components")


    # Add this component
    [System.IO.FileInfo]$filePathObject = Get-Item -Path $FilePathOutputComponent
    
    if ($filePathObject.Exists)
    {
        $fileComponent = $xmlDocument.CreateElement("component")
        if ($filePathObject.Extension -eq ".exe")
        {
            $fileComponent.SetAttribute("type", "application")
        }
        else 
        {
            if ($filePathObject.Extension -eq ".dll" -or
                 $filePathObject.Extension -eq ".lib")
            {
                $fileComponent.SetAttribute("type", "library")
            }
            else
            {
                $fileComponent.SetAttribute("type", "file")
            }
        }

        $name = $xmlDocument.CreateElement("name")
        $name.InnerText = $filePathObject.Name
        $fileComponent.AppendChild($name)

        $version = $xmlDocument.CreateElement("version")
        $version.InnerText =  $filePathObject.VersionInfo.FileVersion
        $fileComponent.AppendChild($version)

        $hash = Get-FileHash -Path $FilePathOutputComponent -Algorithm SHA256
        $hashElement = $xmlDocument.CreateElement("hashes")
        $hashValue = $xmlDocument.CreateElement("hash")
        $hashValue.SetAttribute("alg", "SHA-256")
        $hashValue.InnerText = $hash.Hash
        $hashElement.AppendChild($hashValue)
        $fileComponent.AppendChild($hashElement)

        $properties = $xmlDocument.CreateElement("properties")

        $sizeProperty = $xmlDocument.CreateElement("property")
        $sizeName = $xmlDocument.CreateElement("name")
        $sizeName.InnerText = "size"
        $sizeProperty.AppendChild($sizeName)
        $sizeValue = $xmlDocument.CreateElement("value")
        $sizeValue.InnerText = $filePathObject.Length
        $sizeProperty.AppendChild($sizeValue)
        $properties.AppendChild($sizeProperty)

        $lastModifiedProperty = $xmlDocument.CreateElement("property")
        $lastModifiedName = $xmlDocument.CreateElement("name")
        $lastModifiedName.InnerText = "lastModified"
        $lastModifiedProperty.AppendChild($lastModifiedName)
        $lastModifiedValue = $xmlDocument.CreateElement("value")
        $lastModifiedValue.InnerText = $filePathObject.LastWriteTime.ToString("yyyy-MM-ddTHH:mm:ssZ")
        $lastModifiedProperty.AppendChild($lastModifiedValue)
        $properties.AppendChild($lastModifiedProperty)

        $fileComponent.AppendChild($properties)

        $components.AppendChild($fileComponent)
    }

    # Add NuGet references from the project file
    if (Test-Path "$ProjectFile") {
        [xml]$projectXml = Get-Content "$ProjectFile"
        $nugetReferences = $projectXml.Project.ItemGroup.PackageReference
        foreach ($reference in $nugetReferences) {
            if ($reference.Include -ne $null -and $reference.Include -ne "")
            {
            $nugetComponent = $xmlDocument.CreateElement("component")
            $nugetComponent.SetAttribute("type", "library")

            $name = $xmlDocument.CreateElement("name")
            $name.InnerText = $reference.Include
            $nugetComponent.AppendChild($name)

            $version = $xmlDocument.CreateElement("version")
            $version.InnerText = $reference.Version
            $nugetComponent.AppendChild($version)

            $purl = $xmlDocument.CreateElement("purl")
            $purl.InnerText = "pkg:nuget/$($reference.Include)@$($reference.Version)"
            $nugetComponent.AppendChild($purl)

            $components.AppendChild($nugetComponent)
            }
        }
    }

    $root.AppendChild($components)

    # Save the XML to the output file
    Write-output "Saving SBOM to $($OutfileFilePath)"
    $xmlDocument.Save($OutfileFilePath)

    return 0
}

#Validate parameters not validated in the function
if (-not (Test-Path -Path $PathToStampVer -PathType Leaf)) {
    Write-Error "Version stamping tool '$PathToStampVer' does not exist."
    exit 2
}
if (-not (Test-Path -Path $FilePathOutputComponent -PathType Leaf)) {
    Write-Error "Output component file '$FilePathOutputComponent' does not exist."
    exit 3
}

Write-output " ===== Start post build event for $($ProjectOutputDirectory) ===== "

$fullDate = get-date
$curYear = $fullDate.Year
$curMonth = $fullDate.Month
$curDay = $fullDate.Day
$curMins = (($fullDate.Hour) * 60) + $fullDate.Minute
$version = "$($curYear).$($curMonth).$($curDay).$($CurMins)"

Write-output " ----- Set version $($version) on file $($FilePathOutputComponent)  ----- " 

# Call the function to set the version of the output file
$pinfo = New-Object System.Diagnostics.ProcessStartInfo
$pinfo.FileName = "$($PathToStampVer)"
$pinfo.RedirectStandardError = $true
$pinfo.RedirectStandardOutput = $true
$pinfo.UseShellExecute = $false
$pinfo.Arguments = "$($FilePathOutputComponent) $($version) /va /pv $($version) /s CompanyName `"TMurgent Technologies, LLP`" /s LegalCopyright `"(c) $($curYear) TMurgent Technologies, LLP`""
$p = New-Object System.Diagnostics.Process
$p.StartInfo = $pinfo
$p.Start() | Out-Null
$p.WaitForExit()
$stdout = $p.StandardOutput.ReadToEnd()
$stderr = $p.StandardError.ReadToEnd()
#Write-output "stdout: $stdout"
#Write-output "stderr: $stderr"
#Write-output "exit code: " $p.ExitCode
if ($p.ExitCode -ne 0) {
    Write-Output "Failed to set version on file $($FilePathDll). Error: $stderr"
    Write-Error "Failed to set version on file $($FilePathDll). Error: $stderr"
    exit $p.ExitCode
}
Write-output " -----  Done Set version.  ----- " 



Write-Host " ----- Start generation of SBOM for project at $ProjectOutputDirectory ----- " 
# Generate SBOM and convert to CycloneDX XML
Generate-CycloneDX-SBOM -Directory $ProjectOutputDirectory -FilePathOutputComponent $FilePathOutputComponent -ProjectFile $ProjectFile -OutfileFilePath $OutputSbomFilePath

if (Test-Path -Path $OutputSbomFilePath -PathType Leaf) {
    Write-Host " ----- CycloneDX SBOM (v1.7) has been generated and saved to $OutputSbomFilePath ----- " 

} else {
    Write-Error "Failed to generate SBOM at $OutputSbomFilePath"
    exit 7
}

Write-output " ===== End post build event for $($ProjectOutputDirectory) ===== "
exit 0