#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "stdafx.h"
#include <medium/Medium.h>
#include "RabinKarp.h"
#include "CircleBuffer.h"
#include <medium/DataSplitterOutputStream.h>
#include "MathUtils.h"

#include <iostream>
#include <random>

using namespace std;
using namespace medium;


TEST_CASE("rolling hash", "[RabinKarp]") {
  RabinKarpHash hash(3);
  const char* str = "Hello this is his string";

  hash.eat('h');
  hash.eat('i');
  hash.eat('s');

  RabinKarpHash::hashvalue_t hiscode = hash.hash();
  hash.reset();

  for (int i = 0; i < strlen(str); i++) {
    if (i >= 3) {
      hash.update(str[i], str[i - 3]);
    }
    else {
      hash.eat(str[i]);
    }

    RabinKarpHash::hashvalue_t code = hash.hash();
    cout << "i: " << i << " " << code << endl;
    if (i == 9 || i == 16) {
      REQUIRE(code == hiscode);
    }
  }

}

TEST_CASE("circle buffer", "[CircleBuffer]") {
  CircleBuffer<byte> buffer(5);
  REQUIRE(buffer.size() == 0);
  buffer.put(1);
  REQUIRE(buffer.size() == 1);
  buffer.put(2);
  buffer.put(3);
  buffer.put(4);
  buffer.put(5);
  REQUIRE(buffer.size() == 5);
  REQUIRE(buffer.get() == 1);
  REQUIRE(buffer.size() == 4);
  REQUIRE(buffer.get() == 2);
  buffer.put(6);
  REQUIRE(buffer.get() == 3);
}

sp<OutputStream> createNullOutput()
{
  return std::make_shared<NullOutputStream>();
}

TEST_CASE("data split stream", "[DataSplitterOutputStream]") {
  
  std::default_random_engine gen(1);
  std::uniform_int_distribution<unsigned short> distro;
  int count;
  const int buflen = 1024;
  byte buf[buflen];
  
  DataSplitterOutpuStream dout([&] {
    count++;
    return std::make_shared<NullOutputStream>();
  });

  StorelessStatsF stats;

  for (int q = 0; q < 100; q++) {
    count = 0;
    for (int j = 0; j < 10*1024; j++) {
      for (int i = 0; i < buflen; i++) {
        buf[i] = (byte)distro(gen);
      }
      dout.write(buf, buflen);
    }

    stats.increment(count);

    std::cout << "num splits in 10 MiB: " << count << " avg: " << stats.getMean() << ":" << stats.getStdDiv() << std::endl;
  }

}