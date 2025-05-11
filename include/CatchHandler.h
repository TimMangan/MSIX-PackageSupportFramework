#pragma once

#define LOGGED_CATCHHANDLER(instance, name)\
catch (const std::exception& ex) \
{\
    Log(L"[%d] %s Exception=0x%x", instance, name, GetLastError());\
    std::wstring _wEx = widen(ex.what());\
    Log(L"[%d] %s Msg=%s", instance, name, _wEx.c_str());\
}\
catch (const std::string& ex) \
{\
Log(L"[%d] %s Exception=0x%x", instance, name, GetLastError());\
std::wstring _wEx = widen(ex);\
Log(L"[%d] %s Msg=%s", instance, name, _wEx.c_str());\
}\
catch (...) \
{\
    Log(L"[%d] %s Exception=0x%x", instance, name, GetLastError());\
}


#define LOGGED_CATCHHANDLER_ReturnError(instance, name)\
catch (const std::exception& ex) \
{\
    int _err = win32_from_caught_exception(); \
    Log(L"[%d] %s Exception=0x%x", instance, name, _err);\
    std::wstring _wEx = widen(ex.what());\
    Log(L"[%d] %s Msg=%s", instance, name, _wEx.c_str());\
    return _err;\
}\
catch (const std::string& ex) \
{\
    int _err = win32_from_caught_exception(); \
    Log(L"[%d] %s Exception=0x%x", instance, name, _err); \
    std::wstring _wEx = widen(ex); \
    Log(L"[%d] %s Msg=%s", instance, name, _wEx.c_str()); \
    return _err;\
}\
catch (...) \
{\
    int _err = win32_from_caught_exception(); \
    Log(L"[%d] %s Exception=0x%x", instance, name, _err); \
    return _err;\
}


// MODNAME


#define LOGGED_CATCHHANDLER_MIN(modName, instance, name)\
catch (const std::exception& ex) \
{\
    Log(L"[%s%d] %s Exception=0x%x", modName, instance, name, GetLastError());\
    std::wstring _wEx = widen(ex.what());\
    Log(L"[%s%d] %s Msg=%s", modName, instance, name, _wEx.c_str());\
}\
catch (const std::string& ex) \
{\
Log(L"[%s%d] %s Exception=0x%x", modName, instance, name, GetLastError());\
std::wstring _wEx = widen(ex);\
Log(L"[%s%d] %s Msg=%s", modName, instance, name, _wEx.c_str());\
}\
catch (...) \
{\
    Log(L"[%s%d] %s Exception=0x%x", modName, instance, name, GetLastError());\
}


#define LOGGED_CATCHHANDLER_MIN_ReturnError(modName, instance, name)\
catch (const std::exception& ex) \
{\
    int _err = win32_from_caught_exception(); \
    Log(L"[%s%d] %s Exception=0x%x", modName, instance, name, _err);\
    std::wstring _wEx = widen(ex.what());\
    Log(L"[%s%d] %s Msg=%s", modName, instance, name, _wEx.c_str());\
    return _err;\
}\
catch (const std::string& ex) \
{\
    int _err = win32_from_caught_exception(); \
    Log(L"[%s%d] %s Exception=0x%x", modName, instance, name, _err); \
    std::wstring _wEx = widen(ex); \
    Log(L"[%s%d] %s Msg=%s", modName, instance, name, _wEx.c_str()); \
    return _err;\
}\
catch (...) \
{\
    int _err = win32_from_caught_exception(); \
    Log(L"[%s%d] %s Exception=0x%x", modName, instance, name, _err); \
    return _err;\
}
