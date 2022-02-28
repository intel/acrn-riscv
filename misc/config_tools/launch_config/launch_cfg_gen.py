#!/usr/bin/env python3
#
# Copyright (C) 2022 Intel Corporation.
#
# SPDX-License-Identifier: BSD-3-Clause
#

import sys, os
import argparse
import lxml.etree as etree
import logging
import re
import copy

def eval_xpath(element, xpath, default_value=None):
    return next(iter(element.xpath(xpath)), default_value)

def eval_xpath_all(element, xpath):
    return element.xpath(xpath)

class LaunchScript:
    script_template_path = os.path.join(os.path.dirname(os.path.abspath(__file__)), "launch_script_template.sh")

    class VirtualBDFAllocator:
        def __init__(self):
            # Reserved slots:
            #    0 - For (virtual) hostbridge
            #    1 - For (virtual) LPC
            #    2 - For passthrough integarted GPU (either PF or VF)
            #   31 - For LPC bridge needed by integrated GPU
            self._free_slots = list(range(3, 30))

        def get_virtual_bdf(self, device_etree = None, options = None):
            if device_etree is not None:
                bus = eval_xpath(device_etree, "../@address")
                vendor_id = eval_xpath(device_etree, "vendor/text()")
                class_code = eval_xpath(device_etree, "class/text()")

                # VGA-compatible controller, either integrated or discrete GPU
                if class_code == "0x030000":
                    return 2

            if options:
                if "igd" in options:
                    return 2

            next_vbdf = self._free_slots.pop(0)
            return next_vbdf

    class PassThruDeviceOptions:
        passthru_device_options = {
            # "0x0200": ["enable_ptm"],  # Ethernet controller, added if PTM is enabled for the VM
        }

        def _add_option(self, class_code, option):
            current_option = self._options.setdefault(class_code, [])
            self._options[class_code] = current_option.append("enable_ptm")

        def __init__(self, vm_scenario_etree):
            self._options = copy.copy(self.passthru_device_options)
            if eval_xpath(vm_scenario_etree, ".//PTM/text()") == "y":
                self._add_option("0x0200", "enable_ptm")

        def get_option(self, device_etree):
            passthru_options = []
            if device_etree is not None:
                class_code = eval_xpath(device_etree, "class/text()", "")
                for k,v in self._options.items():
                    if class_code.startswith(k):
                        passthru_options.extend(v)
            return ",".join(passthru_options)

    def __init__(self, board_etree, vm_name, vm_scenario_etree):
        self._board_etree = board_etree
        self._vm_scenario_etree = vm_scenario_etree

        self._vm_name = vm_name
        self._vm_descriptors = {}
        self._init_commands = []
        self._dm_parameters = []
        self._deinit_commands = []

        self._vbdf_allocator = self.VirtualBDFAllocator()
        self._passthru_options = self.PassThruDeviceOptions(vm_scenario_etree)

    def add_vm_descriptor(self, name, value):
        self._vm_descriptors[name] = value

    def add_init_command(self, command):
        if command not in self._init_commands:
            self._init_commands.append(command)

    def add_deinit_command(self, command):
        if command not in self._deinit_commands:
            self._deinit_commands.append(command)

    def add_plain_dm_parameter(self, opt):
        full_opt = f"\"{opt}\""
        if full_opt not in self._dm_parameters:
            self._dm_parameters.append(full_opt)

    def add_dynamic_dm_parameter(self, cmd, opt=""):
        full_cmd = f"{cmd:40s} {opt}".strip()
        full_opt = f"`{full_cmd}`"
        if full_opt not in self._dm_parameters:
            self._dm_parameters.append(full_opt)

    def to_string(self):
        s = ""

        with open(self.script_template_path, "r") as f:
            s += f.read()

        s += """
###
# The followings are generated by launch_cfg_gen.py
###
"""
        s += "\n"

        s += "# Defining variables that describe VM types\n"
        for name, value in self._vm_descriptors.items():
            s += f"{name}={value}\n"
        s += "\n"

        s += "# Initializing\n"
        for command in self._init_commands:
            s += f"{command}\n"
        s += "\n"

        s += "# Invoking ACRN device model\n"
        s += "dm_params=(\n"
        for param in self._dm_parameters:
            s += f"    {param}\n"
        s += ")\n\n"

        s += "echo \"Launch device model with parameters: ${dm_params[*]}\"\n"
        s += "acrn-dm ${dm_params[*]}\n\n"

        s += "# Deinitializing\n"
        for command in self._deinit_commands:
            s += f"{command}\n"

        return s

    def write_to_file(self, path):
        with open(path, "w") as f:
            f.write(self.to_string())
            logging.info(f"Successfully generated launch script {path} for VM '{self._vm_name}'.")

    def add_virtual_device(self, kind, vbdf=None, options=""):
        if "virtio" in kind and eval_xpath(self._vm_scenario_etree, ".//vm_type/text()") == "RTVM":
            self.add_plain_dm_parameter("--virtio_poll 1000000")

        if vbdf is None:
            vbdf = self._vbdf_allocator.get_virtual_bdf()
        self.add_dynamic_dm_parameter("add_virtual_device", f"{vbdf} {kind} {options}")

    def add_passthru_device(self, bus, dev, fun, options=""):
        device_etree = eval_xpath(self._board_etree, f"//bus[@type='pci' and @address='0x{bus:x}']/device[@address='0x{(dev << 16) | fun:x}']")
        if not options:
            options = self._passthru_options.get_option(device_etree)

        vbdf = self._vbdf_allocator.get_virtual_bdf(device_etree, options)
        self.add_dynamic_dm_parameter("add_passthrough_device", f"{vbdf} 0000:{bus:02x}:{dev:02x}.{fun} {options}")

        # Enable interrupt storm monitoring if the VM has any passthrough device other than the integrated GPU (whose
        # vBDF is fixed to 2)
        if vbdf != 2:
            self.add_dynamic_dm_parameter("add_interrupt_storm_monitor", "10000 10 1 100")

    def has_dm_parameter(self, fn):
        try:
            next(filter(fn, self._dm_parameters))
            return True
        except StopIteration:
            return False

