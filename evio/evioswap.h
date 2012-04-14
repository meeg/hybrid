/*
 *  evioswap.c
 *
 *   evioswap() swaps one evio version 2 event
 *       - in place if dest is NULL
 *       - copy to dest if not NULL
 *
 *   swap_int2_t_val() swaps one int32_t, call by val
 *   swap_int32_t() swaps an array of uint32_t's
 *
 *   thread safe
 *
 *
 *   Author: Elliott Wolin, JLab, 21-nov-2003
 *
 *   Notes:
 *     simple loop in swap_xxx takes 50% longer than pointers and unrolled loops
 *     -O is over twice as fast as -g
 *
 *   To do:
 *     use in evio.c, replace swap_util.c
 *
 *   Author: Carl Timmer, JLab, jan-2012
 *      - add doxygen documentation
 *      - add comments & beautify
 *      - simplify swapping routines
 *      - make compatible with evio version 4 (padding info in data type)
 *
 */


/**
 * ################################
 * COMPOSITE DATA:
 * ################################
 *   This is a new type of data (value = 0xf) which originated with Hall B.
 *   It is a composite type and allows for possible expansion in the future
 *   if there is a demand. Basically it allows the user to specify a custom
 *   format by means of a string - stored in a tagsegment. The data in that
 *   format follows in a bank. The routine to swap this data must be provided
 *   by the definer of the composite type - in this case Hall B. The swapping
 *   function is plugged into this evio library's swapping routine.
 *   Here's what it looks like.
 *
 * MSB(31)                          LSB(0)
 * <---  32 bits ------------------------>
 * _______________________________________
 * |  tag    | type |    length          | --> tagsegment header
 * |_________|______|____________________|
 * |        Data Format String           |
 * |                                     |
 * |_____________________________________|
 * |              length                 | \
 * |_____________________________________|  \  bank header
 * |       tag      |  type   |   num    |  /
 * |________________|_________|__________| /
 * |               Data                  |
 * |                                     |
 * |_____________________________________|
 *
 *   The beginning tagsegment is a normal evio tagsegment containing a string
 *   (type = 0x3). Currently its type and tag are not used - at least not for
 *   data formatting.
 *   The bank is a normal evio bank header with data following.
 *   The format string is used to read/write this data so that takes care of any
 *   padding that may exist. As with the tagsegment, the tag, type, & num are ignored.
 */


/* include files */
#include <evio.h>
#include <stdlib.h>
#include <stdio.h>


// from Sergey's composite swap library
int eviofmt(char *fmt, unsigned char *ifmt, int ifmtLen);
int eviofmtswap(uint32_t *iarr, int nwrd, unsigned char *ifmt, int nfmt, int tolocal);



/* entry points */
void evioswap(uint32_t *buffer, int tolocal, uint32_t*dest);
int32_t swap_int32_t_value(int32_t val);
uint32_t *swap_int32_t(uint32_t *data, unsigned int length, uint32_t *dest);
