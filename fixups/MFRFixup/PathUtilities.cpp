//-------------------------------------------------------------------------------------------------------
// Copyright (C) TMurgent Technologies, LLP. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------

#include <filesystem>
#include <dos_paths.h>
#include "PathUtilities.h"
#include "FunctionImplementations.h"
#include <psf_logging.h>

/// <summary>
/// Utility functions to determine if a given file path is relative to a (w)char string, as in the path starts the same.
/// Comparison is perfomed case insensitive.
/// </summary>
template <typename CharT>
bool path_isSubsetOf_StringImpl( std::filesystem::path& basePath, const CharT* pathstring)
{
    // Compare using case insesitive matching
    return std::equal(basePath.native().begin(), basePath.native().end(), pathstring, psf::path_compare{});
}
bool path_isSubsetOf_String( std::filesystem::path& basePath, const wchar_t* pathstring)
{
    return path_isSubsetOf_StringImpl(basePath, pathstring);
}
bool path_isSubsetOf_String( std::filesystem::path& basePath, const char* pathstring)
{
    return path_isSubsetOf_StringImpl(basePath, pathstring);
}


/// <summary>
/// Utility functions to determine if a given file (w)string is relative to a path, as in the string starts the same.
/// Comparison is perfomed case insensitive.
/// </summary>
bool pathString_isSubsetOf_Path(const wchar_t* pathstring, std::filesystem::path& Path)
{
    std::filesystem::path wpathpart = pathstring;
    return std::equal(wpathpart.native().begin(), wpathpart.native().end(), Path.generic_wstring().c_str(), psf::path_compare{});
}
bool pathString_isSubsetOf_Path(const char* pathstring, std::filesystem::path& Path)
{    
    std::filesystem::path wpathpart = widen(pathstring);
    return std::equal(wpathpart.native().begin(), wpathpart.native().end(), Path.generic_wstring().c_str(), psf::path_compare{});
}

/// <summary>
///   Given a Wstring representing a file path, replace the folder portion from one to another.
///   There is no check to see if the input string contains the from portion, it is assumed to be correct (even if not case sensitive).
/// </summary>
/// <param name="inputWstring">A wstring like "C:\Windows\foo.txt"</param>
/// <param name="from">The path "C:\WINDOWS"</param>
/// <param name="to">The path "C:\Program Files\WindowsApps\pkgname\VFS\Windows"</param>
/// <returns>A wstring like "C:\Program Files\WindowsApps\pkgname\VFS\Windows\foo.txt"</returns>
std::wstring ReplacePathPart(std::wstring inputWstring, std::filesystem::path from, std::filesystem::path to)
{
    std::wstring outputWstring = to.c_str();
    size_t removecount = wcslen(from.c_str());
    if (removecount < wcslen(inputWstring.c_str()))
    {
        outputWstring.append(inputWstring.substr(removecount));
        return outputWstring;
    }
    return inputWstring;
} // ReplacePathPart

/// <summary>
/// Given a wstring representing a file path, look for  folder\sub\..\something and remove any .. level and prior folder to simplify the path
/// </summary>
/// <param name="inputWs"></param>
/// <returns></returns>
std::wstring PurgeDotDotFolders(std::wstring inputWs)
{
    std::wstring outputWs = inputWs;
    size_t SlashDotsStartAt = outputWs.find(L"\\..\\");
    while (SlashDotsStartAt != std::wstring::npos)
    {
        size_t replaceStartsAt = SlashDotsStartAt -1; // before slash
        while (replaceStartsAt > 0)
        {
            if (outputWs.at(replaceStartsAt) == L'\\')
            {
                std::wstring temp = outputWs.substr(0, replaceStartsAt);
                temp.append( outputWs.substr(SlashDotsStartAt + 3));
                outputWs = temp;
                replaceStartsAt = 0;  // terminate this while
                SlashDotsStartAt = outputWs.find(L"\\..\\");  // look for another
            }
            else
            {
                replaceStartsAt--;
            }
        }
    }
    return outputWs;
}

/// <summary>
///     Utility to convert a drive relative path (c:foo\fie) to drive normal (c:\{cwd}\foo\fie)
///     Also simplifies c:\foo\fie\foe\..\..\fum to c:\foo\fum
/// </summary>
std::filesystem::path drive_relative_to_normal(std::filesystem::path nativeRelativePath)
{
    std::wstring resultWstr = std::filesystem::current_path().c_str();
    resultWstr.append(L"\\");
    resultWstr.append(nativeRelativePath.c_str() + 2);
    resultWstr = PurgeDotDotFolders(resultWstr);
    std::filesystem::path resultPath = resultWstr;
    return resultPath;
}

