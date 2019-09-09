#!/usr/bin/env python3

# Pre-build:
# > Compiling the uECC library in the nrf SDK
#
# Steps to build the dfu project:
# > Generating keys
# > Building the bootloader.
# > Building the application.
# > Generating the dfu .zip packet, which contatins the new image hex files,
#   the init data, and the signature of the packet.
#   The hex files, which users can select, can be an application, a bootloader,
#   a softdevice, or mixed of them. The default would be only application.
#
# Post-build utilities:
# > Flashing the right hex files
#
# The script only supports NRF52840 at the moment.

# Author: Tuan Nguyen
# Created on: August 05, 2019

import os
import sys
import argparse
import configparser
from pathlib import Path
import subprocess

proj_dir = Path('.')
output_dir = Path('build')
app_dir = proj_dir / 'ble_app_dfu'
app_target = 'nrf52840_xxaa.hex'
bl_dir = proj_dir / 'bootloader'
bl_target = 'nrf52840_xxaa_s140.hex'
sd_dir = proj_dir / 's140_nrf52_6.1.1'
sd_target = 's140_nrf52_6.1.1_softdevice.hex'


dfu_conf = 'dfu.conf'
dfu_target = 'nrf52840_xxaa.zip'
merged_bl_target = 'merged_bl_nrf52840_xxaa.hex'
merged_target = 'merged_nrf52840_xxaa.hex'
dfu_bl_settings = 'bl_settings_nrf52840_xxaa.hex'

# CLI interface
parser = argparse.ArgumentParser(
    description="an utility to build the dfu project")
parser.add_argument("-a", "--app", help="include the application",
                    action="store_true")
parser.add_argument("-b", "--bootloader", help="include the bootloader",
                    action="store_true")
parser.add_argument("-s", "--softdevice", help="include the SoftDevice",
                    action="store_true")
parser.add_argument("-m", "--merge", help="merge application, bootloader, and softdevice into one binary file",
                    action="store_true")
parser.add_argument("-d", "--dry", help="print the configuration",
                    action="store_true")
parser.add_argument("-f", "--flash", help="flash the merged hex to the target",
                    action="store_true")
args = parser.parse_args()

# Configuration file
config = configparser.ConfigParser()
config.read(dfu_conf)
app_version = config['Application'].getint('Version', fallback=1)
hw_version = config['Application'].getint('HwVersion', fallback=52)
bl_version = config['Bootloader'].getint('Version', fallback=1)
private_key = config['Bootloader'].get('PrivateKey', fallback='private.key')
sd_req = config['SoftDevice'].get('SqRequirement', fallback='0xB6')


def init():
    if not output_dir.is_dir():
        print("Build directory not exist. Creating new one.")
        os.mkdir(output_dir)


def print_conf():
    print("application = {}".format(args.app))
    print("bootloader = {}".format(args.bootloader))
    print("softdevice = {}".format(args.softdevice))
    print("merge hex = {}".format(args.merge))
    print("application version = {}".format(app_version))
    print("hardware version = {}".format(hw_version))
    print("private_key = {}".format(private_key))
    print("softdevice requirement = {}".format(sd_req))


def build_binary():
    """
    Build the selected binaries.
    """

    if args.app:
        print("{}: building the application".format(__file__))
        try:
            ret = subprocess.run(['make', '-C', app_dir.as_posix()],
                                 capture_output=True, check=True, text=True)
            print(ret.stdout)
        except subprocess.CalledProcessError as error:
            print(error.stderr, file=sys.stderr)
            raise

    if args.bootloader:
        print("{}: building the bootloader".format(__file__))
        try:
            ret = subprocess.run(['make', '-C', bl_dir.as_posix()],
                                 capture_output=True, check=True, text=True)
            print(ret.stdout)
        except subprocess.CalledProcessError as error:
            print(error.stderr, file=sys.stderr)
            raise


def generate_dfu_package():
    """
    Support only application update at the moment.
    https://devzone.nordicsemi.com/nordic/nordic-blog/b/blog/posts/getting-started-with-nordics-secure-dfu-bootloader
    https://github.com/NordicSemiconductor/pc-nrfutil
    """
    print("{}: generating DFU update package".format(__file__))

    # Create the zip file
    # nrfutil pkg generate --hw-version 52 --application-version 1
    # --application application.hex --sd-req 0x98 --softdevice softdevice.hex
    # --key-file private.key app_dfu_package_softdevice.zip
    nrfutil_args = ['nrfutil', 'pkg', 'generate']
    nrfutil_args.append('--hw-version')
    nrfutil_args.append(str(hw_version))
    if args.app:
        nrfutil_args.append('--application-version')
        nrfutil_args.append(str(app_version))
        nrfutil_args.append('--application')
        nrfutil_args.append((app_dir / output_dir / app_target).as_posix())
    nrfutil_args.append('--sd-req')
    nrfutil_args.append(sd_req)
    nrfutil_args.append('--key-file')
    nrfutil_args.append(private_key)
    nrfutil_args.append((proj_dir / output_dir / dfu_target).as_posix())

    try:
        ret = subprocess.run(
            nrfutil_args, capture_output=True, check=True, text=True)
        print(ret.stdout)
    except subprocess.CalledProcessError as error:
        print(error.stderr, file=sys.stderr)
        raise


