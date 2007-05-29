# ###########################################################################
# Name:        run-perftest1
# Description:
# Revision:    $Id$
# ###########################################################################

source("plotter.R")


testName <- "P01"

minPreSkip    <- 30
minPostSkip   <- 30
segmentLength <- 30
segments      <- 5

PEsSet           <- c(1000, 2500, 2750, 3000)
PUsSet           <- c(1)
reregIntervalSet <- c(250)
interHResTimeSet <- c(1000)
maxHResItemsSet  <- c(3)

interMeasurementDelay <- 60


# ###########################################################################


peColumn              <- c()
puColumn              <- c()
reregIntervalColumn   <- c()
interHResTimeColumn   <- c()
maxHResItemsColumn    <- c()

registrationRates     <- c()
reregistrationRates   <- c()
deregistrationRates   <- c()
handleResolutionRates <- c()
failureReportRates    <- c()
synchronizationRates  <- c()


# cat(readLines(pc <- pipe(paste(sep="", "rm -rf ", testName))))
# close(pc)
dir.create(testName,showWarnings=FALSE)


firstStart <- TRUE
for(PEs in PEsSet) {
for(PUs in PUsSet) {
for(reregInterval in reregIntervalSet) {
for(interHResTime in interHResTimeSet) {
for(maxHResItems in maxHResItemsSet) {
   if(!firstStart) {
      cat(sep="", "Waiting ", interMeasurementDelay, " seconds ...\n")
      readLines(pc <- pipe(paste(sep="", "sleep ", interMeasurementDelay)))
      close(pc)
   }
   firstStart <- FALSE

   # ------ Run performance test --------------------------------------------
   duration <- minPreSkip + minPostSkip + (segments + 1) * segmentLength
   runPrefix <- paste(sep="", testName, "-", PEs, "-", PUs, "-", reregInterval, "-", interHResTime, "-", maxHResItems, "-", duration)
   cmdLine <- paste(sep="", "./perftest ",
                            testName, " ", runPrefix,
                            " ",
                            PEs, " ", PUs, " ", 
                            reregInterval, " ", interHResTime, " ", maxHResItems, " ",
                            duration, " ",
                            ">", testName, "/", runPrefix, ".log")
   cat(sep="", "Running ", cmdLine," ...\n")
   cat(readLines(pc <- pipe(cmdLine)))
   close(pc)

   # ------ Collect results -------------------------------------------------
   cat("Collecting results ...\n")
   data <- loadResults(paste(sep="", testName, "/", runPrefix, "/TestPR-1.data"))
   rtdata <- read.table(paste(sep="", testName, "/", runPrefix, "/runtimes.data"))
   preSkip  <- minPreSkip + (mean(rtdata$MeasurementStart) - mean(rtdata$Startup))
   postSkip <- minPostSkip + (mean(rtdata$Shutdown) - mean(rtdata$MeasurementEnd))
   cat("=> Startup:            0.0\n")
   cat("   Measurement Start: ", mean(rtdata$MeasurementStart) - mean(rtdata$Startup), "\n")
   cat("   Measurement End:   ", mean(rtdata$MeasurementEnd)   - mean(rtdata$Startup), "\n")
   cat("   Shutdown:          ", mean(rtdata$Shutdown)         - mean(rtdata$Startup), "\n")
   cat("=> ", duration,preSkip,postSkip,"\n")

   registrationResults <- analyseCounterResults(data, preSkip, postSkip, segmentLength, segments,
                                                "Registrations", ACRT_Normalized)

   registrationRates     <- append(registrationRates,
                                   registrationResults)
   reregistrationRates   <- append(reregistrationRates,
                                   analyseCounterResults(data, preSkip, postSkip, segmentLength, segments,
                                   "Reregistrations", ACRT_Normalized))
   deregistrationRates   <- append(deregistrationRates,
                                   analyseCounterResults(data, preSkip, postSkip, segmentLength, segments,
                                   "Deregistrations", ACRT_Normalized))
   handleResolutionRates <- append(handleResolutionRates,
                                   analyseCounterResults(data, preSkip, postSkip, segmentLength, segments,
                                   "HandleResolutions", ACRT_Normalized))
   failureReportRates    <- append(failureReportRates,
                                   analyseCounterResults(data, preSkip, postSkip, segmentLength, segments,
                                   "FailureReports", ACRT_Normalized))
   synchronizationRates  <- append(synchronizationRates,
                                   analyseCounterResults(data, preSkip, postSkip, segmentLength, segments,
                                   "Synchronizations", ACRT_Normalized))

   peColumn            <- append(peColumn, rep(PEs, length(registrationResults)))
   puColumn            <- append(puColumn, rep(PUs, length(registrationResults)))
   reregIntervalColumn <- append(reregIntervalColumn, rep(reregInterval, length(registrationResults)))
   interHResTimeColumn <- append(interHResTimeColumn, rep(interHResTime, length(registrationResults)))
   maxHResItemsColumn  <- append(maxHResItemsColumn, rep(maxHResItems, length(registrationResults)))
}}}}}


cat("Writing results ...\n")
result <- data.frame(peColumn,
                     puColumn,
                     reregIntervalColumn,
                     interHResTimeColumn,
                     maxHResItemsColumn,

                     registrationRates,
                     reregistrationRates,
                     deregistrationRates,
                     handleResolutionRates,
                     failureReportRates,
                     synchronizationRates)
colnames(result) <- c("PEs", "PUs", "ReregInterval", "InterHResTime", "MaxHResItems",
                     "RegistrationRate", "ReregistrationRate", "DeregistrationRate",
                     "HandleResolutionRate", "FailureReportRate", "SynchronizationRate")
dir.create(paste(sep="", testName, "/Results"), showWarnings=FALSE)
resultsName <- paste(sep="", testName, "/Results/Summary.data")
write.table(result, resultsName)
cat(readLines(pc <- pipe(paste(sep="", "rm -f ", resultsName, ".bz2 ; bzip2 ", resultsName))))
close(pc)
