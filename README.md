# Concurrency

## Sanchit Arora | 2019101047

---

## Requirements

- GCC compiler
- Linux OS

## Running

1. Compile using `gcc q1.c -lphread` (replace `q1` with `q2` or `q3` as required)
2. Run using `./a.out`

## Programs

1. Concurrent Merge Sort (q1)

- A modified version of Merge Sort (selection sort for a subarray of size < 5) was implemented
- Executed using different methods- normal, concurrent using processes and concurrent using threads
- The timings for different input sizes were reported.

2. Back to College (q2)

- Simulation of vaccine distribution in college
- A number of companies, zones and students are given
- Each company produces some batches of vaccines with given probability and distribute to zones
- Each zones create some no. of slots and starts vaccination after filling every slot
- Each student is tested for antibodies. If antibodies are found, they are sent to college, otherwise repeat the process (max. 2 more times)

3. Musical Mayhem (q3)

- Simulation of performing of musicians and singers on a given no. of stages
- There are two types of stages- Acoustic and Electric
- Each performer can perform for random no. of seconds
- No performer will wait after a max. time limit if there is no stage for them
- Performer with instrument type _p_, _g_ or _s_ can perform on any of the stages
- Performer with instrument type _v_ can only perform on acoustic stages
- Performer with instrument type _b_ can only perform on electric stages
- Performer with instrument type _s_ is a singer and has a special ability to join any ongoing performance by a musician (not another singer). In such a case the singer will leave when the musician leaves
- After the performance they go to collect a T-Shirt as reward
- There are limited coordinators and each requires 2 seconds to distribute T-Shirt to a performer

> Implementation details, input format or any other details can be found in individual README for each question
