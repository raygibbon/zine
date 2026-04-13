#!/usr/bin/env python
#
# Normalizes (canonicalises) paths given.
# Idea taken from Bzr-lib.
#
from __future__ import print_function
import sys, os

try:
  import ntpath
except ImportError as e:
   print ("Failed to import ntpath: %s" % e)
   sys.exit(1)

def _win32_fixdrive (path):
  """Force drive letters to be consistent.

  Win32 is inconsistent whether it returns lower or upper case
  and even if it was consistent the user might type the other
  so we force it to uppercase
  running python.exe under cmd.exe return capital C:\\
  running Win32 python inside a cygwin shell returns lowercase c:\\
  """
  drive, path = ntpath.splitdrive (path)
  return drive.lower() + path

def _win32_abspath (path):
  path = ntpath.abspath (path)
  path = path.replace ('\\', '/')
  return _win32_fixdrive (path)

cwd    = _win32_abspath (os.getcwd()) + '/'
prefix = ''
DEBUG  = 0

def skip_cwd (s1, s2):
  ''' Skip the leading part that is in common with s1 and s2
  '''
  i = 0
  while i < len(s1) and s1[i] == s2[i]:
     i += 1
  if os.path.exists(s2[i:]):
     return s2[i:]
  return s2

def process_line (line):
  if line.startswith('\n'):
    return
  line = line.rstrip ("\r\n")
  parts = line.split()
  if DEBUG:
    print ("line: %s, parts: %d" % (line, len(parts)))
  max = len(parts)-1
  i = 0
  for l in parts:
    i += 1
    if (len(l) > 1) and ('\\' not in l) and ('/' not in l) and os.path.exists(l):
      print (' ' + l, end="")
    elif l.endswith('.o:') or l.endswith('.obj:'):
      print ('\n\n' + prefix + l, end="")
      continue
    elif l != '\\':
      s = skip_cwd (cwd, _win32_abspath(l))
      print (' ' + s, end="")
    if (i <= max):
      print ('\\')

def process_file (fil):
  while True:
    line = fil.readline()
    if not line:
       break
    process_line (line)

def process_args (args):
  if (args[0] == '-'):
    process_file (sys.stdin)
  elif (args[0][0] == '@'):
    fname = args[0][1:]
    try:
      f = open (fname, 'r')
      process_file (f)
      f.close()
    except IOError as e:
      print ("Failed to open %s: %s" % (fname, e))
  else:
    for a in args:
      process_line (a)
  return 0

def usage():
  """ Print usage """
  print ("Normalizes (canonicalises) paths from gcc dependency output.\n"
         "  Usage:\n"
         "  %s <-p prefix> [files..] | [@resp-file] | [-] :\n"
         "    \"-\":          takes list of files from stdin.\n"
         "    \"@resp-file\": takes list of files from resp-file.\n"
         "    \"-p prefix\":  produces \"<prefix>/file.o: file.c file.h\"."
         % _win32_abspath(sys.argv[0]))

#
# Check command-line
#
if (len(sys.argv) > 2) and (sys.argv[1] == '-p'):
  prefix = sys.argv[2]
  if prefix[-1] != '/':
     prefix += '/'
  sys.argv[1:] = sys.argv[3:]

if (len(sys.argv) < 2) or (sys.argv[1] in ('-h','--help', '-?')):
  usage()
  rc = 1
else:
  rc = process_args (sys.argv[1:])

sys.exit (rc)
