
#ifndef WIN32
    #error "OS not currently supported."
#endif

#include "exif.h"

#include <errhandlingapi.h>
#include <fileapi.h>
#include <handleapi.h>

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


int main(int argc, char *argv[])
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
        printf("Failed to change file attributes [%d].\n", errorCode);
        return -2;
    }

    HandleCloser const handleCloser(fileHandle);

    LARGE_INTEGER fileSize{ .QuadPart = -1 };
    BOOL const success = GetFileSizeEx(/*hFile*/ fileHandle, /*lpFileSize*/ &fileSize);
    if (!success)
    {
        DWORD const errorCode = GetLastError();
        printf("Failed to query file size [%d].\n", errorCode);
        return -3;
    }
    else if (0 < fileSize.HighPart)
    {
        printf("File too big to read [%d].\n", fileSize.QuadPart);
        return -4;
    }

    std::vector<unsigned char> buf(fileSize.QuadPart);


    // ReadFile(fileHandle, buff, sizeof(buff), &dwBytesRead, NULL)
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
        printf("Failed to read file [%d].\n", errorCode);
        return -5;
    }
    else if (fileSize.LowPart != numberOfBytesRead)
    {
        printf("Failed to read complete file [%d/%d].\n", numberOfBytesRead, fileSize.QuadPart);
        return -6;
    }


    // Parse EXIF
    easyexif::EXIFInfo result;
    int const code = result.parseFrom(buf.data(), buf.size());
    if (0 != code)
    {
      printf("Error parsing EXIF: code %d\n", code);
      return -7;
    }

    // Dump EXIF information
    printf("Original date/time   : %s\n", result.DateTimeOriginal.c_str());



    // ReadFile(fileHandle, buff, sizeof(buff), &dwBytesRead, NULL)

    // BOOL SetFileTime(
    //     [in]           HANDLE         hFile,
    //     [in, optional] const FILETIME *lpCreationTime,
    //     [in, optional] const FILETIME *lpLastAccessTime,
    //     [in, optional] const FILETIME *lpLastWriteTime
    //     );

    // last_write_time( const std::filesystem::path& p,
    //                      std::filesystem::file_time_type new_time );

    CloseHandle(fileHandle);

    return 0;
}
