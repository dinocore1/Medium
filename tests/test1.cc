#define CATCH_CONFIG_MAIN  // This tells Catch to provide a main() - only do this in one cpp file
#include "catch.hpp"

#include "stdafx.h"
#include "RabinKarp.h"
#include "CircleBuffer.h"

#include <iostream>

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

