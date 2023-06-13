/*
 * CopyRight:	none
 * FileName:	zmq_sender.h
 * Desc:
 * Author:		Conscien
 * Date:		2023/6/13
 * Contact:		wlovelxcc@gmail.com
 * History:		Last update by Conscien 2023/6/13
 * */

#ifndef ZMQTEST_ZMQ_SENDER_H
#define ZMQTEST_ZMQ_SENDER_H

#include <memory>
#include <string>
#include <zmq.h>

namespace CONSCIEN {

class NetworkSender {
public:
  enum { COMMAND_NONE = 0, COMMAND_RESET = 1, COMMAND_RESET_FAST = 2 };
  enum class ZmqMode { REQREP = 0, PUSHPULL = 1, PUBSUB = 2 };

  NetworkSender();

  ~NetworkSender();

  bool Init(const char *end_point);

  void Fini();

  bool SendFrame(const std::vector<uint8_t *> &img_data_list,
                 const std::vector<int> &img_size_list, const uint8_t *meta,
                 int meta_size,
                 const std::vector<std::shared_ptr<std::vector<uint8_t>>>
                     &extra_data_list);

  bool SendFrame(const uint8_t *data, int data_size);

  bool SendDataNoBlock(const uint8_t *data, int data_size);

  bool SendCommand(int cmd);

  void SendDataMsg(zmq_msg_t &msg); // with data_send_

  void ResetDataSender();

  void SetReSend(const bool &flag);

  void SetZmqMode(const ZmqMode &mode) { mode_ = mode; }

  ZmqMode GetZmqMode() const { return mode_; }

  void SetSendTimeout(int timeout) { send_timeout_ = timeout; }

  int GetSendTimeout() const { return send_timeout_; }

  void set_full_data_mode(bool full_data_mode);

  void set_camera_en(std::vector<int> camera_en);

  const std::vector<int> &camera_en() const;

  bool GetReply(int *flag_next_send = nullptr);

private:
  bool WaitForReply(void *socket_recv) const;

  bool WaitForReply(void *socket_recv, int timeout, zmq_msg_t *msg) const;

  void *context_;
  void *data_send_;
  void *cmd_send_;
  std::string end_point_;
  bool resend_flag_;
  ZmqMode mode_;
  int send_timeout_; // ms
  int recv_timeout_; // ms
  bool full_data_mode_;
  std::vector<int> camera_en_;
};
} // namespace CONSCIEN

#endif // ZMQTEST_ZMQ_SENDER_H
