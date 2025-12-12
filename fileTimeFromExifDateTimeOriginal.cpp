
#ifndef WIN32
    #error "OS not currently supported."
#endif

#include "exif.h"

#include <windows.h>

#include <stdexcept>
#include <stdio.h>
#include <vector>


struct HandleCloser
{
    HandleCloser(HANDLE const fp)
        : handle(fp)
    {
        // intentionally empty
    }

    ~HandleCloser()
    {
        if (handle)
        {
            CloseHandle(handle);
        }
    }

    void close()
    {
        if (handle)
        {
            CloseHandle(handle);
            handle = nullptr;
        }
    }

private:

    HANDLE handle;
};

std::string wstringToString(std::wstring const & input)
{
    std::string text;

    int const successSize = WideCharToMultiByte(
        /*CodePage*/ CP_UTF8,
        /*dwFlags*/ WC_ERR_INVALID_CHARS,
        /*lpWideCharStr*/ input.data(),
        /*cchWideChar*/ -1, // until first '\0'
        /*lpMultiByteStr*/ nullptr,
        /*cbMultiByte*/ 0,
        /*lpDefaultChar*/ nullptr,
        /*lpUsedDefaultChar*/ nullptr
        );

    if (0 == successSize)
    {
        throw std::runtime_error("WideCharToMultiByte() failed 1.");
    }

    text.resize(successSize);

    int const successFilled = WideCharToMultiByte(
        /*CodePage*/ CP_UTF8,
        /*dwFlags*/ WC_ERR_INVALID_CHARS,
        /*lpWideCharStr*/ input.data(),
        /*cchWideChar*/ -1, // until first '\0'
        /*lpMultiByteStr*/ text.data(),
        /*cbMultiByte*/ successSize,
        /*lpDefaultChar*/ nullptr,
        /*lpUsedDefaultChar*/ nullptr
        );
    if (0 == successFilled)
    {
        throw std::runtime_error("WideCharToMultiByte() failed 2.");
    }

    return text;
}

void potentiallyRemoveTrailingNull(std::string & text)
{
    // Remove trailing '\0' - std::string takes care of that.
    if (0 < text.size() && '\0' == text.back())
    {
        text.resize(text.size() - 1);
    }
}

std::string filetimeToString(FILETIME const & fileTime)
{
    SYSTEMTIME systemTime;
    {
        BOOL const success = FileTimeToSystemTime(
            /*lpFileTime*/ &fileTime,
            /*lpSystemTime*/ &systemTime
            );

        if (!success)
        {
            throw std::runtime_error("filetimeToString() failed.");
        }
    }

    std::wstring date;
    {
        int const numberOfBytesRequired = GetDateFormatEx(
            /*lpLocaleName*/ LOCALE_NAME_USER_DEFAULT,
            /*dwFlags*/ DATE_SHORTDATE,
            /*lpDate*/ &systemTime,
            /*lpFormat*/ NULL,
            /*lpDateStr*/ nullptr,
            /*cchDate*/ 0,
            /*lpCalendar*/ NULL
            );
        if (0 == numberOfBytesRequired)
        {
            throw std::runtime_error("filetimeToString() failed GetDateFormatEx() 1.");
        }
        date.resize(numberOfBytesRequired);
        int const numberOfBytesFilled = GetDateFormatEx(
            /*lpLocaleName*/ LOCALE_NAME_USER_DEFAULT,
            /*dwFlags*/ DATE_SHORTDATE,
            /*lpDate*/ &systemTime,
            /*lpFormat*/ NULL,
            /*lpDateStr*/ date.data(),
            /*cchDate*/ numberOfBytesRequired,
            /*lpCalendar*/ NULL
            );
        if (0 == numberOfBytesFilled)
        {
            throw std::runtime_error("filetimeToString() failed GetDateFormatEx() 2.");
        }
        else if (numberOfBytesFilled != numberOfBytesRequired)
        {
            throw std::runtime_error("filetimeToString() failed GetDateFormatEx() 3.");
        }
    }

    std::wstring time;
    {
        int const numberOfBytesRequired = GetTimeFormatEx(
            /*lpLocaleName*/ LOCALE_NAME_USER_DEFAULT,
            /*dwFlags*/ 0,
            /*lpTime*/ &systemTime,
            /*lpFormat*/ NULL,
            /*lpTimeStr*/ nullptr,
            /*cchTime*/ 0
            );
        if (0 == numberOfBytesRequired)
        {
            throw std::runtime_error("filetimeToString() failed GetTimeFormatEx() 1.");
        }
        time.resize(numberOfBytesRequired);
        int const numberOfBytesFilled = GetTimeFormatEx(
            /*lpLocaleName*/ LOCALE_NAME_USER_DEFAULT,
            /*dwFlags*/ 0,
            /*lpTime*/ &systemTime,
            /*lpFormat*/ NULL,
            /*lpTimeStr*/ time.data(),
            /*cchTime*/ numberOfBytesRequired
            );
        if (0 == numberOfBytesFilled)
        {
            throw std::runtime_error("filetimeToString() failed GetTimeFormatEx() 2.");
        }
        else if (numberOfBytesFilled != numberOfBytesRequired)
        {
            throw std::runtime_error("filetimeToString() failed GetTimeFormatEx() 3.");
        }
    }

    std::string dateString = wstringToString(date);
    std::string timeString = wstringToString(time);

    potentiallyRemoveTrailingNull(dateString);
    potentiallyRemoveTrailingNull(timeString);

    std::string const together = dateString + " " + timeString;

    return together;
}


