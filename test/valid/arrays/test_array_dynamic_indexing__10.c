int main() {
   int a[2];
   int i=0;
   int j=1;
   int k=10;
   a[j] = k;
   a[i] = a[j];
   return a[i];    //10
}