/// <summary>
///     Utility to convert a rooted relative path (\foo\fie) to drive normal (c:\foo\fie)
/// </summary>
std::filesystem::path rooted_relative_to_normal(std::filesystem::path nativeRelativePath)
{
    std::wstring resultWstr = std::filesystem::current_path().root_name().c_str();
    resultWstr.append(nativeRelativePath.c_str());// .generic_wstring().data());
    resultWstr = PurgeDotDotFolders(resultWstr);
    std::filesystem::path resultPath = resultWstr;
    return resultPath;
}


/// <summary>
///     Utility to convert a cwd relative path (foo\fie) to drive normal (c:\cwd\foo\fie)
/// </summary>
std::filesystem::path cwd_relative_to_normal(std::filesystem::path nativeRelativePath)
{
    std::wstring resultWstr = std::filesystem::current_path().c_str();
    resultWstr.append(L"\\");
    resultWstr.append(nativeRelativePath.c_str());// .generic_wstring().data());
    resultWstr = PurgeDotDotFolders(resultWstr);
    std::filesystem::path resultPath = resultWstr;
    return resultPath;
}



/// <summary>
///     "\\?\C:\path\to\file normalized to C:\path\to\file 
///     Utility to convert a root local  path (foo\fie) to drive normal (c:\cwd\foo\fie)
/// </summary>
std::filesystem::path root_local_relative_to_normal(std::filesystem::path nativeRelativePath)
{
    //std::filesystem::path resultPath = "";
    //resultPath.append(nativeRelativePath.c_str() + 4);// .generic_wstring().data() + 4);
    std::wstring resultWstr = (nativeRelativePath.c_str() + 4);
    resultWstr = PurgeDotDotFolders(resultWstr);
    std::filesystem::path resultPath = resultWstr;
    return resultPath;
}


/// <summary>
///     Simplifies c:\foo\fie\foe\..\..\fum to c:\foo\fum
/// </summary>
std::filesystem::path drive_absolute_to_normal(std::filesystem::path nativeRelativeAbsolutePath)
{
    std::wstring resultWstr = nativeRelativeAbsolutePath.c_str();
    resultWstr = PurgeDotDotFolders(resultWstr);
    std::filesystem::path resultPath = resultWstr;
    return resultPath;
}

///
/// Adjust a file path for common non-standard requests that might or might not work as is,
/// but give our code fits.  Alter the path to look normal.
std::wstring AdjustSlashes(std::wstring path)
{
    std::wstring wPathName = path;
    
    // Part 1:  Spin any backwards slashes around.
    std::replace(wPathName.begin(), wPathName.end(), L'/', L'\\');
    size_t start = 0;

    // Part 2: Replace any double backslashes with a single, except for
    //         Long path references (\\?\ and \\.\) and file share references.
    if (wPathName.find(L"\\\\") != std::wstring::npos)
    {
        start = 2;
    }
    size_t found = wPathName.find(L"\\\\", start);
    while (found != std::wstring::npos)
    {
#ifdef _DEBUG
        Log(L"Adjusting for double backslash.");
#endif
        // We see calls made with extra backslashes which will fail in FindFirst
        //wPathName.replace(found + start, 2, L"\\");
        std::wstring temp = wPathName.substr(0, found);
        temp.append(wPathName.substr(found + 1));
        wPathName = temp;
        found = wPathName.find(L"\\\\", start);
    }
    return wPathName;
}


/// <summary>
/// Given a file path, return a path that in the long path form
/// </summary>
/// <param name="path"></param>
/// <returns></retrns>
std::wstring MakeLongPath(std::wstring path)
{
    // Only add to full paths on a drive letter
    if (path.length() > 0)
    {
        psf::dos_path_type dosType = psf::path_type(path.c_str());
        if (dosType == psf::dos_path_type::drive_absolute)
        {
            if (path.length() > 3)   // Don't extend C:\ as it messes up FindFirstFile(Ex)
            {
                std::wstring outPath = L"\\\\?\\";
                return outPath.append(path);
            }
        }
    }
    return path;
}

