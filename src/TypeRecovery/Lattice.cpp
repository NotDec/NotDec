#include "TypeRecovery/Lattice.h"
#include <iostream>

namespace notdec::retypd {

bool isInt(std::string a) { return a == "int" || a == "sint" || a == "uint"; }

// https://en.wikipedia.org/wiki/Join_and_meet

static bool isFloat(std::string a) { return a == "float" || a == "double"; }
static bool isInt1(std::string a) {
  return a == "int" || a == "sint" || a == "uint";
}

std::string join(std::string a, std::string b) {
  // std::cerr << "joining " << a << " with " << b << "\n";
  if (a == b) {
    return a;
  }
  if (a == "top") {
    return a;
  }
  if (b == "top") {
    return b;
  }

  std::cerr << "unable to handle join: " << a << " with " << b << "\n";

  return a;
}

std::string meet(std::string a, std::string b) {
  if (a == b) {
    return a;
  }
  if (a == "top") {
    return b;
  }
  if (b == "top") {
    return a;
  }

  // prefer float
  if (isFloat(a) && isInt1(b)) {
    return a;
  }
  if (isInt1(a) && isFloat(b)) {
    return b;
  }

  // prefer specific int
  if (isInt1(a) && isInt1(b)) {
    if (a == "int") {
      return b;
    }
    if (b == "int") {
      return a;
    }
  }

  std::cerr << "unable to handle meet: " << a << " with " << b << "\n";

  return a;
}

} // namespace notdec::retypd
