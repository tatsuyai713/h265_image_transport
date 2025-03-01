// Copyright (c) 2021, Clyde McQueen.
// All rights reserved.
//
// Software License Agreement (BSD License 2.0)
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions
// are met:
//
//  * Redistributions of source code must retain the above copyright
//    notice, this list of conditions and the following disclaimer.
//  * Redistributions in binary form must reproduce the above
//    copyright notice, this list of conditions and the following
//    disclaimer in the documentation and/or other materials provided
//    with the distribution.
//  * Neither the name of the copyright holder nor the names of its
//    contributors may be used to endorse or promote products derived
//    from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
// FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
// COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
// INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
// LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
// CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
// LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
// ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#include "h265_image_transport/h265_subscriber.hpp"

#include <memory>
#include <string>

#include "sensor_msgs/image_encodings.hpp"

namespace h265_image_transport
{

H265Subscriber::H265Subscriber()
: logger_(rclcpp::get_logger("H265Subscriber")),
  // seq_(-1),
  consecutive_receive_failures_(0),
  p_codec_(),
  p_codec_context_(),
  p_frame_(),
  packet_(),
  p_sws_context_() {}

H265Subscriber::~H265Subscriber()
{
  avcodec_close(p_codec_context_);
  av_free(p_codec_context_);
  av_frame_free(&p_frame_);
}

void H265Subscriber::subscribeImpl(
  rclcpp::Node * node, const std::string & base_topic,
  const Callback & callback,
  rmw_qos_profile_t custom_qos)
{
  SimpleSubscriberPlugin::subscribeImpl(node, base_topic, callback, custom_qos);

  logger_ = node->get_logger();
  av_init_packet(&packet_);
  av_log_set_level(AV_LOG_WARNING);

  p_codec_ = avcodec_find_decoder(AV_CODEC_ID_H265);
  if (!p_codec_) {
    RCLCPP_ERROR(logger_, "Could not find ffmpeg h265 codec");
    throw std::runtime_error("Could not find ffmpeg h265 codec");
  }

  p_codec_context_ = avcodec_alloc_context3(p_codec_);

  if (avcodec_open2(p_codec_context_, p_codec_, nullptr) < 0) {
    RCLCPP_ERROR(logger_, "Could not open ffmpeg h265 codec");
    throw std::runtime_error("Could not open ffmpeg h265 codec");
  }

  p_frame_ = av_frame_alloc();
}

void H265Subscriber::internalCallback(
  const h265_image_transport::msg::H265Packet::ConstSharedPtr & message,
  const Callback & user_cb)
{
  // Report on sequence problems
  // if (seq_ < 0) {
  //   RCLCPP_INFO(logger_, "First message: %ld", message->seq);
  // } else {
  //   if (message->seq < seq_) {
  //     RCLCPP_INFO(logger_, "Old message: %ld", message->seq);
  //   }
  //   if (message->seq == seq_) {
  //     RCLCPP_INFO(logger_, "Repeat message: %ld", seq_);
  //   }
  //   if (message->seq > seq_ + 1) {
  //     RCLCPP_INFO(logger_, "Missing message(s): %ld-%ld", seq_ + 1, message->seq - 1);
  //   }
  // }
  // seq_ = message->seq;

  packet_.size = static_cast<int>(message->data.size());
  packet_.data = const_cast<uint8_t *>(reinterpret_cast<uint8_t const *>(&message->data[0]));

  // Send packet to decoder
  if (avcodec_send_packet(p_codec_context_, &packet_) < 0) {
    RCLCPP_INFO(logger_, "Could not send packet");
    return;
  }

  // Get decoded frame
  // Failure to decode is common when first starting, avoid spamming the logs
  if (avcodec_receive_frame(p_codec_context_, p_frame_) < 0) {
    if (++consecutive_receive_failures_ % 30 == 0) {
      RCLCPP_INFO(logger_, "Could not receive %d frames", consecutive_receive_failures_);
    }
    return;
  }

  if (consecutive_receive_failures_ > 0) {
    RCLCPP_INFO(logger_, "Could not receive %d frames", consecutive_receive_failures_);
    consecutive_receive_failures_ = 0;
  }

  auto image = std::make_shared<sensor_msgs::msg::Image>();
  image->width = p_frame_->width;
  image->height = p_frame_->height;
  image->step = 3 * p_frame_->width;
  image->encoding = sensor_msgs::image_encodings::BGR8;
  image->header = message->header;

  // Set / update sws context
  p_sws_context_ = sws_getCachedContext(
    p_sws_context_, p_frame_->width, p_frame_->height, p_codec_context_->pix_fmt,
    p_frame_->width, p_frame_->height, AV_PIX_FMT_BGR24,
    SWS_FAST_BILINEAR, nullptr, nullptr, nullptr);

  // Copy and convert from YUYV420P to BGR24
  image->data.resize(p_frame_->width * p_frame_->height * 3);
  int stride = 3 * p_frame_->width;
  uint8_t * destination = &image->data[0];
  sws_scale(
    p_sws_context_, (const uint8_t * const *) p_frame_->data, p_frame_->linesize, 0,
    p_frame_->height, &destination, &stride);

  user_cb(image);
}

}  // namespace h265_image_transport