/// <summary>
/// Given a file path, return a path that is NOT in the long path form
/// </summary>
/// <param name="path"></param>
/// <returns></returns>
std::wstring MakeNotLongPath(std::wstring path)
{
    size_t LongPathStartAt = path.find(L"\\\\?\\");
    if (LongPathStartAt != std::wstring::npos)
    {
        return path.substr(4);;
    }
    return path;
}


// Most internal use of GetFileAttributes is to check to see if a file/directory exists, so provide a helper, but don't redirect and don't affect existing GetLastError
bool PathExists(const wchar_t* path)
{
    DWORD oldErr = GetLastError();
    bool bRet = (impl::GetFileAttributes(MakeLongPath(path).c_str()) != INVALID_FILE_ATTRIBUTES);
    SetLastError(oldErr);
    return bRet;
}

// Sometimes a file/directory doesn't exist, but if it's parent does makes a difference in logic applied
bool PathParentExists(const wchar_t* path)
{
    std::wstring notlongpath = MakeNotLongPath(path);
    size_t position = notlongpath.find_last_of(L'\\');
    if (position != std::wstring::npos &&
        position > 2)
    {
        return PathExists(notlongpath.substr(0, position).c_str());
    }
    return true;
}

/// <summary>
/// Given a file path, ensure all directories are created so that we can do a file operation on the file.
/// </summary>
/// <param name="filepath"></param>
void PreCreateFolders(std::wstring filepath, [[maybe_unused]] DWORD dllInstance, [[maybe_unused]] std::wstring DebugMessage)
{
    std::wstring notlongfilepath = MakeNotLongPath(filepath);
    size_t position;
    std::vector<std::wstring> folderlist;
    bool done = false;
    while (!done)
    {
        position = notlongfilepath.find_last_of(L'\\');
        if (position == std::wstring::npos ||
            position <= 2 )
        {
            done = true;
        }
        else
        {
            notlongfilepath = notlongfilepath.substr(0, position);
            // Skip recreating the folders below WritablePackageRoot folder (must create WritablePackageRoot to be sure).
            if (notlongfilepath.length() < g_writablePackageRootPath.wstring().length())
            {
                if (g_writablePackageRootPath.wstring().find(notlongfilepath) == std::wstring::npos)
                {
                    folderlist.push_back(notlongfilepath);
                }
            }
            else
            {
                folderlist.push_back(notlongfilepath);
            }
        }
    }
    BOOL bDebug;
    for (auto partial = folderlist.rbegin(); partial != folderlist.rend(); partial++)
    {        
        bDebug = ::CreateDirectoryW(MakeLongPath(*partial).c_str(), NULL);
        // Note: Name Collision is expected to occur often here, it just means that it already existed
        if (bDebug != 0)
        {
#if _DEBUG
            Log(L"[%d] %s pre-create folder '%s'", dllInstance, DebugMessage.c_str(), (*partial).c_str());
#endif
        }
        
    }
} // PreCreateFolders()

