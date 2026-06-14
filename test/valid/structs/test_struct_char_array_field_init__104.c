struct Entry { int id; char name[12]; };
struct Entry e = {1, "hello"};
int main(void) {
    return (int)(unsigned char)e.name[0];  /* 'h' = 104 */
}
