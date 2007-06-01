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
segments         <- 10

PEsSet           <- c(1, 10, 100, 250)
puToPERatioSet   <- c(1, 10)
reregIntervalSet <- c(1000)
interHResTimeSet <- c(1000)
maxHResItemsSet  <- c(3)


performMeasurement()
