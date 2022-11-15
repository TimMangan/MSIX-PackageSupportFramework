# PSF Launcher

The PSF Launcher acts as a wrapper for the application, it injects the `psfRuntime` to the application process and then activates it. 
To configure it, change the entry point of the app in the `AppxManifest.xml` and add a new `config.json` file to the package as shown below;<br/>
Based on the manifest's application ID, the `psfLauncher` looks for the app's launch configuration in the `config.json`.  The `config.json` can specify multiple application configurations via the applications array.  Each element maps the unique app ID to an executable which the `psfLauncher` will activate, using the Detours library to inject the `psfRuntime` library into the app process.  The app element must include the app executable.  It may optionally include a custom working directory, which otherwise defaults to Windows\System32.

PSF Launcher also supports running an additional "monitor" app, intended for PsfMonitor. You use PsfMonitor to view the output in conjuction with TraceFixup injection configured to output to events.

Finally, PsfLauncher also support some limited PowerShell scripting.

Additional details about what processes should/should not run inside the container is located in the Readme for PsfRuntime, which implements that detail.

## About PsfLauncher and Debug Logging
PsfLauncher and PsfRuntime are designed to always output logging information to the debug console port. 
This infomration may be viewed with a tool like the SysInternals DebugView as you start the application.
  
This information will provide the details about all of the actions that the launcher will do, including any monitoring, scripting, and how the target process will be started.

For performance reasons, the "fixup" modules generally do not log to the debug console port in release builds.
If you need debugging of the fixup modules, you may include the Debug builds of those modules which output verbosely to the debug console port.
This debug output is more useful in debugging the fixup itself rather than debugging the app.  
The PSF modules use a project predefine of _DEBUG to make this happen.  
Many of the cpp files also contain an optional MOREDEBUG definition that is commented out near the top of the file.
This allows a developer to enable additional debugging in the Debug build specific to that file.

The exception to this is that PsfLauncher and PsfRuntime will log with the Release build, as this simplifies the job of debugging simple launch issues.

If you are not interested in fixing the PSF, you would be better served to include the PsfTraceFixup for debugging instead.

The other option in debugging, for developers, is to use one of the waitForDebugger options:

* To cause a breakpoint at the beginning of PsfLauncher, add the waitForDebugger: true option to PsfLauncher Application config in the config.json file. You **do not** need to add the WaitForDebuggerXxx.dll module to the package.
* To cause a breakpoint at the beginning of the target application, add the WaitForDebugger fixup to the process configuration of the desired process. In this case you **do** need to add the WaitForDebuggerXxx.dll module to the package. The breakpoint will occur when the WaitForDebuggerXxx.dll file is being initialized during it's injection.  When multiple fixups are defined for the process, the WaitForDebugger.dll will be injected as the first of the "fixup" dlls, no matter which order they are listed in the json.

## About PsfLauncher and Processes needing elevation
There is an inherent conflict that exists when an un-elevated launcher process 
wants to start a new process with elevation, plus wants to make sure it starts inside the container,
plus wants to start it suspended so that dll injection can occur. 

Previously, this caused the target application to launch with the elevation but without the injected dlls.

The internal manifest file has now been removed from PsfLauncher. 
This change, by itself, will have no impact on any packages.
However, this change allows someone that is adding the PSF into a package 
to determine situations where child process elevation is going to be needed, and to 
allow for this by adding an external manifest file for PsfLauncher in the package such that PsfLauncher will
immedietly elevate first.   

This external manifest file would use the same name 
as that used for the PsfLauncher copy in the package 
with a `.manifest` file extension added after the `.exe`.
The external manifest file only needs the `requestedExecutionLevel` settings for `level` and `uiAccess` (all additional manifest settings from the target should be ignogred).

