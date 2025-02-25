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

#include <aws/core/utils/logging/AWSLogging.h>
#include <aws/core/utils/logging/LogMacros.h>
#include <aws_common/sdk_utils/client_configuration_provider.h>
#include <aws_ros2_common/sdk_utils/logging/aws_ros_logger.h>
#include <aws_ros2_common/sdk_utils/ros2_node_parameter_reader.h>
#include <cloudwatch_logger/log_node.h>
#include <cloudwatch_logs_common/log_batcher.h>
#include <cloudwatch_logs_common/log_service_factory.h>
#include <cloudwatch_logs_common/log_publisher.h>
#include <cloudwatch_logs_common/log_service.h>
#include <rclcpp/rclcpp.hpp>

using namespace Aws::CloudWatchLogs::Utils;

LogNode::LogNode(int8_t min_log_severity, std::unordered_set<std::string> ignore_nodes)
  : ignore_nodes_(std::move(ignore_nodes))
{
  this->log_service_ = nullptr;
  this->min_log_severity_ = min_log_severity;
}

LogNode::~LogNode() { this->log_service_ = nullptr; }

void LogNode::Initialize(const std::string & log_group, const std::string & log_stream,
                         const Aws::Client::ClientConfiguration & config, Aws::SDKOptions & sdk_options,
                         const Aws::CloudWatchLogs::CloudWatchOptions & cloudwatch_options,
                         std::shared_ptr<LogServiceFactory> factory)
{
  this->log_service_ = factory->CreateLogService(log_group, log_stream, config, sdk_options, cloudwatch_options);
}

bool LogNode::checkIfOnline(std_srvs::srv::Trigger::Request & /*request*/, std_srvs::srv::Trigger::Response & response)
{
  // AWS_LOGSTREAM_DEBUG(__func__, "received request " << request);

  if (!this->log_service_) {
    response.success = false;
    response.message = "The LogService is not initialized";
    return true;
  }

  response.success = this->log_service_->isConnected();
  response.message = response.success ? "The LogService is connected" : "The LogService is not connected";

  return true;
}

bool LogNode::start()
{
  bool is_started = true;
  if (this->log_service_) {
    is_started &= this->log_service_->start();
  }
  is_started &= Service::start();
  return is_started;
}

bool LogNode::shutdown()
{
  bool is_shutdown = Service::shutdown();
  if (this->log_service_) {
    is_shutdown &= this->log_service_->shutdown();
  }
  return is_shutdown;
}

void LogNode::RecordLogs(const rcl_interfaces::msg::Log::SharedPtr log_msg)
{
  if (0 == this->ignore_nodes_.count(log_msg->name)) {
    if (nullptr == this->log_service_) {
      AWS_LOG_ERROR(__func__, "Cannot publish CloudWatch logs with "
                    "NULL Aws::CloudWatchLogs::LogService instance.");
      return;
    }
    if (ShouldSendToCloudWatchLogs(log_msg->level)) {
      auto message = FormatLogs(log_msg);
      this->log_service_->batchData(message);
    }
  }
}

void LogNode::TriggerLogPublisher()
{
  this->log_service_->publishBatchedData();
}

bool LogNode::ShouldSendToCloudWatchLogs(const int8_t log_severity_level)
{
  return log_severity_level >= this->min_log_severity_;
}

const std::string LogNode::FormatLogs(const rcl_interfaces::msg::Log::SharedPtr log_msg)
{
  std::stringstream ss;
  ss << std::chrono::duration_cast<std::chrono::duration<double>>(
       std::chrono::seconds(log_msg->stamp.sec) + std::chrono::nanoseconds(log_msg->stamp.nanosec)
     ).count() << " ";

  switch (log_msg->level) {
    case rcl_interfaces::msg::Log::FATAL:
      ss << "FATAL ";
      break;
    case rcl_interfaces::msg::Log::ERROR:
      ss << "ERROR ";
      break;
    case rcl_interfaces::msg::Log::WARN:
      ss << "WARN ";
      break;
    case rcl_interfaces::msg::Log::DEBUG:
      ss << "DEBUG ";
      break;
    case rcl_interfaces::msg::Log::INFO:
      ss << "INFO ";
      break;
    default:
      ss << log_msg->level << " ";
  }
  ss << "[node name: " << log_msg->name << "] ";

  ss << log_msg->msg << "\n";
  std::cout << log_msg->msg << std::endl;

  return ss.str();
}
