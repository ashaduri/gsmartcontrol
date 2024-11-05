#!/bin/bash

png_name="gsmartcontrol.png"

# needs imagemagick
convert 16/$png_name 22/$png_name 24/$png_name 32/$png_name \
48/$png_name 64/$png_name 128/$png_name 256/$png_name gsmartcontrol.ico
