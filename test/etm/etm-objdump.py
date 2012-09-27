#! /usr/bin/env python3
#
# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the 'License');
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an 'AS IS' BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

import optparse
import re
import subprocess
import sys
import objdump

parser = optparse.OptionParser("usage: %prog [options] tracefile")
parser.add_option("--trace-decoder",
                  dest="trace_decoder", default="./etm",
                  help="execute COMMAND to decode trace-file", metavar="COMMAND")
parser.add_option("--trace-decoder-options", "--tdoption",
                  action="append", dest="trace_decoder_options", default=[],
                  help="pass OPTIONS to trace-decoder", metavar="OPTIONS")
parser.add_option("--objfile",
                  dest="objfile", default="vmlinux",
                  help="pass FILE to objdump", metavar="FILE")
parser.add_option("--objdump",
                  dest="objdump", default="arm-eabi-objdump",
                  help="execute COMMAND to decode executable", metavar="COMMAND")
parser.add_option("-q", "--quiet",
                  action="store_false", dest="verbose", default=True,
                  help="don't print status messages to stdout")

(options, args) = parser.parse_args()

if len(args) != 1:
    parser.error("incorrect number of arguments")

tracefile = args[0]
tracetool = ' '.join([options.trace_decoder] + options.trace_decoder_options)

if options.verbose:
    print("trace file:", tracefile)
    print("trace decoder:", tracetool)
    print("objdump:", options.objdump)
    print("objfile:", options.objfile)
    print()

r_etm_execute = re.compile(r"^\s*([EN])\(([0-9a-f]{8})\)(?:\s*\+(?P<branch_points>\d+) branch points)?(?:\s*Waited\s*(?P<wait>\d+))?$")
r_etm_branch = re.compile(r"^\s*Branch ([0-9a-f]{8})\s*(.*)$")
r_etm_isync = re.compile(r"^\s*I-sync Context (?P<context>[0-9a-f]{8}), IB (?P<ib>[0-9a-f]{2}), Addr (?P<addr>[0-9a-f]{8})$")


if options.verbose:
    print("Running objdump")
    sys.stdout.flush()

instr = objdump.objdump(options.objdump, options.objfile)

if options.verbose:
    print("objdump done")
    sys.stdout.flush()

addr_offset = 0
branch_target = None
need_branch_target = False
return_stack = []

p = subprocess.Popen(tracetool + " <" + tracefile, stdout=subprocess.PIPE, shell=True)
for lineb in p.stdout:
    line = str(lineb, encoding='utf8')
    m = r_etm_execute.match(line);
    if (m):
        guessed_addr = int(m.group(2), 16)

        if branch_target is not None:
            addr_offset = branch_target - guessed_addr
            branch_target = None
        elif need_branch_target:
            if len(return_stack):
                addr_offset = return_stack.pop() - guessed_addr
                #print("Used return stack")
            else:
                print("Missing branch target -- don't trust address")
        need_branch_target = False;

        adjusted_addr = guessed_addr + addr_offset
        obj = instr.get(adjusted_addr)
        if obj is None:
            print("Unknown adjusted_addr,", hex(adjusted_addr), ", offset", addr_offset, ", try clearing offset")
            obj = instr.get(guessed_addr)
            if obj is not None:
                addr_offset = 0
                adjusted_addr = guessed_addr

        executed = m.group(1)
        branch_points = m.group('branch_points')

        if obj is not None and branch_points is not None:
            while obj is not None and not objdump.is_branch(obj):
                comment = obj['comment']
                comment_string = '' if comment is None else comment
                print("   ({}) {:<35}\t{}\t{:<40}\t{}".format(obj['addr'], obj['label'], obj['instr'], obj['args'], comment_string))

                addr_offset += 4 # TODO: add thumb support
                adjusted_addr = guessed_addr + addr_offset
                obj = instr.get(adjusted_addr)
            addr_offset += 4 # TODO: add thumb support

        waited = m.group('wait')
        waited_string = '' if waited is None else "\tWaited " + waited
        if obj is None:
            print("  {}({}) {}".format(executed, m.group(2), waited_string))
        else:
            if objdump.is_branch_and_link(obj):
                return_stack.append(adjusted_addr + 4) # TODO: add thumb support
            comment = obj['comment']
            comment_string = '' if comment is None else comment

            #print('  ' + executed + '(' + obj['addr'] + ') ' + obj['label'], obj['instr'], obj['args'], obj['comment'], sep='\t')
            if (obj):
                print("  {}({}) {:<35}\t{}\t{:<40}\t{}{}".format(executed, obj['addr'], obj['label'], obj['instr'], obj['args'], comment_string, waited_string))
            if executed == 'N' and not objdump.is_conditional(obj):
                print("Error: Unconditional instruction marked as not executed")
            #if (executed == 'N' and objdump.is_branch(obj)):
            #    print("    skipped branch")
            if (executed == 'E' and objdump.is_branch(obj)):
                branch_target = objdump.branch_addr(obj)
                if branch_target is None:
                    #print("    branch to unknown location:", obj['args'])
                    addr_offset = 0;
                    need_branch_target = True
                #else:
                #    print("    branch to", hex(branch_target))
                print()

        #print("instr:", m.group(1), obj)
    else:
        m = r_etm_branch.match(line)
        if m:
            new_branch_target = int(m.group(1), 16)
            if branch_target is not None:
                if branch_target != new_branch_target:
                    print("Warning: Branch target changed from", hex(branch_target), "to", hex(new_branch_target))
                else:
                    print("Unneeded branch")
            branch_target = new_branch_target
        else:
            m = r_etm_isync.match(line)
            if m:
                del return_stack[:]
                new_branch_target = int(m.group('addr'), 16)
                if branch_target is not None:
                    if branch_target != new_branch_target:
                        print("I-sync: Branch target changed from", hex(branch_target), "to", hex(new_branch_target))
                    else:
                        print("I-sync")
                else:
                    print("I-sync")

                branch_target = new_branch_target
            else:
                print(line.rstrip('\n'))


p.wait()



print("done")
