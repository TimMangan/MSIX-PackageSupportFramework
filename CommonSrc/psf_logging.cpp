//-------------------------------------------------------------------------------------------------------
// Copyright (C) Tim Mangan. All rights reserved
// Copyright (C) Microsoft Corporation. All rights reserved.
// Licensed under the MIT license. See LICENSE file in the project root for full license information.
//-------------------------------------------------------------------------------------------------------
#include <windows.h>
#include <stdarg.h>
#include <debugapi.h>
#include <string>
#include <assert.h>

#include <utilities.h>
#include <psf_logging.h>
#include <psf_runtime.h>

bool g_psf_NoLogging = false;
Json_Debug_Levels g_JsonDebugLevel = LogLevel_Launching;


void Log(Json_Debug_Levels debugRequestLevel, const char* fmt, ...)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            try
            {
                int bufferSize = 1024;
                std::string str;
                int count = -1;

                while (true)
                {
                    str.resize(bufferSize);
                    va_list args;
                    va_start(args, fmt);
                    count = _vsnprintf_s(str.data(), bufferSize, _TRUNCATE, fmt, args);
                    va_end(args);

                    if (count >= 0 && count < bufferSize)
                    {
                        str.resize(count);
                        ::OutputDebugStringA(str.c_str());
                        break;
                    }
                    else if (bufferSize >= 65536)
                    {
                        ::OutputDebugStringA("Error in Log() wide string too long or format error");
                        break;
                    }
                    bufferSize *= 2;
                }
            }
            catch (...)
            {
                ::OutputDebugStringA("Exception in Log()");
                ::OutputDebugStringA(fmt);
            }
        }
    }
}

void Log(Json_Debug_Levels debugRequestLevel, const wchar_t* fmt, ...)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            try
            {
                int bufferSize = 1024;
                std::wstring wstr;
                int count = -1;

                while (true)
                {
                    wstr.resize(bufferSize);
                    va_list args;
                    va_start(args, fmt);
                    count = _vsnwprintf_s(wstr.data(), bufferSize, _TRUNCATE, fmt, args);
                    va_end(args);

                    if (count >= 0 && count < bufferSize)
                    {
                        wstr.resize(count);
                        ::OutputDebugStringW(wstr.c_str());
                        break;
                    }
                    else if (bufferSize >= 65536)
                    {
                        ::OutputDebugStringA("Error in Log() wide string too long or format error");
                        break;
                    }
                    bufferSize *= 2;
                }
            }
            catch (...)
            {
                ::OutputDebugStringA("Exception in wide Log()");
                ::OutputDebugStringW(fmt);
            }
        }
    }
}

void LogString(Json_Debug_Levels debugRequestLevel, const char* name, const char* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if ((value != NULL && value[1] != 0x0))
            {
                Log(debugRequestLevel, L"%S=%S\n", name, value);
            }
            else
            {
                Log(debugRequestLevel, L"%s=%s", name, (wchar_t*)value);
            }
        }
    }
}

void LogString(Json_Debug_Levels debugRequestLevel, const char* name, const wchar_t* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if ((value != NULL && ((char*)value)[1] == 0x0))
            {
                Log(debugRequestLevel, L"%S=%s\n", name, value);
            }
            else
            {
                Log(debugRequestLevel, L"%S=%S", name, (char*)value);
            }
        }
    }
}

void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* name, const char* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            Log(debugRequestLevel, L"%s=%S\n", name, widen(value).c_str());
        }
    }
}

void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* name, const wchar_t* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            Log(debugRequestLevel, L"%s=%s\n", name, value);
        }
    }
}

void LogCountedStringW(Json_Debug_Levels debugRequestLevel, const char* name, const wchar_t* value, std::size_t length)
{
    // Still used in PsfRuntime initialization
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if (debugRequestLevel <= g_JsonDebugLevel)
            {
                Log(debugRequestLevel, "\t%s=%.*ls\n", name, length, value);
            }
        }
    }
}

