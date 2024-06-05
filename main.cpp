#include <memory>
#include "chip.h"

int main() {
  std::unique_ptr<Chip> chip (new Chip());
  chip->print_mem();
  return 0;
}
