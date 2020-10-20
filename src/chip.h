struct Chip
{
    uint8_t V[16];
    uint8_t memory[4096];
    uint16_t I;
    uint8_t delay_timer;
    uint8_t sound_timer;
    uint16_t PC;
    uint8_t SP;
    uint16_t stack[16];

    uint8_t display[64 * 32];
    uint8_t keyboard[16];
    bool should_draw;
    uint64_t op_executed;
};

typedef struct Chip chip;

void initialize_chip(chip *c);
int load_rom(chip *c, const char *file_path);
void exec_cycle(chip *c);