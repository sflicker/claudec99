struct Field {
  const char *name;
};

int main(void) {
  struct Field f;
  f.name = "hello";
  return f.name[0] == 'h';
}
