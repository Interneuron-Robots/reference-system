/*
 * @Description: 
 * @Author: Sauron
 * @Date: 2023-04-06 14:23:58
 * @LastEditTime: 2023-07-20 20:52:42
 * @LastEditors: Sauron
 */
// Copyright 2021 Apex.AI, Inc.
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
#ifndef REFERENCE_SYSTEM__NODES__SETTINGS_HPP_
#define REFERENCE_SYSTEM__NODES__SETTINGS_HPP_

#include <chrono>
#include <string>
#include <vector>
#ifdef INTERNEURON
#include "interneuron_lib/time_point.hpp"
#endif

namespace nodes
{

#ifdef INTERNEURON
bool USE_INTRA = true;

#else
bool USE_INTRA = false;
#endif

struct CommandSettings
{
  std::string node_name;
  std::string input_topic;
  
  #ifdef INTERNEURON
  std::vector<std::string> sensor_names;
  #endif
};

struct FusionSettings
{
  std::string node_name;
  std::string input_0;
  std::string input_1;
  std::string output_topic;
  uint64_t number_crunch_limit;

  #ifdef INTERNEURON
  std::vector<std::string> input_0_sensor_names;
  std::vector<std::string> input_1_sensor_names;
  std::vector<std::string> output_sensor_names;

  interneuron::FusionPolicy fusion_policy = interneuron::FusionPolicy::ALL_AVAILABLE;
  #endif
};

struct TransformSettings
{
  std::string node_name;
  std::string input_topic;
  std::string output_topic;
  uint64_t number_crunch_limit;

  #ifdef INTERNEURON
  std::vector<std::string> sensor_names;
  #endif
};

struct IntersectionSettings
{
  struct Connection
  {
    std::string input_topic;
    std::string output_topic;
    uint64_t number_crunch_limit;
  
    #ifdef INTERNEURON
    std::vector<std::string> input_sensor_names;
    std::vector<std::string> output_sensor_names;
    #endif
  };

  std::string node_name;
  std::vector<Connection> connections;
};

struct CyclicSettings
{
  std::string node_name;
  std::vector<std::string> inputs;
  std::string output_topic;
  uint64_t number_crunch_limit;
  std::chrono::nanoseconds cycle_time;
  #ifdef INTERNEURON
  std::vector<std::vector<std::string>> input_sensor_names;
  std::vector<std::string> output_sensor_names;
  #endif
};

struct SensorSettings
{
  std::string node_name;
  std::string topic_name;
  std::chrono::nanoseconds cycle_time;

  #ifdef INTERNEURON
  std::string sensor_name;
  #endif
};
}  // namespace nodes
#endif  // REFERENCE_SYSTEM__NODES__SETTINGS_HPP_
