/*
 * @Description: 
 * @Author: Sauron
 * @Date: 2023-04-06 14:23:58
 * @LastEditTime: 2023-07-19 23:25:49
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
#ifndef REFERENCE_SYSTEM__NODES__RCLCPP__TRANSFORM_HPP_
#define REFERENCE_SYSTEM__NODES__RCLCPP__TRANSFORM_HPP_
#include <chrono>
#include <string>
#include <utility>

#include "rclcpp/rclcpp.hpp"
#include "reference_system/msg_types.hpp"
#include "reference_system/nodes/settings.hpp"
#include "reference_system/number_cruncher.hpp"
#include "reference_system/sample_management.hpp"
#ifdef INTERNEURON
#include "interneuron_lib/time_point_manager.hpp"
#endif
namespace nodes
{
namespace rclcpp_system
{

class Transform : public rclcpp::Node
{
public:
  explicit Transform(const TransformSettings & settings)
  : Node(settings.node_name,rclcpp::NodeOptions().use_intra_process_comms(nodes::USE_INTRA)),
    number_crunch_limit_(settings.number_crunch_limit)
  {
    publisher_ = this->create_publisher<message_t>(settings.output_topic, 1);
    #ifdef INTERNEURON
    subscription_ = this->create_subscription<message_t>(
      settings.input_topic, 1,
      [this](const message_t::SharedPtr msg, const rclcpp::MessageInfo&msg_info) {input_callback(msg, msg_info);});
      interneuron::TimePointManager::getInstance().add_middle_timepoint(subscription_->get_key_tp()+"_sub",settings.sensor_names);
      start_tp_ = interneuron::TimePointManager::getInstance().add_middle_timepoint(subscription_->get_key_tp()+"_app",settings.sensor_names);
      end_tp_ = interneuron::TimePointManager::getInstance().add_middle_timepoint(publisher_->get_key_tp()+"_pub",settings.sensor_names);
    #else
    subscription_ = this->create_subscription<message_t>(
      settings.input_topic, 1,
      [this](const message_t::SharedPtr msg) {input_callback(msg);});
      #endif
  }

private:
#ifdef INTERNEURON
std::shared_ptr<interneuron::MiddleTimePoint> start_tp_;
  std::shared_ptr<interneuron::MiddleTimePoint> end_tp_;
void input_callback(const message_t::SharedPtr input_message, const rclcpp::MessageInfo&msg_info)
  {
    auto message_info = std::make_unique<rclcpp::MessageInfo>(msg_info);
        //---update start tp
        auto run_policy = start_tp_->update_reference_times(message_info->tp_infos_);
        //---
    uint64_t timestamp = now_as_int();
    auto number_cruncher_result = number_cruncher(number_crunch_limit_);

    auto output_message = message_t();
    output_message.size = 0;
    merge_history_into_sample(output_message, input_message);

    uint32_t missed_samples = get_missed_samples_and_update_seq_nr(
      input_message, input_sequence_number_);

    set_sample(
      this->get_name(), sequence_number_++, missed_samples, timestamp,
      output_message);

    // use result so that it is not optimizied away by some clever compiler
    output_message.data[0] = number_cruncher_result;

    //---update end tp
    end_tp_->update_reference_times(message_info->tp_infos_);
    //---
    publisher_->publish(std::move(output_message),std::move(message_info));
  }
#else
  void input_callback(const message_t::SharedPtr input_message)
  {
    uint64_t timestamp = now_as_int();
    auto number_cruncher_result = number_cruncher(number_crunch_limit_);

    auto output_message = publisher_->borrow_loaned_message();
    output_message.get().size = 0;
    merge_history_into_sample(output_message.get(), input_message);

    uint32_t missed_samples = get_missed_samples_and_update_seq_nr(
      input_message, input_sequence_number_);

    set_sample(
      this->get_name(), sequence_number_++, missed_samples, timestamp,
      output_message.get());

    // use result so that it is not optimizied away by some clever compiler
    output_message.get().data[0] = number_cruncher_result;
    publisher_->publish(std::move(output_message));
  }
#endif
private:
  rclcpp::Publisher<message_t>::SharedPtr publisher_;
  rclcpp::Subscription<message_t>::SharedPtr subscription_;
  uint64_t number_crunch_limit_;
  uint32_t sequence_number_ = 0;
  uint32_t input_sequence_number_ = 0;
  
};
}  // namespace rclcpp_system
}  // namespace nodes
#endif  // REFERENCE_SYSTEM__NODES__RCLCPP__TRANSFORM_HPP_
