/*
 * Rainbow Crackalack: crackalack_fpga_gen.cpp
 * Copyright (C) 2018-2019  Joe Testa <jtesta@positronsecurity.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms version 3 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

/*
 * For quick testing, here are the sha256 hashes of the first 16,384 bytes of the
 * tables with indices 0 and 659, respectively:
 *   #0:   9d6d6893d7b107477de7db828472cbe48f2780c42dba918aa6bdea796523a522
 *   #659: 62a42e8de712ad84cdfe1ef50908e1f77b92faa18973c9eb65201ad55f618d11
 */

#ifdef NOT_AWS_FPGA
#define CL_HPP_TARGET_OPENCL_VERSION 120
#define CL_HPP_MINIMUM_OPENCL_VERSION 120
#include <CL/cl2.hpp>
#else
#include "xcl2.hpp"
#endif

#define FPGA_NTLM8_KERNEL "crackalack_fpga_ntlm8.cl"
#define FPGA_NTLM8_KERNEL_ENTRY "crackalack_fpga_ntlm8"

#define FCLOSE(_f) { fclose(_f); _f = NULL; }
#define FREE(_ptr) { free(_ptr); _ptr = NULL; }
#define CL_CHECK(_err, _code) _code; if (_err != CL_SUCCESS) { cerr << __FILE__ << ":" << __LINE__ << ": Error calling " << #_code << "; error code: " << _err << endl << flush; exit(-1); }

#define NUM_CHAINS_TOTAL 67108864

// The interval, in seconds, to update the user on the rate and progress.
#define STATUS_UPDATE_INTERVAL 60 


#include <fstream>
#include <iostream>
#include <sstream>
#include <vector>
#include <unistd.h>
#include <sys/types.h>
#include <time.h>
#include <inttypes.h>


using namespace std;


double get_elapsed(struct timespec *start) {
  double ret = 0.0;
  struct timespec now = {0};

  clock_gettime(CLOCK_MONOTONIC, &now);

  ret = now.tv_sec - start->tv_sec;
  long nsec_diff = now.tv_nsec - start->tv_nsec;
  if (nsec_diff < 0) {
    ret = ret - 1.0;
    ret += ((nsec_diff + 1000000000) / 1000000000.0);
  } else
    ret += (nsec_diff / 1000000000.0);

  return ret;
}


/* Converts number of seconds into human-readable time, such as "X mins, Y secs". */
string seconds_to_human_time(double seconds) {
#define ONE_MINUTE (60)
#define ONE_HOUR (ONE_MINUTE * 60)
#define ONE_DAY (ONE_HOUR * 24)

  char buf[64] = {0};
  unsigned int buf_size = sizeof(buf);
  unsigned int seconds_int = (unsigned int)seconds;
  if (seconds < ONE_MINUTE)
    snprintf(buf, buf_size - 1, "%.1f seconds", seconds);
  else if ((seconds >= ONE_MINUTE) && (seconds < ONE_HOUR))
    snprintf(buf, buf_size - 1, "%u minutes, %u seconds", seconds_int / ONE_MINUTE, seconds_int % ONE_MINUTE);
  else if ((seconds >= ONE_HOUR) && (seconds < ONE_DAY))
    snprintf(buf, buf_size - 1, "%u hours, %u minutes", (unsigned int)(seconds_int / ONE_HOUR), (unsigned int)((seconds_int % ONE_HOUR) / ONE_MINUTE));
  else if (seconds >= ONE_DAY)
    snprintf(buf, buf_size - 1, "%u days, %u hours", (unsigned int)(seconds_int / ONE_DAY), (unsigned int)((seconds_int % ONE_DAY) / ONE_HOUR));

  return (string)buf;
}


