/*
 * CopyRight:	none
 * FileName:	zmq_sender.cpp
 * Desc:
 * Author:		Conscien
 * Date:		2023/6/13
 * Contact:		wlovelxcc@gmail.com
 * History:		Last update by Conscien 2023/6/13
 * */

#include "zmq_sender.h"
#include <chrono>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <utility>
#include <vector>
#include <zmq.h>

namespace HobotADAS {

#define SEND_QUEUE_SIZE 2
#define SEND_BUF_SIZE (2 * 1024 * 1024)

NetworkSender::NetworkSender()
    : context_(NULL), data_send_(NULL), cmd_send_(NULL), end_point_(""),
      resend_flag_(true), mode_(ZmqMode::REQREP), send_timeout_(1000),
      recv_timeout_(5000), full_data_mode_{false} {}

NetworkSender::~NetworkSender() {}

bool NetworkSender::Init(const char *end_point) {
  end_point_ = end_point;

  context_ = zmq_ctx_new();

  // Socket to talk to server
  if (mode_ == ZmqMode::REQREP) {
    data_send_ = zmq_socket(context_, ZMQ_REQ);
    zmq_setsockopt(data_send_, ZMQ_RCVTIMEO, &recv_timeout_, sizeof(int));
    cmd_send_ = zmq_socket(context_, ZMQ_REQ);
    zmq_setsockopt(cmd_send_, ZMQ_RCVTIMEO, &recv_timeout_, sizeof(int));
  } else if (mode_ == ZmqMode::PUSHPULL) {
    data_send_ = zmq_socket(context_, ZMQ_PUSH);
    cmd_send_ = zmq_socket(context_, ZMQ_PUSH);
  } else if (mode_ == ZmqMode::PUBSUB) {
    data_send_ = zmq_socket(context_, ZMQ_PUB);
    int zmq_rate = 1048576;
    int zmq_recovery_ivl = 0;
    zmq_setsockopt(data_send_, ZMQ_RATE, &zmq_rate, sizeof(int));
    zmq_setsockopt(data_send_, ZMQ_RECOVERY_IVL, &zmq_recovery_ivl,
                   sizeof(int));
  } else {
    data_send_ = zmq_socket(context_, ZMQ_REQ);
    zmq_setsockopt(data_send_, ZMQ_RCVTIMEO, &recv_timeout_, sizeof(int));
    cmd_send_ = zmq_socket(context_, ZMQ_REQ);
    zmq_setsockopt(cmd_send_, ZMQ_RCVTIMEO, &recv_timeout_, sizeof(int));
  }
  int zmq_sndhwm = SEND_QUEUE_SIZE; // message list size
  zmq_setsockopt(data_send_, ZMQ_SNDHWM, &zmq_sndhwm, sizeof(int));
  // zmq_setsockopt(data_send_, ZMQ_RCVHWM, &zmq_sndhwm, sizeof(int));
  int zmq_sndbuf = SEND_BUF_SIZE;
  zmq_setsockopt(data_send_, ZMQ_SNDBUF, &zmq_sndbuf, sizeof(int));

  int to = send_timeout_;
  zmq_setsockopt(data_send_, ZMQ_LINGER, &to, sizeof(to));
  zmq_setsockopt(data_send_, ZMQ_SNDTIMEO, &to, sizeof(to));

  int con_ret = 0;
  if (mode_ == ZmqMode::PUBSUB) {
    con_ret = zmq_bind(data_send_, end_point);
    if (con_ret < 0) {
      std::cout << "tcp bind failed: " << zmq_strerror(errno) << std::endl;
    }

    std::this_thread::sleep_for(std::chrono::seconds(2));
  } else {
    zmq_setsockopt(cmd_send_, ZMQ_LINGER, &to, sizeof(to));
    zmq_setsockopt(cmd_send_, ZMQ_SNDTIMEO, &to, sizeof(to));

    con_ret = zmq_connect(data_send_, end_point);
    if (con_ret < 0) {
      std::cout << "connect data failed." << zmq_strerror(errno) << std::endl;
    }

    std::string cmd_end = end_point;
    cmd_end += "0";
    con_ret = zmq_connect(cmd_send_, cmd_end.c_str());
    if (con_ret < 0) {
      std::cout << "connect command failed." << zmq_strerror(errno)
                << std::endl;
    }
  }

  std::cout << "target ip: " << end_point << ", connect status: " << con_ret
            << std::endl;
  return (con_ret == 0);
}

void NetworkSender::Fini() {
  zmq_close(data_send_);
  zmq_close(cmd_send_);
  zmq_ctx_destroy(context_);
}

void NetworkSender::SetReSend(const bool &flag) { resend_flag_ = flag; }

int SendSingleMsg(void *s, zmq_msg_t *msg, int flag, bool resend_flag) {
  int rc = 0;

  bool try_again = true;
  while (try_again) {
    rc = zmq_msg_send(msg, s, flag);
    if (rc >= 0) {
      try_again = false;
    } else {
      if (errno == EAGAIN || errno == EINTR) {
        std::cout << "try again" << std::endl;
        if (!resend_flag) {
          return -1;
        }
        std::this_thread::sleep_for(std::chrono::seconds(10));
      } else {
        std::cout << "ZMQ Get Error " << strerror(errno) << std::endl;
        return -1;
      }
    }
  } // end while

  return 0;
}

void NetworkSender::SendDataMsg(zmq_msg_t &msg) {
  int rc = 0;
  bool try_again = true;
  while (try_again) {
    rc = zmq_msg_send(&msg, data_send_, 0);
    if (rc >= 0) {
      try_again = false;
    } else {
      std::this_thread::sleep_for(std::chrono::milliseconds(10));
      if (mode_ == ZmqMode::REQREP) {
        std::cout << "try again" << std::endl;
        ResetDataSender();
      }
    } // end else
  }   // end while
}

bool NetworkSender::SendFrame(
    const std::vector<uint8_t *> &img_data_list,
    const std::vector<int> &img_size_list, const uint8_t *meta, int meta_size,
    const std::vector<std::shared_ptr<std::vector<uint8_t>>> &extra_data_list) {
  int rc = 0;

  // send image and protobuf
  if (mode_ == ZmqMode::PUSHPULL) {
    int send_flag = ZMQ_SNDMORE;
    if (img_data_list.empty() && extra_data_list.empty()) {
      send_flag = 0;
    }
    zmq_msg_t meta_msg;
    // meta
    {
      zmq_msg_init_size(&meta_msg, meta_size);
      memcpy(zmq_msg_data(&meta_msg), meta, meta_size);
      rc = SendSingleMsg(data_send_, &meta_msg, send_flag, resend_flag_);
      if (rc == -1) {
        std::cout << "send single msg failed";
        return false;
      }
    }
    // image
    auto image_count = img_data_list.size();
    std::vector<zmq_msg_t> image_msgs;
    image_msgs.resize(image_count);
    for (size_t i = 0; i < image_count; ++i) {
      if (!img_data_list[i]) {
        std::cout << "image data is null " << i;
        return false;
      }
      zmq_msg_init_size(&image_msgs[i], img_size_list[i]);
      memcpy(zmq_msg_data(&image_msgs[i]), img_data_list[i], img_size_list[i]);
      // last image
      if (i == (image_count - 1) && extra_data_list.empty()) {
        send_flag = 0;
      }
      rc = SendSingleMsg(data_send_, &image_msgs[i], send_flag, resend_flag_);
      if (rc == -1) {
        std::cout << "send single msg failed";
        return false;
      }
    }

    // extra_data
    auto extra_count = extra_data_list.size();
    std::vector<zmq_msg_t> extra_msgs;
    extra_msgs.resize(extra_count);
    for (size_t i = 0; i < extra_data_list.size(); i++) {
      auto extra_size = extra_data_list[i]->size();
      zmq_msg_init_size(&extra_msgs[i], extra_size);
      memcpy(zmq_msg_data(&extra_msgs[i]), extra_data_list[i]->data(),
             extra_size);
      // last extra
      if (i == (extra_count - 1)) {
        send_flag = 0;
      }

      rc = SendSingleMsg(data_send_, &extra_msgs[i], send_flag, resend_flag_);
      if (rc == -1) {
        std::cout << "send single extra msg failed";
        return false;
      }
    }
  } else if (mode_ == ZmqMode::REQREP) {
    // calculate frame_msg size first and then copy the data to it
    int size = 0;
    size += sizeof(int) + meta_size;
    auto image_count = img_data_list.size();
    if (image_count > 0) {
      size += sizeof(int);
      for (size_t i = 0; i < image_count; ++i) {
        size += sizeof(int);
        size += img_size_list[i];
      }
    }
    auto extra_count = extra_data_list.size();
    if (extra_count > 0) {
      size += sizeof(int);
      for (size_t i = 0; i < extra_count; i++) {
        auto extra_size = extra_data_list[i]->size();
        size += sizeof(int);
        size += static_cast<int>(extra_size);
      }
    }

    zmq_msg_t frame_msg;
    zmq_msg_init_size(&frame_msg, size);
    uint8_t *frame_data = reinterpret_cast<uint8_t *>(zmq_msg_data(&frame_msg));

    int offset = 0;
    // meta
    memcpy(frame_data, &meta_size, sizeof(int));
    offset += sizeof(int);
    memcpy(frame_data + offset, meta, meta_size);
    offset += meta_size;

    // image
    if (image_count > 0) {
      memcpy(frame_data + offset, &image_count, sizeof(int));
      offset += sizeof(int);
      for (size_t i = 0; i < image_count; i++) {
        if (img_data_list[i] == nullptr) {
          std::cout << "image data is null " << i;
          return false;
        }
        memcpy(frame_data + offset, &(img_size_list[i]), sizeof(int));
        offset += sizeof(int);
        memcpy(frame_data + offset, img_data_list[i], img_size_list[i]);
        offset += img_size_list[i];
      }
    }

    // extra_data
    if (extra_count > 0) {
      memcpy(frame_data + offset, &extra_count, sizeof(int));
      offset += sizeof(int);
      for (size_t i = 0; i < extra_count; i++) {
        int extra_size = static_cast<int>(extra_data_list[i]->size());
        memcpy(frame_data + offset, &extra_size, sizeof(int));
        offset += sizeof(int);
        memcpy(frame_data + offset, extra_data_list[i]->data(), extra_size);
        offset += extra_size;
      }
    }

    SendDataMsg(frame_msg);
    zmq_msg_close(&frame_msg);

    zmq_msg_t reply_msg;
    zmq_msg_init(&reply_msg);

    // zmq timeout can only be set before zmq connect
    // int recv_to = 1000;  // 1s
    // rc = zmq_setsockopt(data_send_, ZMQ_RCVTIMEO, &recv_to, sizeof(int));

    zmq_setsockopt(data_send_, ZMQ_RCVTIMEO, &recv_timeout_, sizeof(int));
    rc = zmq_msg_recv(&reply_msg, data_send_, 0);
    zmq_msg_close(&reply_msg);
    if (rc == -1) {
      std::cout << "recv reply msg failed";
      return false;
    }
  } else { // end REQREP
           // do nothing
  }
  return true;
}

struct SendingInfo {
  int image_data_count = 0;
  int extra_data_count = 0;
};

void ReleaseSendPackMsg(void *, void *hint) {
  auto *pack_info_msg = static_cast<SendingInfo *>(hint);
  delete pack_info_msg;
}

void NetworkSender::set_full_data_mode(bool full_data_mode) {
  full_data_mode_ = full_data_mode;
}

void NetworkSender::set_camera_en(std::vector<int> camera_en) {
  camera_en_ = std::move(camera_en);
}

const std::vector<int> &NetworkSender::camera_en() const { return camera_en_; }

bool NetworkSender::GetReply(int *flag_next_send /* = nullptr*/) {
  zmq_msg_t reply_msg;
  const auto ret = WaitForReply(data_send_, -1, &reply_msg);
  if (!ret) {
    zmq_msg_close(&reply_msg);
    std::cout << "Recv reply msg failed";
    return false;
  }

  if (flag_next_send) {
    *flag_next_send = -1;
    if (zmq_msg_size(&reply_msg) >= sizeof(int)) {
      *flag_next_send = *static_cast<int *>(zmq_msg_data(&reply_msg));
    }
  }
  zmq_msg_close(&reply_msg);
  return true;
}

bool NetworkSender::WaitForReply(void *socket_recv) const {
  if (!socket_recv) {
    return false;
  }

  zmq_msg_t reply_msg;
  const auto ret = WaitForReply(socket_recv, recv_timeout_, &reply_msg);
  if (!ret) {
    std::cout << "Recv reply msg failed";
  }
  zmq_msg_close(&reply_msg);
  return ret;
}

bool NetworkSender::WaitForReply(void *socket_recv, int timeout,
                                 zmq_msg_t *msg) const {
  if (!socket_recv || !msg) {
    return false;
  }
  zmq_msg_init(msg);
  zmq_setsockopt(socket_recv, ZMQ_RCVTIMEO, &timeout, sizeof(int));
  return zmq_msg_recv(msg, socket_recv, 0) >= 0;
}

bool NetworkSender::SendFrame(const uint8_t *data, int data_size) {
  int send_flag = 0;
  zmq_msg_t meta_msg;
  int rc;
  zmq_msg_init_size(&meta_msg, data_size);
  memcpy(zmq_msg_data(&meta_msg), data, data_size);
  rc = SendSingleMsg(data_send_, &meta_msg, send_flag, resend_flag_);
  if (rc == -1) {
    std::cout << "send single msg failed " << std::endl;
    return false;
  }
  return true;
}

bool NetworkSender::SendDataNoBlock(const uint8_t *data, int data_size) {
  int send_flag = 0;
  zmq_msg_t meta_msg;
  int rc;
  zmq_msg_init_size(&meta_msg, data_size);
  memcpy(zmq_msg_data(&meta_msg), data, data_size);

  //        rc = zmq_msg_send(&meta_msg, data_send_, send_flag);
  SendDataMsg(meta_msg);
  if (rc < 0) {
    std::cout << "send msg failed, ZMQ Get Error " << strerror(errno);
    return false;
  }
  return true;
}

bool NetworkSender::SendCommand(int cmd) {
  zmq_msg_t msg;
  zmq_msg_init_size(&msg, sizeof(int));
  *(reinterpret_cast<int *>(zmq_msg_data(&msg))) = cmd;
  const int rc = SendSingleMsg(cmd_send_, &msg, 0, resend_flag_);
  zmq_msg_close(&msg);
  if (rc != 0) {
    return false;
  }

  if (mode_ == ZmqMode::REQREP) {
    if (!WaitForReply(cmd_send_)) {
      return false;
    }
  }

  return true;
}

void NetworkSender::ResetDataSender() {
  zmq_close(data_send_);

  if (mode_ == ZmqMode::REQREP) {
    data_send_ = zmq_socket(context_, ZMQ_REQ);
    int recv = 5000;
    zmq_setsockopt(data_send_, ZMQ_RCVTIMEO, &recv, sizeof(int));
  } else if (mode_ == ZmqMode::PUSHPULL) {
    data_send_ = zmq_socket(context_, ZMQ_PUSH);
  } else if (mode_ == ZmqMode::PUBSUB) {
    data_send_ = zmq_socket(context_, ZMQ_PUB);
    int send_timeout = 5000;
    zmq_setsockopt(data_send_, ZMQ_SNDTIMEO, &send_timeout, sizeof(int));
  } else {
    data_send_ = zmq_socket(context_, ZMQ_REQ);
    int recv = 5000;
    zmq_setsockopt(data_send_, ZMQ_RCVTIMEO, &recv, sizeof(int));
  }

  int zmq_sndhwm = SEND_QUEUE_SIZE; // message list size
  zmq_setsockopt(data_send_, ZMQ_SNDHWM, &zmq_sndhwm, sizeof(int));
  // zmq_setsockopt(data_send_, ZMQ_RCVHWM, &zmq_sndhwm, sizeof(int));
  int zmq_sndbuf = SEND_BUF_SIZE;
  zmq_setsockopt(data_send_, ZMQ_SNDBUF, &zmq_sndbuf, sizeof(int));

  int to = send_timeout_;
  zmq_setsockopt(data_send_, ZMQ_LINGER, &to, sizeof(to));
  zmq_setsockopt(data_send_, ZMQ_SNDTIMEO, &to, sizeof(to));

  int con_ret = 0;
  if (mode_ == ZmqMode::PUBSUB) {
    con_ret = zmq_bind(data_send_, end_point_.c_str());
    if (con_ret < 0) {
      std::cout << "tcp bind failed: " << zmq_strerror(errno);
    }
  } else {
    int con_ret = zmq_connect(data_send_, end_point_.c_str());
    if (con_ret < 0) {
      std::cout << "connect data failed.";
    }
  }
}
} // end of namespace HobotADAS
