
## Building the linux kernel from llvm bitcode requires writing scripts to copy, extract and recompile a lot of files in the right order. 
## A script is only valid for one configuration of the kernel

## The goal of this Python script is to automate the script building process for any configuration of the kernel.

## It takes one or more argument : the first argument is where to write the final script, and all the others are paths to files of which we should not use the bitcode.

import sys,os
import subprocess

arg_list="-Qunused-arguments -Wall -Wundef -Wstrict-prototypes -Wno-trigraphs -fno-strict-aliasing -fno-common -fshort-wchar -Werror-implicit-function-declaration -Wno-format-security -std=gnu89 -fno-PIE -mno-sse -mno-mmx -mno-sse2 -mno-3dnow -mno-avx -m64 -mno-80387 -mstack-alignment=8 -mtune=generic -mno-red-zone -mcmodel=kernel -funit-at-a-time -DCONFIG_AS_CFI=1 -DCONFIG_AS_CFI_SIGNAL_FRAME=1 -DCONFIG_AS_CFI_SECTIONS=1 -DCONFIG_AS_FXSAVEQ=1 -DCONFIG_AS_SSSE3=1 -DCONFIG_AS_CRC32=1 -DCONFIG_AS_AVX=1 -DCONFIG_AS_AVX2=1 -DCONFIG_AS_SHA1_NI=1 -DCONFIG_AS_SHA256_NI=1 -pipe  -fno-asynchronous-unwind-tables -O2 -fno-stack-protector  -mno-global-merge -no-integrated-as -fomit-frame-pointer  -fno-strict-overflow -fno-merge-all-constants -fno-stack-check -nostdlib"

slashing_dir="slashing"
# This is where most of the script gets written.
# This function takes as argument:
#    a list of path that should not be translated into bitcode
#    the depth from the root (this is a recursive function)
#    the path from the root to the cukernel/built-in.orrent folder (its length should be depth)
def write_script(excluded_paths, depth, base_dir):
    builtin = open (base_dir+"arbi","w")
    subprocess.call(["ar", "-t", base_dir+"built-in.o"],stdout=builtin )
    builtin.close()
    builtin = open(base_dir+"arbi","r")
    out.writelines("mkdir -p $build_home/built-ins/"+base_dir+" \n")
    # In directories we store the list of depth 1 directories (and files) found in builtin
    directories=[]
    linked_bcs = ""
    for line in builtin.readlines():
        line=line.split('\n')[0]
        words=line.split('/')
        if words[0]=="arch":    # We had to make an exception for the arch folder which does not have a built-in.o at the root.
            words[0]=words[0]+'/'+words[1]
            if words[2] in archbi:  #A second exception for arch, not all files in the folder are referenced in the arch/x86/built-in.o. We store the exceptions in the archi list
                words[0]=words[0]+'/'+words[2]
                del words[2]
            del words[1]
        if words[depth] not in directories and line not in standalone_objects: #Some objects are not listed in a built-in.o except at the root, so we include them separately in order not to mess the order of linking 
            directories.append(words[depth])

    # folders in which there are excluded folders which we will need to act on recursively
    excluded_roots = [path[depth] for path in excluded_paths] 

    for direc in directories:
        dir_created=0
        print base_dir+direc
        # the folder contains an excluded folder
        if direc in excluded_roots:

            # Check if we have an excluded folder
            if base_dir+direc in excluded_dirs:
                link_args.writelines(os.getcwd()+"/" +base_dir + direc +"/built-in.o ")
                final_link_args.writelines(os.getcwd()+"/" +base_dir + direc +"/built-in.o ")
            # Else we filter the excluded_path list for only relevant stuff and we call the recursion on that folder
            else:
                relevant_excluded = [path for path in excluded_paths if (path[depth]==direc and len(path)>depth+1) ]
                if relevant_excluded:
                    write_script(relevant_excluded,depth+1,base_dir+direc+"/")
        else:
            #If the "directory" is a file, we copy it into the relevant objects folder and compile it
            if direc[-2:] == ".o":
                if not dir_created:
                    out.writelines("mkdir -p $build_home/built-ins/"+base_dir+"objects \n")
                    dir_created=1
                out.writelines("get-bc -b "+base_dir+direc+"\n")
                out.writelines("cp "+ base_dir+direc+".bc $build_home/built-ins/"+base_dir+"objects \n")
                # We then add it to the linker arguments file
                link_args.writelines("built-ins/"+base_dir + direc +" ")
                final_link_args.writelines("built-ins/"+base_dir + direc +" ")
            # When dealing with a folder, we get the bitcode from the built-in.o file and check for errors in the log.
            # For each file that does not have a bitcode version (compiled straight from asse
            # mbly) we copy it into the build folder directly and add it to the linker args
            else:
                if os.path.isfile("./wrapper-logs/wrapper.log"):
                    os.rename("./wrapper-logs/wrapper.log","./wrapper-logs/before_"+direc.replace('/','_')+".log")
                    #os.remove("./wrapper-logs/wrapper.log")
                path = base_dir + direc +"/built-in.o"
                subprocess.call(["get-bc", "-b", path ])
                subprocess.call(["touch","./wrapper-logs/wrapper.log"])

                # Checking to see if any file failed to be extracted to bitcode
                llvm_log=open("./wrapper-logs/wrapper.log","r")
                assembly_objects=[]
                for line in llvm_log.readlines():
                    if len(line)>=53 and line[:53]=="ERROR:Error reading the .llvm_bc section of ELF file ":
                        assembly_objects.append(line[53:-2])
                llvm_log.close()

                # Deal with those files
                if assembly_objects:
                    out.writelines("mkdir -p $build_home/built-ins/" + base_dir +direc.replace('/','_')+"\n")
                for asf in assembly_objects:
                    out.writelines("cp "+ asf + " $build_home/built-ins/" + base_dir +direc.replace('/','_') +"\n")
                    filename= asf.split('/')[-1]
                    link_args.writelines("built-ins/"+base_dir + direc.replace('/','_') +'/'+filename+" ")

                    final_link_args.writelines("built-ins/"+base_dir + direc.replace('/','_') +'/'+filename+" ")

                    manifest_olist.append("built-ins/"+base_dir + direc.replace('/','_') +'/'+filename)
                # Deal with the rest
                if os.path.isfile(base_dir + direc +"/built-in.o.a.bc"):
                    out.writelines("cp "+ base_dir + direc +"/built-in.o.a.bc $build_home/built-ins/"+ base_dir + direc.replace('/','_') +"bi.o.bc \n")
                    # link_args.writelines("built-ins/"+ base_dir + direc.replace('/','_') +"bi.o.bc ")
                    # final_link_args.writelines(slashing_dir+"/built-ins_"+ base_dir + direc.replace('/','_') +"bi.o-final.bc ")
                    # manifest_bclist.append("built-ins/"+ base_dir + direc.replace('/','_') +"bi.o.bc")
                    linked_bcs += "built-ins/"+ base_dir + direc.replace('/','_') +"bi.o.bc "
                builtin.close()
    return linked_bcs

