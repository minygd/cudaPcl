import os, re
import subprocess as subp

path = "../data/frames/"
path = "../../rtDDPmeans/data/oneLoop1/"
path = os.path.abspath(path)+"/"

names = []
for root, dirs, files in os.walk(path):
  for file in files:
    name, ending = os.path.splitext(file)
    if ending == '.png' and not re.search("_d$",name) is None:
      names.append(name)

print "found ", len(names), " files"
raw_input()

for name in names:
  print " ---- " + name
  args = ["../build/bin/openniSmoothNormals_file",
      "-i " + path + name + ".png",
      "-o " + path + re.sub("_d", "", name)]
  err = subp.call(" ".join(args), shell=True);
  if err:
    print "error when executing " + " ".join(args)
  print " ---- " + name + " DONE -------"
