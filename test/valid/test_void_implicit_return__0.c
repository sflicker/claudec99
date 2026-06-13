static int g_val = 0;
void set(int v) { g_val = v; }
int main(void) {
    set(42);
    return g_val - 42;
}
