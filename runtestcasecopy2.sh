#!/bin/bash
gcc xcheckcopy.c -o xcheckcopy -Wall -Werror -O
echo ---------------------------------------------------------------------------------
echo good
./xcheckcopy good
echo ---------------------------------------------------------------------------------
echo badinode		
./xcheckcopy badinode
echo ---------------------------------------------------------------------------------
echo badaddr
./xcheckcopy badaddr
echo ---------------------------------------------------------------------------------
echo badindir1
./xcheckcopy badindir1
echo ---------------------------------------------------------------------------------
echo badindr2
./xcheckcopy badindir2
echo ---------------------------------------------------------------------------------
echo badroot
./xcheckcopy badroot
echo ---------------------------------------------------------------------------------
echo badroot2
./xcheckcopy badroot2
echo ---------------------------------------------------------------------------------
echo badfmt
./xcheckcopy badfmt
echo ---------------------------------------------------------------------------------
echo mrkfree
./xcheckcopy mrkfree
echo ---------------------------------------------------------------------------------
echo indirfree
./xcheckcopy indirfree
echo ---------------------------------------------------------------------------------
echo mrkused
./xcheckcopy mrkused
echo ---------------------------------------------------------------------------------
echo addronce
./xcheckcopy addronce
echo ---------------------------------------------------------------------------------
echo addronce2
./xcheckcopy addronce2
echo ---------------------------------------------------------------------------------
echo imrkused
./xcheckcopy imrkused
echo ---------------------------------------------------------------------------------
echo imrkfree
./xcheckcopy imrkfree
echo ---------------------------------------------------------------------------------
echo badrefcnt2
./xcheckcopy badrefcnt2
echo ---------------------------------------------------------------------------------
echo goodlarge
./xcheckcopy goodlarge
echo ---------------------------------------------------------------------------------
echo goodrefcnt
./xcheckcopy goodrefcnt
echo ---------------------------------------------------------------------------------
echo goodlink
./xcheckcopy goodlink
echo ---------------------------------------------------------------------------------
echo goodrm
./xcheckcopy goodrm
echo ---------------------------------------------------------------------------------
echo dironce
./xcheckcopy dironce
echo ---------------------------------------------------------------------------------
echo badlarge
./xcheckcopy badlarge
