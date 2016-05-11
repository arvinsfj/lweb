//
//  vm.c
//  
//
//  Created by cz on 5/10/16.
//
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <memory.h>

#define poolsize (256 * 1024)
#define DEBUG 1

int *stack;
char *data;
int *text;

// instructions
enum { LEA ,IMM ,JMP ,JSR ,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,
    OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,
    OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT };


int machine(int *cs, char *ds, int *ss, int debug){
    
    int opcode, cycle, ax, *pc, *sp, *bp, *tmp;
    
    pc = cs;
    bp = sp = ss;
    ax = 0;
    
    cycle = 0;
    
    while (1) {
        opcode = *pc++; cycle++;
        
        if (debug) {
            printf("%d> %.4s", cycle,
                   &"LEA ,IMM ,JMP ,JSR ,JZ  ,JNZ ,ENT ,ADJ ,LEV ,LI  ,LC  ,SI  ,SC  ,PSH ,"
                   "OR  ,XOR ,AND ,EQ  ,NE  ,LT  ,GT  ,LE  ,GE  ,SHL ,SHR ,ADD ,SUB ,MUL ,DIV ,MOD ,"
                   "OPEN,READ,CLOS,PRTF,MALC,MSET,MCMP,EXIT,"[opcode * 5]);
            if (opcode <= ADJ) printf(" %d\n", *pc); else printf("\n");
        }
        
        //mov
        //IMM <num> and LI/SI and LC/SC
        if (opcode == IMM) {
            //IMM <num> : load num to ax
            ax = *pc; pc++;
        }
        else if(opcode == LC){
            //LC : load char(*ax) to ax
            ax = *(char *)ax;
        }
        else if(opcode == LI){
            //LI : load int(*ax) to ax
            ax = *(int *)ax;
        }
        else if(opcode == SC){
            //SC : save ax to char(**sp)
            ax = *(char *)(*sp) = ax; sp++;
        }
        else if(opcode == SI){
            //SI : save ax to int(**sp)
            *(int *)(*sp) = ax; sp++;
        }
        
        //push
        else if(opcode == PSH){
            //PSH : push ax to int(*--sp)
            sp--; *sp = ax;
        }
        
        //jump
        else if(opcode == JMP){
            //JMP <addr> : save addr to pc
            pc = (int *)*pc;
        }
        else if(opcode == JZ){
            //JZ <addr> : if ax==0 then save addr to pc
            pc = !ax ? (int *)*pc : pc + 1;
        }
        else if(opcode == JNZ){
            //JNZ <addr> : if ax!=0 then save addr to pc
            pc =  ax ? (int *)*pc : pc + 1;
        }
        
        //call subroute
        else if(opcode == ENT){
            //ENT <size> : pre save bp to int(*--sp), save sp to bp, then add sp size
            sp--;*sp = (int)bp;bp = sp;
            sp = sp - *pc; pc++;
        }
        else if(opcode == JSR){
            //JSR <addr> : pre save next instructions addr(pc + 1) to int(*--sp) then save addr to pc
            sp--;*sp = (int)(pc + 1);
            pc = (int *)*pc;
        }
        else if(opcode == ADJ){
            //ADJ <size> : sub sp size
            sp = sp + *pc; pc++;
        }
        else if(opcode == LEV){
            //LEV: pre save bp to sp, save *sp++ to bp, then save *sp++ to pc
            sp = bp; bp = (int *)*sp; sp++; pc = (int *)*sp; sp++;
        }
        else if(opcode == LEA){
            //LEA <offset> : load addr(bp + offset) to ax
            ax = (int)(bp + *pc); pc++;
        }
        
        //math
        else if(opcode == OR){
            //<num1> OR <num2> : save (num1 | num2) to ax
            ax = (int)(*sp | ax); sp++;
        }
        else if(opcode == XOR){
            //<num1> XOR <num2> : save (num1 ^ num2) to ax
            ax = (int)(*sp ^ ax); sp++;
        }
        else if(opcode == AND){
            //<num1> AND <num2> : save (num1 & num2) to ax
            ax = (int)(*sp & ax); sp++;
        }
        else if(opcode == EQ){
            //<num1> EQ <num2> : save (num1 == num2) to ax
            ax = (int)(*sp == ax); sp++;
        }
        else if(opcode == NE){
            //<num1> NE <num2> : save (num1 != num2) to ax
            ax = (int)(*sp != ax); sp++;
        }
        else if(opcode == LT){
            //<num1> LT <num2> : save (num1 < num2) to ax
            ax = (int)(*sp < ax); sp++;
        }
        else if(opcode == LE){
            //<num1> LE <num2> : save (num1 <= num2) to ax
            ax = (int)(*sp <= ax); sp++;
        }
        else if(opcode == GT){
            //<num1> GT <num2> : save (num1 > num2) to ax
            ax = (int)(*sp > ax); sp++;
        }
        else if(opcode == GE){
            //<num1> GE <num2> : save (num1 >= num2) to ax
            ax = (int)(*sp >= ax); sp++;
        }
        else if(opcode == SHL){
            //<num1> SHL <num2> : save (num1 << num2) to ax
            ax = (int)(*sp << ax); sp++;
        }
        else if(opcode == SHR){
            //<num1> SHR <num2> : save (num1 >> num2) to ax
            ax = (int)(*sp >> ax); sp++;
        }
        else if(opcode == ADD){
            //<num1> ADD <num2> : save (num1 + num2) to ax
            ax = (int)(*sp + ax); sp++;
        }
        else if(opcode == SUB){
            //<num1> SUB <num2> : save (num1 - num2) to ax
            ax = (int)(*sp - ax); sp++;
        }
        else if(opcode == MUL){
            //<num1> MUL <num2> : save (num1 * num2) to ax
            ax = (int)(*sp * ax); sp++;
        }
        else if(opcode == DIV){
            //<num1> DIV <num2> : save (num1 / num2) to ax
            ax = (int)(*sp / ax); sp++;
        }
        else if(opcode == MOD){
            //<num1> MOD <num2> : save (num1 % num2) to ax
            ax = (int)(*sp % ax); sp++;
        }
        
        //system function
        else if(opcode == EXIT){
            //EXIT : print exit(*sp) and return *sp
            printf("exit(%d) cycle = %d\n", *sp, cycle);
            return *sp;
        }
        else if(opcode == PRTF){
            //PRTF : save printf to ax
            tmp = sp + *(pc + 1);
            ax = printf((char *)(*(tmp-1)), *(tmp-2), *(tmp-3), *(tmp-4), *(tmp-5), *(tmp-6));
        }
        else if(opcode == MALC){
            //MALC : save malloc(*sp) to ax
            ax = (int)malloc(*sp);
        }
        else if(opcode == MSET){
            //MSET : save memset(*(sp + 2), *(sp + 1), *sp) to ax
            ax = (int)memset((char *)(*(sp+2)), *(sp + 1), *sp);
        }
        else if(opcode == MCMP){
            //MCMP : save memcmp(*(sp + 2), *(sp + 1), *sp) to ax
            ax = (int)memcmp((char *)(*(sp+2)), (char *)(*(sp+1)), *sp);
        }
        else if(opcode == OPEN){
            //OPEN : save open(*(sp + 1), *sp) to ax
            ax = open((char *)(*(sp + 1)), *sp);
        }
        else if(opcode == CLOS){
            //CLOS : save close(*sp) to ax
            ax = close(*sp);
        }
        else if(opcode == READ){
            //READ : save read(*(sp + 2), *(sp + 1), *sp) to ax
            ax = read(*(sp + 2), (char *)(*(sp + 1)), *sp);
        }
        else{
            printf("unknown instruction = %d! cycle = %d\n", opcode, cycle);
            return -1;
        }
    }
    
    return 0;
}

