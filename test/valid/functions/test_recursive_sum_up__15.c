int sum_up(int start, int end) {
   if (start > end)
	return 0;
   return start + sum_up(start+1, end);
}

int main() {
    return sum_up(1,5);
}

