/*
 * Quick and dirty implementation of the algorithm from the seminal
 * paper "List Processing in Real Time on a Serial Computer" by Henry
 * G. Baker, Jr.
 *
 * --Tommy 2018-02-07
 */

#include <unistd.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <stdbool.h>

#define NR 32                           // # of registers (root pointers)

typedef union cell_s cell;
typedef cell *ptr;
union cell_s {
    ptr p;
};

// The Minsky-Fenichel-Yochelson-Cheney-Arnborg [26,18,11,1] Garbage Collector.
size_t heapsize = 50;                   // Size of semispace
ptr heap;
ptr Tstart;
ptr B;                                  // Bottom; points to bottom of free area.
ptr S;                                  // Scan; points to first untraced cell.
ptr T;                                  // Top; points to top of tospace.
ptr R[NR];

bool incrementalgc = true;
unsigned K = 4;                         // Number of objects to scan per cons


void flip(void)
{
    Tstart = Tstart == heap ? heap + heapsize : heap;
    S = B = Tstart;
    T = Tstart + heapsize;
}

void error(void) {printf("out of memory!\n"); exit(1);}
bool tospace(ptr p)  {return (uintptr_t) (p - Tstart) < heapsize;}
bool fromspace(ptr p){return !tospace(p);}
// Forwarding declarations
ptr move(ptr p);
ptr copy(ptr p);
void rplaca(ptr x,ptr y);
void rplacd(ptr x,ptr y);
ptr car(ptr);
ptr cdr(ptr);

ptr cons(ptr x, ptr y)                  // Allocate the list cell (x.y)
{
    ptr new;

    if (B == T) {                       // If there is no more free space,
                                        //    collect all the garbage.
                                        // This block is the "garbage collector".
        flip();                         // Interchange semispaces.
        printf("Flip\n");
        for (int i = 0; i < NR; ++i)    // Update all user registers.
            R[i] = move(R[i]);
        x = move(x); y = move(y);       // Update our arguments.

        if (!incrementalgc) {
            while (S < B) {             // Trace all accessible cells.
                S[0].p = move(S[0].p);  // Update the car and cdr.
                S[1].p = move(S[1].p);
                S = S + 2;              // Point to next untraced cell.
            }
            printf("Tospace after GC: %zd\n", B - Tstart);
        }
    }

    if (incrementalgc)
                                        // Do K iterations of GC
        for (int i = 0; i < K && S < B; ++i) {
            S[0].p = move(S[0].p);      // Update the car and cdr.
            S[1].p = move(S[1].p);
            S = S + 2;                  // Point to next untraced cell.
    }

    // Print the heap
    for (ptr p = heap;; p += 2) {
        if ((p - heap) % heapsize == 0) putchar('|');
        if (p == S) putchar('S');
        if (p == B) putchar('B');
        if (p == T) putchar('T');

        if (p == heap + heapsize * 2)
            break;

        if (fromspace(p))
            putchar(tospace(p[0].p) ? '+' : ' ');

        else if (!incrementalgc && p < S)
            putchar('r');
        else if (!incrementalgc && p < B)
            putchar('n');
        else if (incrementalgc && p < S)
            putchar('r');
        else if (incrementalgc && p < B)
            putchar('t');
        else if (incrementalgc && T <= p)
            putchar('n');
        else
            putchar('_');
    }
    putchar('\n');

    if (T <= B) error();                // Memory is full.

    if (!incrementalgc) {
        new = B;
        B += 2;
    } else {
        T -= 2;
        new = T;
    }

    new[0].p = x; new[1].p = y;          // Create new cell at bottom of free area
    return new;
}

ptr car(ptr x)
{
    if (!incrementalgc)
        return x[0].p;
    else
        return move(x[0].p);
}

ptr cdr(ptr x)
{
    if (!incrementalgc)
        return x[1].p;
    else
        return move(x[1].p);
}

void rplaca(ptr x,ptr y) {x[0].p = y;}  // car(x) = y

void rplacd(ptr x,ptr y) {x[1].p = y;}  // cdr(x) = y

bool eq(ptr x,ptr y) {return x == y;}   // Are x,y the same object?

bool atom(ptr x)                        // Is x an atom?
{
    return x <= heap || heap + heapsize * 2 <= x
        || ((intptr_t)x & 1);
}

bool integerp(ptr p) { return (intptr_t)p & 1; }

intptr_t getinteger(ptr p) { return (intptr_t)p >> 1; }

