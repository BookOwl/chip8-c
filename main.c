#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <SDL.h>
#include "instruction.c"
#include "graphics.c"

typedef struct {
    uint16_t stack[256];
    uint8_t ptr;
} callstack;

callstack new_callstack() {
    callstack stack;
    stack.ptr = 0;
    return stack;
}

int callstack_push(callstack* stack, uintptr_t addr) {
    if (stack->ptr == 255) return -1;
    stack->stack[stack->ptr] = addr;
    return stack->ptr++;
}

int callstack_pop(callstack* stack) {
    if (stack->ptr == 0) return -1;
    return stack->stack[--stack->ptr];
}

uint8_t rand_byte() {
    // Very random number
    return 4;
}

typedef enum {
    SUCCESS,
    ERR_STACK_OVERFLOW,
    ERR_STACK_UNDERFLOW,
    ERR_INVALID,
} tick_result;

#define DIGIT_BASE_ADDR 0
#define DIGIT_LEN 5
static uint8_t DIGIT_SPRITES[80] = {0xF0, 0x90, 0x90, 0x90, 0xF0, \
0x20, 0x60, 0x20, 0x20, 0x70, \
0xF0, 0x10, 0xF0, 0x80, 0xF0, \
0xF0, 0x10, 0xF0, 0x10, 0xF0, \
0x90, 0x90, 0xF0, 0x10, 0x10, \
0xF0, 0x80, 0xF0, 0x10, 0xF0, \
0xF0, 0x80, 0xF0, 0x90, 0xF0, \
0xF0, 0x10, 0x20, 0x40, 0x40, \
0xF0, 0x90, 0xF0, 0x90, 0xF0, \
0xF0, 0x90, 0xF0, 0x10, 0xF0, \
0xF0, 0x90, 0xF0, 0x90, 0x90, \
0xE0, 0x90, 0xE0, 0x90, 0xE0, \
0xF0, 0x80, 0x80, 0x80, 0xF0, \
0xE0, 0x90, 0x90, 0x90, 0xE0, \
0xF0, 0x80, 0xF0, 0x80, 0xF0, \
0xF0, 0x80, 0xF0, 0x80, 0x80};

typedef struct vm {
    uint8_t ram[4096];
    uint8_t reg[16];
    uint16_t i;
    uint16_t pc;
    uint8_t tpu;
    uint8_t delay;
    uint8_t sound;
    callstack stack;
    fps_clock clock;
    sdl_handle* gfx;
} chip8_vm;

void vm_load_program(chip8_vm* vm, sdl_handle* gfx, uint8_t ticks_per_update, uint8_t program[], int program_len) {
    for (int i = 0; i < 16; i++){
        vm->reg[i] = 0;
    }
    vm->i = 0;
    vm->delay = 0;
    vm->sound = 0;
    vm->tpu = ticks_per_update;
    vm->stack = new_callstack();
    vm->clock = new_fps_clock(60);
    vm->gfx = gfx;
    // Chip-8 programs are loaded at address 0x200
    vm->pc = 0x200;
    for (int i = 0; i < program_len; i++) {
        vm->ram[i+0x200] = program[i];
    }
    // Load the digit sprites into memory
    for (int i = 0; i < DIGIT_LEN * 16; i++) {
        vm->ram[i + DIGIT_BASE_ADDR] = DIGIT_SPRITES[i];
    }
}

