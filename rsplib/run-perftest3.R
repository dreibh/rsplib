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
segments         <- 2

PRsSet           <- c(2)
PEsSet           <- c(5, 25, 50)
PUsSet           <- c(5)
puToPERatioSet   <- c(0)
reregIntervalSet <- c(1000)
interHResTimeSet <- c(1000)
maxHResItemsSet  <- c(3)


performMeasurement()
