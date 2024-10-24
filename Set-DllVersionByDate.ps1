#
#  Set-DllVersionByDate  FilePathDll
#
#  Purpose: Used as a dll library post build action, it sets the version of the dll file using today's date.


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
	#Write-output "cmd:Start-Process $($PathToStampVer) -ArgumentList" "-o4" "," "-f`"$($curYear).$($curMonth).$($curDay).$($curMins)`"" ","  "$($FilePathDll)"
	Start-Process $PathToStampVer -ArgumentList "-o4", "-f`"$($curYear).$($curMonth).$($curDay).$($curMins)`"", "-p`"$($curYear).$($curMonth).$($curDay).$($curMins)`"", "$($FilePathDll)"
	Write-output "Done."
	

