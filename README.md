i3lock-color
============

*making i3lock's typing indicator color scheme configurable*

I added the following command line options:
* `--insidevercolor=rrggbbaa` -- Inside of the circle while the password is being verified
* `--insidewrongcolor=rrggbbaa` -- Inside of the circle when a wrong password was entered
* `--insidecolor=rrggbbaa` -- Inside of the circle while typing/idle
* `--ringvercolor=rrggbb` -- Outer ring while the password is being
* `--ringwrongcolor=rrggbb` -- Outer ring when a wrong password was entered
* `--ringcolor=rrggbb` -- Outer ring while typing/idle
* `--linecolor=rrggbb` -- Line separating outer ring from inside of the circle and delimiting the highlight segments
* `--textcolor=rrggbb` -- Text ("verifying", "wrong!")
* `--keyhlcolor=rrggbb` -- Keypress highlight segments
* `--bshlcolor=rrggbb` -- Backspace highlight segments

Unfortunately, because I didn't want to invade the way in which command line options are handled, the shorthand command line options are `-1`, `-2`, `-3`, ..., `-0`. I hope I'll get around to making it more elegant soon.

Examples:

i3lock --insidevercolor=0000a0bf --insidewrongcolor=ff8000bf --insidecolor=ffffffbf --ringvercolor=0020ff --ringwrongcolor=4040ff --ringcolor=404090 --textcolor=ffffff --linecolor=aaaaaa --keyhlcolor=30cccc --bshlcolor=ff8000

i3lock -1 0000a0bf -2 ff8000bf -3 ffffffbf -4 0020ff -5 4040ff -6 404090 -7 ffffff -8 aaaaaa -9 30cccc -0 ff8000

Refer to the original README file for general information and libraries you need.
