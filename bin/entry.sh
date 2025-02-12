#!/usr/bin/env bash

set -e

[ -d "node_modules" ] || npm install;

npm run build-node && npm test

exit