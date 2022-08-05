# Package Drive Test
Validates that `FileRedirectionFixup` handles scenarios involving VFS\AppVPackageDrive.

The package has a file under the following three package files:

> * VFS\AppVPackageDrive\TestDriveFolder1\DriveFïℓè.txt
> * VFS\AppVPackageDrive\TestDriveFolder2\DriveFïℓè.txt
> * VFS\AppVPackageDrive\TestDriveFolder3\DriveFïℓè.txt

The first folder is FRF adjusted for redirection.
The second folder is FRF excluded (meaning it should map for reading but not writing)
The third folder is not specifically listed in FRF packageDrive configuration (meaning if the app asks using native pathing it can't be found, but otherwise redirected.)