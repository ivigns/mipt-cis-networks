#include <fstream>
#include <iostream>

#include "protocol.hpp"

struct Args {
  size_t stations_count{};
  std::string payload_file_path{};
  std::optional<std::chrono::milliseconds> tick_delay{};
};

Args ParseArgs(int argc, char** argv) {
  if (argc < 5) {
    throw std::invalid_argument("");
  }

  std::optional<size_t> stations_count;
  std::optional<std::string> payload_file_path;
  std::optional<std::chrono::milliseconds> tick_delay;
  for (int i = 1; i < argc; i += 2) {
    if (std::string(argv[i]) == "-N") {
      stations_count = std::stoul(argv[i + 1]);
    } else if (std::string(argv[i]) == "-f") {
      payload_file_path = argv[i + 1];
    } else if (std::string(argv[i]) == "-s") {
      tick_delay = std::chrono::milliseconds(std::stoul(argv[i + 1]));
    } else {
      throw std::invalid_argument("");
    }
  }

  if (!stations_count || !payload_file_path) {
    throw std::invalid_argument("");
  }
  return {*stations_count, *payload_file_path, tick_delay};
}

std::vector<csma_cd::Payload> LoadPayloadFromFile(
    const std::string& payload_file_path) {
  std::ifstream payload_file(payload_file_path);
  std::vector<csma_cd::Payload> payload;

  while (!payload_file.eof()) {
    std::string src_id;
    std::string dst_id;
    payload_file >> src_id >> dst_id;

    std::string data(1600, 0);
    payload_file.getline(data.data(), data.size());
    data.erase(data.begin(), std::find_if(data.begin(), data.end(), [](char c) {
                 return !std::isspace(c);
               }));
    if (data.size() > 1500) {
      throw std::invalid_argument(
          "Bad payload: data length must be less than 1500");
    }

    payload.push_back({std::stoul(src_id), std::stoul(dst_id), data});
  }

  return payload;
}

int main(int argc, char** argv) {
  /*Args args;
  try {
    args = ParseArgs(argc, argv);
  } catch (std::invalid_argument&) {
    std::cerr << "Usage:\t" << argv[0] << " -N <stations count> "
              << "-f <path to file with payload> "
              << "-s <tick delay in ms>" << std::endl;
    return 1;
  }

  try {
    csma_cd::Ethernet ethernet(args.stations_count,
                               LoadPayloadFromFile(args.payload_file_path),
                               args.tick_delay, &std::cout);
  } catch (std::invalid_argument& exc) {
    std::cerr << exc.what();
    return 2;
  }
  */

  std::string a;
  std::cin >> a;
  std::string line(200, 0);
  std::cin.getline(line.data(), line.size());
  std::cout << a << "\n" << line;

  return 0;
}
