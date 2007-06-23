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
segments              <- 12

prHosts               <- c("132.252.151.178", "132.252.151.157")
peHosts               <- c("132.252.152.70", "132.252.152.71", "132.252.152.72",
                           "132.252.152.73", "132.252.152.74", "132.252.152.75",
                           "132.252.152.76", "132.252.152.77", "132.252.152.78",
                           "132.252.152.79")
puHosts               <- peHosts

PRsSet                <- c(2)
PEsSet                <- c(1000)
PUsSet                <- c(100, 250, 500, 750, 1000, 1250, 1500, 1750, 2000)
primaryPERegistrarSet <- c(1)
primaryPURegistrarSet <- c(1, 2)
puToPERatioSet        <- c(-1)   # Set PUs directly
reregIntervalSet      <- c(1000)
interHResTimeSet      <- c(100, 250, 500)
maxHResItemsSet       <- c(1)


performMeasurement()
