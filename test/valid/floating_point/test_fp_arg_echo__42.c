float echo(float arg) {
    return arg;
}

int main() {
    float a = 42.0f;
    float b = echo(a);
    return (int)b;
}

