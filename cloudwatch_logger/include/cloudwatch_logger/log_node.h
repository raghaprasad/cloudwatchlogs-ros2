/*
 * Copyright 2018 Amazon.com, Inc. or its affiliates. All Rights Reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License").
 * You may not use this file except in compliance with the License.
 * A copy of the License is located at
 *
 *  http://aws.amazon.com/apache2.0
 *
 * or in the "license" file accompanying this file. This file is distributed
 * on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
 * express or implied. See the License for the specific language governing
 * permissions and limitations under the License.
 */

#pragma once

#include <aws_common/sdk_utils/client_configuration_provider.h>
#include <aws_ros2_common/sdk_utils/logging/aws_ros_logger.h>
#include <aws_ros2_common/sdk_utils/ros2_node_parameter_reader.h>
#include <cloudwatch_logs_common/log_service.h>
#include <cloudwatch_logs_common/log_service_factory.h>
#include <rclcpp/rclcpp.hpp>
#include <rcl_interfaces/msg/log.hpp>
#include <unordered_set>
#include <std_srvs/srv/trigger.hpp>
#include <std_srvs/srv/empty.hpp>

namespace Aws {
namespace CloudWatchLogs {
namespace Utils {

class LogNode : public Service
{
public:
  /**
   * @brief Creates a new CloudWatchLogNode
   *
   * @param min_log_severity the minimum log severity level defined in the configuration file
   *                         logs with severity level equal or above get sent to CloudWatch Logs
   * @param ignore_nodes The set of node names to ignore logs from
   */
  explicit LogNode(int8_t min_log_severity, std::unordered_set<std::string> ignore_nodes);

  /**
   *  @brief Tears down a AWSCloudWatchLogNode object
   */
  ~LogNode();

  /**
   * @brief Reads creds, region, and SDK option to configure log manager
   *
   * @param log_group log group name
   * @param log_stream log stream name
   * @param config aws client configuration object
   * @param sdk_options aws sdk options
   * @param log_service_factory optional log manager factory
   */
  void Initialize(const std::string & log_group, const std::string & log_stream,
                  const Aws::Client::ClientConfiguration & config, Aws::SDKOptions & sdk_options,
                  const Aws::CloudWatchLogs::CloudWatchOptions & cloudwatch_options,
                  std::shared_ptr<LogServiceFactory> log_service_factory = std::make_shared<LogServiceFactory>());

  bool start() override;
  bool shutdown() override;

  /**
   * @brief Emits RecordLog using the log manager
   *
   * @param log_msg A log message from the subscribed topic(s)
   */
  void RecordLogs(const rcl_interfaces::msg::Log::SharedPtr log_msg);

  /**
   * @brief Trigger the log manager to call its Service function to publish logs to cloudwatch
   * periodically
   *
   * @param timer A ros timer
   */
  void TriggerLogPublisher();

  /**
   * Return a Trigger response detailing the LogService online status.
   *
   * @param request input request
   * @param response output response
   * @return true if the request was handled successfully, false otherwise
   */
  bool checkIfOnline(std_srvs::srv::Trigger::Request & request, std_srvs::srv::Trigger::Response & response);

private:
  bool ShouldSendToCloudWatchLogs(const int8_t log_severity_level);
  const std::string FormatLogs(const rcl_interfaces::msg::Log::SharedPtr log_msg);
  std::shared_ptr<Aws::CloudWatchLogs::LogService> log_service_;
  int8_t min_log_severity_;
  std::unordered_set<std::string> ignore_nodes_;
};

}  // namespace Utils
}  // namespace CloudWatchLogs
}  // namespace Aws