# excluded_dirs is a list where the folders excluded from bitcode translation will be stored
excluded_dirs=[]

if len(sys.argv) > 2:
    excluded_dirs=sys.argv[2:]

for i in range(len(excluded_dirs)):
    path=excluded_dirs[i]
    if path[-1]=='/':
        excluded_dirs[i]=path[:-1]


# Output file path
build_dir= sys.argv[1]
if build_dir[-1]!='/':
    build_dir+='/'
#We transform excluded_dirs into a 2D array for better access to individual folders in the path
excluded=[]
for path in excluded_dirs:
    pathsplit=path.split('/')
    if pathsplit[0]=="arch":
        pathsplit[0]=pathsplit[0]+'/'+pathsplit[1]
        del pathsplit[1]
    excluded.append(pathsplit)

out = open("build_script.sh","w+")
compile_cmd = open(build_dir+"compile_occam_final.sh","w+")
link_args = open(build_dir+"link-args","w+")
final_link_args = open(build_dir+"link-args-final","w+")
occam_manifest = open(build_dir+"kernel-manifest.json","w+")
manifest_bclist=[]
manifest_olist=[]

out.writelines("# Script written by the built-in-parsing.py script \n")
out.writelines("export build_home="+build_dir+"\n")


# List exceptions
standalone_objects = ["arch/x86/kernel/head_64.o","arch/x86/kernel/head64.o","arch/x86/kernel/ebda.o","arch/x86/kernel/platform-quirks.o"]
archbi=["lib","pci","video","power"]

# Calling the main function
linked_bcs = write_script(excluded,0,"")
link_args.write(" kernel-linked-bcs.o.bc ")
final_link_args.write(" slashing/kernel-linked-bcs.o-final.bc ")

