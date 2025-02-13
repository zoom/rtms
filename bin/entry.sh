#!/usr/bin/env bash

set -e


lang="${LANG:-js}"


if [ "$lang" == "js" ] && [ ! -d "node_modules" ]; then 
    npm install;
fi


cmd="npm run build-$lang && npm run test-$lang" 

eval $cmd

exit