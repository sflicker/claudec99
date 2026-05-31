struct Field {
   const int kind;
};

int main(void) {
   struct Field f = {42};
   return f.kind;
}