void Loghexdump(Json_Debug_Levels debugRequestLevel, void* pAddressIn, long  lSize, const wchar_t* ModuleName, DWORD instance)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            char szBuf[128];
            long lIndent = 1;
            long lOutLen, lIndex, lIndex2, lOutLen2;
            long lRelPos;
            struct { char* pData; unsigned long lSize; } buf;
            unsigned char* pTmp, ucTmp;
            unsigned char* rememberPtmp;
            unsigned char* pAddress = (unsigned char*)pAddressIn;

            buf.pData = (char*)pAddress;
            buf.lSize = lSize;

            while (buf.lSize > 0)
            {
                pTmp = (unsigned char*)buf.pData;
                lOutLen = (int)buf.lSize;
                if (lOutLen > 16)
                    lOutLen = 16;

                // create a 64-character formatted output line:
                sprintf_s(szBuf, 100, " >                            "
                    "                      "
                    "         ");
                rememberPtmp = pTmp;
                lOutLen2 = lOutLen;

                for (lIndex = 1 + lIndent, lIndex2 = 53 - 15 + lIndent, lRelPos = 0;
                    lOutLen2;
                    lOutLen2--, lIndex += 2, lIndex2++
                    )
                {
                    ucTmp = *pTmp++;

                    sprintf_s(szBuf + lIndex, 100 - lIndex, "%02X ", (unsigned short)ucTmp);
                    if (!isprint(ucTmp))  ucTmp = '.'; // nonprintable char
                    szBuf[lIndex2] = ucTmp;

                    if (!(++lRelPos & 3))     // extra blank after 4 bytes
                    {
                        lIndex++; szBuf[lIndex + 2] = ' ';
                    }
                }

                if (!(lRelPos & 3)) lIndex--;

                sprintf_s(szBuf + lIndex, 100 - lIndex, "<%08lx  ", (unsigned long)(rememberPtmp - pAddress));
                szBuf[lIndex + 14] = 0x0;

                if (ModuleName != nullptr)
                {
                    std::wstring wBuf = widen(szBuf);
                    Log(debugRequestLevel, L"    [%s%d]\t\t%s", ModuleName, instance, wBuf.c_str());
                }
                else
                {

                    if (instance == 0)
                    {
                        ::OutputDebugStringA(szBuf);
                    }
                    else
                    {
                        Log(debugRequestLevel, "    [%d]\t\t%s", instance, szBuf);
                    }
                }
                buf.pData += lOutLen;
                buf.lSize -= lOutLen;
            }
        }
    }
}

/////// WITH_INSTONLY
void LogString(Json_Debug_Levels debugRequestLevel, DWORD inst, const char* name, const char* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if ((value != NULL && value[1] != 0x0))
            {
                Log(debugRequestLevel, L"[%d] %S=%S\n", inst, name, value);
            }
            else
            {
                Log(debugRequestLevel, L"[%d] %S=%s", inst, name, (wchar_t*)value);
            }
        }
    }
}

void LogString(Json_Debug_Levels debugRequestLevel, DWORD inst, const char* name, const wchar_t* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if ((value != NULL && ((char*)value)[1] == 0x0))
            {
                Log(debugRequestLevel, L"[%d] %S=%s\n", inst, name, value);
            }
            else
            {
                Log(debugRequestLevel, L"[%d] %S=%S", inst, name, (char*)value);
            }
        }
    }
}

void LogStringAA(Json_Debug_Levels debugRequestLevel, DWORD inst, const char* name, const char* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            Log(debugRequestLevel, L"[%d] %s=%s\n", inst, name, value);
        }
    }
}
void LogStringAW(Json_Debug_Levels debugRequestLevel, DWORD inst, const char* name, const wchar_t* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            Log(debugRequestLevel, L"[%d] %s=%ls", inst, name, value);
        }
    }
}

void LogString(Json_Debug_Levels debugRequestLevel, DWORD inst, const wchar_t* name, const char* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if ((value != NULL && value[1] != 0x0))
            {
                //Log(L"[%d] %ls=%ls\n", inst, name, widen(value).c_str());
                Log(debugRequestLevel, L"[%d] %s=%S\n", inst, name, value);
            }
            else
            {
                Log(debugRequestLevel, L"[%d] %s=%s", inst, name, (wchar_t*)value);
            }
        }
    }
}

void LogString(Json_Debug_Levels debugRequestLevel, DWORD inst, const wchar_t* name, const wchar_t* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if ((value != NULL && ((char*)value)[1] == 0x0))
            {
                Log(debugRequestLevel, L"[%d] %s=%s\n", inst, name, value);
            }
            else
            {
                if (value != nullptr)
                {
                    //Log(L"[%d] %ls=%ls", inst, name, widen((const char*)value).c_str());
                    Log(debugRequestLevel, L"[%d] %s=%S", inst, name, (char*)value);
                }
                else
                {
                    Log(debugRequestLevel, L"[%d] %ls=NULL", inst, name);
                }
            }
        }
    }
}

void LogStringWA(Json_Debug_Levels debugRequestLevel, DWORD inst, const wchar_t* name, const char* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            Log(debugRequestLevel, L"[%d] %ls=%ls\n", inst, name, widen(value).c_str());
        }
    }
}
void LogStringWW(Json_Debug_Levels debugRequestLevel, DWORD inst, const wchar_t* name, const wchar_t* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            Log(debugRequestLevel, L"[%d] %ls=%ls", inst, name, value);
        }
    }
}


void LogString(Json_Debug_Levels debugRequestLevel, DWORD rememberedInst, DWORD inst, const wchar_t* name, const char* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if ((value != NULL && value[1] != 0x0))
            {
                Log(debugRequestLevel, L"[%d][F%d] %s=%s\n", rememberedInst, inst, name, widen(value).c_str());
            }
            else
            {
                Log(debugRequestLevel, L"[%d][F%d] %s=%s", rememberedInst, inst, name, (wchar_t*)value);
            }
        }
    }
}