BOOL Cow(std::wstring from, std::wstring to, [[maybe_unused]] int dllInstance, [[maybe_unused]] std::wstring DebugString)
{
    switch (MFRConfiguration.COW)
    {
    case (DWORD)mfr::mfr_COW_types::COWdisableAll:
        // This means disable all COW
        return false;
    case (DWORD)mfr::mfr_COW_types::COWenablePe:
        // This means disable the Pe exclusions
       break;
    case (DWORD)mfr::mfr_COW_types::COWdefault:
    default:
        if (from.find(L".exe") != std::wstring::npos)
        {
            if (from.find(L"NONONO") != std::wstring::npos)
            {
                return FALSE;
            }
        }
        if (from.length() > 4)
        {
            std::wstring ext = from.substr(from.length() - 4);
            if (std::equal(ext.begin(), ext.end(), L".exe", psf::path_compare{}))
                return false;
            if (std::equal(ext.begin(), ext.end(), L".dll", psf::path_compare{}))
                return false;
            if (std::equal(ext.begin(), ext.end(), L".com", psf::path_compare{}))
                return false;
            if (std::equal(ext.begin(), ext.end(), L".tlb", psf::path_compare{}))
  
                if (std::equal(ext.begin(), ext.end(), L".ocx", psf::path_compare{}))
                return false;
        }
        break;
    }
    // We may need to pre-create mising directories for the destination in the redirection area
    PreCreateFolders(to, dllInstance, DebugString);

    std::wstring RdlTo = MakeLongPath(to);
    std::wstring RdlFrom = MakeLongPath(from);
    DWORD AFrom = ::GetFileAttributes(RdlFrom.c_str());
    if (AFrom != INVALID_FILE_ATTRIBUTES)
    {
        if ((AFrom & FILE_ATTRIBUTE_DIRECTORY) != 0)
        {
#if _DEBUG
            Log(L"[%d] %s COW folder '%s' just create '%s'", dllInstance, DebugString.c_str(), RdlFrom.c_str(), RdlTo.c_str());
#endif
            BOOL bRet = ::CreateDirectoryW(RdlTo.c_str(), NULL);
#if _DEBUG
            if (bRet == 0)
            {
                DWORD eCode = GetLastError();
                Log(L"[%d] %s COW CreateDirectory failed, error=0x%d", dllInstance, DebugString.c_str(), eCode);
            }
#endif
            return bRet;
        }
        else
        {
#if _DEBUG
            Log(L"[%d] %s COW file '%s' to '%s'", dllInstance, DebugString.c_str(), RdlFrom.c_str(), RdlTo.c_str());
#endif
            BOOL bRet = ::CopyFileW(RdlFrom.c_str(), RdlTo.c_str(), true);
#if _DEBUG
            if (bRet == 0)
            {
                DWORD eCode = GetLastError();
                Log(L"[%d] %s COW failed, error=0x%d", dllInstance, DebugString.c_str(), eCode);
            }
#endif
            return bRet;
        }
    }
    else
    {
#if _DEBUG
        Log(L"[%d] %s COW missing '%s' to '%s'", dllInstance, DebugString.c_str(), RdlFrom.c_str(), RdlTo.c_str());
#endif
        BOOL bRet = ::CopyFileW(MakeLongPath(from).c_str(), MakeLongPath(to).c_str(), true);
#if _DEBUG
        if (bRet == 0)
        {
            DWORD eCode = GetLastError();
            Log(L"[%d] %s COW failed, error=0x%d", dllInstance, DebugString.c_str(), eCode);
        }
#endif
        return bRet;
    }

} // COW()


// The CreateFile family of APIs can open files/folders in many ways for different purposes.
// This function looks at the common arguments to those APIs to determine if the request could be making
// a change in the file system based on this parameters.  This would potentially trigger a COW event.
bool IsCreateForChange(DWORD desiredAccess, DWORD creationDisposition, DWORD flagsAndAttributes)
{
    if ((flagsAndAttributes & FILE_FLAG_BACKUP_SEMANTICS) != 0)
    {
        // This is used for opening a directory, but the app would have to use CreateDirectory to actually create one.
        // So in spite of desiredAccess/creationDisposition settings, we will not trigger a COW.
        return false;
    }
    // Check desiredAccess versus Generic Access Rights (ACCESS_MASK) values
    if ((desiredAccess & (GENERIC_WRITE | GENERIC_ALL | MAXIMUM_ALLOWED | DELETE | WRITE_DAC | WRITE_OWNER)) != 0)
    {
        return true;
    }
    // Check desiredAccess versus File Security and Access Rights values
    if ((desiredAccess & (FILE_GENERIC_WRITE)) != 0)
    {
        return true;
    }
    // Check desiredAccess versus File Access Rights values
    if ((desiredAccess & (FILE_ADD_FILE | FILE_ADD_SUBDIRECTORY | FILE_ALL_ACCESS | FILE_APPEND_DATA | FILE_DELETE_CHILD | FILE_WRITE_ATTRIBUTES | FILE_WRITE_DATA | FILE_WRITE_EA | STANDARD_RIGHTS_WRITE)) != 0)
    {
        return true;
    }
    // Check creationdisposition on request
    if ((creationDisposition & (CREATE_ALWAYS | CREATE_NEW | TRUNCATE_EXISTING)) != 0)
    {
        return true;
    }
    // Check dwFlagsAndAttributes
    if ((flagsAndAttributes & (FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE)) != 0)
    {
        return true;
    }
    return false;
}


std::filesystem::path ConvertPathToShortPath(std::filesystem::path inputPath)
{
    std::filesystem::path outputPath = inputPath;
    DWORD dRet = GetShortPathNameW(inputPath.wstring().c_str(), NULL, 0);
    if (dRet != 0)
    {
        wchar_t* buffer = new wchar_t[dRet];
        dRet = GetShortPathNameW(inputPath.wstring().c_str(), buffer, dRet);
        if (dRet != 0)
        {
            outputPath = buffer;
        }
        delete [] buffer;
    }
    return outputPath;
}
