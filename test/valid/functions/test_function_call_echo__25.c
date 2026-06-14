int echo(int n) {
    return n;
}

int add(int a, int b) {
   return echo(a) + echo(b);
}

int square(int x) {
   return echo(x) * echo(x);
}

int main() {
    return echo(square(add(2,3)));
}

