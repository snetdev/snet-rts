#include <stdlib.h>
#include <stdio.h>

#if __gnu_linux__ && _GNU_SOURCE
#include <gnu/libc-version.h>
#endif

void SNetRuntimeVersion(void)
{
#ifdef PACKAGE_VERSION
  printf("S-Net version %s\n", PACKAGE_VERSION);
#endif
#ifdef PACKAGE_BUGREPORT
  printf("Contact email %s\n", PACKAGE_BUGREPORT);
#endif
#ifdef PACKAGE_URL
  printf("S-Net website %s\n", PACKAGE_URL);
#endif
#if defined(__DATE__) && defined(__TIME__)
  printf("Compiled on   %s\n", __TIME__  " " __DATE__);
#endif
#ifdef __VERSION__
  printf("Compiler version %s\n", __VERSION__);
#elif __GNUC_PATCHLEVEL__
  printf("GCC version   %d.%d.%d\n", __GNUC__, __GNUC_MINOR__, __GNUC_PATCHLEVEL__);
#endif
#ifdef _GNU_LIBC_VERSION_H
  printf("GNU libc version %s %s\n", gnu_get_libc_version(), gnu_get_libc_release());
#endif
  exit(1);
}


