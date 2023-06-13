/*
 * CopyRight:	none
 * FileName:	zmqtest.cpp
 * Desc:
 * Author:		Conscien
 * Date:		2023/6/13
 * Contact:		wlovelxcc@gmail.com
 * History:		Last update by Conscien 2023/6/13
 * */

#include "zmq_receiver.h"
#include "zmq_sender.h"
#include <iostream>
#include <thread>

using namespace CONSCIEN;

std::unique_ptr<NetworkReceiver> network_receiver_ =
    std::make_unique<NetworkReceiver>();

std::unique_ptr<NetworkSender> network_sender_ =
    std::make_unique<NetworkSender>();

std::unique_ptr<std::thread> th_zmq_ = nullptr;

void zmq_sender_module() {
  std::string endpoint;
  int port = 8086;
  int zmq_mode = 2;
  if (zmq_mode == 2) {
    network_sender_->SetZmqMode(NetworkSender::ZmqMode::PUBSUB);
    std::string ip = "127.0.0.1";
    endpoint = "tcp://" + ip + ":" + std::to_string(port);
  } else {
    network_sender_->SetZmqMode(NetworkSender::ZmqMode::REQREP);
    std::string ip = "127.0.0.1";
    endpoint = "tcp://" + ip + ":" + std::to_string(port);
    //        endpoint = "tcp://*:" + std::to_string(port);
  }
  if (!network_sender_->Init(endpoint.c_str())) {
    std::cout << "ERROR: REQREP MODE...Port " << endpoint << " Conflict!!!!!";
    return;
  }

  auto funThd = []() -> int {
    while (true) {
      std::string send_data = "我是傻逼";
      //            bool send_frame =
      //            network_sender_->SendDataNoBlock(reinterpret_cast<uint8_t*>(send_data.data()),
      //            send_data.length());
      bool send_frame = network_sender_->SendFrame(
          reinterpret_cast<uint8_t *>(send_data.data()), send_data.length());
      std::cout << "send frame thread..." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    return 0;
  };

  th_zmq_ = std::make_unique<std::thread>(funThd);

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  if (network_sender_) {
    network_sender_->Fini();
  }
}

void zmq_recevie_module() {
  std::string endpoint;
  int port = 8086;
  int zmq_mode = 2;
  if (zmq_mode == 2) {
    network_receiver_->SetZmqMode(NetworkReceiver::ZmqMode::PUBSUB);
    std::string ip = "127.0.0.1";
    endpoint = "tcp://" + ip + ":" + std::to_string(port);
  } else {
    network_receiver_->SetZmqMode(NetworkReceiver::ZmqMode::REQREP);
    endpoint = "tcp://*:" + std::to_string(port);
  }
  if (!network_receiver_->Init(endpoint.c_str())) {
    std::cout << "ERROR: REQREP MODE...Port " << endpoint << " Conflict!!!!!";
    return;
  }

  auto funThd = []() -> int {
    while (true) {
      bool get_frame = network_receiver_->RecvNetworkInput();
      auto datFrame = network_receiver_->RecvParamDatFrame();
      if (get_frame) {
        std::cout << "get frame success :" << std::endl;
      }
      std::cout << "get frame thread..." << std::endl;
      std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }

    return 0;
  };

  th_zmq_ = std::make_unique<std::thread>(funThd);

  while (true) {
    std::this_thread::sleep_for(std::chrono::milliseconds(5));
  }

  if (network_receiver_) {
    network_receiver_->Finish();
  }
}

int main() {
  std::cout << "Hello zmq Demo~~" << std::endl;
  zmq_recevie_module();
  //    zmq_sender_module();
  return 0;
}