Here is an example external manifest file:
```xml
<?xml version="1.0" encoding="utf-8"?>
<assembly xmlns:amsv3="urn:schemas-microsoft-com:asm.v3" manifestVersion="1.0" xmlns="urn:schemas-microsoft-com:asm.v1">
	<amsv3:trustInfo>
		<security>
			<requestedPrivileges>
				<requestedExecutionLevel level="requireAdministrator" uiAccess="false" />
			</requestedPrivileges>
		</security>
	</amsv3:trustInfo>
</assembly>
```

When PsfLauncher has a manifest for elevation, it is no longer required to specify the `allowElevation` capability in the AppXManifest.xml file. 
Prior to the 20H1 OS it was needed; including it now causes no harm but is not required.

If a child process in the container requires elevation (or self-elevation when the user selects a special feature) 
and that process does not use any fixups, it is possible to achieve this without elevating PsfLaucher, however,
in that case the `allowElevation` capability will need to be added to the AppXManifest.xml file.

## Configuration
PSF Launcher uses a config.json file to configure the behavior.
This file is normally placed in the root folder of the package, however it may be put anywhere in the package.
Should an application happen to also have a file named config.json, a syntax check will be made to ensure the found file is for the PSF.


### Example 1 - Package using FileRedirectionFixup
Given an application package with an `AppxManifest.xml` file containing the following:

```xml
<Package ...>
  ...
  <Applications>
    <Application Id="PSFSample"
                 Executable="PSFLauncher32.exe"
                 EntryPoint="Windows.FullTrustApplication">
      ...
    </Application>
  </Applications>
</Package>
```

A possible `config.json` example that includes fixups would be:

```json
{
    "applications": [
        {
            "id": "PSFSample",
            "executable": "PSFSampleApp/PrimaryApp.exe",
            "workingDirectory": "PSFSampleApp/"
        }
    ],
    "processes": [
        {
            "executable": "PSFSample",
            "fixups": [
                {
                    "dll": "FileRedirectionFixup.dll",
                    "config": {
                        "redirectedPaths": {
                            "packageRelative": [
                                {
                                    "base": "PSFSampleApp/",
                                    "patterns": [
                                        ".*\\/log"
                                    ]
                                }
                            ]
                        }
                    }
                }
            ]
        }
    ]
}
```

```xml
<?xml version="1.0" encoding="utf-8"?>
<configuration>
    <applications>
        <application>
            <id>PsfSample</id>
            <executable>PSFSampleApp/PrimaryApp.exe</executable>
            <workingDirectory>PSFSampleApp</workingDirectory>
        </application>
    </applications>
    <processes>
        <process>
            <executable>PsfLauncher.*</executable>
        </process>
        <process>
            <executable>PSFSample</executable>
            <fixups>
                <fixup>
                    <dll>FileRedirectionFixup.dll</dll>
                    <config>
                        <redirectedPaths>
                            <packageRelative>
                                <pathConfig>
                                    <base>PSFSampleApp/</base>
                                    <patterns>
                                        <pattern>
                                            .*\\/log
                                        </pattern>
                                    </patterns>
                                </pathConfig>
                            </packageRelative>
                        </redirectedPaths>
                    </config>
                </fixup>
            </fixups>
        </process>
    </processes>
</configuration>
```
In this example, the configuration is directing the PsfLauncher to start PsfSample.exe. The CurrentDirectory for that process is set to the folder containing PSFSample.exe.  In the processes section of the example, the json further configures the FileRedirection Fixup to be injected into the PSFSample, and that fixup is configured to perform file redirecton on log files in a particular folder. (See FileRedirectionFixup for additional details and examples on configuring this fixup).


### Example 2
This example shows using PsfLauncher with the TraceFixup and PsfMonitor. This might be used as part of a repackaging effort in order to understand whether fixups might be required for full funtionality of the package.

Given an application package with an `AppxManifest.xml` file containing the following:

```xml
<Package ...>
  ...
  <Applications>
    <Application Id="PrimaryApp"
                 Executable="PrimaryApp.exe"
                 EntryPoint="Windows.FullTrustApplication">
      ...
    </Application>
  <Applications>
    <Application Id="PSFLAUNCHERSixFour"
                 Executable="PSFLauncher64.exe"
                 EntryPoint="Windows.FullTrustApplication">
      ...
    </Application>
  </Applications>
</Package>
```

