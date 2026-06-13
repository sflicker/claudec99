extern int g;
static int h = 5;

static int helper(void) {
    return h;
}

int main() {
    return helper();
}
