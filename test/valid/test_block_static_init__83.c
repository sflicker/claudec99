int next(void) {
   static int x = 40;
   x = x + 1;
   return x;
}

int main() {
    return next() + next();   // expect 83
}
