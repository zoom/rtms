#!/usr/bin/env bash

set -e


lang="${ZM_RTMS_LANG:-js}"


if [ "$lang" == "js" ] && [ ! -d "node_modules" ]; then 
    npm install;
fi


cmd="npm run build-$lang && npm test" 

eval $cmd

exit