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
segments         <- 12

prHosts               <- c("132.252.152.81", "132.252.151.178", "132.252.151.157",
                           "132.252.152.70", "132.252.152.71",  "132.252.152.72")
peHosts               <- c("132.252.152.73", "132.252.152.74", "132.252.152.75",
                           "132.252.152.76", "132.252.152.77", "132.252.152.78",
                           "132.252.152.79")
puHosts               <- peHosts

PRsSet                <- c(1, 2, 3, 4, 5, 6, 7, 8, 9, 10)
PEsSet                <- c(1000)
PUsSet                <- c(-1)   # Calculated from puToPERatio
primaryPERegistrarSet <- c(1)
primaryPURegistrarSet <- c(1)
policySet             <- c("RoundRobin")
puToPERatioSet        <- c(0)
reregIntervalSet      <- c(250, 500, 1000)
interHResTimeSet      <- c(1000)
maxHResItemsSet       <- c(3)


performMeasurement()
