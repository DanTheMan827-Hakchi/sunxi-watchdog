
# sunxi-watchdog
sunxi-watchdog is a program based on retroarch-watchdog written by [CompCom](https://github.com/CompCom).

The purpose of this program is to make it easier for shell scripts to read the power and reset buttons.
## Sample output
```
key home 1
key home 0 253
key reset 1
key reset 0 174
key power 0 5463
key power 1
key power 0 1530
key power 1
```

## Usage
Usage is something like
```bash
/bin/program &
pid=$!
sunxi-watchdog --pid $pid | while read event key value msduration; do
  # check for reset and kill the pid if pushed
  [ "$key" == "reset" ] && kill $pid
  
  # check for power and kill the pid if pushed
  [ "$key" == "power" ] && kill $pid
  
  # check for home button and kill the pid if pushed
  [ "$key" == "home" ] && kill $pid
done
```
If used with the `--pid` option, sunxi-watchdog will automatically terminate when that pid exits.

## retroarch-watchdog license
```
Copyright (C) 2018 CompCom

This program is free software; you can redistribute it and/or
modify it under the terms of the GNU General Public License
as published by the Free Software Foundation; either version 3
of the License, or (at your option) any later version.
```
