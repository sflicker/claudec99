int main() {
    char *names[2];

    names[0] = "ab";
    names[1] = "cd";

    return names[0][1] + names[1][1]; // expect 198: 'b'=98 + 'd'=100
}
