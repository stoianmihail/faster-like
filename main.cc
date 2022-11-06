#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>

#include "include/fft.h"
#include "include/util.h"

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

unsigned match(std::string& str, std::string& pattern) {
  if (str.size() < pattern.size())
    return 0;
  unsigned count = 0;
  for (unsigned i = 0, n = str.size(), m = pattern.size(); i != n - m + 1; ++i) {
    bool match = true;
    for (unsigned j = 0; j != m; ++j)
      match = match && ((str[i + j] == pattern[j]) || (pattern[j] == '_'));
    count += match;
  }
  return count;
}

VI naive_like(Data& data, std::string pattern) {
  Timer timer("naive_like");
  unsigned m = pattern.size();
  VI ret;
  for (unsigned index = 0, limit = data.size(); index != limit; ++index) {
    ret.push_back(match(data[index], pattern));
  }
  timer.stop();
  timer.debug();
  return ret;
}

template <bool check>
VI like(Data& data, std::string pattern, unsigned bound = 0) {
  Timer timer("like");
  unsigned size = pattern.size();
  unsigned lg = FFT::computeLog(4 * size - 1);

  // Setup FFTs.
  FFT pt(4 * size - 1, lg);
  FFT pt2(4 * size - 1, lg, &pt);

  // Init the patterns.
  pt.init<true>(pattern, 0, pattern.size(), [](unsigned c) { return c; });
  pt2.init<true>(pattern, 0, pattern.size(), [](unsigned c) { return c * c; });

  // Precompute \sum_{i=0}^{m - 1} P_i^3.
  int patternCheck = 0;
  for (auto c : pattern) {
    patternCheck += (c == '_') ? 0 : FFT::pow3(encode(c));
  }

  // Transform to frequency domain.
  pt.transform();
  pt2.transform();

  // Setup the FFTs for the slices.
  FFT st(4 * size - 1, lg);
  FFT st2(4 * size - 1, lg, &st);

  // Used for iterations.
  VI *curr = new VI(4 * size - 1);

  // Solve for a single string.
  auto solve = [&](std::string& str) {
    auto matchCount = 0;
    unsigned index = 0, limit = str.size();

    bool force = false;
    while (!force) {
      // Compute the next bound.
      auto bound = index + 2 * size;

      // Does the bound exceed the string size? Then force a match.
      if (bound > limit) {
        bound = limit;
        force = true;
      }

      // Init FFT.
      st.init(str, index, bound, [](unsigned c) { return c; });
      st2.init(str, index, bound, [](unsigned c) { return c * c; });

      // Transform to frequency domain.
      st.transform();
      st2.transform();

      // Perform convolutions. Note: the order matters, as first we call `convolve<true>`.
      st.convolve<true>(pt2, curr, [](int x) { return 2 * x; });
      st2.convolve<false>(pt, curr, [](int x) { return -x; });

      // Compute the number of matches.
      for (unsigned ptr = size - 1; ptr != 3 * size; ++ptr)
        matchCount += (curr->at(ptr) == patternCheck);

      // And clear the transforms before using them again.
      st.clear();
      st2.clear();

      // Increase the current index.
      index += size + 1;
    }
    return matchCount;
  };

  VI checker;
  for (unsigned index = 0, limit = data.size(); index != limit; ++index) {
    auto ret = solve(data[index]);
    if (check) {
#if 0
      std::cerr << "sol => " << ret << std::endl;
      auto actual = match(data[index], pattern);
      std::cerr << "actual => " << actual << std::endl;
      assert(ret == actual);
#endif
      checker.push_back(ret);
    }
  }
  timer.stop();
  timer.debug();
  return checker;
}

template <bool check>
VI faster_like(Data& data, std::string pattern, unsigned bound = 0) {
  Timer timer("faster_like");
  unsigned size = pattern.size();
  unsigned lg = FFT::computeLog(2 * size - 1);

  // Setup FFTs.
  FFT pt(2 * size - 1, lg);
  FFT pt2(2 * size - 1, lg, &pt);

  // Init the patterns.
  pt.init<true>(pattern, 0, pattern.size(), [](unsigned c) { return c; });
  pt2.init<true>(pattern, 0, pattern.size(), [](unsigned c) { return c * c; });

  // Precompute \sum_{i=0}^{m - 1} P_i^3.
  int patternCheck = 0;
  for (auto c : pattern) {
    patternCheck += (c == '_') ? 0 : FFT::pow3(encode(c));
  }

  // Transform to frequency domain.
  pt.transform();
  pt2.transform();

  auto debug = [&](VI& ret) {
    for (unsigned index = 0, limit = ret.size(); index != limit; ++index)
      std::cerr << "index=" << index << " elem=" << ret[index] << std::endl;
  };

  // Setup the FFTs for the slices.
  FFT st(2 * size - 1, lg);
  FFT st2(2 * size - 1, lg, &st);

  VI *prev, *curr;
  prev = new VI(2 * size - 1);
  curr = new VI(2 * size - 1);

  // Solve for a single string.
  auto solve = [&](std::string& str) {
    auto matchCount = 0;
    unsigned index = 0, limit = str.size();

    bool first = true, force = false;
    while (!force) {
      // Compute the next bound.
      auto bound = index + size;

      // Does the bound exceed the string size? Then force a match.
      if (bound > limit) {
        bound = limit;
        force = true;
      }

      // Init FFT.
      st.init(str, index, bound, [](unsigned c) { return c; });
      st2.init(str, index, bound, [](unsigned c) { return c * c; });

      // Transform to frequency domain.
      st.transform();
      st2.transform();

      // Perform convolutions. Note: the order matters, as first we call `convolve<true>`.
      st.convolve<true>(pt2, curr, [](int x) { return 2 * x; });
      st2.convolve<false>(pt, curr, [](int x) { return -x; });

      // Reuse results from last iteration.
      if (!first)
        for (unsigned ptr = 0; ptr != size - 1; ++ptr)
          curr->at(ptr) += prev->at(size + ptr);

      // Compute the number of matches.
      for (unsigned ptr = first ? (size - 1) : 0; ptr != size; ++ptr)
        matchCount += (curr->at(ptr) == patternCheck);

      // Swap `curr` and `prev`.
      std::swap(curr, prev);
      first = false;

      // And clear the transforms before using them again.
      st.clear();
      st2.clear();

      // Increase the current index.
      index += size;
    }
    return matchCount;
  };

  VI checker;
  for (unsigned index = 0, limit = data.size(); index != limit; ++index) {
    auto ret = solve(data[index]);
    if (check) {
#if 0
      std::cerr << "sol => " << ret << std::endl;
      auto actual = match(data[index], pattern);
      std::cerr << "actual => " << actual << std::endl;
      assert(ret == actual);
#endif
      checker.push_back(ret);
    }
  }
  timer.stop();
  timer.debug();
  return checker;
}

void same(VI& a, VI& b) {
  assert(a.size() == b.size());
  for (unsigned index = 0, limit = a.size(); index != limit; ++index) {
    assert(a[index] == b[index]);
  }
}

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
  if (approach == "standard") {
    auto ret1 = check ? like<true>(data, pattern, check) : like<false>(data, pattern, check);
    if (!check)
      return 0;
    auto ret2 = naive_like(data, pattern);
    same(ret1, ret2);
  } else if (approach == "faster") {
    auto ret1 = check ? faster_like<true>(data, pattern, check) : faster_like<false>(data, pattern, check);
    if (!check)
      return 0;
    auto ret2 = naive_like(data, pattern);
    same(ret1, ret2);
  } else {
    assert(0);
  }
  return 0;
}