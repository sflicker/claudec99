int eq(double a, double b) {  return a==b; }
int ne(double a, double b) {  return a!=b; }
int main() {
    double z = 0.0;
    double y = 1.0;
    return eq(z, y)*10 + ne(z,y); // 1

}
