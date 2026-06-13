struct Field {
  const int kind;
};

int main(void) {
  struct Field f = {1};
  f.kind = 2;
  return f.kind;
}
