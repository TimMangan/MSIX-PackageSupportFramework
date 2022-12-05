## Update version fields of other projects using date

$curYear = (get-date).Year
$curMonth = (get-date).Month
$curDay = (get-date).Date


$executingScriptDirectory = Split-Path -Path $MyInvocation.MyCommand.Definition -Parent
$projectList1 = Get-ChildItem -Path "$($executingScriptDirectory)\PsfLauncher" -Filter "*.vcxproj" -Recurse -ErrorAction SilentlyContinue -Force
$projectList2 = Get-ChildItem -Path "$($executingScriptDirectory)\PsfRuntime" -Filter "*.vcxproj" -Recurse -ErrorAction SilentlyContinue -Force
$projectList3 = Get-ChildItem -Path "$($executingScriptDirectory)\PsfRunDll" -Filter "*.vcxproj" -Recurse -ErrorAction SilentlyContinue -Force
$projectList4 = Get-ChildItem -Path "$($executingScriptDirectory)\PsfShimMonitor" -Filter "*.vcxproj" -Recurse -ErrorAction SilentlyContinue -Force
$projectList5 = Get-ChildItem -Path "$($executingScriptDirectory)\Fixups" -Filter "*.vcxproj" -Recurse -ErrorAction SilentlyContinue -Force

foreach ($projectFile in $projectList1) {
    write-Output "Processing " $projectFile.Name 
    [xml]$Data=Get-Content $projectFile.FullName
    #$RootNode_Project = $Data.ChildNodes.NextSibling
    #write-output $RootNode_Project
    write-output $Data.ChildNodes
    write-output "========================================================================="
    foreach ($RootNode in $Data.ChildNodes)
    {
        Write-output $RootNode
        Write-output "---------------------------------------------------------------------"
        foreach ($IDG in $RootNode.ItemDefinitionGroup)
        {
             $IDG | get-member
            Write-output "............................................................"
        }
    }
}