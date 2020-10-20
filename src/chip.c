#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdint.h>
#include <string.h>

#include "chip.h"

uint8_t font_set[80] = 
{                                                                               
    0xF0, 0x90, 0x90, 0x90, 0xF0, // 0                                                                
    0x20, 0x60, 0x20, 0x20, 0x70, // 1                                                                
    0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2                                                                
    0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3                                                                
    0x90, 0x90, 0xF0, 0x10, 0x10, // 4                                                                
    0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5                                                                
    0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6                                                                
    0xF0, 0x10, 0x20, 0x40, 0x40, // 7                                                                
    0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8                                                                
    0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9                                                                
    0xF0, 0x90, 0xF0, 0x90, 0x90, // A                                                                
    0xE0, 0x90, 0xE0, 0x90, 0xE0, // B                                                                
    0xF0, 0x80, 0x80, 0x80, 0xF0, // C                                                                
    0xE0, 0x90, 0x90, 0x90, 0xE0, // D                                                                
    0xF0, 0x80, 0xF0, 0x80, 0xF0, // E                                                                
    0xF0, 0x80, 0xF0, 0x80, 0x80, // F                                                                
};

void initialize_chip(chip *c)
{
    srand(time(NULL));
    memset(c, 0, sizeof(chip));

    for (int i = 0; i < 80; i++)
        c->memory[i] = font_set[i];
}

int load_rom(chip *c, const char *file_path)
{
    printf("Loading %s->\n", file_path);

    FILE *rom = fopen(file_path, "rb");
    if (rom == NULL)
    {
        printf("ROM failed to open!\n");
        return 0;
    }
    
    // Calcula o tamanho do arquivo
    fseek(rom, 0, SEEK_END);
    size_t size = ftell(rom);
    rewind(rom);

    // Aloca buffer para leitura do arquivo
    char *buf = (char *) malloc(size * sizeof(char));
    if (buf == NULL)
    {
        perror("Failure to allocate buffer memory at reading ROM!\n");
        return 0;
    }
    size_t result = fread(buf, sizeof(char), (size_t) size, rom);
    if (result != size)
    {
        perror("Falha no read\n");
        return 0;
    }

    if ((4096 - 512) > size)
    {
        for (int i = 0; i < size; i++)
            c->memory[i + 512] = (uint8_t) buf[i];
    }
    else
    {
        perror("ROM too large to load in memory!\n");
        return 0;
    }

    fclose(rom);
    free(buf);
    c->PC = 512;

    return 1;
}

