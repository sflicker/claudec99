int main() {
    char *names[2];

    names[0] = "ab";
    names[1] = "cd";

    return names[0][0] + names[1][0]; // expect 196: 'a'=97 + 'c'=99
}
