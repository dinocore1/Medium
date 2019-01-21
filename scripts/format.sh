#!/bin/bash

astyle -n --options=astyle.cfg --recursive "src/*.cc"
astyle -n --options=astyle.cfg --recursive "src/*.h"