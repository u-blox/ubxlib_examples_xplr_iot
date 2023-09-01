#!/usr/bin/env python3

# Copyright 2022-2023 u-blox
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#  http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and

# do.py
#
# Wrapper command for building and flashing the XPLR-IOT-1 examples.
# Also handles Visual Studio Code integration.

import os
import sys
import subprocess
import re
from datetime import datetime, timedelta
from os.path import exists
import argparse
import json
from time import time
from serial.tools import list_ports, miniterm
from pathlib import Path
from glob import glob
from shutil import which

# Global variables
settings = dict()
state = dict()
has_jlink = False
cwd = os.getcwd()
top_dir = os.path.dirname(os.path.realpath(__file__))
is_linux = 'linux' in sys.platform
exe_suffix = "" if is_linux else ".exe"
examples_root = top_dir + "/examples/"
args = ""


def error_exit(mess):
    print(f"*** Error. {mess}")
    sys.exit(1)

#--------------------------------------------------------------------

def exec_command(com):
    sub_proc = subprocess.run(com, universal_newlines=True, shell=True)
    return sub_proc.returncode

#--------------------------------------------------------------------

def check_jlink():
    global has_jlink
    sub_proc = subprocess.run("nrfjprog --ids", universal_newlines=True, shell=True,
                              stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    has_jlink = sub_proc.returncode == 0 and len(sub_proc.stdout) > 0

#--------------------------------------------------------------------

def find_uart0():
    if 'uart_name' in settings:
        return settings['uart_name']
    for port in list_ports.comports():
        if re.search("CP210.+Interface 0", str(port)):
            return port.device
    error_exit("Failed to detect the serial port.\nIs the unit connected?")

#--------------------------------------------------------------------

def get_exe_file(args, signed, use_jlink, net_cpu = False):
    file_ext = ".hex" if use_jlink else ".bin"
    file_name = f"{args.build_dir}/zephyr/zephyr" if not net_cpu else f"{args.build_dir}/hci_rpmsg/zephyr/zephyr"
    file_type = "_signed" if signed else ""
    return file_name + file_type + file_ext

#--------------------------------------------------------------------

def build():
    print(f"=== {args.example} ===")
    os.chdir(examples_root + args.example)
    com = f"west build --board nrf5340dk_nrf5340_cpuapp --build-dir {args.build_dir}"
    if args.pristine:
        com += " --pristine"
    start = time()
    ret_val = 0
    build_res = exec_command(com)
    if build_res == 0:
        if os.path.getmtime(get_exe_file(args, False, False, False)) > start:
            ret_val = 1
        path = f"{args.build_dir}/zephyr/zephyr"
        if not args.no_bootloader and ret_val == 1:
            mcuboot_dir = Path(os.environ['NCS_DIR']).as_posix() + "/bootloader/mcuboot"
            for use_jlink in (False, True):
                if exists(get_exe_file(args, False, use_jlink)):
                    sign_com = (f"\"{sys.executable}\" \"{mcuboot_dir}/scripts/imgtool.py\" sign"
                                f" --key \"{mcuboot_dir}/root-rsa-2048.pem\""
                                " --header-size 0x200 --align 4 --version 0.0.0+0"
                                " --pad-header --slot-size 0xe0000"
                                f" {get_exe_file(args, False, use_jlink)}"
                                f" {get_exe_file(args, True, use_jlink)}")
                    exec_command(sign_com)
            ret_val = 1
    print("= Elapsed time:", timedelta(seconds=round(time()-start)))
    if build_res != 0:
        error_exit("Build failed")
    return ret_val

#--------------------------------------------------------------------

def flash_file(file_name, net_cpu = False):
    if not exists(file_name):
        error_exit(f"File not found: {file_name}")
    if has_jlink:
        print(f"Flashing {file_name} using jlink")
        cpu = "CP_APPLICATION" if not net_cpu else "CP_NETWORK"
        com = f"nrfjprog -f nrf53 --coprocessor {cpu} --program {file_name} --sectorerase --verify --reset"
    else:
        print("Flashing using serial port")
        uart0 = find_uart0()
        print("Restart the XPLR-IOT-1 simultaneously pressing button 1")
        input("Press return when ready: ")
        serial_flash_com = f"{top_dir}/newtmgr --conntype serial --connstring \"{uart0},baud=115200\""
        com = f"{serial_flash_com} image upload {file_name}"

    exec_command(com)
    if not has_jlink:
        print("Restarting")
        exec_command(f"{serial_flash_com} reset")

#--------------------------------------------------------------------

def flash():
    if build() == 0 and args.when_changed:
        return
    flash_file(get_exe_file(args, not args.no_bootloader, has_jlink, False))

#--------------------------------------------------------------------

def flash_net():
    flash_file(get_exe_file(args, False, has_jlink, True), True)


#--------------------------------------------------------------------

def monitor():
    sys.argv = [sys.argv[0]]
    sys.argv.append(find_uart0())
    sys.argv.append("115200")
    sys.argv.append("--raw")
    miniterm.main()

#--------------------------------------------------------------------

def reset():
    if has_jlink:
        exec_command("nrfjprog -f nrf53 --reset")

#--------------------------------------------------------------------

def run():
    flash()
    monitor()

#--------------------------------------------------------------------

def flash_bootloader():
    print("Flashing serial mcuboot bootloader...")
    exec_command(
        f"nrfjprog -f nrf53 --program {top_dir}/config/mcuboot_serial.hex --sectorerase --reset --verify")

#--------------------------------------------------------------------

def debug():
    exec_command(f"west debug --build-dir {args.build_dir}")

#--------------------------------------------------------------------

def terminal():
    com = "bash --rcfile .bashrc" if is_linux else "start cmd"
    exec_command(com)

#--------------------------------------------------------------------

settings_file_name = ".settings"

def read_settings():
    global settings
    if exists(settings_file_name):
        settings = json.loads(open(settings_file_name).read())

#--------------------------------------------------------------------

def save():
    try:
        open(settings_file_name, 'w').write(json.dumps(settings))
    except Exception as e:
        error_exit(f"Failed to save settings file {settings_file_name}\n{e}")

#--------------------------------------------------------------------

state_file_name = cwd + "/.state"

def read_state():
    global state
    if exists(state_file_name):
        state = json.loads(open(state_file_name).read())

#--------------------------------------------------------------------

def save_state():
    try:
        open(state_file_name, 'w').write(json.dumps(state))
    except Exception as e:
        error_exit(f"Failed to save state file {state_file_name}\n{e}")

#--------------------------------------------------------------------

def vscode_files():
    vscode_dir = cwd + "/.vscode"
    templ_dir = top_dir + "/.vscode"
    do_com = "\\\"" + Path(top_dir).as_posix() + "/do"
    if not is_linux:
        do_com += ".bat"
    do_com += "\\\""
    if not exists(vscode_dir):
        os.mkdir(vscode_dir)
    gcc_bin_dir = Path(os.path.dirname(which("arm-zephyr-eabi-gcc"))).as_posix()

    # Generate configuration file for vscode from templates
    # Tasks
    with open(templ_dir + "/tasks_tmpl.json", "r") as f:
        tasks = f.read()
    tasks = re.sub("\$DO", do_com, tasks)
    tasks = re.sub("\$EXAMPLES", re.sub("'", "\"", f"{examples}"), tasks)
    tasks = re.sub("\$DEF_EX", f"{args.example}", tasks)
    with open(vscode_dir + "/tasks.json", "w") as f:
        f.write(tasks)

    # Launch
    with open(templ_dir + "/launch_tmpl.json", "r") as f:
        launch = f.read()
    launch = re.sub("\$BUILD_DIR", Path(f"{settings['build_dir']}/{args.example}").as_posix(), launch)
    launch = re.sub("\$EXE_FILE", os.path.basename(get_exe_file(args, not args.no_bootloader, True, False)), launch)
    launch = re.sub("\$TC_DIR", gcc_bin_dir, launch)
    gdb_exe = Path(which("arm-zephyr-eabi-gdb")).as_posix()
    launch = re.sub("\$GDB_PATH", gdb_exe, launch)
    with open(vscode_dir + "/launch.json", "w") as f:
        f.write(launch)

    # C/C++
    # Get defines and includes from the generated Zephyr Ninja build file
    comp_file = f"{settings['build_dir']}/{args.example}/build.ninja"
    if not exists(comp_file):
        # Not built before, must do a first one
        build()
        os.chdir(top_dir)
    defines = ""
    includes = ""
    with open(comp_file, "r") as f:
        for line in f:
            if 'DEFINES = ' in line:
                for d in re.findall(r"-D\S+", line):
                    defines += re.sub("-D", "        \"", d).replace("\\\\", "") + "\",\n"
                defines = re.sub(r",\n$", "", defines)
            elif 'INCLUDES = ' in line:
                for i in re.findall(r"-I\S+", line):
                    includes += re.sub("-I", "        \"", i) + "\",\n"
                includes = re.sub(r",\n$", "", includes)
                break
    with open(templ_dir + "/c_cpp_properties_tmpl.json", "r") as f:
        properties = f.read()
    comp_exe = sorted(Path(gcc_bin_dir).rglob(f"*gcc{exe_suffix}"))[0].as_posix()
    properties = re.sub("\$COMP_EXE", comp_exe, properties)
    properties = re.sub("\$DEFINES", defines, properties)
    properties = re.sub("\$INCLUDES", includes, properties)
    with open(vscode_dir + "/c_cpp_properties.json", "w") as f:
        f.write(properties)

#--------------------------------------------------------------------

def vscode():
    build()
    os.chdir(cwd)
    vscode_files()
    workspace  = glob(f"{cwd}/*.code-workspace")
    if len(workspace) > 0:
        param = workspace[0]
        print(f"Using workspace:", os.path.basename(workspace[0]))
    else:
        param = f". -g {examples_root}{args.example}/src/main.c"
    exec_command(f"code {param}")

#--------------------------------------------------------------------

def select():
    vscode()
    print(f"\n=== \"{args.example}\" is now the selected example for builds ===\n")

#--------------------------------------------------------------------

def set_env():
    os.environ['DO_TOP_DIR'] = top_dir
    os.environ['ZEPHYR_BASE'] = Path(os.environ['NCS_DIR'] + "/zephyr").as_posix()
    os.environ['UBXLIB_DIR'] = os.path.expandvars(settings['ubxlib_dir'])
    if not settings['no_bootloader']:
        os.environ['USE_BL'] = "1"
    if not is_linux:
        # Make sure that JLink directory is in the path when installed
        try:
            import winreg
            key = winreg.OpenKey( winreg.ConnectRegistry(None, winreg.HKEY_LOCAL_MACHINE),
                                r"SOFTWARE\SEGGER\J-Link")
            os.environ['PATH'] = os.environ['PATH'] + ";" + winreg.QueryValueEx(key, "InstallPath")[0]
        except:
            print("Failed to detect Jlink installation")


#--------------------------------------------------------------------

def check_directories():
    global settings

    # Ubxlib location
    if args.ubxlib_dir != None:
        settings['ubxlib_dir'] = args.ubxlib_dir
    elif not 'ubxlib_dir' in settings:
        settings['ubxlib_dir'] = os.path.realpath("ubxlib")
    if not exists(os.path.expandvars(settings['ubxlib_dir'])):
        error_exit("Ubxlib directory not found")

    # The build output root directory
    if args.build_dir != None:
        settings['build_dir'] = args.build_dir
    elif not 'build_dir' in settings:
        settings['build_dir'] = top_dir + "/_build"
    args.build_dir = settings['build_dir'] + "/" + args.example

#--------------------------------------------------------------------


if __name__ == "__main__":

    if not 'NCS_DIR' in os.environ:
        error_exit("This script must be run via the \"do\" command")

    os.chdir(top_dir)
    check_jlink()

    parser = argparse.ArgumentParser()
    parser.add_argument("operation", nargs='+',
                        help="Operation to be performed: vscode, build, flash, run, monitor, debug, terminal",
                        )
    parser.add_argument("-e", "--example",
                        help="Name of the example",
                        )
    parser.add_argument("-p", "--pristine",
                        help="Pristine build (rebuild)",
                        action="store_true"
                        )
    parser.add_argument("--no-bootloader",
                        help="Don't use the bootloader",
                        action="store_true"
                        )
    parser.add_argument("--when-changed",
                        help="Only flash when build was triggered",
                        action="store_true"
                        )
    parser.add_argument("-d", "--build-dir",
                        help="Root directory for the build output"
                        )
    parser.add_argument("-u", "--ubxlib-dir",
                        help="Ubxlib directory"
                        )
    parser.add_argument("--uart-name",
                        help="Uart port name"
                        )
    args = parser.parse_args()

    if not args.operation[0] in locals():
        error_exit(f"Invalid operation: \"{args.operation[0]}\"")

    if not top_dir in cwd and exists(f"{cwd}/src"):
        # Application outside of the repo
        examples_root = os.path.realpath(cwd + "/..")
        if args.example == None:
            args.example = os.path.split(cwd)[1]
        examples_root += "/"

    examples = []
    for entry in os.scandir(examples_root):
        if entry.is_dir() and exists(examples_root + entry.name + "/src"):
            examples.append(entry.name)
    examples.sort()

    read_settings()
    read_state()
    if args.example == None:
        if 'example' in state:
            args.example = state['example']
        else:
            args.example = "blink"
    if not args.example in examples:
        error_exit(f"Invalid example \"{args.example}\"\nAvailable: {examples}")
    settings['no_bootloader'] = args.no_bootloader
    if args.uart_name != None:
        settings['uart_name'] = args.uart_name
    check_directories()
    set_env()
    state['example'] = args.example
    save_state()

    # Execute specified operation
    locals()[args.operation[0]]()
