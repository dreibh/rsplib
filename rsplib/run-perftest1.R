# ###########################################################################
# Name:        run-perftest1
# Description:
# Revision:    $Id$
# ###########################################################################

source("perftest-version1.R")


testName         <- "P01"
minPreSkip       <- 30
minPostSkip      <- 30
segmentLength    <- 30
segments         <- 2

PEsSet           <- c(1000, 2500, 2750, 3000)
puToPERatioSet   <- c(0)
reregIntervalSet <- c(250)
interHResTimeSet <- c(1000)
maxHResItemsSet  <- c(3)


performMeasurement()
