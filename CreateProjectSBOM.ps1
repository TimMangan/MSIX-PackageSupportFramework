# Copyright (c) TMurgent Technologies, LLP 2026
# This script creates a CycloneDX v1.7-compatible Software Bill of Materials (SBOM) for a specified project directory.
# It scans the project build output directory for all files and generates a detailed list of all components.
# It also includes NuGet references from the main project file.

param (
    [string]$ProjectOutputDirectory,
    [string]$OutputFile = "SBOM.xml",
    [string]$ProjectFile = "main.csproj" # Specify the main project file
)

# Function to generate CycloneDX SBOM
function Generate-CycloneDX-SBOM {
    param (
        [string]$Directory,
        [string]$ProjectFile
    )

    # Create the root BOM structure
    $sbom = [PSCustomObject]@{
        bom = @{
            "@xmlns" = "http://cyclonedx.org/schema/bom/1.4"
            "@version" = "1"
            metadata = @(
                @{
                    timestamp = (Get-Date -Format "yyyy-MM-ddTHH:mm:ssZ")
                    tools = @(
                        @{
                            vendor = "TMurgent Technologies"
                            name = "CreateProjectSBOM"
                            version = "1.0"
                        }
                    )
                    component = @{
                        type = "application"
                        name = (Split-Path -Leaf $Directory)
                    }
                }
            )
            components = @()
        }
    }

    # Add components for each file in the directory
    Get-ChildItem -Path $Directory -Recurse -File | ForEach-Object {
        if ($_.Extension -ne ".pdb" -and $_.Extension -ne ".xml") 
        { 
            $component = @{
                type = "file"
                name = $_.Name
                version = "1.0"
                purl = "pkg:generic/$(Split-Path -Leaf $Directory)/$($_.Name)"
                properties = @(
                    @{
                        name = "path"
                        value = $_.FullName
                    },
                    @{
                        name = "size"
                        value = $_.Length
                    },
                    @{
                        name = "lastModified"
                        value = $_.LastWriteTime.ToString("yyyy-MM-ddTHH:mm:ssZ")
                    }
                )
            }
        }
        $sbom.bom.components += $component
    }

    # Add NuGet references from the project file
    if (Test-Path "$Directory\$ProjectFile") {
        [xml]$projectXml = Get-Content "$Directory\$ProjectFile"
        $nugetReferences = $projectXml.Project.ItemGroup.PackageReference
        foreach ($reference in $nugetReferences) {
            $component = @{
                type = "library"
                name = $reference.Include
                version = $reference.Version
                purl = "pkg:nuget/$($reference.Include)@$($reference.Version)"
            }
            $sbom.bom.components += $component
        }
    }

    return $sbom
}

Write-Host " ===== Start generation of SBOM for project at $ProjectOutputDirectory ===== " -ForegroundColor Cyan
# Generate SBOM and convert to CycloneDX XML
$sbom = Generate-CycloneDX-SBOM -Directory $ProjectOutputDirectory -ProjectFile $ProjectFile
$xml = $sbom.bom | ConvertTo-Xml -NoTypeInformation -Depth 3

# Save the XML to the output file
$xml.Save("$ProjectOutputDirectory\$OutputFile")

Write-Host "CycloneDX SBOM (v1.7) has been generated and saved to $ProjectOutputDirectory\$OutputFile" -ForegroundColor Green
