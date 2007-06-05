# ###########################################################################
# Name:        run-perftest2
# Description:
# Revision:    $Id$
# ###########################################################################

source("perftest-version1.R")


testName         <- "P02"
minPreSkip       <- 30
minPostSkip      <- 30
segmentLength    <- 30
segments         <- 2

PEsSet           <- c(10, 100, 1000)
PUsSet           <- c(100, 1000)  # 10, 100, 
puToPERatioSet   <- c(-1)   # Give PUs directory
reregIntervalSet <- c(1000)
interHResTimeSet <- c(1000, 100)
maxHResItemsSet  <- c(3) # , 10)


performMeasurement()
