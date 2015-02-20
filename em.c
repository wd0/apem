#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <limits.h>
#include <stdarg.h>
#include <err.h>

enum { INT8_BITS  = sizeof(uint8_t) * CHAR_BIT };

enum { STACK_SIZE = 256 };

enum CPU_flag {
    CARRY       = 0x01,
    ZERO        = 0x02,
    NEG         = 0x04,
    OVERFLOW    = 0x08,
    DECMODE     = 0x10,
    BREAKCMD    = 0x20,
    INTDIS      = 0x40,
};

struct cpu {
    uint16_t pc;
    uint8_t sp;
    int8_t acc;
    int8_t x;
    int8_t y; 
    enum CPU_flag flag;
};

#define FLAG(cpu, flags) ((cpu)->flag |= (flags))
#define UNFLAG(cpu, flags) ((cpu)->flag &= ~(flags))
#define SET(cpu, flags, v) if (!(v)) { UNFLAG((cpu), (flags)); } else { FLAG((cpu), (flags)); } 
#define GET(cpu, FLAGS) (!!((cpu)->flag & (FLAGS)))
#define MSB(n, size) ((n) & 1 << ((size)-1))
#define LSB(n, size) ((n) & 1)

const char *
load(FILE *fin) {
    const char *text = "LDA #0\n";
    return text;
}

enum OPS { 
    LDA, LDX, LDY, STA, STX, STY,
    /* Register transfers */
    TAX, TAY, TXA, TYA,
    /* Stack operations */
    TSX, TXS, PHA, PHP, PLA, PLP,
    /* Logic */
    AND, EOR, ORA, BIT,
    /* Arithmetic */
    ADC, SBC, CMP, CPX, CPY,
    /* Increments */
    INC, INX, INY, DEC, DEX, DEY,
    /* Shifts */
    ASL, LSR, ROL, ROR,
    /* Jumps and calls */
    JMP, JSR, RTS,
    /* Branches */
    /* Status flag changes */
    CLC, CLD, CLI, CLV, SEC, SED, SEI,
    /* Special */
    NOP,
};

void
push(int8_t *const stack, uint8_t *sp, int8_t v) {
    stack[*sp++] = v;
}
uint8_t
pop(int8_t *stack, uint8_t *sp) {
    return stack[--*sp];
}
void
stack_set(int8_t *stack, uint8_t offset, int8_t v) {
    stack[offset] = v;
}
int8_t
access(const int8_t *stack, uint8_t offset) {
    return stack[offset];
}

/* TODO: Potential undefined behavior */
static int8_t
rol(struct cpu *cpu, register int8_t n) {
    SET(cpu, CARRY, MSB(n, INT8_BITS));
    return n<<1 | GET(cpu, CARRY);
}

static int8_t
ror(struct cpu *cpu, register int8_t n) {
    SET(cpu, CARRY, LSB(n, INT8_BITS));
    return n>>1 | (GET(cpu, CARRY)) << INT8_BITS-1;
}