A possible `config.json` example that uses the Trace Fixup along with PsfMonitor would be:

```json
{
        "applications": [
        {
            "id": "PSFLAUNCHERSixFour",
            "executable": "PrimaryApp.exe",
            "arguments": "/AddedArg"
            "workingDirectory": "",
            "monitor": {
                "executable": "PsfMonitor.exe",
                "arguments": "",
                "asadmin": true,
   	    }
	}
  	],
        "processes": [
        {
            "executable": "PrimaryApp$",
            "fixups": [ 
            { 
                "dll": "TraceFixup64.dll",
                "config": {
                    "traceMethod": "eventlog",
                    "traceLevels": {
                        "default": "always"
                    }
                }
            } 
            ]
        }
        ]
}
```

```xml
<?xml version="1.0" encoding="utf-8"?>
<configuration>
    <applications>
        <application>
            <id>PSFLAUNCHERSixFour</id>
            <executable>PrimaryApp.exe</executable>
            <arguments>/AddedArg</arguments>
            <workingDirectory></workingDirectory>
            <monitor>
                <executable>PsfMonitor.exe</executable>
                <arguments></arguments>
                <asadmin>true</asadmin>
            </monitor>
        </application>
    </applications>
    <processes>
        <process>
            <executable>PrimaryApp$</executable>
            <fixups>
                <fixup>
                    <dll>TraceFixup64.dll</dll>
                    <config>
                        <traceMethod>eventLog</traceMethod>
                        <traceLevels>
                            <default>always</default>
                        </traceLevels>
                    </config>
                </fixup>
            </fixups>
        </process>
    </processes>
</configuration>
```

In this example, the configuration is directing the PsfLauncher to start PsfMonitor and then the referenced Primary App. PsfMonitor to be run using RunAs (enabling the monitor to also trace at the kernel level), followed by PrimaryApp once the monitoring app is stable. The root folder of the PrimaryApp is used for the CurrentDirectory of the PrimaryApp process.  In the processes section of the example, the json further configures Trace Fixup to be injected into the PrimaryApp, and that is to capture all levels of trace to the event log.



### Example 3
This example shows an alternative for the json used in the prior example. This might be used when command line arguments for the target executable include file paths and you want to reference those paths as full path references to their ultimate location in the deployed package.


```json
{
        "applications": [
        {
            "id": "PSFLAUNCHERSixFour",
            "executable": "PrimaryApp.exe",
            "arguments": "%MsixPackageRoot%\filename.ext %MsixPackageRoot%\VFS\LocalAppData\Vendor\file.ext"
            "workingDirectory": "",
            "monitor": {
                "executable": "PsfMonitor.exe",
                "arguments": "",
                "asadmin": true,
   	    }
	}
  	],
        "processes": [
        {
            "executable": "PrimaryApp$",
            "fixups": [ 
            { 
                "dll": "TraceFixup64.dll",
                "config": {
                    "traceMethod": "eventlog",
                    "traceLevels": {
                        "default": "always"
                    }
                }
            } 
            ]
        }
        ]
}
```


In this example, the pseudo-variable %MsixPackageRoot% would be replaced by the folder path that the package was installed to. This pseudo-variable is only available for string replacement by PsfLauncher in the arguments field. The example shows a reference to a file that was placed at the root of the package and another that will exist using VFS pathing. Note that as long as the LOCALAPPDATA file is the only file required from the package LOCALAPPDATA folder, the use of FileRedirectionFixup would not be mandated.

### Example 4
This example shows an alternative for the json used in the prior example. This might be used when an additional script is to be run upon first launch of the application. Such scripts are sometimes used for per-user configuration activities that must be made based on local conditions.

To implement scripting PsfLauncher uses a PowerShell script as an intermediator.  PshLauncher will expect to find a file StartingScriptWrapper.ps1, which is included as part of the PSF, to call the PowerShell script that is referenced in the Json.  The wrapper script should be placed in the package as the same folder used by the launcher.


