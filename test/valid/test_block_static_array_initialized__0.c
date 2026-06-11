int get_step(int i) {
    static int steps[3] = {10, 20, 30};
    return steps[i];
}

int main() {
    return get_step(0) + get_step(1) + get_step(2) - 60;
}
