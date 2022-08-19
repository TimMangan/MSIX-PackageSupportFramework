# Managed File Redirection Fixup (MFRFixup)
The `MFRFixup` is a newer alternative to the `File Redirection Fixup` to handle file accesses when running software in the MSIX Container.

The goals of this alternative are:
> * A clean design that considers the odd needs of applications up-front, eliminating much of the special case patch-work now in the FRF.
> * Support for multiple types of redirection.
> * Inheirent support for Copy-On-Write rather than Copy-On-Access.
> * An empty configuration for the fixup should handle most application needs.
> * The configuration supports specifying changes to the internal configuration rather than needing to specify every little thing.

## Features
When injected into a process, the MFRFixup supports the ability to:
> * Cause certain files accessed in the package to be copied to a safe area where they may be locally manipulated as needed. Two styles of redirection are supported:
> > * Traditional redirection to a folder that is under the user's AppData/Local/Packages folder for the package.
> > * Local redirection to the equivalent native folder location (such as the user's normal Documents folder.)
> * Specify file path/name patterns of folders and files, and redirection style that require special treatment when accessed by the application.
> * Specify file path/name patterns of folders and files that should be excluded from any special treatment.
> * Specify rules for behavior of the appropriate Windows APIs when making file calls that match the pattern.
> * The configuration also allows for exclusion locations and patterns where redirection should not be performed.
> * Once the redirection location is determined the behavior is usually:
> > * In the case of a file, Copy-on-write (COW) is performed. If the file is not present in the redirection area and the call is requesting permisissions to make a modification, then it is copied from the existing location to the redirection location (creating folder structures as needed), and from then on the redirected copy is used.  This is different from the File Redirection Fixup that performs Copy-on-Access (COA).
> > * In the case of a folder the redirected folder is created (including folder structures) as needed and the redirected folder is used.

## Detecting the need for this fixup
A static analysis of the files in the package is often all that is needed:
> * If the package contains files in the VFS/AppData or VFS\LocalAppData folders that are required.
> * If the package contains any files that might be updated.
> * If the application wants to add new files to folders within the package.

At runtime this can sometimes be detected as ACCESS_DENIED results to file operations, however File and Path not found may also indicate the need.

## About Debugging this fixup
The Release build of this fixup produces no output to the debug console port for performance reasons.
Use of the Debug build will enable you to see the intercepts and what the fixup did.
That output is easily seen using the Sysinternals "DebugView" tool.

## About the Layering Models
Applications will often request a file assuming the typical traditional file locations are used and we need to adjust the request so that the app finds the file.  
When an application wants to modify or create a file, package and or some native system locations are read-only, so it becomes appropriate to redirect the request to use a different location via COW.

But even something as simple as the app looking for a file in the user's Documents folder (C:\Users\{UserName}\Documents) may require looking also in the package VFS\Personal folder.

And when running in the container, some apps query an open directory or file handle to determine it's location and create a file path offset from that.  
This can lead to the app looking only in the redirection area for a file that exists only in the package area or a native path. 

So this module implements two different layering techniques (Traditional and Local). 
The fixup configuration allows for specification of which technique should used for which locations, however the fixup defaults to a reasonable set which may be overridden by configuration when necessary.
When looking up a file/directory request, the fixup will first consult the Redirect to Local configured rules list, and if not satisfied then the Traditional Redirect to User's Package Redirection Area configured rules list.

### Redirect to Local Layering Model
The purpose of this layering model is to allow the app to layer package and equivalent local locations for visibility, with modified and new files ending up directed to the local location.
This model is appropriate for situations where it is expected that external applications should also have access to these modified/created files, such as the user's Documents folder.
It is not possible to use this model on any folders for which the MSIX runtime will re-redirect file access to the package.
This model should be sparingly used, as such files will remain on the system after the package is removed.

The following table shows the two possible locations for Documents files and layering order.  They are searched in the following order and the redirection location will always be the top location.

| Location | Description |
| ------------ | ----------- |
| `%UserProfile%\Documents` | The logged in user's Documents folder. |
| `{PackageRootFolder}\VFS\Personal` | The equivalent package folder. |

A file request under either of these locations will kick in the layering.

### Redirect to User's Package Redirection area
The purpose of this layering model is to allow the app to layer the local, package, and redirection areas for visibility, with modified and new files ending up directed to the redirection area.
They are searched in the following order and the redirection location will always be the top location. 

The following table shows the three possible locations and layering order.  They are searched in the following order and the redirection location will always be the top location.

| Location | Description |
| ------------ | ----------- |
| `PackageWritableRoot` | A location under %LocalAppData%\Packages\{PackageFamilyName}\LocalCache\Local\Microsoft\WritablePackageRoot. |
| `PackageRootFolder` |  A location under the package installation folder C:\Program Files\Packages\{PackageFamilyName}. |
| `Native Location` |  Only applicable for requests in the native area and or requests in the Package or Redirection area that include a folder called VFS in the path. |

A file request under either of these locations will kick in the layering.

## Json Configuration
The configuration for the MFRFixup is specified under the element `config` of the fixup structure within the json file when MfrFixup.dll is requested.  An empty `config` element will use the built in configuration for the fixup, and should be appropriate in most cases. You may specify exceptions to this practice in this `config` element if needed.

This `config` element contains an array with possible override elements named below:

| PropertyName | Description |
| ------------ | ----------- |
| `overrideCOW` | Overrides the overall behaviour os Copy-on-Write. |
| `overrideLocalRedirections` | An array. See below. |
| `overrideTraditionalRedirections` | An array. See below. |

### overrideCOW
By default, this fixup will perform copy-on-write operations to file/folder API requests that involve write permissions to locations covered by the active local or traditional redirections.

The value of `overrideCOW` may be one of the following:

| Value | Description |
| ----- | ----------- |
| `disableAll` | Disables all COW support in this fixup. |
| `disablePe` | Disables all COW support in this fixup for files that are well known WinPE filetypes (such as exe, dll, com, tlb, and ocx). |
| `default` | The default behavior. |

### overridelocalRedirections
The `overrideLocalRedirections` element contains array of elements where the name of the element is one of the supported well-known FolderId and a value. 

This name is to be written as standard strings without wildcards (not regex). The supported list of well-known FolderIds for `overrideLocalRedirections` are:

| FolderId | Description |
| ------------ | ----------- |
| `ThisPCDesktopFolder`| This is the known folder ID for the entry for the user's Desktop folder. |
| `Personal`| This is the known folder ID for the entry for the user's Desktop folder. |
| `Common Desktop`| This is the known folder ID for the entry for the Public Desktop folder.  |
| `Common Documents`| This is the known folder ID for the entry for the Public Documents folder.  |

The value is from the following table:

| Value | Description |
| ----- | ----------- |
| `default` | This is the default condition when the entry is not present in the disable list. Layering will be performed between the package and native location and COW to the local native location is performed. |
| `disabled` | Disables the redirection. Calls made by the app to the local native path will not see any package VFS files for this item.  Calls made by the app to the package folder will not see local files and will not receive COW (unless handled by the MSIX runtime itself). |
| `traditional` | Changes the direction of the redirection of this entry to the traditional style. |

### overrideTraditionalRedirections
The `overrideTraditionalRedirections` element contains array of elements where the name of the element is one of the supported well-known FolderId (or FolderId with subfolder) and a value. 

This name is to be written as standard strings without wildcards (not regex).  The supported list of well-known FolderIds for `overrideTraditionalRedirections` are:

| FolderId | Description |
| ------------ | ----------- |
| `FOLDERID_System\Catroot2`| This is the known folder ID for the entry for the Windows\System32\atroot2 folder. |
| `FOLDERID_System\Catroot`| This is the known folder ID for the entry for the Windows\System32\Catroot folder. |
| `FOLDERID_System\drivers\etc`| This is the known folder ID for the entry for the Windows\System32\Drivers\Etc folder. |
| `FOLDERID_System\driverstore`| This is the known folder ID for the entry for the Windows\System32\DriverStore folder. |
| `FOLDERID_System\logfiles`| This is the known folder ID for the entry for the Windows\System32\LogFiles folder. |
| `FOLDERID_System\spool`| This is the known folder ID for the entry for the Windows\System32\Spool folder. |
| `FOLDERID_SystemX86`| This is the known folder ID for the entry for the Windows\SysWOW64 folder on x64 systems or Windows\System32 on x86 systems. |
| `FOLDERID_ProgramFilesCommonX86`| This is the known folder ID for the entry for the 'Program Files (x86)\Common Files' folder on an x64 system, or without the (x86) on a 32bit system. |
| `FOLDERID_ProgramFilesX86`| This is the known folder ID for the entry for the 'Program Files (x86)' folder on an x64 system, or without the (x86) on a 32bit system. |
| `SystemX64`| This is the known folder ID for the entry for the user's Desktop Windows\System32 folder on x64 systems. |
| `ProgramFilesCommonX64`| This is the known folder ID for the entry for the 'Program Files\Common Files' folder on x64 systems. |
| `ProgramFilesX64`| This is the known folder ID for the entry for the 'Program Files' folder on x64 systems. |
| `System`| This is the known folder ID for the entry for the 'Windows\System' folder. |
| `Fonts`| This is the known folder ID for the entry for the 'Windows\Fonts' folder. |
| `Windows`| This is the known folder ID for the entry for the 'Windows' folder. |
| `Common AppData`| This is the known folder ID for the entry for the 'Program Files' folder. |
| `LocalAppDataLow`| This is the known folder ID for the entry for the user 'AppData\LocalAppDataLow' folder. |
| `Local AppData`| This is the known folder ID for the entry for the user 'AppData\Local' folder. |
| `AppData`| This is the known folder ID for the entry for the user 'AppData\Roaming' folder. |
| `Common Programs`| This is the known folder ID for the entry for the 'Program Files' folder. |
| `Profile`| This is the known folder ID for the entry for the 'Program Files' folder. |
| `AppVPackageDrive`| This is the known folder ID for the entry for the 'Program Files' folder. |
| `PVAD`| This is the known folder ID for the entry for the 'Program Files' folder. |

NOTES: 
> * The order shown in the table indicates the order in which folder matching is performed against file/folder API calls, and only the first matchine entry will be used.  
> * While it might seem that this list might be simplified (for excmple, everything under Windows could be a single entry), this is not the case because the packaging tool will seperate out different VFS path names.  
> * Many of these names match against the Microsoft documentation for Known Folder IDs https://docs.microsoft.com/en-us/windows/win32/shell/knownfolderid.  Many of the well-known folders in that list are not specially handled by the packaging tool/runtime, so they are not part of this list.
> * The PVAD entry is made up by this tool to specify files that are in the package path or redirection path but not under a VFS subfolder.

The value is from the following table:

| Value | Description |
| ----- | ----------- |
| `default` | This is the default condition when the entry is not present in the disable list. Layering will be performed between the redirection area, package area and native locations and COW to the redirection area is performed. |
| `disabled` | Disables the redirection. Calls made by the app to an area will only see files in that area and will not receive COW (unless handled by the MSIX runtime itself). |


 the dll based on the process it is running under.

# JSON Examples
To make things simpler to understand, here is a potential example configuration object that is using the built in defaults application to most applications:

```
...
"fixups": [
        {
          "dll": "MFRFixup.dll",
          "config": {}
           }
        }
]
...
```

To apply an override to change the local redirections to traditional or disabled, this might be used:

```
...
"fixups": [
        {
          "dll": "MFRFixup.dll",
          "config": {
            "overideCOW": "default",
             "overrideLocalRedirections": [
                "ThisPCDesktopFolder": "disabled",
                "Personal": "traditional",
                "Common Desktop": "disabled",
                "Common Documents": "disabled"
             ]
           }
        }
]
...
```

To apply an override to prevent binary files to be written to the redirection area, this might be used:

```
...
"fixups": [
        {
          "dll": "MFRFixup.dll",
          "config": {
            "overideCOW": "disablePE"
           }
        }
]
...
```