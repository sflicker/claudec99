void *malloc(unsigned long size);

struct Packet {
    int count;
    unsigned char data[];
};

int main(void) {
    struct Packet *packet = malloc(sizeof(struct Packet) + 4UL);

    packet->count = 4;
    packet->data[0] = 3;
    packet->data[1] = 5;
    packet->data[2] = 7;
    packet->data[3] = 11;

    return packet->count + packet->data[0] + packet->data[1]
        + packet->data[2] + packet->data[3] + (int)sizeof(struct Packet);
}
