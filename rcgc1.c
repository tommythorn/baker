#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

typedef uint32_t ptr;

// The Minsky-Fenichel-Yochelson-Cheney-Arnborg [26,18,11,1] Garbage Collector.
ptr B;                                  // Bottom; points to bottom of free area.
ptr S;                                  // Scan; points to first untraced cell.
ptr T;                                  // Top; points to top of tospace.
                                        // Assertions: S<=B<=T and T-B is even.
ptr CONS(ptr x, ptr y)                  // Allocate the list cell (x.y)
  {
    if (B == T)                         // If there is no more free space,
                                        //    collect all the garbage.
        {                               // This block is the "garbage collector".
          flip();                       // Interchange semispaces.
          for (int i=1; i<NR;++i)       // Update all user registers.
             R[i]=move(R[i]);
          x=move(x); y=move(y);         // Update our arguments.
          while (S<B)                   // Trace all accessible cells.
               {
                 S[0]=move(S[0]);       // Update the car and cdr.
                 S[1]=move(S[1]);
                 S=S+2;                 // Point to next untraced cell.
               }
         }
    if (B>=T) error();                  // Memory is full.
    B[0]=x; B[1]=y;                     // Create new cell at bottom of free area
    B=B+2;                              // Return the current value of B
    return B-2;                         //   after stepping it to next cell.
  }

ptr CAR(ptr x) {return x[0];}           // A cell consists of 2 words:

ptr CDR(ptr x) {return x[1];}           //   car is 1st; cdr is 2nd.

void RPLACA(ptr x,ptr y) {x[0]=y;}      // car(x) = y

void RPLACD(ptr x,ptr y) {x[1]=y;}      // cdr(x) = y

bool EQ(ptr x,ptr y) {return x==y;}     // Are x,y the same object?

bool ATOM(ptr x) {return !tospace(x);}  // Is x an atom?

ptr MOVE(ptr p)                         // Move p if not yet moved;
  {                                     //    return new address.
    if (!fromspace(p))                  // We only need to move old ones.
       p                                // This happens a lot.
    else {
           if (!tospace(p[0]))          // We must move p.
             p[0]=copy(p);              // Copy it into the bottom of free area.
           p[0];                        // Leave and return forwarding address.
         }
  }

ptr COPY(ptr p)                         // Create a copy of a cell.
  {                                     // Allocate space at bottom of free area.
    if (B>=T) error();                  // Memory full?
    B[0]=p[0]; B[1]=p[1];               // Each cell requires 2 words.
    B=B+2;                              // Return the current value of B
    return B-2;                         //   after moving it to next cell.
  }

//  TOSPACE, FROMSPACE test whether a pointer is in that semispace.
