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
segments              <- 12

prHosts               <- c("132.252.151.178", "132.252.151.157")
peHosts               <- c("132.252.152.70", "132.252.152.71", "132.252.152.72",
                           "132.252.152.73", "132.252.152.74", "132.252.152.75",
                           "132.252.152.76", "132.252.152.77", "132.252.152.78",
                           "132.252.152.79")
puHosts               <- peHosts

PRsSet                <- c(2)
PEsSet                <- c(10, 100, 500, 1000, 1500, 2000, 2500, 2750, 3000)
PUsSet                <- c(-1)   # Calculated from puToPERatio
primaryPERegistrarSet <- c(1)
primaryPURegistrarSet <- c(1)
puToPERatioSet        <- c(0)
reregIntervalSet      <- c(1000, 250)
interHResTimeSet      <- c(1000)
maxHResItemsSet       <- c(3)


performMeasurement()
