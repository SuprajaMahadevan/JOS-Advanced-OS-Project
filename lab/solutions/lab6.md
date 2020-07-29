# Lab 6

## Exercise 1

To take into account multiple CPUs all generating time interrupts, I introduced an array of tick counts, where the index in the array is the number of the CPU that generated the time interrupt. To associate tick counts with CPU numbers, I changed some of the methods in `time.c` to accept the CPU number as an argument.

## Exercises 2 & 3

Read/skimmed the relevant chapters of the Intel manual. Added the network vendor and product ID constants to the `e1000.h` file and added a function to attach and enable the network card. I chose to keep the network attach function in the `pci.c` file because that is where the bridge attach function also lives.

## Exercise 4

Mapped the register base memory in `MMIOBASE` -- `MMIOLIM` range and added a couple of register offsets to `e1000.h`. I used the same trick as in `lapic.c`, of dividing the offset by 4 so it can be used as an index in the register array.

## Exercise 5

I decided to statically allocated memory for the transmission queue and the packet buffers. I also moved the `pic_network_attach()` function over to the `e1000.c` file because it started growing and became became very E1000 specific.

## Exercise 6

I added the `e1000_transmit()` function, which takes two arguments: a pointer to the packet data and the length of the packet data. It took me some debugging to figure out that it was the software's responsibility to implement the wrap-around of the tail pointer. I also didn't immediately think to set the end of packet bit but soon realized that the buffers allocated will fit the entire packet. The `e1000_transmit()` function returns -1 if the descriptor queue is full. I thought it would be better for the caller to decide on the strategy to use in case no packets could be transmitted. Implementing a strategy in the driver code would force callers into using that specific strategy, resulting in a less composable and extensible design.

## Exercise 7 & 8

The `output()` function loops forever, at each iteration accepting a new transmission request. If the `sys_e1000_transmit()` system call returns `-E_E1000_TXBUF_FULL`, the packet is dropped. The system call will attempt to transmit the packet 20 times before giving up. Note that after each such attempt, `sys_e1000_transmit()` will yield the CPU so the system is not blocked. I think it's a fair trade off to drop a packet after 20 retries as the upper level protocols should be able to handle it.

While working on these two exercises I did run into a "fun" bug. I was bitwise OR-ing the command section of the transmission descriptor with the following hex literal `0x00001000`. The goal was to turn on the 3rd bit (indexed from 0). Except that even though `0x00001000` looks like binary, it's not, it's a hex literal :) So `0x00001000` is 4096 in decimal and `1 0000 0000 0000` in binary. To fix this I bitwise OR-ed the register with the hex literal `0x8`, which is `1000` in binary and everything worked as expected.

## Exercise 9 & 10

I modeled the receive initialization code after the transmit initialization code. I statically allocate the receive packet buffers and receive descriptors. The receive descriptors are then set to point to the receive buffers. The remaining initialization is done by turning bits on/off using bitwise OR/AND. I chose packet buffers large enough to hold the biggest possible Ethernet packet (2048 byte buffers vs 1518 byte Ethernet packet) and disabled long packet mode. All in all initialization was straight forward except for the MAC byte ordering, that took me a couple of tries to get right.

## Exercise 11 & 12

Getting interrupts to work for the receive functionality was the hardest exercise I've had to complete for this class so far.

I quickly figured out which E1000 registers control the receive interrupts and added `IRQ_E1000` (the E1000 IRQ line is 11) to the trap related files (`trap.c`, `trap.h` and `trapentry.S`). However, even after setting the right bit in the Interrupt Mask Set/Read Register, the interrupt wasn't getting generated. After hours of reading the manual and going over my code, since I don't have access to TAs/Piazza, I decided to look for a hint on Github and stumbled on this [repo](https://github.com/macfij/macfij_jos). To enable the interrupt I had to add it to the IRQ mask set in `irq_setmask_8259A()`. A hint in the lab assignment would have been very helpful!

The next roadblock was not having any runnable environments. The `testinput` binary spawns two environments: `output` and `input`, each controlling transfer and reception to and from the E1000 controller, respectively. After spawning two children, `testinput` waits to receive IPC, which marks the environment as not runnable. `output` also marks itself as not runnable as it waits for IPC after submitting the ARP packet. The only remaining environment, `input`, also becomes not runnable as it waits for an interrupt signaling a new packet. To fix this, I changed the scheduler to run an environment waiting to receive a packet (by checking the `env_e1000_waiting_rx` env field), before dropping into the kernel monitor. The special casing doesn't make this solution very elegant so I'd love to hear of a better one.

Finally, I had a bug in the `sys_e1000_transmit()` system call. Specifically, I forgot to set the return value to `-E_E1000_RXBUF_EMPTY` when yielding the CPU in the case the receive queue was empty. This meant that the system call would break out of the while loop in `input()`, continuously sending the same packet.

### Question

How did you structure your receive implementation? In particular, what do you do if the receive queue is empty and a user environment requests the next incoming packet?

Instead of cycling the CPU waiting for the next packet to hit the E1000 controller, I enabled the Receiver Timer Interrupt, which gets triggered every time a new packet arrives. If the receive queue is empty, `e1000_receive()` returns `-E_E1000_RXBUF_EMPTY` causing `sys_e1000_transmit()` to indicate in its env structure that it is waiting on a packet and to yield the CPU. When the interrupt happens, the handler looks for an environment waiting on a packet, sets it to runnable and exits. `sys_e1000_transmit()` is called in a while loop because after it is marked runnable it returns with `-E_E1000_RXBUF_EMPTY` and the while loop forces another call to `sys_e1000_transmit()`. This time however, the call will succeed because we know there's a packet waiting.

## Exercise 13

The only tough part of this exercise was figuring out which FS related functions to call. Given that `httpd` is running as its own environment, the right level of abstraction is the API presented in `lib.h`.

The web page has a sliding banner that says "Cheesy web page!".

I was on track to finish to lab within a reasonable time frame but getting the interrupts working for receive threw everything off. In total I probably spent 40 hours on this lab.

## Challenge

For the challenge exercise, I chose to implement a system call that would read the MAC address from the EEPROM so that it would no longer have to be hardcoded.

The relevant sections in the manual are 5.3.1, (Software Access to the EEPROM), 5.6 (EEPROM Address Map) and 13.4.4 (EEPROM Read Register). Note that you need to zero out the EEPROM read register in between reads, otherwise it will return the same data. Too bad the manual didn't mention this (or maybe it did but I missed it).

To verify that the code works you can run `make MACADDR=BA:5E:BA:11:67:89 qemu`, with `MACADDR` set to whatever value you like, and as part of the console output QEMU will display the MAC address like so:

```
init: running sh
init: starting sh
ns: ba:5e:ba:11:67:89 bound to static IP 10.0.2.15
NS: TCP/IP initialized.
```

Unfortunately, if you run `make grade` with a different MAC address, the `testinput` test will fail because it hard codes a MAC address in the ARP announcement packet. You can run `make run-http` and point your browser to `http://127.0.0.1:{port}/index.html`, where `{port}` is one of the ports from `make which-ports`, to verify that everything works end to end with a different MAC address.
