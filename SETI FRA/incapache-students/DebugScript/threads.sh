#!/bin/bash

for i in {1..100}; do
    curl http://localhost:80/ &
done