# Dealing with both lib files 
# link_args.write(" lib/lib.a.bc arch/x86/lib/lib.a.bc ")
# final_link_args.write(slashing_dir+"/lib_lib.a.bc "+ slashing_dir+"/arch_x86_lib_lib.a.bc ")
# manifest_bclist.append("lib/lib.a.bc")
# manifest_bclist.append("arch/x86/lib/lib.a.bc")

# Abubakar : since unslashed versions of the two libs are being used
#            these files need to be treated as native_libs
#            added to the native_libs at the end
link_args.write(" lib/lib.a.bc arch/x86/lib/lib.a.bc ")
final_link_args.write(" lib/lib.a.bc arch/x86/lib/lib.a.bc ")



out.writelines("get-bc -b lib/lib.a \n ")
out.writelines("mkdir -p $build_home/lib\n")
out.writelines("cp lib/lib.a.bc $build_home/lib/ \n")

if os.path.isfile("./wrapper-logs/wrapper.log"):
    os.rename("./wrapper-logs/wrapper.log","./wrapper-logs/before_lib.log")
    #os.remove("./wrapper-logs/wrapper.log")
out.writelines("mkdir -p $build_home/arch/x86/lib/objects\n")
subprocess.call(["get-bc", "-b", "arch/x86/lib/lib.a" ])
subprocess.call(["touch","./wrapper-logs/wrapper.log"])
llvm_log=open("./wrapper-logs/wrapper.log","r")
assembly_objects=[]
for line in llvm_log.readlines():
    if len(line)>=53 and line[:53]=="ERROR:Error reading the .llvm_bc section of ELF file ":
        assembly_objects.append(line[53:-2])
for asf in assembly_objects:
    out.writelines("cp "+ asf + " $build_home/arch/x86/lib/objects/ \n")
    filename= asf.split('/')[-1]
    link_args.writelines("arch/x86/lib/objects/" +filename+" ") 
    final_link_args.writelines("arch/x86/lib/objects/" +filename+" ") 
    manifest_olist.append("arch/x86/lib/objects/" +filename+" ")

out.writelines("cp arch/x86/lib/lib.a.bc $build_home/arch/x86/lib/lib.a.bc \n")

#Deal with individual files 
out.writelines("cp arch/x86/kernel/vmlinux.lds $build_home \n")
out.writelines("cp .tmp_kallsyms2.o $build_home \n")
for sto in standalone_objects:
    out.writelines("cp --parents "+ sto+" $build_home \n")
out.writelines("\n#linking command \n")

out.writelines("cd $build_home \n")


link_args.write(" .tmp_kallsyms2.o")
final_link_args.write(" .tmp_kallsyms2.o")
manifest_olist.append(".tmp_kallsyms2.o")

# Final linking command
out.writelines("llvm-link " + linked_bcs + " -o kernel-linked-bcs.o.bc\n")
out.writelines("# clang -Wl,-T,vmlinux.lds,--whole-archive " +arg_list+" ")
for sto in standalone_objects:
    out.writelines(sto+" ")
out.writelines("@link-args ")
out.writelines("-o vmlinux\n")


compile_cmd.writelines("clang -Wl,-T,vmlinux.lds,--whole-archive " +arg_list+" ")
for sto in standalone_objects:
    compile_cmd.writelines(sto+" ")
compile_cmd.writelines("@link-args-final ")
compile_cmd.writelines("-o vmlinux")

out.writelines("chmod a+x $build_home/compile_occam_final.sh")


# Manifest writing
occam_manifest.write('{ "main" :  "kernel-linked-bcs.o.bc"\n, "binary"  : "vmlinux"\n, "modules"    : [')

for bc_ar in manifest_bclist[1:-1]: #we exclude the last one for the comma, and the first one is initbi
    occam_manifest.write('"'+bc_ar+'",')
if len(manifest_bclist)>1:
    occam_manifest.write('"'+manifest_bclist[-1]+'"')
occam_manifest.write(']\n, "native_libs" : [')

for sto in standalone_objects:
    occam_manifest.write('"' + sto + '", ')
for obj in manifest_olist[:-1]:
    occam_manifest.write('"'+obj+'",')
if manifest_olist:
    occam_manifest.write('"'+manifest_olist[-1]+'", ')
occam_manifest.write('"lib/lib.a.bc", "arch/x86/lib/lib.a.bc"')
occam_manifest.write(']\n, "args" : []\n, "ldflags"    : ["'+ arg_list +'"]\n, "name"    : "kernel" \n }')
print "Preparation work done, launch the build_script.sh to start the extraction"
