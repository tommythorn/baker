#include <stdbool.h>

/*
 * In this GC sandbox we have a heap which is an array of objects
 * An object in a quad (four-tuple) of values.
 * A value is either and integer or a heap pointer (index)
 *
 * A mutator is given some initial pointers and can only create new
 * pointers by allocating new objects or deferencing existing
 * pointers.  The only mutation possible is overwriting an object
 * field with a new value.
 *
 */

// Value is either a pointer or a small integer
typedef uint32_t value_t;
value_t heap_start, heap_end;

bool is_int(value_t v);
int int_from_value(value_t v);

// Objects are exactly four values long in this world
value_t heap_get(value_t p, int field); // p must be a pointer, field in {0,..,3}
void heap_set(value_t p, int field, value_t v);
value_t heap_new(void)
{
	for (value_t p = heap_start; p != heap_end; p = heap_next(p))
		if (get_color(p) == RED) {
			set_color(p) = GREY;
			return p;
		}

	fail("OUT OF MEMORY");
}

typedef enum { WHITE // Unknown
             , GREY  // Alive, but unscanned
             , BLACK // Alive, scanned
             , RED   // Collected, available for the allocator
             } color_t;

color_t get_color(value_t p);
void set_color(value_t p, value_t v);

value_t heap_next(value_t p);

// Invariant: BLACK objects can only point to GREY or BLACK objects
// That is: p.color == BLACK ==> p[i].color != WHITE

// Initially: every

void mark(void)
{
    // XXX Mark root set GREY

    for (;;) {
        bool progress;

        do {
            progress = false
            for (value_t p = heap_start; p != heap_end; p = heap_next(p))
                if (get_color(p) == GREY) {
                    for (int i = 0; i < 4; ++i) {
                        value_t pi = heap_get(p, i);
                        if (!is_int(pi))
                            set_color(pi, GREY);
                    }
                    set_color(p, BLACK);
                    progress = true;
                }
        } while (progress);

        // All WHITE objects are garbage

        
