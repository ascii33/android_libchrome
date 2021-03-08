#!/usr/bin/env python3
# Copyright 2020 The Chromium OS Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.
"""
Utility to check if deprecated libchrome calls are introduced.
"""

from __future__ import print_function

import os
import re
import subprocess
import sys

# BAD_KEYWORDS are mapping from bad_keyword_in_regex to
# error_msg_if_match_found.
BAD_KEYWORDS = {
    b'DISALLOW_COPY_AND_ASSIGN':
    'Chromium agreed to return Google C++ style. Use deleted constructor in `public:` manually. See crbug/1010217',
    b'include.*base\/bind_helpers.h':
    'File will be removed, please import base/callback_helpers.h instead',
    b'include.*base/test/bind_test_util.h':
    'File will be removed, please import base/test/bind.h instead',
    b'include.*mojo\/public\/cpp\/bindings\/binding.h':
    'Old mojo bindings types will be deprecated, please convert to the new one. See closed chromium tracking crbug/955171 for details',
}


def check(environ=os.environ, keywords=BAD_KEYWORDS):
    files = environ['PRESUBMIT_FILES'].split('\n')

    errors = []

    for f in files:
        if not (f.endswith('.h') or f.endswith('.cc')):
            continue
        diff = subprocess.check_output(
            ['git', 'show', '--oneline', environ['PRESUBMIT_COMMIT'], '--',
             f]).split(b'\n')
        for line in diff:
            if not line.startswith(b'+'):
                continue
            for bad_pattern, error_message in keywords.items():
                m = re.search(bad_pattern, line)
                if m:
                    errors.append('In File %s, found %s (pattern: %s), %s' %
                                  (f, m.group(0), bad_pattern.decode('ascii'),
                                   error_message))
                    break

    return errors


def main():
    errors = check()
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        sys.exit(1)


if __name__ == '__main__':
    main()
