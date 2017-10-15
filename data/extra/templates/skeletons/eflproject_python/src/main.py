#!/usr/bin/env python
# encoding: utf-8

from __future__ import absolute_import, print_function, division

import sys

from efl import elementary as elm
from efl.evas import EVAS_HINT_EXPAND, EVAS_HINT_FILL, EXPAND_BOTH, FILL_BOTH


def main():

   # make the application quit when the last window is closed
   elm.policy_set(elm.ELM_POLICY_QUIT, elm.ELM_POLICY_QUIT_LAST_WINDOW_CLOSED)

   # Create the main app window
   win = elm.StandardWindow("main", "My first window",
                            autodel=True, size=(300, 200))

   # Create a simple label
   label = elm.Label(win, text="Hello World Python !",
                     size_hint_expand=EXPAND_BOTH, size_hint_fill=FILL_BOTH)
   win.resize_object_add(label)
   label.show()

   # Show the window and start the efl main loop
   win.show()
   elm.run()


if __name__ == "main":
   sys.exit(main())
