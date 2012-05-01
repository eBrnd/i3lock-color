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
