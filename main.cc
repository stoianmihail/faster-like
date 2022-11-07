#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>

#include "include/like.h"

int main(int argc, char** argv) {
  // Example: ./main ../attribute.in ly__pe__al 1
  if ((argc != 4) && (argc != 5)) {
    std::cerr << "Usage: " << argv[0] << " <file> <pattern> <approach['standard', 'faster']> [<check:bool>]" << std::endl;
    exit(-1);
  }

  auto filePath = std::string(argv[1]);
  auto pattern = std::string(argv[2]);
  auto approach = std::string(argv[3]);
  auto check = (argc == 5) ? std::stoi(argv[4]) : 0;

  auto data = readFile(filePath);
  auto exp = Like(data, pattern, check);

  if (approach == "standard") {
    auto ret1 = exp.run<Type::StandardParallel>();
    if (!check)
      return 0;
    auto ret2 = exp.run<Type::Naive>();
    exp.cmp(ret1, ret2);
  } else if (approach == "faster") {
    auto ret1 = exp.run<Type::FasterParallel>();
    if (!check)
      return 0;
    auto ret2 = exp.run<Type::Naive>();
    exp.cmp(ret1, ret2);
  } else if (approach == "double") {
    auto ret1 = exp.run<Type::DoubleParallel>();
    if (!check)
      return 0;
    auto ret2 = exp.run<Type::Naive>();
    exp.cmp(ret1, ret2);
  } else {
    assert(0);
  }
  return 0;
}