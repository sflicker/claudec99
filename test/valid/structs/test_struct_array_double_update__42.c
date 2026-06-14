struct Body { double vx; double vy; };
static struct Body b[2];
int main(void) {
    b[0].vx = 40.0;
    b[0].vy = 2.0;
    b[0].vx = b[0].vx + b[0].vy;
    return (int)b[0].vx;   /* 42 */
}
