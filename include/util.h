#pragma once

#include <filesystem>
#include <iostream>
#include <cstdlib>
#include <fstream>
#include <algorithm>
#include <chrono>
#include <string>
#include <ctime>
#include <cassert>

using namespace std::chrono;

using Data = std::vector<std::string>;

Data readFile(std::string filePath) {
  std::ifstream in(filePath);
  assert(in.is_open());
  Data ret;
  std::string line;
  while (std::getline(in, line)) {
    ret.push_back(line);
  }
  return ret;
}

std::string pretty(std::size_t size) {
  if (size < 1e3) return std::to_string(size);
  if (size < 1e6) return std::to_string(size / 1000) + "K";
  if (size < 1e9) return std::to_string(size / 1000000) + "M";
  return std::to_string(size);
}

void warning(const char* str) {
  std::cerr << str << std::endl;
}

void error(const char* str, std::string tmp) {
  std::cerr << std::string(str).replace(std::string(str).find("..."), 3, tmp) << std::endl;
  assert(0);
}

class Timer {
public:
  Timer(std::string approach, std::string pattern, bool startByDefault = true)
  : approach(approach),
    pattern(pattern),
    calledStop(false) {
      if (startByDefault)
        start();
    }

  void start() {
    start_ = ::high_resolution_clock::now();
  }

  void stop() {
    assert(!calledStop);
    calledStop = true;
    stop_ = ::high_resolution_clock::now();
  }

  void flush() {
    auto start_dir = "experiments/";
    std::string filename = start_dir
                         + approach
                         + "-"
                         + std::to_string(pattern.size())
                         + "-"
                         + pattern
                         + ".out";
    std::ofstream out(filename);
    // assert(out.is_open());
    out << "Approach: " << approach << std::endl;
    out << "Pattern: " << pattern << std::endl;
    out << "Total: " << duration_cast<milliseconds>(stop_ - start_).count() << " ms " << std::endl;
  }

  void debug() {
    auto total = duration_cast<milliseconds>(stop_ - start_).count();
    std::cout << "approach=" << approach
              << ", pattern=" << pattern
              << ", time=" << total << " ms" << std::endl;
  }

private:
  std::string approach;
  std::string pattern;
  bool calledStop;
  high_resolution_clock::time_point start_, stop_;
};