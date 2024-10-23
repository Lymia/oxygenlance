#!/bin/sh -eu
BINDGEN_EXTRA_CLANG_ARGS="--std=c2x" bindgen interface.h > interface.rs
