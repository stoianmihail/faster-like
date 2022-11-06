import random
import string

WILDCARD_PROP = 0.2

def gen(len):
  with open(f'patterns/{len}.in', 'w') as f:
    for i in range(100):
      s = ""
      for j in range(len):
        coef = random.uniform(0, 1)
        if coef < WILDCARD_PROP:
          s += '_'
        else:
          s += random.choice(string.ascii_letters[:26])
      f.write(s + '\n')

def main():
  lens = [4, 8, 16, 32]
  for len in lens:
    gen(len)

if __name__ == '__main__':
  main()