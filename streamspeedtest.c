/**
 * stream-speed-test.c
 *
 * Copyright (c) 2017 TiePie engineering
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <stdbool.h>
#include <inttypes.h>
#include <math.h>
#include <poll.h>
#include <sys/eventfd.h>
#include <string.h>
#include <libtiepie.h>

#define FD_COUNT 3

int main(int argc, char* argv[])
{
  int status = EXIT_SUCCESS;
  unsigned int resolution = 0;
  unsigned int active_channel_count = 0;
  double sample_frequency = 10e3; // 10 kHz
  uint64_t record_length = 5000; // 5 kS
  double duration = 60; // 60 seconds
  bool raw = false;
  uint32_t serial_number = 0;

  {
    int opt;
    while((opt = getopt(argc, argv, "b:c:f:l:d:s:rh")) != -1)
    {
      switch(opt)
      {
        case 'b':
          sscanf(optarg, "%u", &resolution);
          break;

        case 'c':
          sscanf(optarg, "%u", &active_channel_count);
          break;

        case 'f':
          sscanf(optarg, "%lf", &sample_frequency);
          break;

        case 'l':
          sscanf(optarg, "%" PRIu64, &record_length);
          break;

        case 'd':
          sscanf(optarg, "%lf", &duration);
          break;

        case 's':
          sscanf(optarg, "%" PRIu32, &serial_number);
          break;

        case 'r':
          raw = true;
          break;

        case 'h':
        default:
          fprintf(stderr, "Usage: %s [-b resolution] [-c active channel count] [-f sample frequency] [-l record length] [-d duration] [-s serial number] [-r]\n", argv[0]);
          exit(opt == 'h' ? EXIT_SUCCESS : EXIT_FAILURE);
      }
    }
  }

  //===========================================================================

  TpVersion_t version = LibGetVersion();
  printf("libtiepie v%u.%u.%u.%u%s\n" ,
          (uint16_t)TPVERSION_MAJOR(version) ,
          (uint16_t)TPVERSION_MINOR(version) ,
          (uint16_t)TPVERSION_RELEASE(version) ,
          (uint16_t)TPVERSION_BUILD(version) ,
          LibGetVersionExtra());

  LibInit();

  // Open device:
  LstUpdate();

  LibTiePieHandle_t scp;
  if(serial_number != 0)
    scp = LstOpenOscilloscope(IDKIND_SERIALNUMBER, serial_number);
  else
    scp = LstOpenOscilloscope(IDKIND_INDEX, 0);

  if(scp == LIBTIEPIE_HANDLE_INVALID)
  {
    fprintf(stderr, "LstOpenOscilloscope failed: %s\n", LibGetLastStatusStr());
    status = EXIT_FAILURE;
    goto exit_no_mem;
  }

  const uint16_t channel_count = ScpGetChannelCount(scp);

  // Setup device:
  ScpSetMeasureMode(scp, MM_STREAM);

  if(active_channel_count == 0 || active_channel_count > channel_count)
    active_channel_count = channel_count;
  printf("Active channel count: %u\n", active_channel_count);
  for(uint16_t i = 0; i < channel_count; i++)
    printf("  Ch%u: %s\n", i + 1, ScpChSetEnabled(scp, i, i < active_channel_count ? BOOL8_TRUE : BOOL8_FALSE) ? "enabled" : "disabled");

  if(resolution != 0)
    ScpSetResolution(scp, resolution);

  sample_frequency = ScpSetSampleFrequency(scp, sample_frequency);
  printf("Sample frequency: %f MHz\n", sample_frequency / 1e6);

  printf("Resolution: %u bit\n", ScpGetResolution(scp));

  record_length = ScpSetRecordLength(scp, record_length);
  printf("Record length: %" PRIu64 " Samples\n", record_length);

  printf("Data rate: %f MB/s\n", (active_channel_count * ceil(ScpGetResolution(scp) / 8.0) * sample_frequency) / 1e6);

  unsigned int num_blocks = ceil(duration * sample_frequency / record_length);
  duration = (record_length * num_blocks) / sample_frequency;
  printf("Duration: %f s\n", duration);

  printf("Data type: %s\n", raw ? "raw" : "float");

  {
    // Alloc memory:
    const size_t sample_size = raw ? ceil(ScpGetResolution(scp) / 8.0) : sizeof(float);
    void* buffers[channel_count];
    for(uint16_t i = 0; i < channel_count; i++)
      if(i < active_channel_count)
        buffers[i] = malloc(record_length * sample_size);
      else
        buffers[i] = NULL;

    // Setup events:
    int fd_removed = eventfd(0, EFD_NONBLOCK);
    int fd_data_ready = eventfd(0, EFD_NONBLOCK);
    int fd_data_overflow = eventfd(0, EFD_NONBLOCK);

    DevSetEventRemoved(scp, fd_removed);
    ScpSetEventDataReady(scp, fd_data_ready);
    ScpSetEventDataOverflow(scp, fd_data_overflow);

    struct pollfd fds[FD_COUNT] = {
      {fd: fd_removed, events: POLLIN, revents: 0},
      {fd: fd_data_ready, events: POLLIN, revents: 0},
      {fd: fd_data_overflow, events: POLLIN, revents: 0}
    };

    if(!ScpStart(scp))
    {
      fprintf(stderr, "ScpStart failed: %s\n", LibGetLastStatusStr());
      status = EXIT_FAILURE;
      goto exit;
    }

    unsigned int n = 0;
    int r;
    while(n < num_blocks && (r = poll(fds, FD_COUNT, -1)) >= 0)
    {
      for(int i = 0; i < FD_COUNT; i++)
      {
        if(fds[i].revents & POLLIN)
        {
          eventfd_t tmp;
          eventfd_read(fds[i].fd, &tmp);

          if(fds[i].fd == fd_removed)
          {
            fprintf(stderr, "Device gone!\n");
            status = EXIT_FAILURE;
            goto exit;
          }

          if(fds[i].fd == fd_data_overflow)
          {
            fprintf(stderr, "Data overflow!\n");
            status = EXIT_FAILURE;
            goto exit;
          }

          if(fds[i].fd == fd_data_ready)
          {
            if(raw)
              ScpGetDataRaw(scp, buffers, channel_count, 0, record_length);
            else
              ScpGetData(scp, (float**)buffers, channel_count, 0, record_length);

            if(LibGetLastStatus() < LIBTIEPIESTATUS_SUCCESS)
            {
              fprintf(stderr, "%s failed: %s\n", raw ? "ScpGetDataRaw" : "ScpGetData", LibGetLastStatusStr());
              status = EXIT_FAILURE;
              goto exit;
            }

            n++;
            printf("\r%0.1f %%", 100.0 * n / num_blocks);
            fflush(stdout);
          }
        }

        fds[i].revents = 0;
      }
    }

    ScpStop(scp);

exit:
    for(uint16_t i = 0; i < channel_count; i++)
      free(buffers[i]);
  }

exit_no_mem:
  LibExit();
  return status;
}
