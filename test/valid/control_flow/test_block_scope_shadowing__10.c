//
// Created by scott on 6/24/25.
//
int main() {
    int x = 5;
    {
        int x = 10;
        if (x == 10) return x;
    }
    return x;
}