#include "exif.h"

#include <stdio.h>
#include <vector>

struct FileCloser
{
    FileCloser(FILE *const fp)
        : fp(fp)
    {
        // intentionally empty
    }

    ~FileCloser()
    {
        if (fp)
        {
            fclose(fp);
        }
    }

private:

    FILE *fp;
};


int main(int argc, char *argv[]) {
  if (argc < 2) {
    printf("Usage: %s <JPEG file>\n", argv[0]);
    return -1;
  }

  // Read the JPEG file into a buffer
  FILE *fp = fopen(argv[1], "rb");
  if (!fp)
  {
    printf("Can't open file.\n");
    return -1;
  }
  FileCloser fileCloser(fp);

  fseek(fp, 0, SEEK_END);
  unsigned long fsize = ftell(fp);
  rewind(fp);
  std::vector<unsigned char> buf(fsize);

  size_t const numberOfObjectsReadSuccessfully = fread(buf.data(), 1, fsize, fp);
  if (numberOfObjectsReadSuccessfully != fsize)
  {
    printf("Failed to read file.\n");
    return -2;
  }

  // Parse EXIF
  easyexif::EXIFInfo result;
  int const code = result.parseFrom(buf.data(), fsize);
  if (0 != code)
  {
    printf("Error parsing EXIF: code %d\n", code);
    return -3;
  }

  // Dump EXIF information
  printf("Original date/time   : %s\n", result.DateTimeOriginal.c_str());


  return 0;
}
