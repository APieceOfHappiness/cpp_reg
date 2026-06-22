#!/usr/bin/env bash
set -euo pipefail

g++ -O2 -std=c++17 -o regex_matcher main.cpp
tar -czf test_bins.tar.gz regex_matcher