void exec_cycle(chip *c)
{
    c->op_executed++;
    uint16_t op = (c->memory[c->PC] << 8) | c->memory[c->PC + 1];
    uint16_t op_type = op & 0xF000;
    uint8_t x = (op & 0x0F00) >> 8;
    uint8_t y = (op & 0x00F0) >> 4;
    uint8_t byte = op & 0x00FF;
    uint16_t addr = op & 0x0FFF;
    uint8_t op_id = op & 0x00FF;
    uint8_t nibble = op & 0x000F;
    uint16_t pixel;
    bool key_pressed = false;
    //printf("TESTE\t%.4X\n", op);

    switch (op_type)
    {
        case 0x000:
            switch (op)
            {
                // CLS
                case 0x00E0:
                    //memset(&c->display, 0, 64 * 32);
                    for (int i = 0; i < 64 * 32; i++)
                        c->display[i] = 0;
                    c->should_draw = true;
                    c->PC += 2;
                    break;
                // RET
                case 0x00EE:
                    c->SP--;
                    c->PC = c->stack[c->SP];
                    c->PC += 2;
                    break;

                default:
                    printf("Invalid op %X\n", op);
                    exit(1);
            }
            break;

        // JP addr
        case 0x1000:
            c->PC = addr;
            break;

        // Call addr
        case 0x2000:
            c->stack[c->SP] = c->PC;
            c->SP++;
            c->PC = addr;
            break;

        // SE Vx, byte
        case 0x3000:
            if (c->V[x] == byte)
                c->PC += 4;
            else
                c->PC += 2;

            break;

        // SNE Vx, byte
        case 0x4000:
            if (c->V[x] != byte) c->PC += 4;
            else c->PC += 2;

            break;

        // SE Vx, Vy
        case 0x5000:
            if (c->V[x] == c->V[y]) c->PC += 4;
            else c->PC += 2;

            break;

        // LD Vx, byte
        case 0x6000:
            c->V[x] = byte;
            c->PC += 2;
            break;

        // ADD Vx, byte
        case 0x7000:
            c->V[x] += byte;
            c->PC += 2;
            break;

        // math instructions->
        case 0x8000:
            switch (op & 0x000F)
            {
                // LD Vx, Vy
                case 0x0000:
                    c->V[x] = c->V[y];
                    c->PC += 2;
                    break;

                // OR Vx, Vy
                case 0x0001:
                    c->V[x] |= c->V[y];
                    c->PC += 2;
                    break;

                case 0x0002:
                // AND Vx, Vy
                    c->V[x] &= c->V[y];
                    c->PC += 2;
                    break;

                // XOR Vx, Vy
                case 0x0003:
                    c->V[x] ^= c->V[y];
                    c->PC += 2;
                    break;

                // ADD Vx, Vy
                case 0x0004:
                    c->V[x] += c->V[y];
                    if ((c->V[y]) > (0xFF - c->V[x])) c->V[0xF] = 1;
                    else c->V[0xF] = 0;

                    c->PC += 2;
                    break;

                // SUB Vx, Vy
                case 0x0005:
                    if (c->V[y] > c->V[x]) c->V[0xF] = 0;
                    else c->V[0xF] = 1;
                    //c->V[x] -= c->V[y];
                    c->V[x] = c->V[x] - c->V[y];
                    c->PC += 2;
                    break;

                // SHR Vx {, Vy}
                case 0x0006:
                    c->V[0xF] = c->V[x] & 0x1;
                    c->V[x] = c->V[x] >> 1;
                    //c->V[x] >>= 1;
                    c->PC += 2;
                    break;

                // SUBN Vx, Vy
                case 0x0007:
                    if (c->V[x] > c->V[y]) c->V[0xF] = 0;
                    else c->V[0xF] = 1;
                    c->V[x] = c->V[y] - c->V[x];
                    c->PC += 2;
                    break;

                // SHL Vx {, Vy}
                case 0x000E:
                    c->V[0xF] = c->V[x] >> 7;
                    c->V[x] = c->V[x] <<  1;
                    c->PC += 2;
                    break;

                default:
                    printf("Invalid op at 0x8 %X\n", op);
                    exit(1);
            }
            break;

        // SNE Vx, Vy
        case 0x9000:
            if (c->V[x] != c->V[y]) c->PC += 4;
            else c->PC += 2;
            break;

        // LD I, addr
        case 0xA000:
            c->I = addr;
            c->PC += 2;
            break;

        // JP V0, addr
        case 0xB000:
            c->PC = addr + c->V[0];
            break;

        // RND vx, byte
        case 0xC000:
            c->V[x] = (rand() % 256) & byte;
            c->PC += 2;
            break;

        // DRW Vx, Vy, nibble
        case 0xD000:
        {
            unsigned short x = c->V[(op & 0x0F00) >> 8];
            unsigned short y = c->V[(op & 0x00F0) >> 4];
            unsigned short height = op & 0x000F;
            unsigned short pixel;

            c->V[0xF] = 0;
            for (int yline = 0; yline < height; yline++)
            {
                pixel = c->memory[c->I + yline];
                for(int xline = 0; xline < 8; xline++)
                {
                    if((pixel & (0x80 >> xline)) != 0)
                    {
                        if(c->display[(x + xline + ((y + yline) * 64))] == 1)
                        {
                            c->V[0xF] = 1;
                        }
                        c->display[x + xline + ((y + yline) * 64)] ^= 1;
                    }
                }
            }

            c->should_draw = true;
            c->PC += 2;
        }
            break;

        // Keyboard instructions->
        case 0xE000:
            switch (op_id)
            {
                // SKP Vx
                case 0x9E:
                    if (c->keyboard[c->V[x]] != 0) c->PC += 4;
                    else c->PC += 2;
                    break;

                // case 0xA1:
                case 0xA1:
                    if (c->keyboard[c->V[x]] == 0) c->PC += 4;
                    else c->PC += 2;
                    break;
            }
            break;

        // F
        case 0xF000:
            switch (op_id)
            {
                // LD Vx, DT
                case 0x0007:
                    c->V[x] = c->delay_timer;
                    c->PC += 2;
                    break;

                // LD Vx, K
                case 0x000A:
                    //bool key_pressed = false;
                    printf("Acho q to aq\n");
                    while (!key_pressed)
                    {
                        for (int i = 0; i < 16; i++)
                        {
                            if (c->keyboard[i] != 0)
                            {
                                c->V[x] = i;
                                key_pressed = true;
                            }
                        }
                    }
                    c->PC += 2;
                    break;

                // LD Dt, Vx
                case 0x0015:
                    c->delay_timer = c->V[x];
                    c->PC += 2;
                    break;

                // LD ST, Vx
                case 0x0018:
                    c->sound_timer = c->V[x];
                    c->PC += 2;
                    break;

                // ADD I, Vx
                case 0x001E:
                /*
                    c->I += c->V[x];
                    c->PC += 2;
                    break;
                    */
                    if ((c->I + c->V[x]) > 0xFFF) c->V[0xF] = 1;
                    else c->V[0xF] = 0;
                    c->I += c->V[x];
                    c->PC += 2;
                    break;

                // LD F, Vx
                case 0x0029:
                   c->I = c->V[x] * 0x5;
                   c->PC += 2;
                   break;

                // LD B, Vx
                case 0x0033:
                    c->memory[c->I] = c->V[x] / 100;
                    c->memory[c->I + 1] = (c->V[x] / 10) % 10;
                    c->memory[c->I + 2] = c->V[x] % 10;
                    c->PC += 2;
                    break;

                // LD [I], Vx
                case 0x0055:
                    for (int i = 0; i <= x; i++)
                        c->memory[c->I + i] = c->V[i];
                    
                    // No interpreador original, essa operação é feita, retirar se tiver bug
                    //c->I += x + 1;
                    c->PC += 2;
                    break;

                // LD Vx, [I]
                case 0x0065:
                    for (int i = 0; i <= x; i++)
                        c->V[i] = c->memory[c->I + i];

                    // Mesma operação em 0x55
                    //c->I += x + 1;
                    c->PC += 2;
                    break;

                default:
                    printf("Invalid op at 0xF %X\n", op);
                    exit(1);
            }
        break;

        default:
            printf("general invalid op %X\n", op);
            exit(1);
    }

    if (c->delay_timer > 0)
    {
        //printf("delay_timer = %X\n", c->delay_timer);
        c->delay_timer--;
    }

    if (c->sound_timer > 0)
    {
        if (c->sound_timer == 1)
            printf("sound here\n");
            // TODO: implementar som
    
        //printf("sound_timer = %X\n", c->sound_timer);
        c->sound_timer--;
    }
}