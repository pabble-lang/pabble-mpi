#!/bin/sh

set x

VERSION=$(dpkg-parsechangelog --show-field Version)
sed -i "s/^#define PABBLE_MPI_TOOL_VERSION.*$/#define PABBLE_MPI_TOOL_VERSION \"$VERSION\"/" src/pabble-mpi-tool.c
