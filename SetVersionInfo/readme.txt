--------------------------------------------------------------------------
               StampVer - Win32 Version Resource Stamping
             Written by Paul Dixon (paul@elphin.com) Jan 1998
			  http://www.elphin.com/products/stampver.html
--------------------------------------------------------------------------

1. Introduction
2. History
3. Syntax and Options
4. About StampVer response files
5. Integrating with Microsoft Developer Studio
6. Notes and Queries
7. Licence Agreement
8. Tech support



1. Introduction
===============
StampVer was written to allow easy stamping of a build number into the
version resource of a Win32 executable. This build number can be 
specified on the command line, or via a response file. If a response
file is used, StampVer can autoincrement the version number contained
it it.

This can be useful if you are frequently passing application builds 
around and want something more definitive than file dates to determine 
which version someone has.

2. History
==========
v1.40    Sep99    PE checksum is now updated (uses the ImageHlp DLL). 
                  Can update a response file without updating an EXE


v1.30    Jun98    Improved error reporting for .inf file parsing.
                  Added readme information about better DevStudio 
				  integration.
				  Added strftime style date support to format specs

v1.20    Feb98    Added support for zero padding, e.g. 1.05.0047
                  Allowed %a,%b,... to be used instead of %1,%2,... in
				  format specifications to clarify them when using
				  the zero padding feature!
				  Version strings are now padded to the maximum length
				  with spaces. Use -nopad to disable this.

v1.10    Jan98    -s param became -f (stamp explicit file version)
                  -p param altered (now stamps explicit product version)
				  -j param added (increment product version)
				  ProductVersion field added to response file
				  FileFormat field added to response file
				  ProductFormat field added to response file
				  -k param added (keep timestamp)
				  Default information level is -o2 rather than -o3

v1.00    Nov97    First release.


3. Syntax and options
=====================

StampVer [options] <exefile/s>

  -? displays this message
  -f"1.2.3.4" stamp with specified file version number, e.g. -f"2.10.0.0"
  -p"1.2.3.4" stamp with specified product version number, e.g. -p"2.10.0.0"
  -v"<filename>" get version number from response file, e.g. -v"stampver.inf"
  -i[1|2|3|4] preincrement file version number in response file
               1,2,3,4 correspond to a element of the version number, so
               to increment the minor version, use -i4. Useful for
               auto-incrementing a build number.
  -j[1|2|3|4] preincrement product version number in response file (as above).
  -k keep original file timestamp
  -nopad prevent padding of version strings with spaces.
  -o[1|2|3|4] sets the amount of output produced:
               -o1 errors only
               -o2 show warnings
               -o3 show informational messages
               -o4 show verbose diagnostics
Quotes around parameters are optional unless the parameter contains spaces.



4. About StampVer response files
================================
The response file is a simple text file that contains the last version
number that was applied. Here is an example:

    ;StampVer information file
    FileVersion=1.0.0.4
    ProductVersion=5.0.0.4
    FileFormat=%a.%b.%c Build %04d
    ProductFormat=%a.%b.%c Build %04d

The response file allows you to have separately maintained file and
product numbers. If you do not wish to stamp either the file or product
number, then simply ensure the response file does not contain that
entry!

The FileFormat and ProductFormat entries are optional, but if present
they allow you to customise the format of the version strings. These 
entries are vaguely similar to printf format specs. The default is 
simply %a.%b.%c.%d. You can add zero padding to a field by specifying
a zero followed by the required field width, e.g. %04d would expand
to something like 0047.

[**new in version 1.3**]

You can also embed date and time information into the version strings
using strftime style format specs preceded by a %t. For example

    FileFormat=%a.%b Build %04d (%td %tb)

would produce something like this:

    1.23 Build 456 (23 Jan)

Here are some common strftime codes you could use:

    %ta   Abbreviated weekday name
    %td   Day of month (01-31)
    %tj   Day of year (001-366)
    %tm   Month (01-12)
    %tb   Abbreviated month name
    %ty   Year without century (00-99)
    %tY   Year with century 
    %tH   Hour in 24-hour format (00-23)
    %tM   Minute (00-59)
    %tS   Second (00-59)

Check your C run time strftime documentation for others. Remember
to insert the t though!


If you specify a response file that doesn't exist, or do not specify
one at all, one will be created for you in the current directory with
the name stampver.inf

Note that if you use the -f or -p options these will override any response 
file settings.


5. Integrating with Microsoft Developer Studio
==============================================
You can set up StampVer to autoincrement build numbers as part of your
normal build process. Assuming that your EXE has a version resource,
and StampVer is on the path, you simply set up a custom build step for
the target.

To do this, get the project settings dialog (Alt-F7), highlight the top
level folder for a build, and select the Custom Build tab. Add a build
command that looks like this: 

    StampVer -vstampver.inf -i4 -j4 $(TargetPath)

Set the output file to be the following:

    $(WkspDir)\stampver.inf

These settings will store the version number in stampver.inf file in
the workspace directory, and will autoincrement the version numbers
contained in it before applying it to the FileVersion and ProductVersion
elements of the target EXE. Setting the output file to stampver.inf
means that the version number is stamped only when the EXE actually
changes. 


NOTE: Here an alternative technique, that will work no matter what 
StampVer options are used (the above only works with the -i or -j
options, as they cause stampver.inf to be updated)

    StampVer -vstampver.inf $(TargetPath)
    echo stampver executed > "$(OutDir)\stampver.trg"

And set the output file to the following:

    $(OutDir)\stampver.trg

This is a general purpose technique that can be used for any tool
that doesn't produce an output file.



6. Notes and Queries
====================
Please note that StampVer only operates on exe/dll files that already have
a version resource. It cannot add a version resource if one doesn't exist.

Also be aware that StampVer can only update the FileVersion and ProductVersion
strings if there is enough space to do so. An easy way to ensure this is to
set your version resource to contain 000.000.000.000 for these strings, and
let StampVer overwrite these with the real version numbers.

StampVer will pad version strings to their maximum length with spaces. This
feature can be disabled with the -nopad option. This was added when it was
found that Dev Studio crashes when it tries to display the version resource
for an EXE that StampVer has processed.

Since StampVer operates on the final exe, it will confuse your incremental 
linker into never linking incrementally! Not much I can do about that though.

Note that from v1.4, StampVer will update the EXE/DLL checksum. The NT
loader is picky about this for core system DLLs, and no doubt Win2000 will
be too. StampVer uses the ImageHlp dll to perform this task.


7. Licence Agreement
====================
StampVer is freeware, and comes with no warranty - you use it at your own
risk. All distributions of StampVer must include all files contained in the 
archive it was originally packaged in. Any questions regarding distribution 
should be directed to Paul Dixon at paul@elphin.com


8. Tech Support
===============
If you do experience problems, or have ideas for enhancements, please
contact the author, Paul Dixon at paul@elphin.com. 

Latest releases can be found via the StampVer home page at
http://www.elphin.com/products/stampver.html





