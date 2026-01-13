/* benos7/src/user_app.S */

/* 将代码放在自定义段中，段名为你的名字拼音，例如 .yuxianze */
.section .yuxianze, "ax"
.global user_entry

user_entry:
    /* 1. 准备系统调用参数 */
    /* 实验6约定：s7 存放系统调用号 */
    li s7, 11   /* 11 是我们在 trap.c 中定义的打印页表的功能号 */
    
    /* 2. 发起系统调用 (Environment Call) */
    ecall

    /* 3. 死循环，防止程序跑飞 */
loop:
    j loop