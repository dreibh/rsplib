# ###########################################################################
# Name:        run-perftest1
# Description:
# Revision:    $Id$
# ###########################################################################

source("perftest-version2.R")


testName              <- "P01"
minPreSkip            <- 30
minPostSkip           <- 30
segmentLength         <- 30
segments              <- 2

PRsSet                <- c(2)
PEsSet                <- c(10, 1000, 3000) # 100, 500, 1000, 1500, 2000, 2500, 2750, 3000, 3250, 3500)
PUsSet                <- c(-1)   # Calculated from puToPERatio
primaryPERegistrarSet <- c(1)
primaryPURegistrarSet <- c(1)
puToPERatioSet        <- c(0)
reregIntervalSet      <- c(1000, 250)
interHResTimeSet      <- c(1000)
maxHResItemsSet       <- c(3)


performMeasurement()
