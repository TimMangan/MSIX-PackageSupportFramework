# Registry Legacy Fixups
When injected into a process, RegLegacyFixups supports the ability to:
> * Modify certain registry calls that do not work due to the MSIX container restrictions by modifying the call parameters to a form that would be allowed.

There is no guarantee that these changes will be enough to allow the application to perform properly, but often the change can solve application compatibility issues by removing request incompatible parameters that the application did not need.
The fixup is generally considered safe to apply with common rules as shown in the example below, even for apps where it is not needed.

### Detecting the need for this fixup
For the IT Pro without access to the application source code, it is not possible to detect the need for this fixup by static analysis; the need must be determined by testing.
This is generally easy to determine by running the application in the container with a tracing tool, either TraceFixup or Process Monitor trace.
The application needing this fixup will experience ACCESS_DENIED results on registry operations. Most common are denials when the application attempts to open a registry key.  
Less often seen is ACCESS_DENIED when the applicatoin tries to delete an item from the conrainter registry.

For the developer, you should address the code making such calls directly instead of fixing with this fixup.  
In the first case, you should now open registry keys with either the exact permissionis that you need, or MAX_ALLOWED.
In all cases you should check and gracefully handle any error conditions you get.

## About Debugging this fixup
The Release build of this fixup produces no output to the debug console port for performance reasons.
Use of the Debug build will enable you to see the intercepts and what the fixup did.
That output is easily seen using the Sysinternals "DebugView" tool.

### Dependencies for RegLegacyFixups
When using this fixup, you must also supply the following dependencies.  It is recommended that these be
placed in the VFS\SystemX64 and VFS\SystemX86 folders of the package, such that if the
application uses a different version of these dlls, the app specific version will be found.

| Release Build | Debug Build |
| --- | --- |
| msvcp140.dll | msvcp140d.dll |
| vcruntime140.dll | vcruntime140d.dll |
| | ucrtbased.dll |

A copy of these may be found in the OBSOLETE folder of the PSF release.


# General Configuration
The configuration for the Registry Legacy Fixups is specified in the element `config` of the fixup structure within the `proceses` section of the json file when RegLegacyFixups.dll is requested.

The `config` element for RegLegacyFixups.dll is an array of remediations.  Each remediation has two elements:

| Name | Purpose |
| ---- | ------- |
| `remediation` | An array of remediations. Structure for array elements is given in a tables for the specfic remediation type. |

Each remediation array object starts with a type field:
| `type` | Remediation type.  The values supported are given in a table below. |

| Remediation Type | Purpose |
| --------------- | ------- |
| `ModifyKeyAccess` | Allows for modification of access parameters in calls to open registry keys.  This remediation targets the `samDesired` parameter that specifies the permissions granted to the application when opening the key. This remediation type does not target calls for registry values.|
| `FakeDelete`      | Returns success to the application if it attempts to delete a key or registry item and "ACCESS_DENIED" occurs.  The app may or may not depend upon the delete occuring at some later point of running the application, so significant testing of the app is suggested when attempting this fixup.|
| `DeletionMarker`  | Returns PATH_NOT_FOUND to the application if it attempts to access a key or registry item at or below where a deletion marker is present in this configuration, even if the key/value is present in either the virtual or real registry.|
| `JavaBlocker`     | Returns PATH_NOT_FOUND to calls to locate a Java version higher than the specified value. This is a simply configured support for apps that require a specific version of Java and we need to keep the app from seeing a newer vesion if locally deployed.|

## ModifyKeyAccess Remediation Type
The following Windows API calls are supported for this fixup type. 

> * RegCreateKeyEx
> * RegDeleteKey
> * RegDeleteKeyEx
> * RegDeleteKeyTransacted
> * RegDeleteValue
> * RegOpenKeyEx
> * RegOpenKeyTransacted

### Configuration for ModifyKeyAccess
When the `type` is specified as `ModifyKeyAccess`, the `remediation` element is an array of remediations with a structure shown here:

