#
#  Set-DllVersionByDate  FilePathDll
#
#  Purpose: Used as a dll library post build action, it sets the version of the dll file using today's date.


Param( 
	[Parameter(Mandatory=$true)]
	[string]$FilePathDll 
)

	$fullDate = get-date
	$curYear = $fullDate.Year
	$curMonth = $fullDate.Month
	$curDay = $fullDate.Date
	$curMins = $fullDate.Hour * 60 + $fullDate.Minute

	Write-output "Set version ($curYear).$($curMonth).$($curDay).$($curMins) on file $($FilePathDll)"
	ILMerge.exe "$($FilePathDll)" "/ver:$($curYear).$($curMonth).$($curDay).$($curMins)" "/out:$($FilePathDll)"
	Write-output "Done."
	write-host hi