def cpu_id_to_lapic_id(board_etree, vm_name, cpus):
    ret = []

    for cpu in cpus:
        lapic_id = eval_xpath(board_etree, f"//processors//thread[cpu_id='{cpu}']/apic_id/text()", None)
        if lapic_id is not None:
            ret.append(int(lapic_id, 16))
        else:
            logging.warning(f"CPU {cpu} is not defined in the board XML, so it can't be available to VM {vm_name}")

    return ret

def generate_for_one_vm(board_etree, hv_scenario_etree, vm_scenario_etree, vm_id):
    vm_name = eval_xpath(vm_scenario_etree, "./name/text()", f"ACRN Post-Launched VM")
    script = LaunchScript(board_etree, vm_name, vm_scenario_etree)

    script.add_init_command("probe_modules")

    ###
    # VM types and guest OSes
    ###

    if eval_xpath(vm_scenario_etree, ".//os_type/text()") == "Windows OS":
        script.add_plain_dm_parameter("--windows")
    script.add_vm_descriptor("vm_type", f"'{eval_xpath(vm_scenario_etree, './/vm_type/text()', 'STANDARD_VM')}'")
    script.add_vm_descriptor("scheduler", f"'{eval_xpath(hv_scenario_etree, './/SCHEDULER/text()')}'")

    ###
    # CPU and memory resources
    ###
    cpus = set(eval_xpath_all(vm_scenario_etree, ".//cpu_affinity/pcpu_id[text() != '']/text()"))
    lapic_ids = cpu_id_to_lapic_id(board_etree, vm_name, cpus)
    if lapic_ids:
        script.add_dynamic_dm_parameter("add_cpus", f"{' '.join([str(x) for x in sorted(lapic_ids)])}")

    script.add_plain_dm_parameter(f"-m {eval_xpath(vm_scenario_etree, './/memory/whole/text()')}M")

    if eval_xpath(vm_scenario_etree, "//SSRAM_ENABLED") == "y" and \
       eval_xpath(vm_scenario_etree, ".//vm_type/text()") == "RTVM":
        script.add_plain_dm_parameter("--ssram")

    ###
    # Guest BIOS
    ###
    if eval_xpath(vm_scenario_etree, ".//vbootloader/text()") == "Enable":
        script.add_plain_dm_parameter("--ovmf /usr/share/acrn/bios/OVMF.fd")

    ###
    # Devices
    ###

    # Emulated platform devices
    if eval_xpath(vm_scenario_etree, ".//vm_type/text()") != "RTVM":
        script.add_virtual_device("lpc", vbdf="1:0")

    if eval_xpath(vm_scenario_etree, ".//vuart0/text()") == "Enable":
        script.add_plain_dm_parameter("-l com1,stdio")

    # Emulated PCI devices
    script.add_virtual_device("hostbridge", vbdf="0:0")

    for ivshmem in eval_xpath_all(vm_scenario_etree, "//IVSHMEM_REGION[PROVIDED_BY = 'Device model' and .//VM_NAME = 'vm_name']"):
        script.add_virtual_device("ivshmem", options=f"dm:/{ivshmem.find('NAME').text},{ivshmem.find('IVSHMEM_SIZE').text}")

    if eval_xpath(vm_scenario_etree, ".//console_vuart/text()") == "PCI":
        script.add_virtual_device("uart", options="vuart_idx:0")

    for idx, conn in enumerate(eval_xpath_all(hv_scenario_etree, f".//vuart_connection[endpoint/vm_name = '{vm_name}']"), start=1):
        if eval_xpath(conn, "./type") == "pci":
            script.add_virtual_device("uart", options="vuart_idx:{idx}")

    # Mediated PCI devices, including virtio
    for usb_xhci in eval_xpath_all(vm_scenario_etree, ".//usb_xhci[text() != '']/text()"):
        script.add_virtual_device("xhci", options=usb_xhci)

    for virtio_input in eval_xpath_all(vm_scenario_etree, ".//virtio_devices/input[text() != '']/text()"):
        script.add_virtual_device("virtio-input", options=virtio_input)

    for virtio_console in eval_xpath_all(vm_scenario_etree, ".//virtio_devices/console[text() != '']/text()"):
        script.add_virtual_device("virtio-console", options=virtio_console)

    for virtio_network in eval_xpath_all(vm_scenario_etree, ".//virtio_devices/network[text() != '']/text()"):
        params = virtio_network.split(",", maxsplit=1)
        tap_conf = f"tap={params[0]}"
        params = [tap_conf] + params[1:]
        script.add_init_command(f"mac=$(cat /sys/class/net/e*/address)")
        params.append(f"mac_seed=${{mac:0:17}}-{vm_name}")
        script.add_virtual_device("virtio-net", options=",".join(params))

    for virtio_block in eval_xpath_all(vm_scenario_etree, ".//virtio_devices/block[text() != '']/text()"):
        params = virtio_block.split(":", maxsplit=1)
        if len(params) == 1:
            script.add_virtual_device("virtio-blk", options=virtio_block)
        else:
            block_device = params[0]
            rootfs_img = params[1]
            var = f"dir_{os.path.basename(block_device)}"
            script.add_init_command(f"{var}=`mount_partition {block_device}`")
            script.add_virtual_device("virtio-blk", options=os.path.join(f"${{{var}}}", rootfs_img))
            script.add_deinit_command(f"unmount_partition ${{{var}}}")

    # Passthrough PCI devices
    bdf_regex = re.compile("([0-9a-f]{2}):([0-1][0-9a-f]).([0-7])")
    for passthru_device in eval_xpath_all(vm_scenario_etree, ".//pci_devs/*/text()"):
        m = bdf_regex.match(passthru_device)
        if not m:
            continue
        bus = int(m.group(1), 16)
        dev = int(m.group(2), 16)
        func = int(m.group(3), 16)
        device_node = eval_xpath(board_etree, f"//bus[@type='pci' and @address='{hex(bus)}']/device[@address='hex((dev << 16) | func)']")
        if device_node and \
           eval_xpath(device_node, "class/text()") == "0x030000" and \
           eval_xpath(device_node, "resource[@type='memory'") is None:
            script.add_passthru_device(bus, dev, func, options="igd-vf")
        else:
            script.add_passthru_device(bus, dev, func)

    ###
    # Miscellaneous
    ###
    if eval_xpath(vm_scenario_etree, ".//vm_type/text()") == "RTVM":
        script.add_plain_dm_parameter("--rtvm")
        if eval_xpath(vm_scenario_etree, ".//lapic_passthrough/text()") == "y":
            script.add_plain_dm_parameter("--lapic_pt")
    script.add_dynamic_dm_parameter("add_logger_settings", "console=4 kmsg=3 disk=5")

    ###
    # Lastly, conclude the device model parameters with the VM name
    ###
    script.add_plain_dm_parameter(f"{vm_name}")

    return script

