#include "../collectable.h"

#include <cstdio>

class IntItem : public AutoCollectable<IntItem> {
 public:
  int id;
};

int main() {
  auto *items = new IntItem[10];
  for (int i = 0; i < 10; ++i) items[i].id = i;
  for (IntItem *item : *Collector<IntItem>::getInstance()) {
    printf("%d\n", item->id);
  }
  delete[] items;
  return 0;
}
