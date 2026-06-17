struct Flags {
    unsigned int low : 3;
    unsigned int mid : 5;
    unsigned int high : 10;
};

int main(void) {
    struct Flags flags;

    flags.low = 5;
    flags.mid = 17;
    flags.high = 513;

    return (int)(flags.low + flags.mid + flags.high);
}
