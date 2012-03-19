#!/bin/bash

PLATFORM=$1
PLATFORMLOWER=`echo ${PLATFORM} | tr A-Z a-z`

echo "// Don't edit it manually!" > def.cfg
echo ":chip: ${PLATFORM}" >> def.cfg
echo ":${PLATFORMLOWER}:" >> def.cfg
