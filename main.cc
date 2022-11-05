#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>

#include "include/fft.h"

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

unsigned like(Data& data, std::string pattern) {
  return 0;
}

unsigned faster_like(Data& data, std::string pattern) {
  unsigned size = pattern.size();
  unsigned lg = FFT::computeLog(2 * size - 1);
  FFT pt(2 * size - 1, lg), pt2(2 * size - 1, lg, &pt);
  pt.init(pattern, 0, pattern.size(), [](unsigned c) { return c; });
  pt2.init(pattern, 0, pattern.size(), [](unsigned c) { return c * c; });

  // TODO: we can reuse transform of pattern for pattern^2?
  std::cerr << "Starting transforms.." << std::endl;
  pt.transform();
  pt2.transform();

  FFT st(2 * size - 1, lg), st2(2 * size - 1, lg, &st);
  auto solve = [&](std::string& str) {
    unsigned index = 0, limit = str.size();
    while (index + size <= limit) {
      // Init the transforms.
      std::cerr << "Slice: [" << index << ", " << (index + size) << "[" << std::endl;
      st.init(str, index, index + size, [](unsigned c) { return c; });
      st2.init(str, index, index + size, [](unsigned c) { return c * c; });
      
      // Perform convolutions.
      st.convolve(pt2);
      st2.convolve(pt);

      // And clear the transforms before using them again.
      st.clear();
      st2.clear();
      
      // Increase the current index.
      index += size - 1;
    }
    return 1;
  };

  unsigned count = 0;
  for (unsigned index = 0, limit = data.size(); index != limit; ++index) {
    std::cerr << "str: " << data[index] << std::endl;
    count += solve(data[index]);
  }
  std::cerr << "Finish!" << std::endl;
  return count;
}

int main(int argc, char** argv) {
  // Example: ./main ../attribute.in ly__pe__al 1
  if (argc != 4) {
    std::cerr << "Usage: " << argv[0] << " <file> <pattern> <approach['standard', 'faster']>" << std::endl;
    exit(-1);
  }

  auto filePath = std::string(argv[1]);
  auto pattern = std::string(argv[2]);
  auto approach = std::string(argv[3]);

  auto data = readFile(filePath);
  if (approach == "standard") {
    like(data, pattern);
  } else if (approach == "faster") {
    faster_like(data, pattern);
  } else {
    assert(0);
  }
  return 0;
}