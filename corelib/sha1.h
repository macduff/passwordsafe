#pragma once

class SHA1
{
 public:
  enum {HASHLEN = 20};
  SHA1();
  ~SHA1();
  void Update(const unsigned char* data, unsigned int len);
  void Final(unsigned char digest[HASHLEN]);
 private:
  unsigned long state[5];
  unsigned long count[2];
  unsigned char buffer[64];
};