def main(board_xml, scenario_xml, user_vm_id, out_dir):
    board_etree = etree.parse(board_xml)
    scenario_etree = etree.parse(scenario_xml)

    service_vm_id = eval_xpath(scenario_etree, "//vm[load_order = 'SERVICE_VM']/@id")
    hv_scenario_etree = eval_xpath(scenario_etree, "//hv")
    post_vms = eval_xpath_all(scenario_etree, "//vm[load_order = 'POST_LAUNCHED_VM']")
    if service_vm_id is None and len(post_vms) > 0:
        logging.error("The scenario does not define a service VM so no launch scripts will be generated for the post-launched VMs in the scenario.")
        return 1
    service_vm_id = int(service_vm_id)

    try:
        os.mkdir(out_dir)
    except FileExistsError:
        if os.path.isfile(out_dir):
            logging.error(f"Cannot create output directory {out_dir}: File exists")
            return 1
    except Exception as e:
        logging.error(f"Cannot create output directory: {e}")
        return 1

    if user_vm_id == 0:
        post_vm_ids = [int(vm_scenario_etree.get("id")) for vm_scenario_etree in post_vms]
    else:
        post_vm_ids = [user_vm_id + service_vm_id]

    for post_vm in post_vms:
        post_vm_id = int(post_vm.get("id"))
        if post_vm_id not in post_vm_ids:
            continue

        script = generate_for_one_vm(board_etree, hv_scenario_etree, post_vm, post_vm_id)
        script.write_to_file(os.path.join(out_dir, f"launch_user_vm_id{post_vm_id - service_vm_id}.sh"))

    return 0

if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--board", help="the XML file summarizing characteristics of the target board")
    parser.add_argument("--scenario", help="the XML file specifying the scenario to be set up")
    parser.add_argument("--launch", default=None, help="(obsoleted. DO NOT USE)")
    parser.add_argument("--user_vmid", type=int, default=0, help="the post-launched VM ID (as is specified in the launch XML) whose launch script is to be generated, or 0 if all post-launched VMs shall be processed")
    parser.add_argument("--out", default="output", help="path to the directory where generated scripts are placed")
    args = parser.parse_args()

    logging.basicConfig(level="INFO")

    sys.exit(main(args.board, args.scenario, args.user_vmid, args.out))
