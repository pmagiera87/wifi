import os
import sys
import array

print 'Number of Files:', len(sys.argv)
print 'File List:', str(sys.argv)
#file("out.bin", 'wb').write(file('firmware/0x00000.bin','rb').read())
with open(sys.argv[1], "wb") as f:
	f.seek(0)
	f.write(file(sys.argv[2],'rb').read())
	f.seek(0x10000)
	#f.write(file('firmware/0x91000.bin','rb').read())
	f.write(file(sys.argv[3],'rb').read())
	#f.seek(0x4F000)
	#f.write(file(sys.argv[4],'rb').read())
#file("out.bin", 'ab').write(file('firmware/0x00000.bin','rb').read())