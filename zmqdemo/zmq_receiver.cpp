/*
 * CopyRight:	none
 * FileName:	zmq_receiver.cpp
 * Desc:
 * Author:		Conscien
 * Date:		2023/6/13
 * Contact:		wlovelxcc@gmail.com
 * History:		Last update by Conscien 2023/6/13
 * */

#include "zmq_receiver.h"
#include <cstring>
#include <iostream>
#include <memory>
#include <mutex>
#include <string>
#include <zmq.h>

namespace HobotADAS {
#define RECV_QUEUE_SIZE 2
#define RECV_BUF_SIZE (2 * 1024 * 1024)

std::string NetworkReceiver::PARAM_FRAME_NAME = "CamCalibParam-";
std::string NetworkReceiver::META_FRAME_NAME = "FrameShuffle-";
std::string NetworkReceiver::IMAGE_FRAME_NAME = "RawImage-";

bool NetworkReceiver::Init(const char *end_point) {
  if (inited_) {
    return false;
  }
  end_point_ = end_point;
  context_ = zmq_ctx_new();

  // Socket to talk to server
  if (mode_ == ZmqMode::REQREP) {
    data_recv_ = zmq_socket(context_, ZMQ_REP);
    cmd_recv_ = zmq_socket(context_, ZMQ_REP);
  } else if (mode_ == ZmqMode::PUSHPULL) {
    data_recv_ = zmq_socket(context_, ZMQ_PULL);
    cmd_recv_ = zmq_socket(context_, ZMQ_PULL);
  } else if (mode_ == ZmqMode::PUBSUB) {
    data_recv_ = zmq_socket(context_, ZMQ_SUB);
    cmd_recv_ = zmq_socket(context_, ZMQ_SUB);
    // receive all message
    zmq_setsockopt(data_recv_, ZMQ_SUBSCRIBE, "", 0);
    zmq_setsockopt(cmd_recv_, ZMQ_SUBSCRIBE, "", 0);
  } else {
    data_recv_ = zmq_socket(context_, ZMQ_REP);
    cmd_recv_ = zmq_socket(context_, ZMQ_REP);
    mode_ = ZmqMode::REQREP;
  }

  // ZMQ_SNDTIMEO must be set before zmq connect
  zmq_setsockopt(data_recv_, ZMQ_SNDTIMEO, &recv_to_, sizeof(int));
  // ZMQ_LINGER can be changed after zmq connect
  zmq_setsockopt(data_recv_, ZMQ_LINGER, &recv_linger_, sizeof(int));

  int rc = 0;
  int hwm = RECV_QUEUE_SIZE;
  rc = zmq_setsockopt(data_recv_, ZMQ_RCVHWM, &hwm, sizeof(int));
  if (rc != 0) {
    std::cout << "zmq_setsockopt ZMQ_RCVHWM failed";
  }

  int rcvbuf = RECV_BUF_SIZE;
  rc = zmq_setsockopt(data_recv_, ZMQ_RCVBUF, &rcvbuf, sizeof(int));
  if (rc != 0) {
    std::cout << "zmq_setsockopt ZMQ_RCVBUF failed";
  }

  int con_ret;
  if (mode_ != ZmqMode::PUBSUB) {
    con_ret = zmq_bind(data_recv_, end_point);
    if (con_ret != 0) {
      return false;
    }

    con_ret = zmq_bind(cmd_recv_, (end_point_ + "0").c_str());
    if (con_ret != 0) {
      return false;
    }
  } else {
    con_ret = zmq_connect(data_recv_, end_point);
    if (con_ret != 0) {
      std::cout << "Error occurred during zmq_connect(): "
                << zmq_strerror(errno);
      return false;
    }
    con_ret = zmq_connect(cmd_recv_, (end_point_ + "0").c_str());
    if (con_ret != 0) {
      std::cout << "Error occurred during zmq_connect(): "
                << zmq_strerror(errno);
      return false;
    }
  }

  inited_ = true;
  return true;
}

void NetworkReceiver::ZMQReconnect() {
  Finish();
  if (!Init(end_point_.c_str())) {
    std::cout << "ZMQReconnect failed";
  }
}

void NetworkReceiver::Finish() {
  if (!inited_) {
    return;
  }

  zmq_close(data_recv_);
  zmq_close(cmd_recv_);
  zmq_ctx_destroy(context_);

  std::lock_guard<std::mutex> lck(mutex_recv_);
  frame_cache_map_.clear();
}

bool NetworkReceiver::RecvNetworkInput() {
  if (HasCacheFrame()) {
    return true;
  }

  int rc = 0;
  for (int i = 0; i < RECV_QUEUE_SIZE; ++i) {
    const int recv_time_out = 500; // ms
    zmq_setsockopt(data_recv_, ZMQ_RCVTIMEO, &recv_time_out, sizeof(int));
    if (mode_ == ZmqMode::REQREP) {
      zmq_msg_t frame_data_msg;
      zmq_msg_init(&frame_data_msg);
      rc = zmq_msg_recv(&frame_data_msg, data_recv_, 0);
      if (rc == -1) {
        zmq_msg_close(&frame_data_msg);
        std::cout << "izmq_msg_recv failed " << std::endl;
        return false;
      }

      ReplyRequest();

      const int frame_size = static_cast<int>(zmq_msg_size(&frame_data_msg));
      auto *frame_data = static_cast<uint8_t *>(zmq_msg_data(&frame_data_msg));
      if (frame_size <= 0) {
        zmq_msg_close(&frame_data_msg);
        std::cout << "invalid frame size:" << frame_size << std::endl;
        continue;
      }

      auto dat_frame = std::make_shared<std::vector<uint8_t>>(frame_size);
      memcpy(dat_frame->data(), frame_data, frame_size);
      std::cout << "Receive data: " << dat_frame->data() << std::endl;
      zmq_msg_close(&frame_data_msg);
    } else if (mode_ == ZmqMode::PUBSUB) {
      zmq_msg_t topic_msg;
      zmq_msg_init(&topic_msg);
      int rc = zmq_msg_recv(&topic_msg, data_recv_, 0);
      if (rc == -1) {
        zmq_msg_close(&topic_msg);
        return false;
      }
      const int topic_size = static_cast<int>(zmq_msg_size(&topic_msg));
      char *topic_data = static_cast<char *>(zmq_msg_data(&topic_msg));
      char zsTopic[100] = {0};
      memcpy(zsTopic, topic_data, topic_size);
      std::cout << "Receive topic: " << zsTopic << std::endl;
      zmq_msg_close(&topic_msg);

      // payload data
      zmq_msg_t payload_msg;
      zmq_msg_init(&payload_msg);
      rc = zmq_msg_recv(&payload_msg, data_recv_, 0);
      if (rc == -1) {
        zmq_msg_close(&payload_msg);
        return false;
      }

      const int payload_size = static_cast<int>(zmq_msg_size(&payload_msg));
      char *payload_data = static_cast<char *>(zmq_msg_data(&payload_msg));
      auto dat_frame = std::make_shared<std::vector<uint8_t>>(payload_size);
      memcpy(dat_frame->data(), payload_data, payload_size);
      std::cout << "Receive topic data: " << dat_frame->data() << std::endl;
      zmq_msg_close(&payload_msg);
    } else {
      std::cout << "not implement";
      return false;
    }
  }
  return HasCacheFrame();
}

std::shared_ptr<uint8_t> NetworkReceiver::RecvMetaDatFrame() {
  return RecvDatFrame(META_FRAME_NAME);
}

std::shared_ptr<uint8_t> NetworkReceiver::RecvImageDatFrame() {
  return RecvDatFrame(IMAGE_FRAME_NAME);
}

std::shared_ptr<uint8_t> NetworkReceiver::RecvParamDatFrame() {
  return RecvDatFrame(PARAM_FRAME_NAME);
}

std::shared_ptr<uint8_t>
NetworkReceiver::RecvDatFrame(const std::string &name) {
  std::lock_guard<std::mutex> lck(mutex_recv_);
  auto iter = frame_cache_map_.find(name);
  if (iter == frame_cache_map_.end()) {
    return nullptr;
  }

  auto &frame_list = iter->second;
  if (frame_list.empty()) {
    return nullptr;
  }

  auto first = frame_list.front();
  frame_list.pop_front();
  return first;
}

bool NetworkReceiver::ReplyRequest() {
  if (reset_flag_.load()) {
    reset_flag_.store(false);
  }
  if (!reset_flag_.load()) {
    zmq_msg_t reply_msg;
    zmq_msg_init(&reply_msg);
    int rc = zmq_msg_send(&reply_msg, data_recv_, 0);
    zmq_msg_close(&reply_msg);
    if (rc == -1) {
      std::cout << "send reply msg failed";
      return false;
    }
  }
  return true;
}

bool NetworkReceiver::HasCacheFrame() {
  std::unique_lock<std::mutex> lck(mutex_recv_);
  bool img_empty = frame_cache_map_[IMAGE_FRAME_NAME].empty();
  bool meta_empty = frame_cache_map_[META_FRAME_NAME].empty();
  bool param_empty = frame_cache_map_[PARAM_FRAME_NAME].empty();
  if (!img_empty && (!meta_empty || !param_empty)) {
    return true;
  } else {
    return false;
  }
}
} // end of namespace HobotADAS