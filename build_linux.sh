#! /usr/bin/bash
cmake -B build/linux/
cmake --build build/linux --config Release
cp build/linux/GCTRealMate Release/GCTRealMate
