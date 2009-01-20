-- $Id$

--	Pie chart demo.

-- initialise Lua bindings to PLplot
if string.sub(_VERSION,1,7)=='Lua 5.0' then
	lib=loadlib('./plplotluac.so','luaopen_plplotluac') or loadlib('plplotluac.dll','luaopen_plplotluac')
	assert(lib)()
else
	require('plplotluac')
end

text = { "Maurice", "Geoffrey", "Alan",
         "Rafael", "Vince" }

--------------------------------------------------------------------------
-- main
--
-- Does a simple pie chart.
--------------------------------------------------------------------------

per = { 10, 32, 12, 30, 16 }

-- Parse and process command line arguments

pl.parseopts(arg, pl.PL_PARSE_FULL);

-- Initialize plplot

pl.init()

pl.adv(0)

-- Ensure window has aspect ratio of one so circle is 
-- plotted as a circle.
pl.vasp(1)
pl.wind(0, 10, 0, 10)
pl.col0(2)

-- n.b. all theta quantities scaled by 2*M_PI/500 to be integers to avoid
--floating point logic problems.
theta0 = 0
dthet = 1
for i = 1, 5 do
  x = { 5 }
  y = { 5 }
  j = 2
  -- n.b. the theta quantities multiplied by 2*M_PI/500 afterward so
  -- in fact per is interpreted as a percentage. 
	theta1 = theta0 + 5 * per[i]
	if i == 5 then theta1 = 500 end
  
	for theta = theta0, theta1, dthet do
	    x[j] = 5 + 3 * math.cos((2*math.pi/500)*theta)
	    y[j] = 5 + 3 * math.sin((2*math.pi/500)*theta)
      j = j + 1
	end
	pl.col0(i)
	pl.psty(math.mod((i + 2), 8) + 1)
	pl.fill(x, y)
	pl.col0(1)
	pl.line(x, y)
	just = (2*math.pi/500)*(theta0 + theta1)/2
	dx = .25 * math.cos(just)
	dy = .25 * math.sin(just)
	if (theta0 + theta1)<250 or (theta0 + theta1)>750 then
	  just = 0
	else 
    just = 1
  end

	pl.ptex((x[(j-1) / 2] + dx), (y[(j-1) / 2] + dy), 1.0, 0.0, just, text[i]);
	theta0 = theta1 - dthet
end

pl.font(2)
pl.schr(0, 1.3)
pl.ptex(5, 9, 1, 0, 0.5, "Percentage of Sales")

pl.plend()

