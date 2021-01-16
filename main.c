#include <stdio.h>
#include <stdint.h>
#include "instruction.c"

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

void chip8_clear() {
    puts("Cleared Screen");
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

} chip8_vm;

void vm_load_program(chip8_vm* vm, uint8_t ticks_per_update, uint8_t *program[], int program_len) {
    for (int i = 0; i < 16; i++){
        vm->reg[i] = 0;
    }
    vm->i = 0;
    vm->delay = 0;
    vm->sound = 0;
    vm->tpu = ticks_per_update;
    vm->stack = new_callstack();
    // Chip-8 programs are loaded at address 200
    vm->pc = 200;
    for (int i = 0; i < program_len; i++) {
        vm->ram[i+200] = *program[i];
    }
    // Load the digit sprites into memory
    for (int i = 0; i < DIGIT_LEN * 16; i++) {
        vm->ram[i + DIGIT_BASE_ADDR] = DIGIT_SPRITES[i];
    }
}

tick_result vm_tick(chip8_vm* vm) {
    uint8_t raw_inst = vm->ram[vm->pc];
    instruction inst = decode_instruction(raw_inst);
    vm->pc++;
    switch (inst.tag) {
        case CLEAR:
            chip8_clear();
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
            uint16_t ret_addr = callstack_pop(&vm->stack);
            if (ret_addr < 0) return ERR_STACK_UNDERFLOW;
            // Add 1 so that we start execution after the call instruction
            // that pushed this return address to the stack
            vm->pc = ret_addr + 1;
            break;
        case SKP_EQ:
            if (vm->reg[inst.reg1] == inst.data) vm->pc++;
            break;
        case SKP_NEQ:
            if (vm->reg[inst.reg1] != inst.data) vm->pc++;
            break;
        case SKP_EQ_REG:
            if (vm->reg[inst.reg1] == vm->reg[inst.reg2]) vm->pc++;
            break;
        case SKP_NEQ_REG:
            if (vm->reg[inst.reg1] != vm->reg[inst.reg2]) vm->pc++;
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
        case DRAW:
            //TODO: Need SDL hooked up first.
            break;
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
}


int main() {
    instruction i = decode_instruction(0x8452);
    printf("Tag is AND: %s\nreg1: %d\nreg2: %d", (i.tag == AND) ? "TRUE" : "FALSE", i.reg1, i.reg2);
    return 0;
}
