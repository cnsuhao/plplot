#!/usr/bin/env python
#Have not yet converted to arrays.

#	Font demo.

import math
import sys
import os

module_dir = "@MODULE_DIR@"

if module_dir[0] == '@':
	module_dir = os.getcwd ()

sys.path.insert (0, module_dir)

from Numeric import *
from pl import *

# main
#
# Displays the entire "plsym" symbol (font) set.

def main():

    base = [0, 200, 500, 600, 700, 800, 900, 2000, 2100,
	    2200, 2300, 2400, 2500, 2600, 2700, 2800, 2900]

    # Parse and process command line arguments

    plParseOpts(sys.argv, PARSE_FULL)

    # Initialize plplot

    plinit()

    plfontld(1)

    for l in range(17):
	pladv(0)

	# Set up viewport and window

	plcol0(2)
	plvpor(0.15, 0.95, 0.1, 0.9)
	plwind(0.0, 1.0, 0.0, 1.0)

	# Draw the grid using plbox

	plbox("bcgt", 0.1, 0, "bcgt", 0.1, 0)

	# Write the digits below the frame

	plcol0(15)
	for i in range(10):
	    plmtex("b", 1.5, (0.1 * i + 0.05), 0.5, `i`)

	k = 0
	for i in range(10):

	    # Write the digits to the left of the frame

	    text = `base[l] + 10 * i`
	    plmtex("lv", 1.0, (0.95 - 0.1 * i), 1.0, text)

	    for j in range(10):
		x = [ 0.1 * j + 0.05 ]
		y = [ 0.95 - 0.1 * i ]

		# Display the symbols

		plsym(x, y, base[l] + k)
		k = k + 1

	plmtex("t", 1.5, 0.5, 0.5, "PLplot Example 7 - PLSYM symbols")

	pleop()

    plend()

main()
