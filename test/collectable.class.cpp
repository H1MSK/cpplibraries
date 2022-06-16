#include "../collectable.h"

#include <iostream>
#include <string>

class CollectableData : public AutoCollectable<CollectableData> {
 public:
  virtual void printData() = 0;
};

class CollectableInt : public CollectableData {
 public:
  int data;
  void printData() final { std::cout << data; }
};

class CollectableString : public CollectableData {
 public:
  std::string data;
  void printData() final { std::cout << data; }
};

int main() {
  auto *ints = new CollectableInt[10];
  auto *strings = new CollectableString[10];
  for (int i = 0; i < 10; ++i) {
    ints[i].data = i;
    strings[i].data = "S" + std::to_string(i);
  }
  for (auto *item : *Collector<CollectableData>::getInstance()) {
    item->printData();
    std::cout << '\n';
  }
  delete[] ints;
  delete[] strings;
  return 0;
}