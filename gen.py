import random
import string

WILDCARD_PROP = 0.2

def computeAvg():
  with open('o_comment.in', 'r') as f:
    count = 0
    avg = 0.0
    for line in f.readlines():
      avg += len(line.strip())
      count += 1
    print(avg / count)

def gen_large():
  with open(f'random512.in', 'w') as f:
    for i in range(1500000):
      s = ""
      for j in range(512):
        s += random.choice(string.ascii_letters[:26])
      f.write(s + '\n')
  
def gen(len):
  with open(f'patterns/{len}.in', 'w') as f:
    for _ in range(10):
      s = ""
      for _ in range(len):
        coef = random.uniform(0, 1)
        if coef < WILDCARD_PROP:
          s += '_'
        else:
          s += random.choice(string.ascii_letters[:26])
      f.write(s + '\n')

def main():
  lens = [4, 8, 16, 32, 64]
  computeAvg()
  # gen_large()
  # for len in lens:
  #   gen(len)

if __name__ == '__main__':
  main()