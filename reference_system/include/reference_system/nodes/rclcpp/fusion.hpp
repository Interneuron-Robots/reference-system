/*
 * @Description: 
 * @Author: Sauron
 * @Date: 2023-04-06 14:23:58
 * @LastEditTime: 2023-08-10 14:42:33
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
#ifndef REFERENCE_SYSTEM__NODES__RCLCPP__FUSION_HPP_
#define REFERENCE_SYSTEM__NODES__RCLCPP__FUSION_HPP_
#include <chrono>
#include <string>
#include <utility>

#include "rclcpp/rclcpp.hpp"
#include "reference_system/msg_types.hpp"
#include "reference_system/nodes/settings.hpp"
#include "reference_system/number_cruncher.hpp"
#include "reference_system/sample_management.hpp"

namespace nodes
{
namespace rclcpp_system
{

class Fusion : public rclcpp::Node
{
public:
  explicit Fusion(const FusionSettings & settings)
  : Node(settings.node_name,rclcpp::NodeOptions().use_intra_process_comms(nodes::USE_INTRA)),
    number_crunch_limit_(settings.number_crunch_limit)
  {
    publisher_ = this->create_publisher<message_t>(settings.output_topic, 1);
    #ifdef INTERNEURON
    subscriptions_[0].subscription = this->create_subscription<message_t>(
      settings.input_0, 1,
      [this](const message_t::SharedPtr msg, const rclcpp::MessageInfo&msg_info) {input_callback(0U, msg, msg_info);});

    subscriptions_[1].subscription = this->create_subscription<message_t>(
      settings.input_1, 1,
      [this](const message_t::SharedPtr msg, const rclcpp::MessageInfo&msg_info) {input_callback(1U, msg, msg_info);});

interneuron::TimePointManager::getInstance().add_middle_timepoint(subscriptions_[0].subscription->get_key_tp()+"_sub",settings.input_0_sensor_names);
interneuron::TimePointManager::getInstance().add_middle_timepoint(subscriptions_[1].subscription->get_key_tp()+"_sub",settings.input_1_sensor_names);
      start_tp_ = interneuron::TimePointManager::getInstance().add_middle_timepoint(subscriptions_[0].subscription->get_key_tp()+"_app",settings.output_sensor_names);//we only need one start timepoint, and we use the first subscriber's key, it's ok to use any unique string defined in settings
      end_tp_ = interneuron::TimePointManager::getInstance().add_middle_timepoint(publisher_->get_key_tp()+"_pub",settings.output_sensor_names);
    #else
    subscriptions_[0].subscription = this->create_subscription<message_t>(
      settings.input_0, 1,
      [this](const message_t::SharedPtr msg) {input_callback(0U, msg);});

    subscriptions_[1].subscription = this->create_subscription<message_t>(
      settings.input_1, 1,
      [this](const message_t::SharedPtr msg) {input_callback(1U, msg);});
    #endif
  }

private:
#ifdef INTERNEURON
std::shared_ptr<interneuron::MiddleTimePoint> start_tp_;
std::shared_ptr<interneuron::MiddleTimePoint> end_tp_;
void input_callback(
    const uint64_t input_number,
    const message_t::SharedPtr input_message,
    const rclcpp::MessageInfo&msg_info)
  {
    uint64_t timestamp = now_as_int();
    subscriptions_[input_number].cache = input_message;
    subscriptions_[input_number].msg_info = msg_info;

    // only process and publish when we can perform an actual fusion, this means
    // we have received a sample from each subscription
    if (!subscriptions_[0].cache || !subscriptions_[1].cache) {
      return;
    }

    auto number_cruncher_result = number_cruncher(number_crunch_limit_);

    auto output_message = message_t();

    uint32_t missed_samples =
      get_missed_samples_and_update_seq_nr(
      subscriptions_[0].cache, subscriptions_[0].sequence_number) +
      get_missed_samples_and_update_seq_nr(
      subscriptions_[1].cache,
      subscriptions_[1].sequence_number);

    output_message.size = 0;
    merge_history_into_sample(output_message, subscriptions_[0].cache);
    merge_history_into_sample(output_message, subscriptions_[1].cache);
    set_sample(
      this->get_name(), sequence_number_++, missed_samples, timestamp,
      output_message);

    output_message.data[0] = number_cruncher_result;
    TODO:
    publisher_->publish(std::move(output_message));

    subscriptions_[0].cache.reset();
    subscriptions_[1].cache.reset();
  }

#else
  void input_callback(
    const uint64_t input_number,
    const message_t::SharedPtr input_message)
  {
    uint64_t timestamp = now_as_int();
    subscriptions_[input_number].cache = input_message;

    // only process and publish when we can perform an actual fusion, this means
    // we have received a sample from each subscription
    if (!subscriptions_[0].cache || !subscriptions_[1].cache) {
      return;
    }

    auto number_cruncher_result = number_cruncher(number_crunch_limit_);

    auto output_message = publisher_->borrow_loaned_message();

    uint32_t missed_samples =
      get_missed_samples_and_update_seq_nr(
      subscriptions_[0].cache, subscriptions_[0].sequence_number) +
      get_missed_samples_and_update_seq_nr(
      subscriptions_[1].cache,
      subscriptions_[1].sequence_number);

    output_message.get().size = 0;
    merge_history_into_sample(output_message.get(), subscriptions_[0].cache);
    merge_history_into_sample(output_message.get(), subscriptions_[1].cache);
    set_sample(
      this->get_name(), sequence_number_++, missed_samples, timestamp,
      output_message.get());

    output_message.get().data[0] = number_cruncher_result;
    publisher_->publish(std::move(output_message));

    subscriptions_[0].cache.reset();
    subscriptions_[1].cache.reset();
  }
  #endif

private:
  struct subscription_t
  {
    rclcpp::Subscription<message_t>::SharedPtr subscription;
    uint32_t sequence_number = 0;
    message_t::SharedPtr cache;
    #ifdef INTERNEURON
    rclcpp::MessageInfo msg_info;
    #endif
  };
  rclcpp::Publisher<message_t>::SharedPtr publisher_;

  subscription_t subscriptions_[2];

  uint64_t number_crunch_limit_;
  uint32_t sequence_number_ = 0;
};
}  // namespace rclcpp_system
}  // namespace nodes
#endif  // REFERENCE_SYSTEM__NODES__RCLCPP__FUSION_HPP_
