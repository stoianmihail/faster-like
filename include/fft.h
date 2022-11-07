#pragma once

#include <cmath>
#include <functional>

// Inspired from http://web.mit.edu/~ecprice/acm/acm08/notebook.html

typedef std::vector<int> VI;
static constexpr double PI = std::acos(0) * 2;

static constexpr char extra[] = " .,-:?!;";

static int encode(char c) {
  if (islower(c))
    return c - 'a' + 1;
  if (isupper(c))
    return c - 'A' + 26 + 1;
  auto shift = 26 + 26;
  for (auto e : extra) {
    ++shift;
    if (c == e)
      return shift;
  }
  assert(0);
  return -1;
}

class complex {
public:
  double a, b;
  complex() {a = 0.0; b = 0.0;}
  complex(double na, double nb) {a = na; b = nb;}
  const complex operator+(const complex &c) const
    {return complex(a + c.a, b + c.b);}
  const complex operator-(const complex &c) const
    {return complex(a - c.a, b - c.b);}
  const complex operator*(const complex &c) const
    {return complex(a*c.a - b*c.b, a*c.b + b*c.a);}
  void operator *= (const complex &c) {
    *this = *this * c;
  }
  void operator /= (int n) {
    a /= n, b /= n;
  }
  double magnitude() {return sqrt(a*a+b*b);}
  void print() {printf("(%.3f %.3f)\n", a, b);}
};

typedef std::vector<complex> VC;

class FFT {
public:
  FFT(unsigned size, unsigned lg, FFT* sibling = nullptr)
  : size_(size), sibling_(sibling) {
    setSize(lg);
  }

  template <bool reverse = false>
  void init(std::string& str, unsigned begin, unsigned end, std::function<unsigned(unsigned)>&& f) {
    assert(end <= str.size());
    for (unsigned index = begin; index != end; ++index) {
      auto c = str[reverse ? (end - index + begin - 1) : index];
      if (c == '_') {
        continue;
      } else {
        data_[index - begin] = complex(f(encode(c)), 0);
      }
    }
  }

  void clear() {
    std::fill(data_.begin(), data_.end(), complex(0, 0));
  }

  void bitReverse(VC &array) {
    VC temp(n);
    int i;
    VI* currRev = sibling_ ? &(sibling_->rev_) : &rev_;
    for (i = 0; i < n; i++)
      temp[i] = array[currRev->at(i)];
    for (i = 0; i < n; i++)
      array[i] = temp[i];
  }

  void transform(bool inverse = false) {
    bitReverse(data_);
    VC* roots = sibling_ ? &(sibling_->roots_) : &roots_;
    int i, j, k;
    for (i = 1; i <= s; i++) {
      int m = (1 << i), md2 = m / 2;
      int start = 0, increment = (1 << (s-i));
      if (inverse) {
        start = n;
        increment *= -1;
      }
      complex t, u;
      for (k = 0; k < n; k += m) {
        int index = start;
        for (j = k; j < md2+k; j++) {
          t = roots->at(index) * data_[j+md2];
          index += increment;
          data_[j+md2] = data_[j] - t;
          data_[j] = data_[j] + t;
        }
      }
    }
    if (inverse) {
      for (i = 0; i < n; i++) {
        data_[i] /= n;
      }
    }
  }

  template <bool erase>
  void convolve(FFT& other, VI* ret, std::function<int(int)>&& f) {
    assert(n == other.n);

    // Convolution in frequency domain.
    for (unsigned i = 0; i != n; i++)  data_[i] *= other.data_[i];

    // Inverse transform.
    transform(true);

    // And translate back.
    assert(ret->size() == size_);
    for (unsigned i = 0; i != size_; i++) {
      if (erase) ret->at(i) = 0;  
      ret->at(i) += f(static_cast<int>(data_[i].a + 0.5));
    }
  }

private:
  void setSize(int ns) {
    s = ns;
    n = (1 << s);
    data_ = VC(n, complex(0, 0));
    
    if (!sibling_) {
      rev_ = VI(n);
      roots_ = VC(n + 1);
      for (int i = 0; i != n; i++)
        for (int j = 0; j != s; j++)
          if ((i & (1 << j)) != 0)
            rev_[i] += (1 << (s - j - 1));
      roots_[0] = complex(1, 0);
      complex mult = complex(cos(2*PI/n), sin(2*PI/n));
      for (int i = 1; i <= n; i++)
        roots_[i] = roots_[i-1] * mult;
    }
  }

  FFT* sibling_;
  VC data_, roots_;
  VI rev_;
  unsigned size_;
  int s, n;
};