# ###########################################################################
# Name:        run-perftest2
# Description:
# Revision:    $Id$
# ###########################################################################

source("perftest-version2.R")


testName              <- "P02"
minPreSkip            <- 30
minPostSkip           <- 30
segmentLength         <- 30
segments              <- 2

PRsSet                <- c(2)
PEsSet                <- c(1000)   # 10, 100, 1000)
PUsSet                <- c(100, 1000, 2000) # 250, 500, 750, 1000, 1250, 1500, 1750, 2000, 2250)
primaryPERegistrarSet <- c(1)
primaryPURegistrarSet <- c(1)
puToPERatioSet        <- c(-1)   # Set PUs directly
reregIntervalSet      <- c(1000)
interHResTimeSet      <- c(100, 250) # 100, 250, 1000)
maxHResItemsSet       <- c(1)


performMeasurement()
