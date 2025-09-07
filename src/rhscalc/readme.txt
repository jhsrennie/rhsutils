RhsCalc
=======

RhsCalc is a (very) simple reverse Polish calculator. I wrote it
because I'm so used to using RP calculators that I find the Windows
Calculator a pain to use.

Few people will e remotely interested in this app. For those few, the
comamnd supported are (all case insensitive):

Stack manipulation:

POP   - remove the item at the top of the stack
SWAP  - swap the top two items
DUP   - duplicate the top item
CLEAR - delete everything in the stack

Numeric constants:

PI
E

Simple arithmetic

+
-
*
/

Slightly more complex arithmatic

SQR          - square
SQRT         - square root
RECIP or INV - reciprocal
**           - raise to a power e.g. 3 2 ** = 3^2 = 9

Simple functions

CHS - change sign
INT - truncate to integer (rounds down)

Log functions

LOG or LN  - natural log
EXP or ALN - e to the power of the top item
LOG10      - base 10 log
ALOG10     - 10 to the power of the top item

Trigonometric functions

SIN
COS
TAN
ASIN
ACOS
ATAN
SINH
COSH
TANH

Memory functions

STORE - store to memory e.g. "4 store" stores the top item in slot 4
RCL   - recall from memory

Misc

EXIT - close RhsCalc

John Rennie
john.rennie@ratsauce.co.uk
23rd November 2009
