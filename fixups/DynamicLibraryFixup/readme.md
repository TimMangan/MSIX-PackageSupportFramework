# DynamicLibrary Fixup
When injected into a process, the DynamicLibraryFixup supports the ability to:
> * Define mappings between dll name and location within the package in the Config.Json file.
> * Ensure that the named dll will be found within the package anytime the application tries to load the dll.

There are a bunch of reasons that the traditional ways of apps locating their dlls fail under MSIX.
Although there are specific fixes for many of these issues, this fixup creates a sure-fire way to get it loaded.

### Detecting the need for this fixup
The primary causes for needing this fixup are situations where the original product depended up changes to the Path Environment Variable, or App Paths registration of additional folders that contain dlls.
It can also happen due to a change in Working Directory.

At runtime within the container, the symptom showing the need will be a dll not found issue for a dll that is in the package.


## About Debugging this fixup
The Release build of this fixup produces no output to the debug console port for performance reasons.
Use of the Debug build will enable you to see the intercepts and what the fixup did.
That output is easily seen using the Sysinternals "DebugView" tool.

## Configuration
The configuration for the EnvVarFixup is specified under the element `config` of the fixup structure within the json file when EnvVarFixup.dll is requested.
This `config` element contains a an array called "EnvVars".  Each Envar contains three values:

| PropertyName | Description |
| ------------ | ----------- |
| `forcePackageDllUse` | Boolean.  Set to true.|
| `relativeDllPaths` | An array. See below. |

Each element of the array has the following structure:

| PropertyName | Description |
| ------------ | ----------- |
| `name`| This is the name as requested by the application. This will be the name of the file, without any path information and without the filename extension.|
| `filepath`| The filepath relative to the root folder of the package. |
| `architecture`| An optional value to speficy the 'bitness' of the dll.  Supported values include `x86`, `x64`, and `anyCPU`. When not specified, no checking for archtecture of the process and dll will be made.|

The `architecure` is optional and normally need not be specified for simplicity. It is included because sometimes an app contains both 32 and 64 bit exes for different purposes that need to load the correct version of the same named dll, typically stored in a different folder. When the package has this situation, it is then necessary to specify the architecture.  The fixup for LoadDll will match up the appropriate version of the dll based on the process it is running under.

# JSON Examples
To make things simpler to understand, here is a potential example configuration object that is not using the optional parameters:

```json
"config": {
    "forcePackageDllUse": "true",
    "relativeDllPaths": [
        {
            "name" : "DllName_without_DotDll",
            "filepath" : "RelativePathToFile_including_DllName_without_DotDll.dll"
        },
        {
            "name" : "DllName2",
            "filepath" : "VFS\ProgramFilesX84\Vendor\App\Subfolder\DllName2.dll"
        },
        {
            "name" : "DllNameX",
            "filepath" : "VFS\ProgramFilesX84\Vendor\App\x64\DllNameX.dll",
            "architecture" : "x64"
        },
        {
            "name" : "DllNameX",
            "filepath" : "VFS\ProgramFilesX84\Vendor\App\32bit\DllNameX.dll",
            "architecture" : "x86"
        },
        {
            "name" : "DllNameY",
            "filepath" : "VFS\ProgramFilesX84\Vendor\App\Subdir\DllNameY.dll",
            "architecture" : "anyCPU"
        }
    ]
}
```

