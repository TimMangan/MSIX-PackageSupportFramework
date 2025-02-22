#
#  Set-DllVersionByDate  FilePathDll
#
#  Purpose: Used as a dll library post build action, it sets the version of the dll file using today's date.
#  or just manually set in rc files before build


Param( 
	
	[Parameter(Mandatory=$true)]
	[string]$PathToStampVer,
	[Parameter(Mandatory=$true)]
	[string]$FilePathDll
)

	$fullDate = get-date
	$curYear = $fullDate.Year
	$curMonth = $fullDate.Month
	$curDay = $fullDate.Day
	$curMins = (($fullDate.Hour) * 60) + $fullDate.Minute

	Write-output "Set version $($curYear).$($curMonth).$($curDay).$($CurMins) on file $($FilePathDll)"

	$pinfo = New-Object System.Diagnostics.ProcessStartInfo
    $pinfo.FileName = "$($PathToStampVer)"
    $pinfo.RedirectStandardError = $true
    $pinfo.RedirectStandardOutput = $true
    $pinfo.UseShellExecute = $false
    $pinfo.Arguments = "$($FilePathDll) $($curYear).$($curMonth).$($curDay).$($curMins) /pv $($curYear).$($curMonth).$($curDay).$($fullDate.Hour) /s CompanyName `"TMurgent Technologies, LLP`" /s LegalCopyright `"(c) $($curYear)`"" 
    $p = New-Object System.Diagnostics.Process
    $p.StartInfo = $pinfo
    $p.Start() | Out-Null
    $p.WaitForExit()
    $stdout = $p.StandardOutput.ReadToEnd()
    $stderr = $p.StandardError.ReadToEnd()
    Write-output "stdout: $stdout"
    Write-output "stderr: $stderr"
    Write-output "exit code: " $p.ExitCode

	Write-output "Done Set version."
	

