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
#ifndef REFERENCE_SYSTEM__NODES__RCLCPP__COMMAND_HPP_
#define REFERENCE_SYSTEM__NODES__RCLCPP__COMMAND_HPP_

#include <chrono>
#include <string>

#include "rclcpp/rclcpp.hpp"
#include "reference_system/msg_types.hpp"
#include "reference_system/nodes/settings.hpp"
#include "reference_system/sample_management.hpp"

namespace nodes
{
namespace rclcpp_system
{

class Command : public rclcpp::Node
{
public:
  explicit Command(const CommandSettings & settings)
  : Node(settings.node_name,rclcpp::NodeOptions().use_intra_process_comms(nodes::USE_INTRA))
  {
    #ifdef INTERNEURON
    subscription_ = this->create_subscription<message_t>(
      settings.input_topic, 10,
      [this](const message_t::SharedPtr msg,const rclcpp::MessageInfo&msg_info) {input_callback(msg,msg_info);});
    interneuron::TimePointManager::getInstance().add_middle_timepoint(subscription_->get_key_tp()+"_sub",settings.sensor_names);
    finish_tp_ = interneuron::TimePointManager::getInstance().add_sink_timepoint("sink",settings.sensor_names);
    #else
    subscription_ = this->create_subscription<message_t>(
      settings.input_topic, 10,
      [this](const message_t::SharedPtr msg) {input_callback(msg);});
    #endif
  }

private:
#ifdef INTERNEURON
std::shared_ptr<interneuron::SinkTimePoint> finish_tp_;
void input_callback(const message_t::SharedPtr input_message, const rclcpp::MessageInfo&msg_info)
  {
    auto message_info = std::make_unique<rclcpp::MessageInfo>(msg_info);
    finish_tp_->update_finish_times(message_info->tp_infos_);
    uint32_t missed_samples =
      get_missed_samples_and_update_seq_nr(input_message, sequence_number_);
    print_sample_path(this->get_name(), missed_samples, input_message);
  }
#else
  void input_callback(const message_t::SharedPtr input_message)
  {
    uint32_t missed_samples =
      get_missed_samples_and_update_seq_nr(input_message, sequence_number_);
    print_sample_path(this->get_name(), missed_samples, input_message);
  }
#endif
private:
  rclcpp::Subscription<message_t>::SharedPtr subscription_;
  uint32_t sequence_number_ = 0;
};
}  // namespace rclcpp_system
}  // namespace nodes
#endif  // REFERENCE_SYSTEM__NODES__RCLCPP__COMMAND_HPP_
