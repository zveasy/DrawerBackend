#!/bin/sh
set -e

# Generate client and sync schema
npx prisma generate
npx prisma db push

node dist/index.js
