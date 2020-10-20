#include <stdbool.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include "chip.h"

uint8_t keymap[16] = {
    SDLK_x,
    SDLK_1,
    SDLK_2,
    SDLK_3,
    SDLK_q,
    SDLK_w,
    SDLK_e,
    SDLK_a,
    SDLK_s,
    SDLK_d,
    SDLK_z,
    SDLK_c,
    SDLK_4,
    SDLK_r,
    SDLK_f,
    SDLK_v,
};

int main(int argc, char *argv[])
{
    if (argc != 2)
    {
        printf("Insira rom...\n");
        exit(1);
    }

    chip c;
    printf("%s\n", argv[1]);

    int width = 1024;
    int height = 512;

    SDL_Window *window = NULL;
    if (SDL_Init(SDL_INIT_EVERYTHING) < 0)
    {
        printf("SDL nao ta funcionando porra: %s\n", SDL_GetError());
        exit(1);
    }

    window = SDL_CreateWindow(
        "chip8 emulator",
        SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED,
        width,
        height,
        SDL_WINDOW_SHOWN
    );
    if (window == NULL)
    {
        printf("pqp nao da nem pra criar janela dessa bagaca: %s\n", SDL_GetError());
        exit(2);
    }

    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, 0);
    SDL_RenderSetLogicalSize(renderer, width, height);

    SDL_Texture *sdl_texture = SDL_CreateTexture(
        renderer,
        SDL_PIXELFORMAT_ARGB8888,
        SDL_TEXTUREACCESS_STREAMING,
        64,
        32
    );

    uint32_t pixels[2048];

    load:
    initialize_chip(&c);
    if (!load_rom(&c, argv[1]))
    {
        perror("Falha no load\n");
        exit(1);
    }

    while (1)
    {
        exec_cycle(&c);
        SDL_Event e;
        while(SDL_PollEvent(&e))
        {
            if (e.type == SDL_QUIT) exit(0);

            if (e.type == SDL_KEYDOWN)
            {
                if (e.key.keysym.sym == SDLK_ESCAPE)
                { 
                    printf("Instructions executed = %lu\n", c.op_executed);
                    exit(0);
                }

                if (e.key.keysym.sym == SDLK_F1) goto load;

                for (int i = 0; i < 16; i++)
                {
                    if (e.key.keysym.sym == keymap[i])
                        c.keyboard[i] = 1;
                }
            }
        }

        // Keyups
        if (e.type == SDL_KEYUP)
        {
            for (int i = 0; i < 16; i++)
            {
                if (e.key.keysym.sym == keymap[i])
                    c.keyboard[i] = 0;
            }
        }

        if (c.should_draw)
        {
            c.should_draw = false;
            for (int i = 0; i < 2048; i++)
            {
                uint8_t pixel = c.display[i];
                pixels[i] = (0x00FFFFFF * pixel) | 0xFF000000;
            }

            SDL_UpdateTexture(sdl_texture, NULL, pixels, 64 * sizeof(uint32_t));
            SDL_RenderClear(renderer);
            SDL_RenderCopy(renderer, sdl_texture, NULL, NULL);
            SDL_RenderPresent(renderer);
        }

        usleep(1200);
    }
}