tick_result vm_tick(chip8_vm* vm) {
    // Load the next two bytes that make up the instruction
    uint16_t raw_inst = (((uint16_t) vm->ram[vm->pc]) << 8) + (uint16_t) vm->ram[vm->pc+1];
    instruction inst = decode_instruction(raw_inst);
    vm->pc += 2;
    switch (inst.tag) {
        case CLEAR:
            clear_screen(vm->gfx);
            break;
        case LOAD:
            vm->reg[inst.reg1] = inst.data;
            break;
        case MOV:
            vm->reg[inst.reg1] = vm->reg[inst.reg2];
            break;
        case ADD_NUM:
            vm->reg[inst.reg1] += inst.data;
            break;
        case ADD_REG: {
            uint16_t x = (uint16_t) vm->reg[inst.reg1];
            uint16_t y = (uint16_t) vm->reg[inst.reg2];
            uint16_t res = x + y;
            if (res > 255) {
                res = res % 256;
                vm->reg[0xF] = 0x01;
            } else {
                vm->reg[0xF] = 0x00;
            }
            vm->reg[inst.reg1] = (uint8_t) res;
            break;
        }
        case SUB_REG: {
            uint8_t x = vm->reg[inst.reg1];
            uint8_t y = vm->reg[inst.reg2];
            uint8_t res = x - y;
            if (x < y) {
                vm->reg[0xF] = 0x01;
            } else {
                vm->reg[0xF] = 0x00;
            }
            vm->reg[inst.reg1] = res;
            break;
        }
        case SUB_FROM: {
            uint8_t x = vm->reg[inst.reg1];
            uint8_t y = vm->reg[inst.reg2];
            uint8_t res = y - x;
            if (y < x) {
                vm->reg[0xF] = 0x01;
            } else {
                vm->reg[0xF] = 0x00;
            }
            vm->reg[inst.reg1] = res;
            break;
        }
        case AND:
            vm->reg[inst.reg1] &= vm->reg[inst.reg2];
            break;
        case OR:
            vm->reg[inst.reg1] |= vm->reg[inst.reg2];
            break;
        case XOR:
            vm->reg[inst.reg1] ^= vm->reg[inst.reg2];
            break;
        case RSHIFT: {
            uint8_t y = vm->reg[inst.reg2];
            vm->reg[inst.reg1] = y >> 1;
            vm->reg[0xF] = y & 0b1;
            break;
        }
        case LSHIFT: {
            uint8_t y = vm->reg[inst.reg2];
            vm->reg[inst.reg1] = y << 1;
            vm->reg[0xF] = (y & 0b10000000) >> 7;
            break;
        }
        case RAND:
            vm->reg[inst.reg1] = rand_byte() & inst.data;
            break;
        case JMP:
            vm->pc = inst.data;
            break;
        case JMP_REL:
            vm->pc = inst.data + vm->reg[0];
            break;
        case CALL:
            if (callstack_push(&vm->stack, vm->pc) < 0) return ERR_STACK_OVERFLOW;
            vm->pc = inst.data;
            break;
        case RET:
            int16_t ret_addr = callstack_pop(&vm->stack);
            if (ret_addr < 0) return ERR_STACK_UNDERFLOW;
            // Add 1 so that we start execution after the call instruction
            // that pushed this return address to the stack
            vm->pc = ret_addr + 2;
            break;
        case SKP_EQ:
            if (vm->reg[inst.reg1] == inst.data) vm->pc += 2;
            break;
        case SKP_NEQ:
            if (vm->reg[inst.reg1] != inst.data) vm->pc += 2;
            break;
        case SKP_EQ_REG:
            if (vm->reg[inst.reg1] == vm->reg[inst.reg2]) vm->pc += 2;
            break;
        case SKP_NEQ_REG:
            if (vm->reg[inst.reg1] != vm->reg[inst.reg2]) vm->pc += 2;
            break;
        case SET_DELAY:
            vm->delay = vm->reg[inst.reg1];
            break;
        case STORE_DELAY:
            vm->reg[inst.reg1] = vm->delay;
            break;
        case SET_SOUND:
            vm->sound = (vm->reg[inst.reg1] > 1) ? vm->reg[inst.reg1] : 0;
            break;
        case WAIT_FOR_KEY:
        case SKP_IF_KEY:
        case SKP_IF_NOT_KEY:
            // TODO: Need SDL hooked up first.
            break;
        case LOAD_I:
            vm->i = inst.data;
            break;
        case ADD_I:
            vm->i += vm->reg[inst.reg1];
            break;
        case DRAW: {
            uint8_t x = vm->reg[inst.reg1];
            uint8_t y = vm->reg[inst.reg2];
            bool changed = draw_sprite(vm->gfx, x, y, &vm->ram[vm->i], (uint8_t) inst.data);
            vm->reg[0xF] = changed ? 0x1 : 0x0;
            break;
        }
        case LOAD_DIGIT_SPRITE:
            vm->i = DIGIT_BASE_ADDR + DIGIT_LEN * vm->reg[inst.reg1];
            break;
        case STORE_BCD: {
            uint8_t x = vm->reg[inst.reg1];
            vm->ram[vm->i] = x / 100;
            vm->ram[vm->i + 1] = (x / 10) % 10;
            vm->ram[vm->i + 2] = x % 100;
            break;
        }
        case SAVE_REG:
            for (int j = 0; j <= inst.reg1; j++) {
                vm->ram[vm->i + j] = vm->reg[j];
            }
            vm->i += inst.reg1 + 1;
            break;
        case RESTORE_REG:
            for (int j = 0; j <= inst.reg1; j++) {
                vm->reg[j] = vm->ram[vm->i + j];
            }
            vm->i += inst.reg1 + 1;
            break;
        case INVALID:
        default:
            printf("!!! INVALID INSTRUCTION %#04x !!!", raw_inst);
            return ERR_INVALID;
    }
    return SUCCESS;
}

tick_result vm_run_frame(chip8_vm* vm) {
    tick_result res;
    for (int i = 0; i < vm->tpu; i++) {
        res = vm_tick(vm);
        if (res != SUCCESS) return res;
    }
    vm->sound--;
    vm->delay--;
    display_screen(vm->gfx);
    fps_clock_tick(&vm->clock);
    return SUCCESS;
}

uint8_t* read_binary_file(char *path, long* size_out) {
    FILE* file = fopen(path, "rb");
    if (file == NULL) {
        printf("Error opening file \"%s\"", path);
        return NULL;
    }
    // Get file size by seeking to the end of the file
    fseek(file, 0, SEEK_END);
    long size = ftell(file);
    rewind(file);
    uint8_t* buf = malloc(size);
    fread(buf, sizeof(uint8_t), size, file);
    if (ferror(file)) return NULL;
    *size_out = size;
    return buf;
}


int main(int argc, char *argv[]) {
    if (argc < 2) {
        puts("ERROR: ROM path not given");
        return 1;
    }
    char* rom_path = argv[1];
    long rom_size;
    uint8_t* rom = read_binary_file(rom_path, &rom_size);
    sdl_handle h = graphics_init();
    clear_screen(&h);
    display_screen(&h);
    chip8_vm vm;
    vm_load_program(&vm, &h, 8, rom, rom_size);
    bool quit = false;
    SDL_Event e;
    tick_result res;
    puts("Starting main loop");
    while (!quit) {
        while (SDL_PollEvent(&e) != 0) {
            if (e.type == SDL_QUIT) {
                quit = true;
                break;
            }
        }
        res = vm_run_frame(&vm);
        switch (res) {
            case ERR_INVALID:
                puts("Invalid instruction!!!");
                break;
            case ERR_STACK_OVERFLOW:
                puts("Stack Overflow!!!");
                break;
            case ERR_STACK_UNDERFLOW:
                puts("Stack Underflow!!!");
                break;
            default:
                break;
        }
    }
    return 0;
}
