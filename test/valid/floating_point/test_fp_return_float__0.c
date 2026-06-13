static float half(float x) { return x / 2.0f; }
int main(void) {
    float r = half(84.0f);
    return (int)r - 42;
}
