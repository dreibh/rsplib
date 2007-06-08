# ###########################################################################
# Name:        run-perftest2
# Description:
# Revision:    $Id$
# ###########################################################################

source("perftest-version1.R")


testName         <- "P02"
minPreSkip       <- 30
minPostSkip      <- 30
segmentLength    <- 60
segments         <- 10

PEsSet           <- c(10, 100, 1000)
PUsSet           <- c(100, 250, 500, 750, 1000, 1250, 1500, 1750, 2000, 2250)
puToPERatioSet   <- c(-1)   # Set PUs directly
reregIntervalSet <- c(1000)
interHResTimeSet <- c(100, 250, 1000)
maxHResItemsSet  <- c(1,10)


performMeasurement()
