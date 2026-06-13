int count_calls(void) {
    static int calls[1];
    calls[0]++;
    return calls[0];
}

int main() {
    count_calls(); count_calls();
    return count_calls() - 3;
}
