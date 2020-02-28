Import('env')
import os
import shutil
from datetime import datetime 

OUTPUT_DIR = "build_output{}".format(os.path.sep)

def bin_copy(source, target, env):
    
    variant = "firmware "
    
    print("Variant: ", variant)
    
    # check if output directories exist and create if necessary
    if not os.path.isdir(OUTPUT_DIR):
        os.mkdir(OUTPUT_DIR)

    #for d in ['firmware']:
    #    if not os.path.isdir("{}{}".format(OUTPUT_DIR, d)):
    #        os.mkdir("{}{}".format(OUTPUT_DIR, d))
            
    now = datetime.now()
    # dd/mm/YY H:M:S
    dt_string = now.strftime("%d-%m-%Y %H.%M")
    print("date and time =", dt_string)	

    # create string with location and file names based on variant
    #Wbin_file = "{}firmware{}{}{}.bin".format(OUTPUT_DIR, os.path.sep, variant, dt_string)
    bin_file = "{}{}{}.bin".format(OUTPUT_DIR, variant, dt_string)
    
    print("Bin File: ", bin_file)

    # check if new target files exist and remove if necessary
    for f in [bin_file]:
        if os.path.isfile(f):
            os.remove(f)

    # copy firmware.bin to firmware/<variant>.bin
    shutil.copy(str(target[0]), bin_file)

env.AddPostAction("$BUILD_DIR/${PROGNAME}.bin", [bin_copy])
