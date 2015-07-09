#!/bin/bash

(time ./msort -i input$1.bin -m $2 -k$3 -o output$1_m$2_$3V.bin) 2> time_output$1_m$2_$3V.txt
cat time_output$1_m$2_$3V.txt | grep real | cut -d$'\t' -f2
