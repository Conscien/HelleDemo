/*
 * CopyRight:	none
 * FileName:	zmq_receiver.h
 * Desc:
 * Author:		Conscien
 * Date:		2023/6/13
 * Contact:		wlovelxcc@gmail.com
 * History:		Last update by Conscien 2023/6/13
 * */

#ifndef ZMQTEST_ZMQ_RECEIVER_H
#define ZMQTEST_ZMQ_RECEIVER_H

#include <atomic>
#include <condition_variable>
#include <list>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

namespace CONSCIEN {
class NetworkReceiver {
public:
  enum class ZmqMode { REQREP = 0, PUSHPULL = 1, PUBSUB = 2 };

  bool Init(const char *end_point);

  void ZMQReconnect();

  void Finish();

  void SetZmqMode(const ZmqMode &mode) { mode_ = mode; }

  bool RecvNetworkInput();

  std::shared_ptr<uint8_t> RecvMetaDatFrame();

  std::shared_ptr<uint8_t> RecvImageDatFrame();

  std::shared_ptr<uint8_t> RecvParamDatFrame();

  std::shared_ptr<uint8_t> RecvDatFrame(const std::string &name);

private:
  bool ReplyRequest();

  bool HasCacheFrame();

  static std::string PARAM_FRAME_NAME;
  static std::string META_FRAME_NAME;
  static std::string IMAGE_FRAME_NAME;

  ZmqMode mode_{ZmqMode::PUBSUB};
  std::string end_point_{};
  bool inited_{false};
  void *context_{nullptr};
  void *data_recv_{nullptr};
  void *cmd_recv_{nullptr};
  int recv_to_{1000};
  int recv_linger_{1000};
  std::atomic_bool reset_flag_{false};

  std::mutex mutex_recv_;
  std::condition_variable cond_recv_;

  typedef std::list<std::shared_ptr<uint8_t>> CacheList;
  std::map<std::string, CacheList> frame_cache_map_;
};
} // end of namespace CONSCIEN

#endif // ZMQTEST_ZMQ_RECEIVER_H
