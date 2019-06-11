#!/usr/bin/python3

import os, shutil, sys

rtc_dir = sys.argv[1]

for rtc in os.listdir(rtc_dir):
    if not rtc.endswith(".rtc"):
        continue

    upos = rtc.rfind('_')
    dotpos = rtc.rfind('.')
    old_index = int(rtc[upos+1:dotpos])
    new_index = old_index + 3600

    rtc_new = '%s%d.rtc' % (rtc[0:upos+1], new_index)
    
    print("Renaming %s to %s..." % (rtc, rtc_new))
    shutil.move(os.path.join(rtc_dir, rtc), os.path.join(rtc_dir, rtc_new))