FILETIME stringToFileTime(std::string const & timeString)
{
    // Assumes YYYY:MM:DD HH:MM:SS as per https://www.imagekit.com/IK8Help/source/controlreference/imagekitcontrol/file/exif/propertydatetimeoriginal.htm .
    char const separator = ':';
    if (19 != timeString.size() ||
        separator != timeString[4] ||
        separator != timeString[7] ||
        ' ' != timeString[10] ||
        separator != timeString[13] ||
        separator != timeString[16])
    {
        throw std::runtime_error("stringToFileTime() not of format \"YYYY:MM:DD HH:MM:SS\".");
    }

    WORD const year = std::stoul(timeString.substr(0, 4));
    WORD const month = std::stoul(timeString.substr(5, 2));
    WORD const day = std::stoul(timeString.substr(8, 2));
    WORD const hour = std::stoul(timeString.substr(11, 2));
    WORD const minute = std::stoul(timeString.substr(14, 2));
    WORD const second = std::stoul(timeString.substr(17, 2));

    SYSTEMTIME systemTime{
        /*wYear*/ static_cast<WORD>(year),
        /*wMonth*/ month,
        /*wDayOfWeek*/ 0,  // ignored in SystemTimeToFileTime()
        /*wDay*/ day,
        /*wHour*/ hour,
        /*wMinute*/ minute,
        /*wSecond*/ second,
        /*wMilliseconds*/ 0
    };

    FILETIME fileTime{0, 0};
    {
        BOOL const success = SystemTimeToFileTime(
            /*lpSystemTime*/ &systemTime,
            /*lpFileTime*/ &fileTime
            );
        if (!success)
        {
            // DWORD const errorCode = GetLastError();
            // printf("SystemTimeToFileTime() failed [%lu].\n", errorCode);
            throw std::runtime_error("SystemTimeToFileTime() failed.");
        }
    }

    return fileTime;
}

