DEBOUNCE = 150                                  ; 150ms seems about right

.import __RAM_START__

PROGRAM_START = __RAM_START__ + $100            ; memory location for user programs
PROGRAM_END = $8000                             ; End of RAM