int main(int ac, char **av) {

  if ((ac != 3) && (ac != 4)) {
    cerr << "Usage:  " << av[0] << " output_dir table_number [gws]" << endl;
    return -1;
  }

  char *output_dir = av[1];
  unsigned int part_index = atoi(av[2]);

  unsigned int gws = 16384;
  if (ac == 4) {
    gws = atoi(av[3]);
    if (gws == 0) {
      cerr << "Error: GWS must be greater than zero." << endl;
      return -1;
    }
    cout << "Using user-supplied GWS of " << gws << "." << endl;
  } else
    cout << "Using default GWS of " << gws << "." << endl;

  string output_filename = output_dir;
  output_filename += "/ntlm_ascii-32-95#8-8_0_422000x67108864_";
  output_filename += to_string(part_index);
  output_filename += ".rt";
  FILE *f = fopen(output_filename.c_str(), "w+");
  if (f == NULL) {
    cerr << "Failed to open file for writing: " << strerror(errno) << endl;
    return -1;
  }


  vector<cl::Platform> platforms;
  cl::Platform::get(&platforms);
  if (platforms.size() == 0) {
    cerr << "Failed to load platforms!" << endl;
    return -1;
  }
  cout << "Number of platforms: " << platforms.size() << endl;

#ifdef NOT_AWS_FPGA

  // On non-FPGA builds, the GWS is fixed at 1024 to help with debugging.
  gws = 1024;
  cout << endl << "Note: GWS is reset to 1024, as this is a non-FPGA build." << endl << endl << flush;
  
  vector<cl::Device> devices;
  platforms[0].getDevices(CL_DEVICE_TYPE_ALL, &devices);
  if (devices.size() == 0) {
    cerr << "Failed to get devices!" << endl;
    return -1;
  }
  cout << "Number of devices: " << devices.size() << endl;
#else /* On AWS FPGA... */
  std::vector<cl::Device> devices = xcl::get_xil_devices();
#endif
  cl_int err = 0;

  cl::Device device = devices[0];
  CL_CHECK(err, cl::Context context(device, NULL, NULL, NULL, &err));
  CL_CHECK(err, cl::CommandQueue queue(context, device, CL_QUEUE_PROFILING_ENABLE, &err));
  CL_CHECK(err, std::string device_name = device.getInfo<CL_DEVICE_NAME>(&err));
  cout << "Device name: " << device_name.c_str() << endl;


#ifdef NOT_AWS_FPGA
  ifstream file;
  file.open(FPGA_NTLM8_KERNEL);
  if (!file) {
    cerr << "Failed to open file containing kernel: " << FPGA_NTLM8_KERNEL << endl;
    return -1;
  }

  stringstream fileBuffer;
  fileBuffer << file.rdbuf();
  file.close();

  cl::Program::Sources sources;
  sources.push_back({fileBuffer.str().c_str(), fileBuffer.str().length()});
  cl::Program program(context, sources);
  if (program.build({device}) != CL_SUCCESS) {
    cerr << "Failed to build kernel!" << endl << endl << program.getBuildInfo<CL_PROGRAM_BUILD_LOG>(device) << endl;
    return -1;
  }

  CL_CHECK(err, cl::Kernel kernel(program, FPGA_NTLM8_KERNEL_ENTRY, &err));
#else /* On AWS FPGA... */

  std::string binaryFile = xcl::find_binary_file(device_name,"crackalack_fpga_ntlm8");
  cl::Program::Binaries bins = xcl::import_binary_file(binaryFile);
  devices.resize(1);

  vector<cl_int> binStatus;
  CL_CHECK(err, cl::Program program(context, devices, bins, &binStatus, &err));
  CL_CHECK(err, cl::Kernel kernel(program, FPGA_NTLM8_KERNEL_ENTRY, &err));
  cout << "Return value from Kernel constructor: " << err << endl << flush;
#endif

  cl_ulong start_indices[gws] = {0}, end_indices[gws] = {0};


  CL_CHECK(err, cl::Buffer startIndicesBuffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_READ_ONLY, sizeof(cl_ulong) * gws, start_indices, &err));
  CL_CHECK(err, cl::Buffer endIndicesBuffer(context, CL_MEM_USE_HOST_PTR | CL_MEM_WRITE_ONLY, sizeof(cl_ulong) * gws, end_indices, &err));

  std::vector<cl::Memory> readBufferVector, writeBufferVector;
  readBufferVector.push_back(startIndicesBuffer);
  writeBufferVector.push_back(endIndicesBuffer);


  cl_ulong *buf = (cl_ulong *)malloc(gws * sizeof(cl_ulong) * 2);
  if (buf == NULL) {
    cerr << "Error while creating buffer for file output!" << endl << flush;
    return -1;
  }

  cl_ulong start = (cl_ulong)part_index * (cl_ulong)NUM_CHAINS_TOTAL;

  // Calculate the number of kernel invokations we need to make.
  unsigned int num_blocks = NUM_CHAINS_TOTAL / gws;
  if ((NUM_CHAINS_TOTAL % gws) != 0)
    num_blocks++;

  struct timespec start_time = {0}, last_update_time = {0};
  clock_gettime(CLOCK_MONOTONIC, &start_time);
  clock_gettime(CLOCK_MONOTONIC, &last_update_time);

  cout << "Starting chain generation..." << endl << flush;
  unsigned int num_chains_generated = 0;
  for (unsigned int block = 0; block < num_blocks; block++) {
    for (unsigned int i = 0; i < gws; i++)
      start_indices[i] = start++;


    CL_CHECK(err, err = queue.enqueueMigrateMemObjects(readBufferVector, 0));

    CL_CHECK(err, err = kernel.setArg(0, startIndicesBuffer));
    CL_CHECK(err, err = kernel.setArg(1, endIndicesBuffer));

    CL_CHECK(err, err = queue.enqueueNDRangeKernel(kernel, cl::NullRange, cl::NDRange(gws)));

    CL_CHECK(err, err = queue.enqueueMigrateMemObjects(writeBufferVector, CL_MIGRATE_MEM_OBJECT_HOST));

    CL_CHECK(err, err = queue.finish());

    num_chains_generated += gws;


    double last_update_diff = get_elapsed(&last_update_time);
    if (last_update_diff >= 60.0) {
      double total_elapsed_time = get_elapsed(&start_time);
      unsigned int rate = (unsigned int)(num_chains_generated / total_elapsed_time);
      double estimated_seconds_remaining = (NUM_CHAINS_TOTAL - num_chains_generated) / (double)rate;
      cout << "Run time: " << seconds_to_human_time(total_elapsed_time) << "; Chains generated: " << num_chains_generated << "; Rate: " << rate << "/s" << endl << "Estimated time remaining: " << seconds_to_human_time(estimated_seconds_remaining) << endl;
      clock_gettime(CLOCK_MONOTONIC, &last_update_time);
    }

    // Make a single block of memory that contains the start & end indices, that way
    // we can write this all at once.
    for (unsigned int i = 0; i < gws; i++) {
      buf[(i * 2)] = start_indices[i];
      buf[(i * 2) + 1] = end_indices[i];
      //cout << start_indices[i] << " -> " << end_indices[i] << endl;
    }
    fwrite(buf, sizeof(cl_ulong), gws * 2, f);

// On non-FPGA builds, only generate the first 1K chains for testing.
#ifdef NOT_AWS_FPGA
    if (num_chains_generated >= 1024) {
      if (ftruncate(fileno(f), 1024 * 16) != 0)
	cerr << "Failed to truncate. :(" << endl << flush;

      FCLOSE(f);

      cout << "Non-FPGA build terminating after 1K chains generated." << endl << flush;
      if (part_index == 0)
	cout << endl << "The SHA256 checksum of " << output_filename << " should be:" << endl << "  9d6d6893d7b107477de7db828472cbe48f2780c42dba918aa6bdea796523a522" << endl << flush;
      else if (part_index == 652)
	cout << endl << "The SHA256 checksum of " << output_filename << " should be:" << endl << "  62a42e8de712ad84cdfe1ef50908e1f77b92faa18973c9eb65201ad55f618d11" << endl << flush;
      else
	cout << endl << "Note: the SHA256 sums of table parts 0 and 652, respectively, are:" << endl << "  9d6d6893d7b107477de7db828472cbe48f2780c42dba918aa6bdea796523a522" << endl << "  62a42e8de712ad84cdfe1ef50908e1f77b92faa18973c9eb65201ad55f618d11" << endl << endl << "Try re-generating using these part numbers, and using the hashes to verify correctness." << endl << endl << flush;
      return 0;
    }
#endif
  }

  // If the file is larger than it should be, then truncate it.  Or, if it is
  // shorter than it should be, this is an error.
  bool success = true;
  long file_pos = ftell(f);
  if (file_pos > NUM_CHAINS_TOTAL * 16) {
    if (ftruncate(fileno(f), NUM_CHAINS_TOTAL * 16) != 0) {
      cerr << "Error while truncating file!: " << strerror(errno) << endl;
      success = false;
    } else
      cout << "File is " << file_pos << " bytes.  Truncated to " << NUM_CHAINS_TOTAL * 16 << "." << endl;
  } else if (file_pos < NUM_CHAINS_TOTAL * 16) {
    cerr << endl << endl << "!! ERROR !!" << endl << endl << "File size is short!  Table generation FAILED." << endl << endl << flush;
    success = false;
  }

  FCLOSE(f);
  FREE(buf);

  if (success) {
    cout << endl << "Done!  Table generation successfully completed." << endl << endl << flush;
    return 0;
  } else
    return -1;
}
