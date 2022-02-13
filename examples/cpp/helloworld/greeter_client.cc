/*
 *
 * Copyright 2015 gRPC authors.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <iostream>
#include <memory>
#include <string>
#include <vector>

#include "file_reader.h"
#include "metrics.h"
#include "timer.h"

#include <grpcpp/grpcpp.h>
#ifdef BAZEL_BUILD
#include "examples/protos/helloworld.grpc.pb.h"
#else
#include "helloworld.grpc.pb.h"
#endif

using grpc::Channel;
using grpc::ClientContext;
using grpc::Status;
using helloworld::Greeter;
using helloworld::HelloReply;
using helloworld::HelloRequest;

class GreeterClient {
 public:
  GreeterClient(std::shared_ptr<Channel> channel)
      : stub_(Greeter::NewStub(channel)) {}

  // Assembles the client's payload, sends it and presents the response back
  // from the server.
  std::string SayHello(const std::string& user) {
    // Data we are sending to the server.
    HelloRequest request;
    request.set_name(user);

    // Container for the data we expect from the server.
    HelloReply reply;

    // Context for the client. It could be used to convey extra information to
    // the server and/or tweak certain RPC behaviors.
    ClientContext context;

    // The actual RPC.
    Status status = stub_->SayHello(&context, request, &reply);

    // Act upon its status.
    if (status.ok()) {
      return reply.message();
    } else {
      std::cout << status.error_code() << ": " << status.error_message()
                << std::endl;
      return "RPC failed";
    }
  }

 private:
  std::unique_ptr<Greeter::Stub> stub_;
};

std::string generateString(int n) {
  std::string s = "";
  for (int i = 0; i < n; i++) s += (char)('a' + rand() % 26);

  return s;
}

// Metrics GlobalRequestMetric;
std::vector<int> packet_sizes = {
    64,    128,   256,   512,   1024,  2048,  4096,  8192,  16384, 20000,
    25000, 30000, 32768, 35000, 40000, 45000, 50000, 55000, 60000, 65000};
int main(int argc, char** argv) {
  // Instantiate the client. It requires a channel, out of which the actual RPCs
  // are created. This channel models a connection to an endpoint specified by
  // the argument "--target=" which is the only expected argument.
  // We indicate that the channel isn't authenticated (use of
  // InsecureChannelCredentials()).
  std::string target_str;
  Timer timer;
  std::string arg_str("--target");
  if (argc > 1) {
    std::string arg_val = argv[1];
    size_t start_pos = arg_val.find(arg_str);
    if (start_pos != std::string::npos) {
      start_pos += arg_str.size();
      if (arg_val[start_pos] == '=') {
        target_str = arg_val.substr(start_pos + 1);
      } else {
        std::cout << "The only correct argument syntax is --target="
                  << std::endl;
        return 0;
      }
    } else {
      std::cout << "The only acceptable argument is --target=" << std::endl;
      return 0;
    }
  } else {
    target_str = "localhost:50053";
  }
  GreeterClient greeter(
      grpc::CreateChannel(target_str, grpc::InsecureChannelCredentials()));

  // std::string header = "packet_size(in
  // bytes),req_oh_min,req_oh_max,req_oh_mean,req_oh_med,req_oh_std_dev,rep_oh_min,rep_oh_max,rep_oh_mean,rep_oh_med,rep_oh_std_dev,rtt_min,rtt_max,rtt_mean,rtt_med,rtt_std_dev,bandwidth
  // (MB/s)\n";
  std::string header =
      "packet_size(in "
      "bytes),req_oh_min,req_oh_max,req_oh_mean,req_oh_med,req_oh_std_dev,rtt_"
      "min,rtt_max,rtt_mean,rtt_med,rtt_std_dev,bandwidth (MB/s)\n";

  FileReader file_reader("helloworld.csv");
  file_reader.write_to_file(header);
  // Try to generate Marshelling from variable string Length
  std::string user("world");
  int len = packet_sizes.size();
  for (int l = 0; l < len; l++) {
    Metrics metric;
    GlobalRequestMetric = metric;
    // GlobalReplyMetric = metric;
    double bandwidth = 0.0;
    int iterations = 10000;
    double time_accumulator = 0.0;
    for (int i = 1; i <= iterations; i++) {
      user = generateString(packet_sizes[l]);
      timer.start();
      std::string reply = greeter.SayHello(user);
      timer.stop();
      metric.add(timer.get_time_in_nanoseconds());
      time_accumulator += timer.get_time_in_nanoseconds();
      // std::cout<<"RTT: "<<timer.get_time_in_nanoseconds()<<std::endl;
      // std::cout << "Greeter received: " << reply << std::endl;
    }
    bandwidth = 1e3 * ((packet_sizes[l] * iterations) / time_accumulator);
    std::cout << "String Len: " << packet_sizes[l] << std::endl;
    // metric.pretty_print();
    std::string fileString = "";
    fileString += (to_string(packet_sizes[l]) + ",");
    fileString += (GlobalRequestMetric.get_metrics() + ",");
    // fileString+=(GlobalReplyMetric.get_metrics()+",");
    fileString += (metric.get_metrics() + ",");
    fileString += (to_string(bandwidth) + "\n");

    file_reader.write_to_file(fileString, true);
    // std::cout<<"Serialize"<<std::endl;
    // GlobalRequestMetric.pretty_print();
  }
  return 0;
}
