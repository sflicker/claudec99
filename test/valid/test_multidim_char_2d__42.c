int main() {
    char names[2][4];

    names[0][0] = 'O';
    names[0][1] = 'K';
    names[0][2] = '\n';

    return names[0][0] + names[0][1] - 112;
}
