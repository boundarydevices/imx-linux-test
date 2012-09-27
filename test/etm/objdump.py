# objdump module
#
# Copyright (C) 2012 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

import re
import subprocess

r_objdump_label = re.compile(r'^([0-9a-f]{8}) <(.*)>:$')
r_objdump_instr = re.compile(r'^(?P<addr>[0-9a-f]{8}):'
                              '\t(?P<opcode>[0-9a-f]{8})'
                              '\s*(?:\t(?P<instr>[^\t\n]*))?'
                              '(?:\t(?P<args>[^\t\n]*))?'
                              '(?:\t(?P<comment>.*))?$')
r_objdump_branch_addr = re.compile(r'^([0-9a-f]{8}).*$')

instr_condition = frozenset([
    'eq',  # Equal
    'ne',  # Not equal
    'cs',  # Carry set (identical to HS)
    'hs',  # Unsigned higher or same (identical to CS)
    'cc',  # Carry clear (identical to LO)
    'lo',  # Unsigned lower (identical to CC)
    'mi',  # Minus or negative result
    'pl',  # Positive or zero result
    'vs',  # Overflow
    'vc',  # No overflow
    'hi',  # Unsigned higher
    'ls',  # Unsigned lower or same
    'ge',  # Signed greater than or equal
    'lt',  # Signed less than
    'gt',  # Signed greater than
    'le',  # Signed less than or equal
    'al',  # Always (this is the default)
    ])

instr_branch_and_link = frozenset([
    'bl',
    'blx',
    ])

instr_branch_without_link = frozenset([
    'b',
    'bx',
    ])

instr_branch = frozenset(instr_branch_and_link | instr_branch_without_link)

instr_cp = frozenset([
    'mrc',
    'mcr',
    'mcr2',
    'mcrr',
    'mcrr2',
    'mrs',
    'msr',
    ])

instr_ignore = frozenset([
    'cmp',
    'cmn',
    'str',
    'strt',
    'strb',
    'strbt',
    'strh',
    'strd',
    'strex',
    'strexd',
    'tst',
    'teq',
    'nop',
    'push',
    'stm',
    'stmia',
    'stmib',
    'stmda',
    'stmdb',
    'cpsid',
    'cpsie',
    'isb',
    'dsb',
    'dmb',
    'wfi',
    'wfe',
    'svc',
    'fmxr',
    'fmsr',
    'vstmia',
    'vldmia',
    'pld',
    'clrex',
    'sev',
    '.word',
    ])

instr_destreg = frozenset([
    'mov',
    'movt',
    'movw',
    'mvn',
    'add',
    'adc',
    'sub',
    'sbc',
    'and',
    'orr',
    'ror',
    'eor',
    'asr',
    'lsr',
    'lsl',
    'rrx',
    'rsb',
    'rsc',
    'bic',
    'mul',
    'mla',
    'mls',
    'rev',
    'ldr',
    'ldrb',
    'ldrbt',
    'ldrh',
    'ldrht',
    'ldrd',
    'ldrt',
    'ldrsb',
    'ldrsh',
    'ldrex',
    'ldrexd',
    'clz',
    'rev',
    'rev16',
    'bfc',
    'bfi',
    'sxtb16',
    'sxtb',
    'sxth',
    'sxtab16',
    'sxtab',
    'sxtah',
    'uxtb16',
    'uxtb',
    'uxth',
    'uxtab16',
    'uxtab',
    'uxtah',
    'ubfx',
    'sbfx',
    'umull',
    'umlal',
    'smull',
    'smulbb',
    'smlal',
    'fmrx',
    'fmrs',
    'vmov',
    ])

instr_ldm = frozenset([
    'ldm',
    'ldmia',
    'ldmib',
    'ldmda',
    'ldmdb',
    'pop',
    ])

instr_all = frozenset(
    instr_branch |
    instr_cp |
    instr_ignore |
    instr_destreg |
    instr_ldm
    )


def is_branch(obj):
    instr = obj['instr']

    if ((instr[:-2] in instr_all or
         (len(instr) > 3 and instr[-3] == 's' and instr[:-3] in instr_all)
        ) and instr[-2:] in instr_condition):
        instr = instr[:-2]

    if instr[-1:] == 's' and instr[:-1] in instr_all:
        instr = instr[:-1]

    if instr in instr_branch:
        return True

    if instr in instr_cp:
        return False

    if instr in instr_ignore:
        return False

    args = obj['args']
    if args:
        args = args.split(',')

    if instr in instr_destreg:
        #print("check dest reg", args[0])
        if args[0] == 'pc':
            return True
        return False

    if instr in instr_ldm:
        #args = set(map(lambda arg: arg.strip('{ }'), args))
        args = set([arg.strip('{ }') for arg in args])
        #print("check dest reg", args, 'pc' in args)
        if 'pc' in args:
            return True
        return False

    print('unknown instruction', instr, args, obj)

    return False


def is_branch_and_link(obj):
    instr = obj['instr']

    if ((instr[:-2] in instr_all or (len(instr) > 3 and instr[-3] == 's' and
            instr[:-3] in instr_all)) and instr[-2:] in instr_condition):
        instr = instr[:-2]

    return instr in instr_branch_and_link


def is_conditional(obj):
    instr = obj['instr']

    return (instr[:-2] in instr_all or (len(instr) > 3 and instr[-3] == 's' and
            instr[:-3] in instr_all)) and instr[-2:] in instr_condition


def branch_addr(obj):
    branch_target_match = r_objdump_branch_addr.match(obj['args'])
    if branch_target_match:
        return int(branch_target_match.group(1), 16)
    else:
        return None


def objdump(command, filename):
    instr = dict()
    label = ''
    p = subprocess.Popen([command, '-d', filename], stdout=subprocess.PIPE)
    for lineb in p.stdout:
        line = str(lineb, encoding='utf8')
        m = r_objdump_label.match(line)
        if m:
            base_addr = int(m.group(1), 16)
            label = m.group(2)
            #print("label:", m.group(1), '-', m.group(2))
        m = r_objdump_instr.match(line)
        if m:
            #print("instr:", m.group(1), '-', m.group(2), '-', m.group(3), "-",
            #      m.group(4), '-', m.group(5))
            obj = m.groupdict()
            addr = int(obj['addr'], 16)
            obj['label'] = label + ' + ' + hex(addr - base_addr)
            instr[addr] = obj
            #is_branch(obj) # test all instructions

        #line = line.rstrip('\n')
        #t = line.split('\t')
        #print("got line:")
        #print(line)
        #print("got components:")
        #print(t)
        #if (t[0].endswith(':') and len(t) > 2):
        #  print('instruction at ', t[0].rstrip(':'), ', opcode ',
        #        t[1], ' - ', t[2:])

    p.wait()
    return instr

