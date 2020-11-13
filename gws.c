#include <string.h>

#include "opencl_setup.h"

#include "gws.h"


/* Given a GPU device, returns the optimal GWS setting (found through manual experimentation).  Returns 0 if the optimal setting on the device is unknown. */
unsigned int get_optimal_gws(cl_device_id device) {
  char vendor[128] = {0}, name[64] = {0};


  get_device_str(device, CL_DEVICE_VENDOR, vendor, sizeof(vendor) - 1);
  get_device_str(device, CL_DEVICE_NAME, name, sizeof(name) - 1);

  if (strcmp(vendor, "NVIDIA Corporation") == 0) {
    if (strcmp(name, "GeForce GTX 1080 Ti") == 0)
      return 28 * 768; /* Guess based on the 1070 & 1070 Ti's performance. */
    else if (strcmp(name, "GeForce GTX 1080") == 0)
      return 20 * 768; /* Guess based on the 1070 & 1070 Ti's performance. */
    else if (strcmp(name, "GeForce GTX 1070 Ti") == 0)
      return 19 * 768; /* NTLM 8-char: ?/s */

    else if (strcmp(name, "GeForce GTX 1070") == 0)
      return 15 * 768; /* NTLM 8-char: 3,028/s */

    else if (strcmp(name, "GeForce GTX 1660 Ti") == 0)
      return 24 * 1536; /* NTLM 8-char: 8,070/s */

    else if (strcmp(name, "GeForce RTX 2060") == 0)
#ifdef _WIN32
      return 30 * 256; /* This is a guess based on the RTX 2070's Windows performance.  The RTX 2070's optimal performance is at 36 compute units x 256 = 9216, so maybe the RTX 2060's is 30 compute units x 256 = 7680? */
#else
      return 30 * 512; /* NTLM 8-char: 5287/s */
#endif

    else if (strcmp(name, "GeForce RTX 2070") == 0)
#ifdef _WIN32
      return 36 * 256; /* NTLM 8-char: 4,683/s */
#else
      return 36 * 512; /* NTLM 8-char: 6,345/s */
#endif

    /* The RTX 2080 numbers are an educated guess based on how the RTX 2070 and 2060's numbers.  Their compute units times 512 is optimal for Linux; their compute units times 256 is optimal for Windows. */
    else if (strcmp(name, "GeForce RTX 2080") == 0)
#ifdef _WIN32
      return 46 * 256;
#else
      return 46 * 512;
#endif

    /* The RTX 2080 Ti numbers are an educated guess based on how the RTX 2070 and 2060's numbers.  Their compute units times 512 is optimal for Linux; their compute units times 256 is optimal for Windows. */
    else if (strcmp(name, "GeForce RTX 2080 Ti") == 0)
#ifdef _WIN32
      return 68 * 256;
#else
      return 68 * 512;
#endif

    else if (strcmp(name, "Tesla V100-SXM2-16GB") == 0) /* Amazon EC2 p3.2xlarge instance */
      return 80 * 512;
  }

  if (strcmp(vendor, "Advanced Micro Devices, Inc.") == 0) {
    if (strcmp(name, "gfx900") == 0) /* AMD Vega 64 */
#ifdef _WIN32
      return 64 * 256; /* NTLM 8-char: 2,560/s */
#else
      return 64 * 768; /* NTLM 8-char: 5,671/s */
#endif
  }

  return 0;
}
