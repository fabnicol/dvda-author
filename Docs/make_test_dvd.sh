#!/bin/bash
ls *.wav | sort | grep -v -E 'a.?6'