def generate_bl_settings():
    """
    Generate the bootloader settings. It is used to detect if a valid application 
    is flashed on the chip or not.
    """
    print("{}: generating bootloader settings".format(__file__))

    # nrfutil settings generate --family NRF52 --application yourApplication.hex
    # --application-version 0 --bootloader-version 0 --bl-settings-version 1 bootloader_setting.hex
    nrfutil_args = ['nrfutil', 'settings', 'generate', '--family', 'NRF52840']
    nrfutil_args.append('--softdevice')
    nrfutil_args.append((sd_dir / sd_target).as_posix())
    nrfutil_args.append('--application')
    nrfutil_args.append((app_dir / output_dir / app_target).as_posix())
    nrfutil_args.append('--application-version')
    nrfutil_args.append(str(app_version))
    nrfutil_args.append('--bootloader-version')
    nrfutil_args.append(str(bl_version))
    nrfutil_args.append('--bl-settings-version')
    nrfutil_args.append('2') # nrf sdk version > 15.3.0 requires bl settings version 2
    nrfutil_args.append((proj_dir / output_dir / dfu_bl_settings).as_posix())

    try:
        ret = subprocess.run(
            nrfutil_args, capture_output=True, check=True, text=True)
        print(ret.stdout)
    except subprocess.CalledProcessError as error:
        print(error.stderr, file=sys.stderr)
        raise


def merge_hex():
    """
    Merge application, bootloader, and softdevice into one binary file.
    If one of them is missing, the operation is aborted.
    """
    print("{}: merging application, booatloader, and softdevice".format(__file__))

    app_hex = app_dir / output_dir / app_target
    bl_hex = bl_dir / output_dir / bl_target
    sd_hex = sd_dir / sd_target

    if not (app_hex.is_file() and bl_hex.is_file() and sd_hex.is_file()):
        print("{}: error: missing files".format(__file__))
        return None

    generate_bl_settings()
    bl_settings = proj_dir / output_dir / dfu_bl_settings

    # The maximum number of hex file to merge is currently 3 
    mergehex_args = ['mergehex', '--merge']
    mergehex_args.append(bl_settings.as_posix())
    mergehex_args.append(bl_hex.as_posix())
    mergehex_args.append('--output')
    mergehex_args.append((proj_dir / output_dir / merged_bl_target).as_posix())

    try:
        ret = subprocess.run(
            mergehex_args, capture_output=True, check=True, text=True)
        print(ret.stdout)
    except subprocess.CalledProcessError as error:
        print(error.stderr, file=sys.stderr)
        raise

    mergehex_args.clear()
    mergehex_args = ['mergehex', '--merge']
    mergehex_args.append((proj_dir / output_dir / merged_bl_target).as_posix())
    mergehex_args.append(app_hex.as_posix())
    mergehex_args.append(sd_hex.as_posix())
    mergehex_args.append('--output')
    mergehex_args.append((proj_dir / output_dir / merged_target).as_posix())

    try:
        ret = subprocess.run(
            mergehex_args, capture_output=True, check=True, text=True)
        print(ret.stdout)
    except subprocess.CalledProcessError as error:
        print(error.stderr, file=sys.stderr)
        raise


def flash():
    """
    Flash the merged firmware to the target
    """

    # nrfjprog -f nrf52 --program $(SDK_ROOT)/components/softdevice/s140/hex/s140_nrf52_6.1.1_softdevice.hex --sectorerase
    flash_args = ['nrfjprog', '-f', 'nrf52']
    flash_args.append('--program')
    flash_args.append((output_dir / merged_target).as_posix())
    flash_args.append('--sectorerase')
    flash_args.append('--reset')

    try:
        ret = subprocess.run(
            flash_args, capture_output=True, check=True, text=True)
        print(ret.stdout)
    except subprocess.CalledProcessError as error:
        print(error.stderr, file=sys.stderr)
        raise


if __name__ == "__main__":
    if args.dry:
        print_conf()
        exit(0)

    init()
    build_binary()

    if args.app:
        generate_dfu_package()

    if args.merge:
        merge_hex()

    if args.flash:
        flash()
