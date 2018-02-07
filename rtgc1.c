/*
 * Quick and dirty implementation of Henry G. Baker, Jr.'s algorithm
 * from "List Processing in Real Time on a Serial Computer"
 *
 * --Tommy 2018-02-06
 */

#define INCREMENTALGC 1

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>

typedef union value_s value;
typedef value *ptr;

union value_s {
    ptr p;
    int i;
};

#define HEAPSIZE 200
#define K 1
#define NR 32

// The Minsky-Fenichel-Yochelson-Cheney-Arnborg [26,18,11,1] Garbage Collector.
ptr B;                                  // Bottom; points to bottom of free area.
ptr S;                                  // Scan; points to first untraced cell.
ptr T;                                  // Top; points to top of tospace.
ptr R[NR];

value HEAP[HEAPSIZE];
ptr Tstart, Tend, Fstart, Fend;

void flip(void)
{
    ptr x = Tstart;
    Tstart = Fstart;
    Fstart = x;

    ptr y = Tend;
    Tend = Fend;
    Fend = y;

    S = B = Tstart;
    T = Tstart + HEAPSIZE/2;
}

void error(void) {printf("out of memory!\n"); exit(1);}
bool tospace(ptr p)  {return Tstart <= p && p < Tend;}
bool fromspace(ptr p){return Fstart <= p && p < Fend;}
// Forwarding declarations
ptr move(ptr p);
ptr copy(ptr p);
ptr cons(ptr x, ptr y);
void rplaca(ptr x,ptr y);
void rplacd(ptr x,ptr y);
ptr car(ptr);
ptr cdr(ptr);

// Assertions: S<=B<=T and T-B is even.
void invariants(void)
{
    assert(S <= B);
    assert(B <= T);
    assert((T - B) % 2 == 0);
}


/////////////////////////////////////////////////

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

#if INCREMENTALGC == 0
        while (S < B) {                 // Trace all accessible cells.
            S[0].p = move(S[0].p);      // Update the car and cdr.
            S[1].p = move(S[1].p);
            S = S + 2;                  // Point to next untraced cell.
        }
        printf("Tospace after GC: %zd\n", B - Tstart);
#endif
    }

#if INCREMENTALGC == 1
                                        // Do K iterations of GC
    for (int i = 0; i < K && S < B; ++i) {
        S[0].p=move(S[0].p);            // Update the car and cdr.
        S[1].p=move(S[1].p);
        S=S+2;                          // Point to next untraced cell.
    }
#endif

   // printf("Scanned %zd Allocated %zd\n", S - Tstart, B - Tstart);

    // Print the heap
    for (ptr p = HEAP;; p += 2) {
        if (p == HEAP || p == HEAP+HEAPSIZE/2 || p == HEAP+HEAPSIZE) putchar('|');
        if (p == S) putchar('S');
        if (p == B) putchar('B');
        if (p == T) putchar('T');

        if (p == HEAP + HEAPSIZE)
            break;

        if (fromspace(p))
            putchar(tospace(p[0].p) ? '+' : ' ');

#if INCREMENTALGC == 0
        else if (p < S)
            putchar('r');
        else if (p < B)
            putchar('n');
#endif

#if INCREMENTALGC == 1
        else if (p < S)
            putchar('r');
        else if (p < B)
            putchar('t');
        else if (T <= p)
            putchar('n');
#endif
        else
            putchar('_');
    }
    putchar('\n');

    if (T <= B) error();                // Memory is full.

#if INCREMENTALGC == 0
    new = B;
    B += 2;
#endif

#if INCREMENTALGC == 1
    T -= 2;
    new = T;
#endif

    new[0].p = x; new[1].p = y;          // Create new cell at bottom of free area
    return new;
}


#if INCREMENTALGC == 0
ptr car(ptr x) {return x[0].p;}         // A cell consists of 2 words:
ptr cdr(ptr x) {return x[1].p;}         //   car is 1st; cdr is 2nd.
#endif

#if INCREMENTALGC == 1
ptr car(ptr x) {return move(x[0].p);}   // A cell consists of 2 words:
ptr cdr(ptr x) {return move(x[1].p);}   //   car is 1st; cdr is 2nd.
#endif

void rplaca(ptr x,ptr y) {x[0].p=y;}    // car(x) = y

void rplacd(ptr x,ptr y) {x[1].p=y;}    // cdr(x) = y

bool eq(ptr x,ptr y) {return x==y;}     // Are x,y the same object?

bool atom(ptr x)                        // Is x an atom?
{
    return x <= HEAP || HEAP+HEAPSIZE <= x
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
        if (!tospace(p[0].p))           // We must move p.
            p[0].p = copy(p);           // Copy it into the bottom of free area.
        return p[0].p;                  // Leave and return forwarding address.
    }
}

ptr copy(ptr p)                         // Create a copy of a cell.
{                                       // Allocate space at bottom of free area.
    assert(!atom(p));
    if (B>=T) error();                  // Memory full?
    B[0]=p[0]; B[1]=p[1];               // Each cell requires 2 words.
    B=B+2;                              // Return the current value of B
    return B-2;                         //   after moving it to next cell.
}

//  TOSPACE, FROMSPACE test whether a pointer is in that semispace.


void workload1(void)
{
    for (int i = 1; i < 32; ++i)
        R[i] = cons(R[0],R[i-1]);

    for (int n = 200; n > 0; --n) {
        for (int i = 1; i < 32; i += 2)
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
        R[i % 32] = cons(integer(1), integer(2));
}

void workload(void)
{
    // [2..K, 0]
    for (int i = 50; 2 <= i; --i)
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
}

int main(int c, char **v)
{
#if INCREMENTALGC == 1
    printf("S - Scan pointer\n"
           "B - Free space for live objects\n"
           "T - Allocation pointer (for new objects)\n"
           "  - garbage\n"
           "+ - moved to tospace\n"
           "s - scanned\n"
           "t - to be scanned\n"
           "n - brand new\n"
           );
#endif
    Tstart=HEAP; Tend=HEAP+HEAPSIZE/2;
    Fstart=Tend; Fend=HEAP+HEAPSIZE;
    flip();

    for (int i = 0; i < 32; ++i)
        R[i] = integer(666);

    workload();

    return 0;
}
