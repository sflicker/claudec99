typedef enum Dir Dir;
enum Dir { NORTH = 0, SOUTH = 1, EAST = 2, WEST = 3 };
int opposite(Dir d) {
    switch (d) {
        case NORTH: return SOUTH;
        case SOUTH: return NORTH;
        case EAST:  return WEST;
        default:    return EAST;
    }
}
int main() { return opposite(NORTH) - 1; }