| `hive` | Specifies the registry hive targeted. |
| `patterns` | An array of regex strings. The pattern will be used against the path of the registry key being processed. |
| `access` | Defines the type of access to be modified and what it should be modified to. |

The value of the `hive` element may specified as shown in this table:

| Value | Purpose |
| HKCU  | HKEY_CURRENT_USER |
| HKLM  | HKEY_LOCAL_MACHINE |

The value for the element `patterns` is a regex pattern string that specifies the registry paths to be affected.  This regex string is applied against the path of the key/subkey combination without the hive.

The value of the `access` element is given in the following table:

| Access | Purpose |
| ------ | ------- |
| Full2RW | If the caller requested full access (for example STANDARD_RIGHTS_ALL), modify the call to remove KEY_CREATE_LINK. |
| Full2R  | If the caller requested full access (for example STANDARD_RIGHTS_ALL), modify the call to remove KEY_CREATE_LINK, KEY_CREATE_SUB_KEY, and KEY_CREATE_VALUE. |
| Full2MaxAllowed  | If the caller requested full access (for example STANDARD_RIGHTS_ALL), modify the call to request MAXIMUM_ALLOWED.|
| RW2R    | If the caller requested Read/Write access, modify the call to to remove KEY_CREATE_LINK, KEY_CREATE_SUB_KEY, and KEY_CREATE_VALUE. |
| RW2MaxAllowed  | If the caller requested Read/Write access, modify the call to request MAXIMUM_ALLOWED.|

## FakeDelete Remediation Type
The following Windows API calls are supported for this fixup type. 

> * RegDeleteKey
> * RegDeleteKeyEx
> * RegDeleteKeyTransacted
> * RegDeleteKeyValue
> * RegDeleteValue
> * RegEnumValue
> * RegGetValue
> * RegQueryValueEx

### Configuration for FakeDelete
When the `type` is specified as `FakeDelete`, the `remediation` element is an array of remediations with a structure shown here:

| `hive` | Specifies the registry hive targeted. |
| `patterns` | An array of regex strings. The pattern will be used against the path of the registry key being processed. |

The value of the `hive` element may specified as shown in this table:

| Value | Purpose |
| HKCU  | HKEY_CURRENT_USER |
| HKLM  | HKEY_LOCAL_MACHINE |

The value for the element `patterns` is a regex pattern string that specifies the registry paths to be affected.  This regex string is applied against the path of the key/subkey combination without the hive.


## DeletionMarker Remediation Type
The following Windows API calls are supported for this fixup type. 

> * RegCreateKeyEx
> * RegDeleteKey           
> * RegDeleteKeyEx         
> * RegDeleteKeyTransacted 
> * RegDeleteTree          [notimplemented]
> * RegDeleteValue         
> * RegEnumKeyEx           
> * RegEnumValue           [notimplemented]
> * RegGetValue            [notimplemented, handled by ReqQueryValueEx which is called by this]
> * RegOpenKey
> * RegOpenKeyEx
> * RegOpenKeyTransacted
> * RegQueryInfoKey        [notimplemented]
> * RegQueryMultipleValues [notimplemented]
> * RegQueryValueEx        
> * RegQueryRenameKey      [notimplemented]
> * RegSetKeyValue         [notimplemented]
> * RegSetKeyValueEx       [notimplemented]

The API calls marked as [notimplemented] are not yet implemented because they are unlikly to be used under a deletion key, but will be considered for future releases if needed.

### Configuration for DeletionMarker
When the `type` is specified as `DeletionMarker`, the `remediation` element is an array of remediations with a structure shown here:

| `hive` | Specifies the registry hive targeted. |
| `key` | Specifies a regex string pattern to match the base key being the hive targeted.  For example `Software\\\\Vendor.*`. |
| `patterns` | An array of regex strings. The pattern will be used, alone with the key against the path of the registry key/valuename being processed. Use `.*` to be greedy. |