```json
  "applications": [
    {
      "id": "Sample",
      "executable": "Sample.exe",
      "workingDirectory": "",
      "stopOnScriptError": false,
      "scriptExecutionMode": "-ExecutionPolicy Bypass",
	  "startScript":
	  {
		"waitForScriptToFinish": true,
		"timeout": 30000,
		"runOnce": true,
		"showWindow": false,
		"scriptPath": "PackageStartScript.ps1",
		"waitForScriptToFinish": true
		"scriptArguments": "%MsixWritablePackageRoot%\\VFS\LocalAppData\\Vendor",
	  },
	  "endScript":
	  {
		"scriptPath": "\\server\scriptshare\\RunMeAfter.ps1",
		"scriptArguments": "ThisIsMe.txt"
	  }
    }
  ],
  "processes": [
    ...(taken out for brevity)
  ]
}
```

In this example two scripts are defined. The startScript is configured so that PsfLauncher will run this script only the first time that this user runs the application. It will wait for completion of the script before starting the executable associated with the application.  
PsfLauncher will resolve the %MsixWritablePackageRoot% pseudo-variable allowing the script to write the configuration into the package redirected area (necessary since package scripts do not inject the FileRedirectionFixup).  The endScript example is configured to run after each time the application is run.

Note that the scriptPath must point to a powershell ps1 file; it may be a full path reference or a reference relative to the package root folder.  In addition to this script file, the package must also include a file named "StartingScriptWrapper.ps1", which is included in the Psf sources.  This script is run by PsfLauncher for any startScript or endScript, and the wrapper will invoke the script defined in the json configuration.

The use of scriptExecutionMode may only be necessary in environments when Group Policy setting of default PowerShell ExecutionPolicy is expressed. 

### Example 5
This example shows how the launcher may be used to start an executable that is not in the package, but by referencing it using VFS pathing. 
Referencing using VFS pathing allows for end-user systems where the standard paths might be altered from what we normally expect.

We should note that there may be reasons to force a particular program (as is done in this example), or to instead let the end-users's system decide what program to use to open the txt file. Instead of the configuration shown below, you could configure the json application entry with the executable field with the .txt file (and blank arguments field) and let the client system use whatever file type assocated program is regitered for that file type. 


```json
  "applications": [
    {
      "id": "Sample5",
      "executable": "VFS\\Windows\\Notepad.exe",
      "arguments" : "VFS\\ProgramFilesX64\\Sample5\\Readme.txt"
      "workingDirectory": ""
    }
  ],
  "processes": [
    ...(taken out for brevity)
  ]
}
```

