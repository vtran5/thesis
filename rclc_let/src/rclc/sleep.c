// Copyright (c) 2019 - for information on the respective copyright owner
// see the NOTICE file and/or the repository https://github.com/ros2/rclc.
// Copyright 2014 Open Source Robotics Foundation, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "rclc/sleep.h"

#ifdef WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#endif

void
rclc_sleep_ms(
  unsigned int ms)
{
#ifdef WIN32
  Sleep(ms);
#else
  usleep(ms * 1000);
#endif
}

void
rclc_sleep_ns(uint64_t ns)
{
#ifdef WIN32
  Sleep(ns/1000000);
#else
  uint64_t ns_per_tick = (1000*1000*1000)/configTICK_RATE_HZ;
  const TickType_t xTicksToDelay = (TickType_t) ns/ns_per_tick;
  vTaskDelay(xTicksToDelay);
#endif  
}