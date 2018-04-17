#!/bin/sh

glslangValidator -H -o vertex.vert.sipr vertex.vert > vertex.vert.sipr.text
glslangValidator -H -o fragment.frag.sipr fragment.frag > fragment.frag.sipr.text