int
execute(struct cpu *cpu, int op, const int8_t *args, int8_t *const stack) {
    register int8_t ret;

    switch (op) { 
        case LDA:
            cpu->acc = *args;
            break;
        case LDX:
            cpu->x = *args;
            break;
        case LDY:
            cpu->y = *args;
            break;
        case STA:
            stack_set(stack, *args, cpu->acc);
            break;
        case STX:
            stack_set(stack, *args, cpu->x);
            break;
        case STY:
            stack_set(stack, *args, cpu->y);
            break;

        case TAX:
            cpu->x = cpu->acc;
            break;
        case TAY:
            cpu->y = cpu->acc;
            break;
        case TXA:
            cpu->acc = cpu->x;
            break;
        case TYA:
            cpu->acc = cpu->y;
            break;

        case TSX:
            cpu->x = cpu->sp;
            break;
        case TXS:
            cpu->sp = cpu->x; 
            break;
        case PHA:
            push(stack, &cpu->sp, cpu->acc);
            break;
        case PHP:
            push(stack, &cpu->sp, cpu->flag);
            break;
        case PLA:
            cpu->acc = pop(stack, &cpu->sp);
            break;
        case PLP:
            cpu->flag = pop(stack, &cpu->sp);
            break;

        case AND:
            ret = cpu->acc & access(stack, *args);
            SET(cpu, NEG, ret < 0);
            SET(cpu, ZERO, ret == 0);
            break;
        case EOR:
            ret = cpu->acc ^ access(stack, *args);
            SET(cpu, NEG, ret < 0);
            SET(cpu, ZERO, ret == 0);
            break;
        case ORA:
            ret = cpu->acc | access(stack, *args);
            SET(cpu, NEG, ret < 0);
            SET(cpu, ZERO, ret == 0);
            break;
        case BIT:
            break;

        case INC:
            ++stack[*(uint8_t *)args];
            ret = access(stack, *args);
            SET(cpu, NEG, ret < 0);
            SET(cpu, ZERO, ret == 0);
            break;
        case INX:
            ++cpu->x;
            SET(cpu, NEG, cpu->x < 0)
            SET(cpu, ZERO, cpu->x == 0)
            break;
        case INY:
            ++cpu->y;
            SET(cpu, NEG, cpu->y < 0)
            SET(cpu, ZERO, cpu->y == 0)
            break;
        case DEX:
            --cpu->x;
            SET(cpu, NEG, cpu->x < 0)
            SET(cpu, ZERO, cpu->x == 0)
            break;
        case DEY:
            --cpu->y;
            SET(cpu, NEG, cpu->y < 0)
            SET(cpu, ZERO, cpu->y == 0)
            break;
        
        case ASL:
            /* Surely not the correct way to decide where to do this. */
            if (*args == -1) {
                SET(cpu, CARRY, MSB(cpu->acc, sizeof(cpu->acc)));
                cpu->acc <<= 1; 
            } else {
                SET(cpu, CARRY, MSB(stack[*(uint8_t *)args], sizeof *stack));
                stack[*(uint8_t *)args] <<= 1;
            }
            break;
        case LSR:
            if (*args == -1) {
                SET(cpu, CARRY, LSB(cpu->acc, sizeof(cpu->acc)));
                cpu->acc >>= 1; 
            } else {
                SET(cpu, CARRY, LSB(stack[*(uint8_t *)args], sizeof *stack));
                stack[*(uint8_t *)args] >>= 1;
            }
            break;
        case ROL:
            if (*args == -1) {
                SET(cpu, CARRY, MSB(cpu->acc, sizeof(cpu->acc)));
                cpu->acc <<= 1; 
            } else {
                SET(cpu, CARRY, MSB(stack[*(uint8_t *)args], sizeof *stack));
                stack[*(uint8_t *)args] <<= 1;
            }
            break;
        case ROR:
            if (*args == -1) 
                cpu->acc = ror(cpu, cpu->acc);
            else
                stack[*(uint8_t *)args] = ror(cpu, stack[*(uint8_t *)args]); 
            break;

        case CLC:
            UNFLAG(cpu, CARRY);
            break;
        case CLD:
            UNFLAG(cpu, DECMODE);
            break;
        case CLI:
            UNFLAG(cpu, INTDIS);
            break;
        case CLV:
            UNFLAG(cpu, OVERFLOW); 
            break;
        case SEC:
            FLAG(cpu, CARRY);
            break;
        case SED:
            FLAG(cpu, DECMODE);
            break;
        case SEI:
            FLAG(cpu, INTDIS);
            break;
        case NOP:
            ;
            break;

        default:
            err(1, "Execution failed on `%d'", op);
    }

    return 0;
}

void
show_cpu_state(const struct cpu *cpu) {
    printf("cpu %p {\n"
            "    uint16_t pc  = %8d;\n"
            "    uint8_t  sp  = %8d;\n"
            "    uint8_t acc  = %8d;\n"
            "    uint8_t   x  = %8d;\n"
            "    uint8_t   y  = %8d;\n"
            "    uint8_t flag {\n"
            "        carry    = %d\n"
            "        zero     = %d\n"
            "        neg      = %d\n"
            "        overflow = %d\n"
            "        decmode  = %d\n"
            "        breakcmd = %d\n"
            "        intdis   = %d\n"
            "    }\n"
            "}\n", cpu, cpu->pc, cpu->sp, cpu->acc, cpu->x, cpu->y,
            GET(cpu, CARRY),
            GET(cpu, ZERO),
            GET(cpu, NEG),
            GET(cpu, OVERFLOW),
            GET(cpu, DECMODE),
            GET(cpu, BREAKCMD),
            GET(cpu, INTDIS));
}

void
show_stack_state(const int8_t *stack, size_t stack_size) {
    for (size_t i = 0; i < stack_size; ++i)
        printf("%4d%c", stack[i], (i+1)%(2<<3) == 0 ? '\n' : ' ');
    putchar('\n');
}

/* TODO: args being signed has invited a lot of undefined behavior. */
int
run(struct cpu *cpu, const char *const text, int8_t *const stack) {
    int8_t args[4];
    cpu->sp = 0;
    size_t i;
    size_t nshift;

    *args = 8;
    execute(cpu, LDA, args, stack);
    for (i = 0; i < STACK_SIZE; ++i) {
        *args = i;
        execute(cpu, STA, args, stack);
    }
    printf("+ stack set to 8\n");
    printf("+ ror\n");
    *args = 1;
    for (i = 0; i < STACK_SIZE; ++i) {
        *args = i;
        for (nshift = 0; nshift < 7; ++nshift) {
        execute(cpu, ROR, args, stack);
        }
        show_stack_state(stack, STACK_SIZE);

    }
    return 0;
} 

int
main(const int argc, const char *const *const argv) {
    struct cpu c6502 = { 0, 0, 0, 0, 0, 0 };
    struct cpu *cpu = &c6502;
    FILE *sourcefile;
    const char *text;
    const char *filename;
    int8_t stack[STACK_SIZE];
    if (argc == 2) {
        filename = argv[1];
        if ((sourcefile = fopen(filename, "r")) == NULL)
            err(1, "Could not open file %s", filename);
    } else 
        sourcefile = stdin;
    if ((text = load(sourcefile)) == NULL)
        err(1, "Could not load program text from %s", filename);
    return run(cpu, text, stack);
}
