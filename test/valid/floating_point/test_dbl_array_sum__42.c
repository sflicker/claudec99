int main() {
    double a[5];
    a[0] = 1.0;
    a[1] = 10.0;
    a[2] = 25.0;
    a[3] = 5.0;
    a[4] = 1.0;
    double sum = 0;
    for (int i=0;i<5;i++) {
        sum += a[i];
    }
    return sum;
} 
