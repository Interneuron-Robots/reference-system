/*
 * @Description: 
 * @Author: Sauron
 * @Date: 2023-04-06 14:23:58
 * @LastEditTime: 2023-07-26 14:41:03
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
#ifndef REFERENCE_SYSTEM__NODES__RCLCPP__SENSOR_HPP_
#define REFERENCE_SYSTEM__NODES__RCLCPP__SENSOR_HPP_
#include <chrono>
#include <string>
#include <utility>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "reference_system/msg_types.hpp"
#include "reference_system/nodes/settings.hpp"
#include "reference_system/sample_management.hpp"
#ifdef INTERNEURON
#include "interneuron_lib/time_point_manager.hpp"
#endif

namespace nodes
{
namespace rclcpp_system
{

class Sensor : public rclcpp::Node
{
public:
  explicit Sensor(const SensorSettings & settings)
  : Node(settings.node_name,rclcpp::NodeOptions().use_intra_process_comms(nodes::USE_INTRA)),
    settings_(settings) 
  {
    publisher_ = this->create_publisher<message_t>(settings.topic_name, 1);
    timer_ = this->create_wall_timer(
      settings.cycle_time,
      [this] {timer_callback();});
      #ifdef INTERNEURON
      source_tp_ = interneuron::TimePointManager::getInstance().add_source_timepoint(settings.sensor_name,settings.cycle_time.count()*2,settings.cycle_time.count());
      #endif
  }

private:
  void timer_callback()
  {
#ifdef INTERNEURON
    //loaned message is not supported in intra-communication
auto message = message_t();
    message.size = 0;
    auto message_info = std::make_unique<rclcpp::MessageInfo>(rclcpp::MessageInfo({settings_.sensor_name}));
    auto policy = source_tp_->update_sample_time(message_info->get_TP_Info(settings_.sensor_name));
    set_sample(
      this->get_name(), sequence_number_++, 0, source_tp_->get_last_sample_time(),
      message);
        publisher_->publish(message, std::move(message_info));
#else
    uint64_t timestamp = now_as_int();
    auto message = publisher_->borrow_loaned_message();
    message.get().size = 0;
    set_sample(
      this->get_name(), sequence_number_++, 0, timestamp,
      message.get());

    publisher_->publish(std::move(message));
#endif
  }

private:
  rclcpp::Publisher<message_t>::SharedPtr publisher_;
  rclcpp::TimerBase::SharedPtr timer_;
  uint32_t sequence_number_ = 0;
  SensorSettings settings_;
  #ifdef INTERNEURON
    std::shared_ptr<interneuron::SourceTimePoint> source_tp_;
    #endif
};
}  // namespace rclcpp_system
}  // namespace nodes
#endif  // REFERENCE_SYSTEM__NODES__RCLCPP__SENSOR_HPP_
