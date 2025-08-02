# Managed File Redirection Fixup (MFRFixup)
The `MFRFixup` is a newer alternative to the `File Redirection Fixup` to handle file accesses when running software in the MSIX Container.  If file redirection help is needed in a package, you must choose between these two fixups as they intercept the same APIs (in other words, you can't include both the `MFRFixup` and `File Redirection Fixup`). 

The goals of this alternative are:
> * A clean design that considers the odd needs of applications up-front, eliminating much of the special case patch-work now in the FRF.
> * Support for multiple types of redirection.
> * Inheirent support for Copy-On-Write rather than Copy-On-Access.
> * An empty configuration for the fixup should handle most application needs.
> * The configuration supports specifying changes to the internal configuration rather than needing to specify every little thing.
> * To leverage the InstalledLocationVirtualization feature of the MSIX runtime to avoid unnecessary redirection.


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
> * Detects .Net based "Winforms" apps that might crash when saving settings and forces them to use COW on the config file, even when they don't ask.

## Detecting the need for this fixup
A static analysis of the files in the package is often all that is needed:
> * If the package contains files in the VFS/AppData or VFS\LocalAppData folders that are required.
> * If the package contains any files that might be updated.
> * If the application wants to add new files to folders within the package.

At runtime this can sometimes be detected as ACCESS_DENIED results to file operations, however File and Path not found may also indicate the need.

### Dependencies for MfrFixup
When using this fixup, you must also supply the following dependencies.  The fixup uses version 14032 of the runtimes.

It is recommended that these be placed in the VFS\SystemX64 and VFS\SystemX86 folders of the package, such that if the
application uses a different version of these dlls, the app specific version will be found.

It may be possible to exclude these files from the package, and use a package dependency for the Microsoft package of VCRuntimes 2015-2022, but only if the release build of the PSF is used.  Keep in mind that the target application may also be adding VCRuntimes into the package.

| Release Build | Debug Build | Notes
| --- | --- | --- |
| msvcp140.dll | msvcp140d.dll | x86 and x64 |
| vcruntime140.dll | vcruntime140d.dll | x86 and x64 |
| vcruntime140_1.dll | vcruntime140_1d.dll | x64 only |
| | ucrtbased.dll | x86 and x64 |

A copy of these may be found in the OBSOLETE folder of the PSF release.
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
| `ilv-aware` | Configures MFR to be avoid changes incompatible with InstallLocationVirtualization. |
| `overrideCOW` | Overrides the overall behaviour os Copy-on-Write. |
| `overrideLocalRedirections` | An array. See below. |
| `overrideTraditionalRedirections` | An array. See below. |

### ilvAware
By default this value is set to `false`. Set to `true` to enable. The MFR and ILV solve some of the same issues for applications, but there are things that each do that are unique.  
Generally, you should set this value to true, but this setting exists so that you can try the MFR without ILV present.  
When set to `true`, the MFR will avoid known conflicts and try to avoid duplication of effort.


### overrideCOW
By default, this fixup will perform copy-on-write operations to file/folder API requests that involve write permissions to locations covered by the active local or traditional redirections.

The value of `overrideCOW` may be one of the following:

| Value | Description |
| ----- | ----------- |
| `disableAll` | Disables all COW support in this fixup. |
| `enablePe` | Enables COW for all file types. |
| `default` | The default behavior of COW on all files except for files that are well known WinPE filetypes (exe, dll, com, tlb, and ocx). |

When `ilvAware` is enabled, the `default` setting will cause MFR to block writing to the WinPE filetypes in the package and returning AccessDenied to the application.  
Similarly, if ilv-aware is enabled along with `disableAll` for the `overrideCOW`, then the MFR will return AccessDenied to the application whenever the file is part of the package.

Without `ilvAware` setting, the MFR will use Copy-on-write and succeed for files specified in this setting.

### overrideLocalRedirections
The MFR is preconfigured with a set of folders (such as the User's documents folder) for which redirection to the redirection area is prefered.
The `overrideLocalRedirections` element allows you to specify override this behavior on a folder by folder basis.

When specified, the `overrideLocalRedirections` element contains array of elements where each element contains a `name` and `mode`. 

The value of the `name` is to match a well-known FolderId shown in the table below. This name is to be written as standard strings without wildcards (not regex). The supported list of well-known FolderIds for `overrideLocalRedirections` are:

| FolderId | Description |
| ------------ | ----------- |
| `ThisPCDesktopFolder`| This is the known folder ID for the entry for the user's Desktop folder. |
| `Personal`| This is the known folder ID for the entry for the user's Desktop folder. |
| `Common Desktop`| This is the known folder ID for the entry for the Public Desktop folder.  |
| `Common Documents`| This is the known folder ID for the entry for the Public Documents folder.  |

The `mode` is from the following table:

| Value | Description |
| ----- | ----------- |
| `default` | This is the default condition when the entry is not present in the disable list. Layering will be performed between the package and native location and COW to the local native location is performed. |
| `disabled` | Disables the redirection. Calls made by the app to the local native path will not see any package VFS files for this item.  Calls made by the app to the package folder will not see local files and will not receive COW (unless handled by the MSIX runtime itself). |
| `traditional` | Changes the direction of the redirection of this entry to the traditional style. |

### overrideTraditionalRedirections
The MFR is preconfigured with a set of folders (such as the program folder) for which redirection to the native path is prefered.
The `overrideTraditionalRedirections` element allows you to specify override this behavior on a folder by folder basis.

The `overrideTraditionalRedirections` element contains array of elements where each element contains a `name` and `mode`. 

The value of the `name` is to match a well-known FolderId shown in the table below. This name is to be written as standard strings without wildcards (not regex). The supported list of well-known FolderIds for  `overrideTraditionalRedirections` are:

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
| `ProgramFilesX64`| This is the known folder ID for the entry for the  'Program Files' folder on x64 systems. |
| `FOLDERID_UserProgramFiles`| This is the known folder ID for the entry for the user's appdata 'Programs' folder. |
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

The `mode` is from the following table:

| Value | Description |
| ----- | ----------- |
| `default` | This is the default condition when the entry is not present in the disable list. Layering will be performed between the redirection area, package area and native locations and COW to the redirection area is performed. |
| `disabled` | Disables the redirection. Calls made by the app to an area will only see files in that area and will not receive COW (unless handled by the MSIX runtime itself). |
| `local` |  Changes the direction of the redirection of this entry to the local style. |




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
            "overrideCOW": "default",
             "overrideLocalRedirections": [
                {
                    "name" : "ThisPCDesktopFolder",
                    "mode" : "disabled"
                },
                {
                    "name" : "Personal",
                    "mode" :  "traditional"
                },
                {
                    "name" : "Common Desktop",
                    "mode" :  "disabled"
                },
                {
                    "name" : "Common Documents",
                    "mode" :  "disabled"
                }
             ],
             "overrideTraditionalRedirectsions" : []
           }
        }
]
...
```

To apply a configuration to assume InstalledLocationVirtualization and override to enable binary files to be written to the redirection area, this might be used:

```
...
"fixups": [
        {
          "dll": "MFRFixup.dll",
          "config": {
            "ilvAware": "true",
            "overrideCOW": "enablePe"
           }
        }
]
...
```

## Intercepts Supported
The following APIs from Kernel32 and lower level are targeted.  Those from Kernel32 often have ansi and wide varients.  The current status for this work-in-progress is shown in the table below.  Most of these APIs have A/W variants for ansi/wide(Unicode) character variants in arguments.  These APIs represent the active list in the FileRedirectionFixup; those marked as `may not be needed` provide no functional changes in the intercepts in the FileRedirectionFixup but merely log that they were called.  This fixup might follow suit.

| Dll | API | Status |
| --- | --- | ------ |
| Kernel32 | CopyFile | Complete |
| Kernel32 | CopyFileEx | Complete |
| Kernel32 | CopyFile2  | Complete |
| Kernel32 | CreateDirectory | Complete |
| Kernel32 | CreateDirectoryEx | Complete |
| Kernel32 | CreateFile | Complete |
| Kernel32 | CreateFile2 | Complete |
| Kernel32 | CreateHardLink | Complete |
| Kernel32 | CreateSymbolicLink | Complete |
| Kernel32 | DeleteFile | Complete |
| Kernel32 | FindClose | Complete |
| Kernel32 | FindFirstFile | Complete |
| Kernel32 | FindFirstFileEx | Complete |
| Kernel32 | FindNextFile | Complete |
| Kernel32 | GetFileAttributes | Complete |
| Kernel32 | GetFileAttributesEx | Complete |
| Kernel32 | GetPrivateProfileInt | Complete |
| Kernel32 | GetPrivateProfileSection | Complete |
| Kernel32 | GetPrivateProfileSectionNames | Complete |
| Kernel32 | GetPrivateProfileString | Complete |
| Kernel32 | GetPrivateProfileStruct | Complete |
| Kernel32 | MoveFile | Complete |
| Kernel32Ex | MoveFileEx | Complete |
| KernelBase | MoveFileExW | Complete |
| Kernel32 | MoveFileWithProgress | Complete |
| NtDll | NtQueryDirectoryFile | Complete |
| Kernel32 | RemoveDirectory | Complete |
| Kernel32 | ReplaceFile | Complete |
| Kernel32 | SetCurrentDirectory | Complete |
| Kernel32 | SetFileAttributes | Complete |
| Kernel32 | ShellExecute | Complete |
| Kernel32 | ShellExecuteEx | Complete |
| Kernel32 | WritePrivateProfileSection | Complete |
| Kernel32 | WritePrivateProfileString | Complete |
| Kernel32 | WritePrivateProfileStruct | Complete |


The following APIs are intercepted but only for logging purposes at this time.  
There are known instances of Microsoft dlls that skip past Kernel32/KernelBase and go directly to NtDll. They may be implemented in the future as we find the need.
| Dll | API | Status |
| --- | --- | ------ |
| Windows.Storage | ShellExecute | Intercept for logging only at this time |
| Windows.Storage | ShellExecuteEx | Intercept for logging only at this time |
| Ntdll | NtQueryDirectoryFileEx | Intercept for logging only at this time |
| Kernel32 | DeviceIoControl | Intercept for logging only at this time |

The following APIs have been investigated but are intentionally not intercepted:
| Dll | API | Status |
| --- | --- | ------ |
| Kernel32 | GetCurrentDirectory | not needed |
| Kernel32 | ReadDirectoryChangesW | not supported |
| Kernel32 | ReadDirectoryChangesExW | not supported |
| Kernel32 | SearchPath | not  needed |
| NtDll | ZwCreateFile | not supported at this time |
| NtDll | ZwOpenFile | not supported at this time |

Additionally, there are numerous "Transacted" API calls that are generally not used and are ignored.
Also currently ignored is FindFirstFileName/Next as it deals only with hard links and probably has little usage.
Ditto FindFirstStream/Next.
Ditto PrivCopyFileEx, which is undocumented but used by robocopy apparently.
OpenFile may need to be considerd (possibly implementation ends up in CreateFile so we don't need to).
Calls that use open file handles, such as ReadFile, WriteFile, and CloseHandle, are also ignored as there is no need to intercept those.
