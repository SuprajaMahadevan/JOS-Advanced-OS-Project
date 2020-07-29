# Lab 5

## Warning

My solution to the challenge broke the testing code. The `testshell` test fails when run via the grading script but passes when run manually. I dug into the grading script and ran the exact same `qemu` commands to run the test and it passes when run standalone. After some more debugging I traced the problem to the call to `sys_cgetc()` on line 194 of `sh.c`. I don't know why polling the console for input fails the test. When running the test via the grading script, it fails almost instantaneously whereas the runtime of the test is a couple of seconds. I don't think `sys_cgetc()` is somehow consuming the output so that the grading script can't see it. I feel like the first `sys_cgetc()` causes the connection to `qemu` to die. I've already spent a lot of time on the challenge exercise and on debugging the testing code, so I'm going to leave it as is.

## Exercise 1

Nothing else needs to be done in order to ensure this privilege gets preserved as we switch from one environment to another. There's no danger that this flag somehow 'leaks' to other processes. When switching from the file system environment to another environment, the two trapframes are completely separate. If the file system environment forks, then the child would inherit the `EFLAGS` register value and hence the I/O privilege. That is acceptable because presumably the kernel trusts the file system environment not to fork off a malicious child. A random user environment cannot set this flag by itself because it is running at CPL 3 and the CPU only lets processes running at CPL 0 edit the `EFLAGS` register.

## Exercises 2 & 3 & 4

Exercises were pretty straightforward, code is done.

## Exercises 5 & 6

I had some trouble with the big file test in `testfile`. I was not setting the indirect block number in `file_block_walk()` after allocating it. I was also treating the `f_indirect` field as a pointer to the block as opposed to the block number. It turns out that other part of the source rely on `f_indirect` being a block number and not a pointer. This mean that whenever I want to find an indirect block, I have to transform `f_indirect` into the in memory address, cast it to a pointer and then dereference it as an array.

## Exercise 7

Implementing `exec` in user space is difficult because the function is meant to replace the caller's process image in-place without returning. So after calling `exofork()` and returning in the child, a call to `exec` should load up a process image and start executing it. It would be easiest to go in the kernel and load the image into the process that way but if we had to implement it in user space, I would load the `exec` code in the child's exception stack, jump there, execute `exec`, load the process image and return.

## Exercise 8

While working on this, I found a bug in the IPC code, specifically in `ipc_recv()`. I was incorrectly setting the permissions of the incoming pages. Took me a while to debug but finally tracked it down.

## Exercise 9

Straight forward exercise.

## Exercise 10

Discovered a problem with `duppage()` while working on this. For pages that were present but not writable I was using the wrong mask for permissions (`0xFFF` instead of `PTE_SYSCALL`). The input redirection code was an exact mirror copy of the output redirection code except that you're opening the file with different permissions and duping it to stdin instead of stdout.

## Challenge

For the challenge exercise, I chose to add ctl-c support to the shell. The "feature request" statement in the lab was a little unclear: "ctl-c to kill the current environment". Which environment do you treat as the current environment? There are three environments at play in the shell: `init` -- which continuously spawns the shell binary in a loop; `sh` -- which parses commands/deals with I/O and pipes; and the forked child from `sh` that actually executes the commands. I decided to add support for killing the shell and for killing the child.

To try out the new feature, build JOS with `make run-icode`. That will drop you at the shell prompt. A single ctl-c from here will kill the shell and drop you in the kernel monitor because there are no more environments to run. You can also run the `cpuhog` binary from the shell and send a ctl-c. `cpuhog` is a program I added that just spins the CPU, never giving control back to the shell. If you send a ctl-c while `cpuhog` is running, the process gets killed and you return to the shell.

To support killing the shell environment, I had to introduce environment return codes. It is not enough to simply exit from the shell every time we detect a ctl-c because our parent will just restart us (`init` runs the shell in a `while (1)` loop). The shell needs a mechanism to convey its intention to the caller. I added environment return codes by extending `struct Env` with an extra field, `env_retcode`. This field is set when the environment is destroyed, via `sys_env_destroy()`/`exit()` (the signatures of both of these methods were adjusted to take as an argument the return code). The field is initialized in `env_alloc()` and is read by `wait()`. Thus, when the shell see a ctl-c (as detected by `readline()`), it will kill itself with a return code of -2. In the parent, the call to `wait()` will return the retcode and `init` can decide whether to restart the shell or not. In our case, a retcode of -2 means that we don't want to be restarted.

Unfortunately, there is a major race in the above design. Nothing prevents `wait()` from reading an incorrect retcode value. Consider a scenario where a forked environment exits, `env_destroy()` is called and the environment is marked as free. At this point that `struct Env` can be reused by the scheduler for another environment. Theoretically, it is possible for the scheduler to re-assign the same `struct Env` to another environment, that in turn executes, exits and sets the retcode to something different. The CPU then resumes execution of the `wait()` call, which reads the updated retcode believing it is the retcode of the previous environment. This can be solved by introducing a zombie state for environments: when an environment exits, it becomes a zombie (whose envid and `struct Env` cannot yet be reassigned) until its parent reaps it via a call to `wait()`, at which point the environment completely frees up. This guarantees that `wait()` will see the correct retcode because the environment is not completely free, and hence not re-useable, until `wait()` completes. I believe this is how Linux implements return codes.

As a first pass implementation for killing a forked child, I added code in `kbd_intr()` that would detect a ctl-c and then `sys_env_destroy()` whatever `curenv` was pointing to. This is incorrect because we have no guarantee that `curenv` will point to the forked child. Every time a process gets scheduled, `curenv` gets updated. Trying to kill the forked child in this manner is a lottery. A better design is to have the shell "listen" for a ctl-c and kill the forked child if it hears such a signal. To make this work I added a `custom_wait()` function in `sh.c` that yields the CPU waiting for the child to free up, and also retrieves characters from the console, checking if any of them are a ctl-c. As soon as it sees a ctl-c, it kills the forked child.

Finally, in order to figure out that character code 3 corresponds to ctl-c, I had to spend some time stepping through the code in `console.c`. Special characters such as ctl and alt are stored in a static variable and influence the meaning of the next character by changing its look up table.
