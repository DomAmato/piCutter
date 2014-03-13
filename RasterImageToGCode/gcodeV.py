## verbose version of gcode parser
## will output x,y,z format no matter of deltaX,Y,Z

class gcodeV:
	lastx = lasty = lastz = lastgcode = None
	lastfeed = None

	def __init__(self, homeheight = 1.5, safetyheight = 0.04):
		self.homeheight = homeheight
		self.safetyheight = self.lastz = safetyheight

	def begin(self):
		return "G00 Z%.4f\n" % (self.safetyheight) + \
			"G17 G40 G49\n" + "G80 G90 G94\n" + \
			"M5\n" + "G04 P3"
# G17 use XY Plane G40 compensation off G49 tool length compensation off G90 absolute distance mode 
# G94 Units per Minute Mode an F word is interpreted to mean the controlled point should move at 
#      a certain number of inches per minute, millimeters per minute

	def end(self):
		return self.home() + "\n" + "M2\n"

	def exactpath(self):
		return "G61"

	def continuous(self):
		return "G64"

	def rapid(self, x = None, y = None, z = None, gcode = "G00", feed=None):
		laserstring = gcodestring = xstring = ystring = zstring = ""
		if x == None: x = self.lastx
		if y == None: y = self.lasty
		if z == None: z = self.lastz
		if z>0:
			gcodestring = gcode
			if self.lastz==0: 
				laserstring = "G00" + " X%.4f" % (self.lastx) + " Y%.4f" % (self.lasty) + " Z%.4f" % (self.lastz) + "\nM3\n"
			xstring = " X%.4f" % (x)
			self.lastx = x
			ystring = " Y%.4f" % (y)
			self.lasty = y
			zstring = " Z%.4f" % (z)
			self.lastz = z
		elif z<=0 and y==self.lasty:
			if self.lastz>0: 
				laserstring ="M5"
				gcodestring = "G01"
				xstring = " X%.4f" % (x)
				self.lastx = x
				ystring = " Y%.4f" % (y)
				self.lasty = y
				zstring = " Z%.4f" % (z)
				self.lastz = z
				return  gcodestring + xstring + ystring + zstring +"\n" + laserstring + "\nG00" + xstring + ystring + zstring +"\n"
			else :
				self.lastx = x
				self.lasty = y
				self.lastz = z
				return ""
		elif z<=0 and y!=self.lasty:
			gcodestring = "G00"
			xstring = " X%.4f" % (x)
			self.lastx = x
			ystring = " Y%.4f" % (y)
			self.lasty = y
			zstring = " Z%.4f" % (z)
			self.lastz = z
		return laserstring + gcodestring + xstring + ystring + zstring +"\n"

	def cut(self, x = None, y = None, z = None, feed=None):
		if x == None: x = self.lastx
		if y == None: y = self.lasty
		if z == None: z = self.lastz
		return self.rapid(x, y, z, gcode="G01", feed=feed)

	def home(self):
		return self.rapid(x=0,y=0,z=0,)