void LogString(Json_Debug_Levels debugRequestLevel, DWORD rememberedInst, DWORD inst, const wchar_t* name, const wchar_t* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if ((value != NULL && ((char*)value)[1] == 0x0))
            {
                Log(debugRequestLevel, L"[%d][%d] %s=%s\n", rememberedInst, inst, name, value);
            }
            else
            {
                if (value != nullptr)
                {
                    Log(debugRequestLevel, L"[%d][%d] %s=%s", rememberedInst, inst, name, widen((const char*)value).c_str());
                }
                else
                {
                    Log(debugRequestLevel, L"[%d][%d] %ls=NULL", rememberedInst, inst, name);
                }
            }
        }
    }
}



///// WITH_MODULE_AND_INST
void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const char* name, const char* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if ((value != NULL && value[1] != 0x0))
            {
                Log(debugRequestLevel, L"[%s%d] %S=%S\n", moduleName, inst, name, value);
            }
            else
            {
                Log(debugRequestLevel, L"[%s%d] %S=%s", moduleName, inst, name, (wchar_t*)value);
            }
        }
    }
}

void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const char* name, const wchar_t* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if ((value != NULL && ((char*)value)[1] == 0x0))
            {
                Log(debugRequestLevel, L"[%s%d] %S=%s\n", moduleName, inst, name, value);
            }
            else
            {
                Log(debugRequestLevel, L"[%s%d] %S=%S", moduleName, inst, name, (char*)value);
            }
        }
    }
}

void LogStringAA(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const char* name, const char* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            Log(debugRequestLevel, L"[%s%d] %s=%s\n", moduleName, inst, name, value);
        }
    }
}
void LogStringAW(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const char* name, const wchar_t* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            Log(debugRequestLevel, L"[%s%d] %s=%ls", moduleName, inst, name, value);
        }
    }
}

void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const wchar_t* name, const char* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if ((value != NULL && value[1] != 0x0))
            {
                Log(debugRequestLevel, L"[%s%d] %s=%S\n", moduleName, inst, name, value);
            }
            else
            {
                Log(debugRequestLevel, L"[%s%d] %s=%s", moduleName, inst, name, (wchar_t*)value);
            }
        }
    }
}

void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const wchar_t* name, const wchar_t* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if ((value != NULL && ((char*)value)[1] == 0x0))
            {
                Log(debugRequestLevel, L"[%s%d] %s=%s\n", moduleName, inst, name, value);
            }
            else
            {
                if (value != nullptr)
                {
                    Log(debugRequestLevel, L"[%s%d] %s=%S", moduleName, inst, name, (char*)value);
                }
                else
                {
                    Log(debugRequestLevel, L"[%s%d] %ls=NULL", moduleName, inst, name);
                }
            }
        }
    }
}
void LogStringWA(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const wchar_t* name, const char* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            Log(debugRequestLevel, L"[%s%d] %ls=%ls\n", moduleName, inst, name, widen(value).c_str());
        }
    }
}
void LogStringWW(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD inst, const wchar_t* name, const wchar_t* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            Log(debugRequestLevel, L"[%s%d] %ls=%ls", moduleName, inst, name, value);
        }
    }
}


void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD rememberedInst, DWORD inst, const wchar_t* name, const char* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if ((value != NULL && value[1] != 0x0))
            {
                Log(debugRequestLevel, L"[%s%d][%s%d] %s=%s\n", moduleName, rememberedInst, moduleName, inst, name, widen(value).c_str());
            }
            else
            {
                Log(debugRequestLevel, L"[%s%d][%s%d] %s=%s", moduleName, rememberedInst, moduleName, inst, name, (wchar_t*)value);
            }
        }
    }
}

void LogString(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD rememberedInst, DWORD inst, const wchar_t* name, const wchar_t* value)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            if ((value != NULL && ((char*)value)[1] == 0x0))
            {
                Log(debugRequestLevel, L"[%s%d][%s%d] %s=%s\n", moduleName, rememberedInst, moduleName, inst, name, value);
            }
            else
            {
                if (value != nullptr)
                {
                    Log(debugRequestLevel, L"[%s%d][%s%d] %s=%s", moduleName, rememberedInst, moduleName, inst, name, widen((const char*)value).c_str());
                }
                else
                {
                    Log(debugRequestLevel, L"[%s%d][%s%d] %ls=NULL", moduleName, rememberedInst, moduleName, inst, name);
                }
            }
        }
    }
}

void LogCountedStringW(Json_Debug_Levels debugRequestLevel, const wchar_t* moduleName, DWORD dllInstance, const char* name, const wchar_t* value, std::size_t length)
{
    if (!g_psf_NoLogging)
    {
        if (debugRequestLevel <= g_JsonDebugLevel)
        {
            Log(debugRequestLevel, "[%S%d]\t%s=%.*ls\n", moduleName, dllInstance, name, length, value);
        }
    }
}




