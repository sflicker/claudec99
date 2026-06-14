//
// Created by scott on 6/24/25.
//
int side_effect() {
    return 99;
}

int main() {
    if (0 && side_effect()) return 1;
    return 42;
}