int main(int argc, char *argv[])
{
    try
    {
        if (argc != 2)
        {
            printf("Usage: %s <JPEG file>\n", argv[0]);
            return -1;
        }

        char const * const filePath = argv[1];

        // Read the JPEG file into a buffer


        HANDLE const fileHandle = CreateFileA(/*lpFileName*/ filePath,
                                              /*dwDesiredAccess*/ FILE_READ_DATA | FILE_WRITE_ATTRIBUTES,
                                              /*dwShareMode*/ 0,
                                              /*lpSecurityAttributes*/ nullptr,
                                              /*dwCreationDisposition*/ OPEN_EXISTING,
                                              /*dwFlagsAndAttributes*/ FILE_ATTRIBUTE_NORMAL,
                                              /*hTemplateFile*/ NULL
                                              );

        if (INVALID_HANDLE_VALUE == fileHandle)
        {
            DWORD const errorCode = GetLastError();
            printf("Failed to change file attributes [%lu].\n", errorCode);
            return -2;
        }

        HandleCloser const handleCloser(fileHandle);

        LARGE_INTEGER fileSize{ .QuadPart = -1 };
        {
            BOOL const success = GetFileSizeEx(/*hFile*/ fileHandle, /*lpFileSize*/ &fileSize);
            if (!success)
            {
                DWORD const errorCode = GetLastError();
                printf("Failed to query file size [%lu].\n", errorCode);
                return -3;
            }
            else if (0 < fileSize.HighPart)
            {
                printf("File too big to read [%d].\n", fileSize.QuadPart);
                return -4;
            }
        }

        std::vector<unsigned char> buf(fileSize.QuadPart);
        {
            DWORD numberOfBytesRead = -1;
            BOOL const sucess = ReadFile(
                /*hFile*/ fileHandle,
                /*lpBuffer*/ buf.data(),
                /*nNumberOfBytesToRead*/ fileSize.LowPart,
                /*lpNumberOfBytesRead*/ &numberOfBytesRead,
                /*lpOverlapped*/ nullptr
                );

            if (!sucess)
            {
                DWORD const errorCode = GetLastError();
                printf("Failed to read file [%lu].\n", errorCode);
                return -5;
            }
            else if (fileSize.LowPart != numberOfBytesRead)
            {
                printf("Failed to read complete file [%d/%d].\n", numberOfBytesRead, fileSize.QuadPart);
                return -6;
            }
        }


        // Parse EXIF
        easyexif::EXIFInfo result;
        int const code = result.parseFrom(buf.data(), buf.size());
        if (0 != code)
        {
          printf("Error parsing EXIF: code %d\n", code);
          return -7;
        }

        // // Dump EXIF information
        // printf("Original date/time   : %s\n", result.DateTimeOriginal.c_str());
        FILETIME const originalTime = stringToFileTime(result.DateTimeOriginal);

        FILETIME creationTime{0, 0};
        FILETIME lastAccessTime{0, 0};
        FILETIME lastWriteTime{0, 0};
        {
            BOOL const success = GetFileTime(
                /*hFile*/ fileHandle,
                /*lpCreationTime*/ &creationTime,
                /*lpLastAccessTime*/ &lastAccessTime,
                /*lpLastWriteTime*/ &lastWriteTime
                );

            if (!success)
            {
                DWORD const errorCode = GetLastError();
                printf("Error querying file timestamps [%lu].\n", errorCode);
                return -7;
            }
            // else
            // {
            //     printf("creationTime   : %s\n", );
            //     printf("lastAccessTime   : %s\n", );
            //     printf("lastWriteTime   : %s\n", filetimeToString(lastWriteTime).c_str());
            // }
        }


        BOOL const success = SetFileTime(
            /*hFile*/ fileHandle,
            /*lpCreationTime*/ &originalTime,
            /*lpLastAccessTime*/ nullptr, // Do not modify this, it will be overwritten every one opens it in a viewer anyway.
            /*lpLastWriteTime*/ &originalTime
            );

        if (success)
        {
            printf("\"%s\" creation and modification time changed from \"%s\" and \"%s\" to \"%s\".\n",
                   filePath,
                   filetimeToString(creationTime).c_str(),
                   filetimeToString(lastWriteTime).c_str(),
                   filetimeToString(originalTime).c_str());
        }
        else
        {
            DWORD const errorCode = GetLastError();
            printf("Failed to replace creation and modification time for \"%s\" [%lu].\n",
                   filePath,
                   errorCode);
        }
    }
    catch (std::exception const & e)
    {
        printf("Exception: %s\n", e.what());
        return -100;
    }
    catch (...)
    {
        printf("Exception: unknown.\n");
        return -101;
    }

    return 0;
}