//gcc vm.c -o vm -m32 -ansi
//./vm
int main(int ac, char **av)
{
    int i, *tmp;
    // allocate memory for virtual machine
    if (!(text = malloc(poolsize))) {
        printf("could not malloc(%d) for text area\n", poolsize);
        return -1;
    }
    if (!(data = malloc(poolsize))) {
        printf("could not malloc(%d) for data area\n", poolsize);
        return -1;
    }
    if (!(stack = malloc(poolsize))) {
        printf("could not malloc(%d) for stack area\n", poolsize);
        return -1;
    }
    memset(text, 0, poolsize);
    memset(data, 0, poolsize);
    memset(stack, 0, poolsize);
    
    //setup stack
    stack = (int *)((int)stack + poolsize);
    *--stack = EXIT; // call exit if main returns
    *--stack = PSH; tmp = stack;
    *--stack = ac;
    *--stack = (int)av;
    *--stack = (int)tmp;
    
    //test vm asm
    i = 0;
    data[i++] = 'H';
    data[i++] = 'o';
    data[i++] = 'l';
    data[i++] = 'l';
    data[i++] = 'e';
    data[i++] = ' ';
    data[i++] = 'W';
    data[i++] = 'o';
    data[i++] = 'r';
    data[i++] = 'l';
    data[i++] = 'd';
    data[i++] = '!';
    data[i++] = '\n';
    i = 0;
    text[i++] = IMM;
    text[i++] = (int)data;
    text[i++] = PSH;
    text[i++] = PRTF;
    text[i++] = ADJ;
    text[i++] = 1;
    text[i++] = IMM;
    text[i++] = 100;
    text[i++] = PSH;
    text[i++] = IMM;
    text[i++] = 200;
    text[i++] = PSH;
    text[i++] = IMM;
    text[i++] = 400;
    text[i++] = ADD;
    text[i++] = ADD;
    text[i++] = PSH;
    text[i++] = EXIT;
    
    machine(text, data, stack, DEBUG);
    
    return 0;
}