ptr integer(intptr_t v)
{
    ptr p = (ptr) (v*2 + 1);
    assert(atom(p));
    assert(integerp(p));
    assert(getinteger(p) == v);

    return p;
}

ptr move(ptr p)                         // Move p if not yet moved;
{                                       //    return new address.
    if (atom(p) || !fromspace(p))       // We only need to move old ones.
        return p;                       // This happens a lot.
    else {
        if (atom(p[0].p) || !tospace(p[0].p))// We must move p.
            p[0].p = copy(p);           // Copy it into the bottom of free area.
        return p[0].p;                  // Leave and return forwarding address.
    }
}

ptr copy(ptr p)                         // Create a copy of a cell.
{                                       // Allocate space at bottom of free area.
    assert(!atom(p));
    if (B >= T) error();                // Memory full?
    B[0] = p[0]; B[1] = p[1];           // Each cell requires 2 words.
    B = B + 2;                          // Return the current cell of B
    return B - 2;                       //   after moving it to next cell.
}

//  TOSPACE, FROMSPACE test whether a pointer is in that semispace.


void workload1(void)
{
    for (int i = 1; i < NR; ++i)
        R[i] = cons(R[0],R[i-1]);

    for (int n = 200; n > 0; --n) {
        for (int i = 1; i < NR; i += 2)
            R[i] = cons(R[0],R[i-1]);

        assert(car(R[0]) != R[7]);
        rplaca(R[0], R[7]);
        assert(car(R[0]) == R[7]);
        assert(cdr(R[0]) != R[9]);
        rplacd(R[0], R[9]);
        assert(cdr(R[0]) == R[9]);
    }
}

void workload2(void)
{
    for (int i = 0; i < 9999; ++i)
        R[i % NR] = cons(integer(1), integer(2));
}

void workload(void)
{
    // [2..K, 0]
    for (int i = 28; 2 <= i; --i)
        R[0] = cons(integer(i), R[0]);

    printf("Sieve it!\n");

    // Sieve it
    while (!atom(R[0])) {
        int p = getinteger(car(R[0]));
        R[2] = cons(car(R[0]), R[2]);
        R[1] = R[0];
        while (!atom(cdr(R[1])))
            if (getinteger(car(cdr(R[1]))) % p == 0)
                rplacd(R[1],cdr(cdr(R[1])));
            else
                R[1] = cdr(R[1]);
        R[0] = cdr(R[0]);
    }

    for (;!atom(R[2]); R[2] = cdr(R[2]))
        printf(" %zd", getinteger(car(R[2])));
    printf("\n");
}

void usage(char *progname, char *msg1, char *msg2)
{
    fprintf(stderr,
            "%s%s\n"
            "Usage: %s $opts\n"
            "       -s N  for size of semispace in number of cell pairs\n"
            "       -i 1   for incremental GC (default)\n"
            "       -i 0   for stop-the-world GC\n"
            "       -k N  of cell pairs to scan per cons (implies -i)\n",
            msg1, msg2, progname);
    exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
    char ch;
    while ((ch = getopt(argc, argv, "s:k:i:")) != -1) {
        switch (ch) {
            long v;
            char *ep;

        case 'i':
            v = strtol(optarg, &ep, 10);
            if (*ep)
                usage(argv[0], "Can't parse boolean: ", optarg);
            incrementalgc = v != 0;
            break;

        case 's':
            heapsize = strtol(optarg, &ep, 10) * 2;
            if (*ep)
                usage(argv[0], "Can't parse size: ", optarg);
            break;

        case 'k':
            K = strtol(optarg, &ep, 10) * 2;
            if (K < 1 || *ep)
                usage(argv[0], "Illegal incremental size: ", optarg);
            incrementalgc = true;
            break;

        case '?':
        default: {
            char msg[2] = { ch, 0};
            usage(argv[0], "Unknown option: ", msg);
            break;
        }}
    }
    argc -= optind;
    argv += optind;

    if (incrementalgc)
        printf("S - Scan pointer\n"
               "B - Free space for live objects\n"
               "T - Allocation pointer (for new objects)\n"
               "  - garbage\n"
               "+ - moved to tospace\n"
               "s - scanned\n"
               "t - to be scanned\n"
               "n - brand new\n"
               );

    Tstart = heap = calloc(sizeof *heap, heapsize * 2);
    flip();

    for (int i = 0; i < NR; ++i)
        R[i] = integer(666);

    workload();

    return 0;
}