In this example the executable file is not inside the package.  The launcher will attempt to see if the executable is in the VFS path of the package,
and if not it will attempt to use the reverse-vfs path well known folder (typically translated to C"":\Windows\Notepad.exe").

This reverse-vfs only applies to the executable field as process launching for the exe is not subject to the normal MSIX runtime searching.
In the case of the arguments field, the executable will be passed the argument as a relative path from the package root, however the MSIX runtime and/or FileRedirectionFixup can
intercept any file open call to redirect as appropriate.  In our case, we expect that the text file is part of the package and notepad will open the file from the package, 
or the user-redirected area for the package if previously altered.

### Json Schema

| Array | key | Value |
|-------|-----------|-------|
| applications | id |  Use the value of the `Id` attribute of the `Application` element in the package manifest. |
| applications | waitForDebugger | (Optional, default=false) Boolean. This option is for debugging of the launcher itself and is only available in debug builds of the launcher, it is silently ignored if present in a release build. When set to true, the launcher will wait to receive a signal that a debugger has attached to the process. The wait occurs immediately after reading enough of the json file to detect this setting, before processing other settings, launching scripts, monitor, and target. |
| applications | executable | The path to the executable that you want to start. This path is typically specified relative to the package root folder. In most cases, you can get this value from your package manifest file before you modify it. It's the value of the `Executable` attribute of the `Application` element. Pseudo-variables and Environment variables are supported for this path. |
| applications | arguments | (Optional) Command line arguments for the executable.  If the PsfLauncher.exe receives any arguments, these will be appended to the command line after those from the config.json file. Pseudo-variables and Environment variables are supported in the arguments. |
| applications | workingDirectory | (Optional) A path to use as the working directory of the application that starts. This is typically a relative path of the package root folder, however full paths may be specified. If you don't set this value, the operating system uses the `System32` directory as the application's working directory. If you supply a value in the form of an empty string, it will use the directory of the referenced executable. Pseudo-variables and Environment variables are supported in the workingDirectory. |
| applications | monitor | (Optional) If present, the monitor identifies a secondary program that is to be launched prior to starting the primary application.  A good example might be `PsfMonitor.exe`.  The monitor configuration consists of the following items: |
| | |   `'executable'` - This is the name of the executable relative to the root of the package. |
| | |   `'arguments'`  - This is a string containing any command line arguments that the monitor executable requires. Any use of the string "%MsixPackageRoot%" in the arguments will be replaced by the a string containing the actual package root folder at runtime. |
| | |   `'asadmin'` - This is a boolean (0 or 1) indicating if the executable needs to be launched as an admin.  To use this option set to 1, you must also mark the package with the RunAsAdministrator capability.  If the monitor executable has a manifest (internal or external) it is ignored.  If not expressed, this defaults to a 0. |
| | |   `'wait'` - This is a boolean (0 or 1) indicating if the launcher should wait for the monitor program to exit prior to starting the primary application.  When not set, the launcher will WaitForInputIdle on the monitor before launching the primary application. This option is not normally used for tracing and defaults to 0. |
| applications | stopOnScriptError| (Optional) Boolean. Indicates that if a startScript returns an error then the launch of the application should be skipped. |
| applications | ScriptExecutionMode | (Optional) String value that will be added to the powershell launch of any startScript or endScript. |
| applications | startScript | (Optional) If present, used to define a PowerShell script that will be run prior running the application executable. |
| | |  `'waitForScriptToFinish'` - (Optional, default=false) Boolean. When true, PsfLauncher will wait for the script to complete or timeout before running the application executable. |
| | | `'timeout'` - (Optional, default is none) Expressed in ms.  Only applicable if waitForScriptToFinish is true.  If a timeout occurs it is treated as an error for the purpose of `'stopOnScriptError'`. The value 0 means an immediate timeout, if you do not want a timeout do not specify a value. |
| | | `'runOnce'` - (Optional, default=false) Boolean. When true, the script will only be run the first time the user runs the application. |
| | | `'showWindow'` - (Optional, default=true). Boolean. When false, the PowerShell window is hidden. |
| | | `'scriptPath'` - Relative or full path to a ps1 file. May be in package or on a network share. Use of pseudo-variables or environment variables are supported. |
| | | `'scriptArguments'` - (Optional) Arguments for the `'scriptPath'` PowerShell file.  Use of pseudo-variables or environment variables are supported. |
| applications | endScript | (Optional) If present, used to define a PowerShell script that will be run after completion of the application executable. |
| | | `'runOnce'` - (Optional, default=false) Boolean. When true, the script will only be run the first time the user runs the application. |
| | | `'showWindow'` - (Optional, default=true). Boolean. When false, the PowerShell window is hidden. |
| | | `'scriptPath'` - Relative or full path to a ps1 file. May be in package or on a network share. Use of pseudo-variables or environment variables are supported. |
| | | `'scriptArguments'` - (Optional) Arguments for the `'scriptPath'` PowerShell file.  Use of pseudo-variables or environment variables are supported. |
| processes | executable | In most cases, this will be the name of the `executable` configured above with the path and file extension removed. |
| fixups | dll | Package-relative path to the fixup, .msix/.appx  to load. |
| fixups | config | (Optional) Controls how the fixup dl behaves. The exact format of this value varies on a fixup-by-fixup basis as each fixup can interpret this "blob" as it wants. |

The `applications`, `processes`, and `fixups` keys are arrays. That means that you can use the config.json file to specify more than one application, process, and fixup DLL.

An entry in `processes` has a value named `executable`, the value for which should be formed as a RegEx string to match the name of an executable process without path or file extension. The launcher will expect Windows SDK std:: library RegEx ECMAList syntax for the RegEx string.

### PsfLauncher Pseudo-Variables
The PSF Launcher supports the use of two special purpose "pseudo-variables". These pseudo-variables provide package specific locations are present on the target system.  These pseudo-variables may be used on the application executable, arguments, and workingDirectory fields, as well as in startScript/endScript scriptPath and scriptArguments.  The PSF Launcher will derefernce these variables (and any specified environment variables) prior to launching the script/application. The PSF Launcher pseudo-vairables are:

| Variable| Value |
|---------|-------|
| %MsixPackageRoot% | The root folder of the package. While nominally this would be a subfolder under "C:\\Program Files\\WindowsApps" it is possible for the volume to be mounted in other locations. |
| %MsixWritablePackageRoot% | The package specific redirection location for this user when the FileRedirectionFixup is in use. | 

### PsfLauncher Additional Requirements
PsfLauncher will expect to find, under certain conditions, additional script files with specific names located in the package:
* [StartingScriptWrapper.ps1] This script file is required if the config.json includes either a `StartScript` or `EndScript` entry.
* [StartMenuCmdScriptWrapper.ps1] This script file is required if the `executable` file listed for an application entry of the `config.json` file references a file with a "`.cmd`" or "`.bat`" file extension.
* [StartMenuCmdShellLaunchWrapperScript.ps1] This script file is required if the `executable` file listed for an application entry of the `config.json` file references a file that does NOT end in one of these file extensions: "`.exe`", "`.cmd`", or "`.bat`".

These script wrapper files may be placed anywhere in the package, although traditionally they are placed either in the root folder of the package or in the same folder as the executable file listed in the config.json application.

### Will Launched Processes run in the container?
By default, processes created by the existance of an application entry of the `config.json` file with an application executable value that is an `.exe` file type, and it's child processes, all run inside the container. (This is a change in 2021.11.02 release, previously exe files not located inside the package ran outside of the container).

The  `StartScript` and `EndScript` script referenced in a `config.json` application will always run inside of the container.  The launcher will use a powershell process to accomplish this.  

Starting with the 2021.11.02 release, `CMD/BAT` scripts listed as the config.json application executable value, will now also run inside the container. A powershell process will be used to inject the cmd/bat script back into the container.  This solution will not work on end-user systems that are not on 21h1 OS release.  Consider using the launcher from prior PSF versions for back-rev client systems (where the cmd\bat will run outside of the container).

Starting with the 2021.11.02 release, the Other `non-exe` file types listed as the config.json application executable value will be executed using the default file association for the file type on the end-user system.  If the executable file associated with the default action is a windows exe, it will run inside the container; if it is a console app (such as hh.exe that is the normal default to display .chm files) it will run outside of the container and an extra cmd window will appear visible to the end-user.  A powershell process will be used to inject the cmd/bat script back into the container.  This solution will not work on end-user systems that are not on 21h1 OS release.  Consider using the launcher from prior PSF versions for back-rev client systems (where the cmd\bat will run outside of the container).

=======
Submit your own fixup(s) to the community:
1. Create a private fork for yourself
2. Make your changes in your private branch
3. For new files that you are adding include the following Copyright statement.\
//-------------------------------------------------------------------------------------------------------\
// Copyright (c) #YOUR NAME#. All rights reserved.\
// Licensed under the MIT license. See LICENSE file in the project root for full license information.\
//-------------------------------------------------------------------------------------------------------
4. Create a pull request into 'fork:Microsoft/MSIX-PackageSupportFramework' 'base:master'
