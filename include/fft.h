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
	std::cerr << "c=" << c << std::endl;
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

  static unsigned computeLog(unsigned n) {
    unsigned lg = 0;
    while ((1u << lg) < n)
      ++lg;
    return lg;
  }

	static int pow3(int x) {
		return x * x * x;
	}

	template <bool erase>
  void convolve(FFT& other, VI* ret, std::function<int(int)>&& f) {
    assert(n == other.n);

    // Convolution in frequency domain.
    for (unsigned i = 0; i != n; i++)	data_[i] *= other.data_[i];

    // Inverse transform.
		transform(true);

    // And translate back.
		assert(ret->size() == size_);
		for (unsigned i = 0; i != size_; i++) {
			if (erase) ret->at(i) = 0;
			
			ret->at(i) += f(static_cast<int>(data_[i].a + 0.5));
			// std::cerr << "i=" << i << " elem=" << data_[i].a << std::endl;
      // ret[i] = static_cast<int>(data_[i].a + 0.5);
    }
  }

	// void debug(const char* msg) {
	// 	for (unsigned index = 0; index != n; ++index) {
	// 		std::cerr << "index=" << index << " elem=" << data_[
	// 	}
	// }

#if 0
	static VI convolution(VI &a, VI &b) {
		int alen = a.size(), blen = b.size();
		int resn = alen + blen - 1;	// size of the resulting array
		int s = 0, i;
		while ((1 << s) < resn) s++;	// n = 2^s
		int n = 1 << s;	// round up the the nearest power of two

		FFT pga, pgb;
		pga.setSize(s);	// fill and transform first array
		for (i = 0; i < alen; i++) pga.data[i] = complex(a[i], 0);
		for (i = alen; i < n; i++)	pga.data[i] = complex(0, 0);
		pga.transform();

		pgb.setSize(s);	// fill and transform second array
		for (i = 0; i < blen; i++)	pgb.data[i] = complex(b[i], 0);
		for (i = blen; i < n; i++)	pgb.data[i] = complex(0, 0);
		pgb.transform();

		for (i = 0; i < n; i++)	pga.data[i] = pga.data[i] * pgb.data[i];
		pga.transform(true);	// inverse transform
		VI result = VI (resn);	// round to nearest integer
		for (i = 0; i < resn; i++)	result[i] = (int) (pga.data[i].a + 0.5);

		int actualSize = resn - 1;	// find proper size of array
		while (result[actualSize] == 0)
			actualSize--;
		if (actualSize < 0) actualSize = 0;
		result.resize(actualSize+1);
		return result;
	}
#endif
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