//
// Created by scott on 6/24/25.
//
int main() {
    int x = 1;
    {
        int x = 2;
        {
            int x = 3;
            return x;
        }
    }
    return x;
}