The value of the `hive` element may specified as shown in this table:

| Value | Purpose |
| HKCU  | HKEY_CURRENT_USER |
| HKLM  | HKEY_LOCAL_MACHINE |

Note that when the call by the application specifies a regkey, subkey, and a valuename as separate items in the call, the subkey and valuename are combined to perform matching against the key and patterns.



## JavaBlocker Remediation Type
The following Windows API calls are supported for this fixup type. 

> * RegOpenKey
> * RegOpenKeyEx
> * RegOpenKeyTransacted

### Configuration for JavaBlocker
When the `type` is specified as `JavaBlocker`, the `remediation` element is an array of remediations with a structure shown here:

| `majorVersion` | Specifies the major version number of the java version to be supported.  For example, for Java 1.8U133 this is `1`.|
| `minorVersion` | Specifies the minor version number of the java version to be supported.  For example, for Java 1.8U133 this is `8`. |
| `updateVersion` | Specifies the update version number of the java version to be supported.  For example, for Java 1.8U133 this is `133`. Specify `999` for any. |


# General Notes
NOTES:
> * All registry regex pattern matches will be done case insensitive.
> * Sometimes calling code will end up with an application hive key, which looks like =\REGISTRY\USER\S-1-5... or =\REGISTRY\MACHINE\S-1-5-...
      The RegLegacyFixup will match these patterns against the HKCU/HKLM `hive` element, however care is cautioned on the pattern that you use.
	  The regex pattern will match against what comes after HKEY_CURRENT_USER\ or =\REGISTRY\USER\S-1-5...\ which should be strings starting with "Software" or "System".  See examples below.

# JSON Example
Here is an example of the same fixup that adds a DeletionMarker and a JavaBlocker.

```json
"fixups": [
	{
		"dll":"RegLegacyFixups.dll",
		"config": [
		  {
			"remediation": [
				{
					"type": "ModifyKeyAccess",
					"hive": "HKCU",
					"patterns": [
						"^Software\\\\Vendor.*"
					],
					"access": "Full2RW"
				},
				{
					"type": "ModifyKeyAccess",
					"hive": "HKLM",
					"patterns": [
						"^Software\\\\Vendor.*"
					],
					"access": "RW2MaxAllowed"
				},
				{
					"type": "FakeDelete",
					"hive": "HKCU",
					"patterns": [
						"^SOftWARE\\\\Vendor\\\\.*"
					],
				},
                {
                  "type": "JavaBlocker",
                  "majorVersion": "1",
                  "minorVersion": "7",
                  "updateVersion": "1"
                },
                {
                  "type": "DeletionMarker",
                  "hive": "HKCU",
                  "key": "Software\\\\Vendor_Deletion\\\\SubKey\\\\.*",
                  "patterns": [
                    "HideThis.*",
					"OrThat.*"
                  ]
                }
			]
		  }
		]
	}
]
```

#XML Example
Here is the equivalent config section when using XML to specify.

```xml
<fixups>
	<fixup>
		<dll>RegLegacyFixups</dll>
		<config>
			<rediations>
				<remediation>
					<type>ModifyKeyAccess</type>
					<hive>HKCU</hive>
					<patterns>
						<patern>^Software\\\\Vendor.*</pattern>
					</patterns>
					<access>Full2RW</access>
				</remediation>
				<remediation>
					<type>ModifyKeyAccess</type>
					<hive>HKLM</hive>
					<patterns>
						<patern>^SOFTWARE\\\\Vendor.*</pattern>
					</patterns>
					<access>RW2R</access>
				</remediation>
				<remediation>
					<type>FakeDelete</type>
					<hive>HKCU</hive>
					<patterns>
						<patern>^SOftWARE\\\\Vendor\\\\.*</pattern>
					</patterns>
					<access>RW2R</access>
				</remediation>
			</remediations>
		</config>
	</fixup>
</fixups>


