# ###########################################################################
# Name:        run-perftest3
# Description:
# Revision:    $Id$
# ###########################################################################

source("perftest-version2.R")


testName         <- "P03"
minPreSkip       <- 30
minPostSkip      <- 30
segmentLength    <- 30
segments         <- 5

PRsSet           <- c(2)
PEsSet           <- c(10, 100, 500, 1000, 1500, 2000, 2500, 2750, 3000)
PUsSet           <- c(-1)   # Calculated from puToPERatio
puToPERatioSet   <- c(0)
reregIntervalSet <- c(1000, 250)
interHResTimeSet <- c(1000)
maxHResItemsSet  <- c(3)


performMeasurement()
