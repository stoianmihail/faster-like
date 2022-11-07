#include <vector>
#include <cassert>
#include <iostream>
#include <fstream>
#include <atomic>
#include <thread>
#include <mutex>

#include "fft.h"
#include "util.h"

#define DEBUG 1

static const unsigned numThreads = 2 * std::thread::hardware_concurrency();

// Types.
enum class Type {
  Naive,
  Standard,
  Faster,
};

// Prepare the transforms.
// For overlapping substrings: factor = 2 (standard like)
// For non-overlapping substrings (our approach): factor = 1 (faster like)
#define preprocess(factor)                                             \
  auto lg = computeLog((1 + factor) * size - 1);                       \
  FFT pt((1 + factor) * size - 1, lg);                                 \
  FFT pt2((1 + factor) * size - 1, lg, &pt);                           \
  pt.init<true>(pattern_, 0, size, [](unsigned c) { return c; });      \
  pt2.init<true>(pattern_, 0, size, [](unsigned c) { return c * c; }); \
  pt.transform(), pt2.transform();                                     \
  auto patternCheck = computePatternCheck();                           \

class Like {
public:
  Like(Data& data, std::string pattern, bool check = false)
  : data_(data), pattern_(pattern), check_(check) {}

  template <Type type>
  VI run() {
    switch (type) {
      case Type::Naive: return runNaive();
      case Type::Standard: return runStandardParallel();
      case Type::Faster: runFasterParallel();
      default: return {};
    }
  }

  static void cmp(VI& a, VI& b) {
    assert(a.size() == b.size());
    for (unsigned index = 0, limit = a.size(); index != limit; ++index) {
      assert(a[index] == b[index]);
    }
  }

private:
  void debug(std::string& str, unsigned sol) {
    auto actual = match(str);
    if (sol != actual) {
      mutex.lock();
      std::cerr << "str: " << str << std::endl;
      std::cerr << "sol => " << sol << std::endl;
      std::cerr << "actual => " << actual << std::endl;
      assert(sol == actual);
      mutex.unlock();
    }
  }

  VI runNaive() {
    Timer timer("naive-like", pattern_);
    unsigned m = pattern_.size();
    VI ret;
    for (unsigned index = 0, limit = data_.size(); index != limit; ++index) {
      ret.push_back(match(data_[index]));
    }
    timer.stop();
    timer.flush();
    timer.debug();
    return ret;
  }

  VI runStandardParallel() {
    Timer timer("standard-like", pattern_);
    unsigned size = pattern_.size();

    // Prepare the transforms.
    preprocess(2);

    VI checker(check_ ? data_.size() : 1);
    unsigned taskSize = 10'000;
    unsigned numTasks = data_.size() / taskSize + (data_.size() % taskSize != 0);
    std::atomic<unsigned> taskIndex(0);
    auto consume = [&](unsigned threadIndex) -> void {
      // Init the slices transforms.
      FFT st(3 * size - 1, lg);
      FFT st2(3 * size - 1, lg, &st);
      
      // Used for iterations.
      VI *curr = new VI(3 * size - 1);

      // Solve for a single string.
      auto solve = [&](std::string& str) {
        if (str.size() < pattern_.size())
          return 0;
        auto matchCount = 0;
        unsigned index = 0, limit = str.size();

        bool force = false;
        while ((!force) && (index < limit)) {
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
          // Note: `bound - index <= 2 * size`, which is the last monomial for a full match.
          for (unsigned ptr = size - 1, lim = bound - index; ptr != lim; ++ptr)
            matchCount += (curr->at(ptr) == patternCheck);

          // And clear the transforms before using them again.
          st.clear();
          st2.clear();

          // Increase the current index.
          index += size + 1;
        }
        return matchCount;
      };

      while (taskIndex.load() < numTasks) {
        unsigned i = taskIndex++;
        if (i >= numTasks)
          return;
        
        // Compute the range.
        unsigned startIndex = taskSize * i;
        unsigned stopIndex = taskSize * (i + 1);
        if (i == numTasks - 1)
          stopIndex = data_.size();
        
        for (unsigned index = startIndex; index != stopIndex; ++index) {
          auto ret = solve(data_[index]);
          if (check_) {
            checker[index] = ret;
#if DEBUG
            debug(data_[index], ret);
#endif
          }
        }
      }
    };

    std::vector<std::thread> threads;
    for (unsigned index = 0, limit = numThreads; index != limit; ++index)
      threads.emplace_back(consume, index);
    for (auto& thread : threads)
      thread.join();

    timer.stop();
    timer.flush();
    timer.debug();
    return checker;
  }

  VI runFasterParallel() {
    Timer timer("faster-like", pattern_);
    unsigned size = pattern_.size();

    // Prepare the transforms.
    preprocess(1);

    VI checker(check_ ? data_.size() : 1);
    unsigned taskSize = 10'000;
    unsigned numTasks = data_.size() / taskSize + (data_.size() % taskSize != 0);
    std::atomic<unsigned> taskIndex(0);
    auto consume = [&](unsigned threadIndex) -> void {
      // Init the slices transforms.
      FFT st(2 * size - 1, lg);
      FFT st2(2 * size - 1, lg, &st);
      
      VI *prev, *curr;
      prev = new VI(2 * size - 1);
      curr = new VI(2 * size - 1);

      // Solve for a single string.
      auto solve = [&](std::string& str) {
        if (str.size() < pattern_.size())
          return 0;

        auto matchCount = 0;
        unsigned index = 0, limit = str.size();

        bool first = true, force = false;
        while ((!force) && (index < limit)) {
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
          // Note: `bound - index <= size`, which is the last monomial for a full match.
          for (unsigned ptr = first ? (size - 1) : 0; ptr != bound - index; ++ptr)
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

      while (taskIndex.load() < numTasks) {
        unsigned i = taskIndex++;
        if (i >= numTasks)
          return;
        
        // Compute the range.
        unsigned startIndex = taskSize * i;
        unsigned stopIndex = taskSize * (i + 1);
        if (i == numTasks - 1)
          stopIndex = data_.size();
        
        for (unsigned index = startIndex; index != stopIndex; ++index) {
          auto ret = solve(data_[index]);
          if (check_) {
#if DEBUG
            debug(data_[index], ret);
#endif
            checker[index] = ret;
          }
        }
      }
    };

    std::vector<std::thread> threads;
    for (unsigned index = 0, limit = numThreads; index != limit; ++index)
      threads.emplace_back(consume, index);
    for (auto& thread : threads)
      thread.join();

    timer.stop();
    timer.flush();
    timer.debug();
    return checker;
  }

  unsigned match(std::string& str) const {
    if (str.size() < pattern_.size())
      return 0;
    unsigned count = 0;
    for (unsigned i = 0, n = str.size(), m = pattern_.size(); i != n - m + 1; ++i) {
      bool match = true;
      for (unsigned j = 0; j != m; ++j)
        match = match && ((str[i + j] == pattern_[j]) || (pattern_[j] == '_'));
      count += match;
    }
    return count;
  }

  int computePatternCheck() const {
    int ret = 0;
    for (auto c : pattern_)
      ret += (c == '_') ? 0 : pow3(encode(c));
    return ret;
  }
  
  static unsigned computeLog(unsigned n) {
    unsigned lg = 0;
    while ((1u << lg) < n)
      ++lg;
    return lg;
  }

  static int pow3(int x) {
    return x * x * x;
  }

  bool check_;
  Data& data_;
  std::string pattern_;
  std::mutex mutex;
};