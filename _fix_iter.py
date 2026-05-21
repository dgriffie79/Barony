#!/usr/bin/env python3
"""Fix all cJSON for-loop iterations that use ++ptr instead of ptr = ptr->next."""

import re

with open('src/ui/GameUI.cpp', 'r') as f:
    lines = f.readlines()

fixed = []
i = 0
while i < len(lines):
    line = lines[i]
    
    # Match: for ( TYPE VAR = ...->child; ...; ++VAR )
    # The for-loop header may span multiple lines (when ; is not on same line as for)
    if '->child' in line and ('for (' in line or 'for (' in lines[i-1] if i > 0 else False):
        # Collect the for-loop header (from 'for' to ')')
        header_start = i
        if 'for (' in line:
            header_start = i
        else:
            header_start = i - 1
            
        # Collect lines until we find the closing )
        j = header_start
        header_lines = []
        paren_depth = 0
        found_for = False
        while j < len(lines):
            hl = lines[j]
            for ch in hl:
                if ch == '(':
                    paren_depth += 1
                elif ch == ')':
                    paren_depth -= 1
            header_lines.append(hl)
            if paren_depth == 0 and found_for:
                j += 1
                break
            if 'for' in hl:
                found_for = True
            j += 1
        
        header = ''.join(header_lines)
        
        # Check if this is a cJSON iteration (has ->child and the increment is ++VAR)
        m = re.search(r'for\s*\(\s*(?:\w+\s+\**)?(\w+)\s*=.*?->child.*?;\s*(.*?);\s*(\+\+(\w+))\s*\)', header, re.DOTALL)
        if m:
            var_name = m.group(1)
            inc_var = m.group(4)
            if var_name == inc_var:
                # This is a cJSON for-loop with ++VAR - fix it
                old_inc = f'++{var_name}'
                new_inc = f'{var_name} = {var_name}->next'
                # Apply fix to the correct line
                for k in range(header_start, j):
                    if old_inc in lines[k]:
                        lines[k] = lines[k].replace(old_inc, new_inc)
                        print(f"Fixed line {k+1}: {var_name}")
                        break
    
    i += 1

with open('src/ui/GameUI.cpp', 'w') as f:
    f.writelines(lines)

print("Done")
