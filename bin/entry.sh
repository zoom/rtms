#!/usr/bin/env bash

set -e


lang="${RTMS_SDK_LANG}" # Assign the environment variable to a regular variable for easier use

if [ -z "$lang" ]; then
  echo "RTMS_SDK_LANG is blank. Defaulting to 'js'..."
  lang=js
fi

if [ "$lang" == "js" ] && [ ! -d "node_modules" ]; then 
    npm install;
fi



cmd="npm run build-$lang" 

eval $cmd

exit