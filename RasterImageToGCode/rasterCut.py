## GNU Public License
## This script creates g-code from raster images

from PIL import Image
import argparse, time

parser = argparse.ArgumentParser(description="Raster Image to Gcode")
group = parser.add_mutually_exclusive_group(required=True)
group.add_argument("-f", "--filepath", help="Path to image file")
parser.add_argument('-m', dest='measure', choices=['in','mm'],help='set measurement to millimeters (mm) or inches (in), default is inches')
parser.add_argument('-v', dest='verbose', action='store_true' , help='set to verbose output')
parser.add_argument('-d', dest='useDepth', action='store_true' , help='set depth threshold')
args = parser.parse_args()

start = time.clock()

if args.verbose:
    print "set to verbose mode"
    import gcodeV
else:
    print "set to light mode"
    import gcode

depthVal=.1225
if args.useDepth:
    while True:
        try:
            delpthVal = float(raw_input("Set depth threshold (default .1225): "))
        except ValueError:
            print("Not a valid input")
        else:
            if delpthVal>1: var=1
            if delpthVal<0: var=0
            break

filename=args.filepath;

im = Image.open(open(filename, 'rb'))
size = im.size
im = im.convert("L") #grayscale
w, h = im.size

step = .25
x, y = 0, 0

filename=args.filepath+'-output.nc'
f =  open(filename, 'w');

if args.verbose:
    g = gcodeV.gcodeV()
else:
    g = gcode.gcode()

f.write(str(g.begin()))
f.write('\n')
f.write(str(g.continuous()))
f.write('\n')
f.write(str(g.rapid(0,0)))
f.write('\n')

    
if args.measure=='in':
    f.write("G20 ")
    print "Measurement set to inches"
elif args.measure=='mm':
    f.write("G21 ")
    print "Measurement set to millimeters"
else:
    f.write("G20 ")
    print "Measurements not set, defaulting to inches"
print "Converting Raster Image to Gcode\nPlease Wait..."
for j in range(h-1,-1,-1):
    if j%2==1:
        for i in range(w):
            d = 1-float(im.getpixel((i, h-j-1)) / 255.0)
            if d<depthVal:
                d=0
            f.write(str(g.cut(x, y, d, feed=12)))
            x += step
        x -= step
    else:
        for i in range(w-1,-1,-1):
            d = 1-float(im.getpixel((i, h-j-1)) / 255.0)
            if d<depthVal:
                d=0
            f.write(str(g.cut(x, y, d, feed=12)))
            x -= step
        x += step
    y -= step
    f.write(str(g.cut(y=y)))
f.write(str(g.end()))
f.close()
end = time.clock()
print "finished, the script took %.2f seconds to complete" %(end-start)